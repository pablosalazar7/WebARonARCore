// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/chromoting_host.h"

#include "base/bind.h"
#include "base/callback.h"
#include "build/build_config.h"
#include "remoting/base/constants.h"
#include "remoting/base/encoder.h"
#include "remoting/base/encoder_row_based.h"
#include "remoting/base/encoder_vp8.h"
#include "remoting/host/capturer.h"
#include "remoting/host/chromoting_host_context.h"
#include "remoting/host/continue_window.h"
#include "remoting/host/curtain.h"
#include "remoting/host/desktop_environment.h"
#include "remoting/host/disconnect_window.h"
#include "remoting/host/event_executor.h"
#include "remoting/host/host_config.h"
#include "remoting/host/host_key_pair.h"
#include "remoting/host/local_input_monitor.h"
#include "remoting/host/screen_recorder.h"
#include "remoting/host/user_authenticator.h"
#include "remoting/jingle_glue/xmpp_signal_strategy.h"
#include "remoting/proto/auth.pb.h"
#include "remoting/protocol/connection_to_client.h"
#include "remoting/protocol/client_stub.h"
#include "remoting/protocol/host_stub.h"
#include "remoting/protocol/input_stub.h"
#include "remoting/protocol/jingle_session_manager.h"
#include "remoting/protocol/session_config.h"

using remoting::protocol::ConnectionToClient;
using remoting::protocol::InputStub;

static const int kContinueWindowTimeoutSecs = 5 * 60;

namespace remoting {

// static
ChromotingHost* ChromotingHost::Create(ChromotingHostContext* context,
                                       MutableHostConfig* config,
                                       AccessVerifier* access_verifier) {
  Capturer* capturer = Capturer::Create();
  EventExecutor* event_executor =
      EventExecutor::Create(context->ui_message_loop(), capturer);
  Curtain* curtain = Curtain::Create();
  DisconnectWindow* disconnect_window = DisconnectWindow::Create();
  ContinueWindow* continue_window = ContinueWindow::Create();
  LocalInputMonitor* local_input_monitor = LocalInputMonitor::Create();
  return Create(context, config,
                new DesktopEnvironment(capturer, event_executor, curtain,
                                       disconnect_window, continue_window,
                                       local_input_monitor),
                access_verifier);
}

// static
ChromotingHost* ChromotingHost::Create(ChromotingHostContext* context,
                                       MutableHostConfig* config,
                                       DesktopEnvironment* environment,
                                       AccessVerifier* access_verifier) {
  return new ChromotingHost(context, config, environment, access_verifier);
}

ChromotingHost::ChromotingHost(ChromotingHostContext* context,
                               MutableHostConfig* config,
                               DesktopEnvironment* environment,
                               AccessVerifier* access_verifier)
    : context_(context),
      config_(config),
      desktop_environment_(environment),
      access_verifier_(access_verifier),
      state_(kInitial),
      protocol_config_(protocol::CandidateSessionConfig::CreateDefault()),
      is_curtained_(false),
      is_monitoring_local_inputs_(false),
      is_it2me_(false) {
  DCHECK(desktop_environment_.get());
}

ChromotingHost::~ChromotingHost() {
}

void ChromotingHost::Start() {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::Start, this));
    return;
  }

  DCHECK(!jingle_client_);
  DCHECK(access_verifier_.get());

  // Make sure this object is not started.
  {
    base::AutoLock auto_lock(lock_);
    if (state_ != kInitial)
      return;
    state_ = kStarted;
  }

  std::string xmpp_login;
  std::string xmpp_auth_token;
  std::string xmpp_auth_service;
  if (!config_->GetString(kXmppLoginConfigPath, &xmpp_login) ||
      !config_->GetString(kXmppAuthTokenConfigPath, &xmpp_auth_token) ||
      !config_->GetString(kXmppAuthServiceConfigPath, &xmpp_auth_service)) {
    LOG(ERROR) << "XMPP credentials are not defined in the config.";
    return;
  }

  // Connect to the talk network with a JingleClient.
  signal_strategy_.reset(
      new XmppSignalStrategy(context_->jingle_thread(), xmpp_login,
                             xmpp_auth_token,
                             xmpp_auth_service));
  jingle_client_ = new JingleClient(context_->jingle_thread(),
                                    signal_strategy_.get(),
                                    NULL, NULL, NULL, this);
  jingle_client_->Init();
}

// This method is called when we need to destroy the host process.
void ChromotingHost::Shutdown(Task* shutdown_task) {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&ChromotingHost::Shutdown, this, shutdown_task));
    return;
  }

  // No-op if this object is not started yet.
  {
    base::AutoLock auto_lock(lock_);
    if (state_ == kInitial || state_ == kStopped) {
      // Nothing to do if we are not started.
      state_ = kStopped;
      context_->main_message_loop()->PostTask(FROM_HERE, shutdown_task);
      return;
    }
    if (shutdown_task)
      shutdown_tasks_.push_back(shutdown_task);
    if (state_ == kStopping)
      return;
    state_ = kStopping;
  }

  // Make sure ScreenRecorder doesn't write to the connection.
  if (recorder_.get()) {
    recorder_->RemoveAllConnections();
  }

  // Stop local inputs monitor.
  MonitorLocalInputs(false);

  // Disconnect the clients.
  for (size_t i = 0; i < clients_.size(); i++) {
    clients_[i]->Disconnect();
  }
  clients_.clear();

  // Stop chromotocol session manager.
  if (session_manager_) {
    session_manager_->Close(
        NewRunnableMethod(this, &ChromotingHost::ShutdownJingleClient));
    session_manager_ = NULL;
  } else {
    ShutdownJingleClient();
  }
}

void ChromotingHost::AddStatusObserver(HostStatusObserver* observer) {
  DCHECK_EQ(state_, kInitial);
  status_observers_.push_back(observer);
}

////////////////////////////////////////////////////////////////////////////
// protocol::ConnectionToClient::EventHandler implementations
void ChromotingHost::OnConnectionOpened(ConnectionToClient* connection) {
  DCHECK_EQ(context_->network_message_loop(), MessageLoop::current());
  VLOG(1) << "Connection to client established.";
  // TODO(wez): ChromotingHost shouldn't need to know about Me2Mom.
  if (is_it2me_) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::ProcessPreAuthentication, this,
                              make_scoped_refptr(connection)));
  }
}

void ChromotingHost::OnConnectionClosed(ConnectionToClient* connection) {
  DCHECK_EQ(context_->network_message_loop(), MessageLoop::current());

  VLOG(1) << "Connection to client closed.";
  context_->main_message_loop()->PostTask(
      FROM_HERE, base::Bind(&ChromotingHost::OnClientDisconnected, this,
                            make_scoped_refptr(connection)));
}

void ChromotingHost::OnConnectionFailed(ConnectionToClient* connection) {
  DCHECK_EQ(context_->network_message_loop(), MessageLoop::current());

  LOG(ERROR) << "Connection failed unexpectedly.";
  context_->main_message_loop()->PostTask(
      FROM_HERE, base::Bind(&ChromotingHost::OnClientDisconnected, this,
                            make_scoped_refptr(connection)));
}

void ChromotingHost::OnSequenceNumberUpdated(ConnectionToClient* connection,
                                             int64 sequence_number) {
  // Update the sequence number in ScreenRecorder.
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::OnSequenceNumberUpdated, this,
                              make_scoped_refptr(connection), sequence_number));
    return;
  }

  if (recorder_.get())
    recorder_->UpdateSequenceNumber(sequence_number);
}

////////////////////////////////////////////////////////////////////////////
// JingleClient::Callback implementations
void ChromotingHost::OnStateChange(JingleClient* jingle_client,
                                   JingleClient::State state) {
  DCHECK_EQ(MessageLoop::current(), context_->network_message_loop());

  if (state == JingleClient::CONNECTED) {
    std::string jid = jingle_client->GetFullJid();

    DCHECK_EQ(jingle_client_.get(), jingle_client);
    VLOG(1) << "Host connected as " << jid;

    // Create and start session manager.
    protocol::JingleSessionManager* server =
        new protocol::JingleSessionManager(context_->jingle_thread());
    // TODO(ajwong): Make this a command switch when we're more stable.
    server->set_allow_local_ips(true);

    // Assign key and certificate to server.
    HostKeyPair key_pair;
    CHECK(key_pair.Load(config_))
        << "Failed to load server authentication data";

    server->Init(jid, jingle_client->session_manager(),
                 NewCallback(this, &ChromotingHost::OnNewClientSession),
                 key_pair.CopyPrivateKey(), key_pair.GenerateCertificate());

    session_manager_ = server;

    for (StatusObserverList::iterator it = status_observers_.begin();
         it != status_observers_.end(); ++it) {
      (*it)->OnSignallingConnected(signal_strategy_.get(), jid);
    }
  } else if (state == JingleClient::CLOSED) {
    VLOG(1) << "Host disconnected from talk network.";
    for (StatusObserverList::iterator it = status_observers_.begin();
         it != status_observers_.end(); ++it) {
      (*it)->OnSignallingDisconnected();
    }
    // TODO(sergeyu): Don't shutdown the host and let the upper level
    // decide what needs to be done when signalling channel is
    // disconnected.
    Shutdown(NULL);
  }
}

void ChromotingHost::OnNewClientSession(
    protocol::Session* session,
    protocol::SessionManager::IncomingSessionResponse* response) {
  base::AutoLock auto_lock(lock_);
  if (state_ != kStarted) {
    *response = protocol::SessionManager::DECLINE;
    return;
  }

  // If we are running Me2Mom and already have an authenticated client then
  // reject the connection immediately.
  if (is_it2me_ && AuthenticatedClientsCount() > 0) {
    *response = protocol::SessionManager::DECLINE;
    return;
  }

  // Check that the client has access to the host.
  if (!access_verifier_->VerifyPermissions(session->jid(),
                                           session->initiator_token())) {
    *response = protocol::SessionManager::DECLINE;

    // Notify observers.
    for (StatusObserverList::iterator it = status_observers_.begin();
         it != status_observers_.end(); ++it) {
      (*it)->OnAccessDenied();
    }
    return;
  }

  // TODO(simonmorris): The resolution is set in the video stream now,
  // so it doesn't need to be set here.
  *protocol_config_->mutable_initial_resolution() =
      protocol::ScreenResolution(2048, 2048);
  // TODO(sergeyu): Respect resolution requested by the client if supported.
  protocol::SessionConfig* config = protocol_config_->Select(
      session->candidate_config(), true /* force_host_resolution */);

  if (!config) {
    LOG(WARNING) << "Rejecting connection from " << session->jid()
                 << " because no compatible configuration has been found.";
    *response = protocol::SessionManager::INCOMPATIBLE;
    return;
  }

  session->set_config(config);
  session->set_receiver_token(
      GenerateHostAuthToken(session->initiator_token()));

  *response = protocol::SessionManager::ACCEPT;

  VLOG(1) << "Client connected: " << session->jid();

  // We accept the connection, so create a connection object.
  ConnectionToClient* connection = new ConnectionToClient(
      context_->network_message_loop(), this);

  // Create a client object.
  ClientSession* client = new ClientSession(
      this,
      UserAuthenticator::Create(),
      connection,
      desktop_environment_->event_executor());
  connection->set_host_stub(client);
  connection->set_input_stub(client);

  connection->Init(session);

  clients_.push_back(client);
}

void ChromotingHost::set_protocol_config(
    protocol::CandidateSessionConfig* config) {
  DCHECK(config_.get());
  DCHECK_EQ(state_, kInitial);
  protocol_config_.reset(config);
}

void ChromotingHost::LocalMouseMoved(const gfx::Point& new_pos) {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::LocalMouseMoved, this, new_pos));
    return;
  }
  ClientList::iterator client;
  for (client = clients_.begin(); client != clients_.end(); ++client) {
    client->get()->LocalMouseMoved(new_pos);
  }
}

void ChromotingHost::PauseSession(bool pause) {
  if (context_->main_message_loop() != MessageLoop::current()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &ChromotingHost::PauseSession,
                          pause));
    return;
  }
  ClientList::iterator client;
  for (client = clients_.begin(); client != clients_.end(); ++client) {
    client->get()->set_awaiting_continue_approval(pause);
  }
  StartContinueWindowTimer(!pause);
}

void ChromotingHost::OnClientDisconnected(ConnectionToClient* connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());

  // Find the client session corresponding to the given connection.
  ClientList::iterator client;
  for (client = clients_.begin(); client != clients_.end(); ++client) {
    if (client->get()->connection() == connection)
      break;
  }
  if (client == clients_.end())
    return;

  // Remove the connection from the session manager and stop the session.
  // TODO(hclam): Stop only if the last connection disconnected.
  if (recorder_.get()) {
    recorder_->RemoveConnection(connection);
    // The recorder only exists to serve the unique authenticated client.
    // If that client has disconnected, then we can kill the recorder.
    if (client->get()->authenticated()) {
      recorder_->Stop(NULL);
      recorder_ = NULL;
    }
  }

  // Close the connection to connection just to be safe.
  connection->Disconnect();

  // Also remove reference to ConnectionToClient from this object.
  int old_authenticated_clients = AuthenticatedClientsCount();
  clients_.erase(client);

  // Notify the observers of the change, if any.
  int authenticated_clients = AuthenticatedClientsCount();
  if (old_authenticated_clients != authenticated_clients) {
    for (StatusObserverList::iterator it = status_observers_.begin();
         it != status_observers_.end(); ++it) {
      (*it)->OnAuthenticatedClientsChanged(authenticated_clients);
    }
  }

  // Enable the "curtain", if at least one client is actually authenticated.
  if (AuthenticatedClientsCount() > 0) {
    EnableCurtainMode(false);
    if (is_it2me_) {
      MonitorLocalInputs(false);
      ShowDisconnectWindow(false, std::string());
      ShowContinueWindow(false);
      StartContinueWindowTimer(false);
    }
  }
}

// TODO(sergeyu): Move this to SessionManager?
Encoder* ChromotingHost::CreateEncoder(const protocol::SessionConfig* config) {
  const protocol::ChannelConfig& video_config = config->video_config();

  if (video_config.codec == protocol::ChannelConfig::CODEC_VERBATIM) {
    return EncoderRowBased::CreateVerbatimEncoder();
  } else if (video_config.codec == protocol::ChannelConfig::CODEC_ZIP) {
    return EncoderRowBased::CreateZlibEncoder();
  }
  // TODO(sergeyu): Enable VP8 on ARM builds.
#if !defined(ARCH_CPU_ARM_FAMILY)
  else if (video_config.codec == protocol::ChannelConfig::CODEC_VP8) {
    return new remoting::EncoderVp8();
  }
#endif

  return NULL;
}

std::string ChromotingHost::GenerateHostAuthToken(
    const std::string& encoded_client_token) {
  // TODO(ajwong): Return the signature of this instead.
  return encoded_client_token;
}

int ChromotingHost::AuthenticatedClientsCount() const {
  int authenticated_clients = 0;
  for (ClientList::const_iterator it = clients_.begin(); it != clients_.end();
       ++it) {
    if (it->get()->authenticated())
      ++authenticated_clients;
  }
  return authenticated_clients;
}

void ChromotingHost::EnableCurtainMode(bool enable) {
  // TODO(jamiewalch): This will need to be more sophisticated when we think
  // about proper crash recovery and daemon mode.
  // TODO(wez): CurtainMode shouldn't be driven directly by ChromotingHost.
  if (is_it2me_ || enable == is_curtained_)
    return;
  desktop_environment_->curtain()->EnableCurtainMode(enable);
  is_curtained_ = enable;
}

void ChromotingHost::MonitorLocalInputs(bool enable) {
  if (enable == is_monitoring_local_inputs_)
    return;
  if (enable) {
    desktop_environment_->local_input_monitor()->Start(this);
  } else {
    desktop_environment_->local_input_monitor()->Stop();
  }
  is_monitoring_local_inputs_ = enable;
}

void ChromotingHost::LocalLoginSucceeded(
    scoped_refptr<ConnectionToClient> connection) {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::LocalLoginSucceeded, this,
                              connection));
    return;
  }

  protocol::LocalLoginStatus* status = new protocol::LocalLoginStatus();
  status->set_success(true);
  connection->client_stub()->BeginSessionResponse(
      status, new DeleteTask<protocol::LocalLoginStatus>(status));

  // Disconnect all other clients.
  // Iterate over a copy of the list of clients, to avoid mutating the list
  // while iterating over it.
  ClientList clients_copy(clients_);
  for (ClientList::const_iterator client = clients_copy.begin();
       client != clients_copy.end(); client++) {
    ConnectionToClient* connection_other = client->get()->connection();
    if (connection_other != connection) {
      OnClientDisconnected(connection_other);
    }
  }
  // Those disconnections should have killed the screen recorder.
  CHECK(recorder_.get() == NULL);

  // Create a new RecordSession if there was none.
  if (!recorder_.get()) {
    // Then we create a ScreenRecorder passing the message loops that
    // it should run on.
    Encoder* encoder = CreateEncoder(connection->session()->config());

    recorder_ = new ScreenRecorder(context_->main_message_loop(),
                                   context_->encode_message_loop(),
                                   context_->network_message_loop(),
                                   desktop_environment_->capturer(),
                                   encoder);
  }

  // Immediately add the connection and start the session.
  recorder_->AddConnection(connection);
  recorder_->Start();
  // TODO(jamiewalch): Tidy up actions to be taken on connect/disconnect,
  // including closing the connection on failure of a critical operation.
  EnableCurtainMode(true);
  if (is_it2me_) {
    MonitorLocalInputs(true);
    std::string username = connection->session()->jid();
    size_t pos = username.find('/');
    if (pos != std::string::npos)
      username.replace(pos, std::string::npos, "");
    ShowDisconnectWindow(true, username);
    StartContinueWindowTimer(true);
  }

  // Notify observers that there is at least one authenticated client.
  for (StatusObserverList::iterator it = status_observers_.begin();
       it != status_observers_.end(); ++it) {
    (*it)->OnAuthenticatedClientsChanged(AuthenticatedClientsCount());
  }
}

void ChromotingHost::LocalLoginFailed(
    scoped_refptr<ConnectionToClient> connection) {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::LocalLoginFailed, this,
                              connection));
    return;
  }

  protocol::LocalLoginStatus* status = new protocol::LocalLoginStatus();
  status->set_success(false);
  connection->client_stub()->BeginSessionResponse(
      status, new DeleteTask<protocol::LocalLoginStatus>(status));
}

void ChromotingHost::ProcessPreAuthentication(
    const scoped_refptr<ConnectionToClient>& connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());
  // Find the client session corresponding to the given connection.
  ClientList::iterator client;
  for (client = clients_.begin(); client != clients_.end(); ++client) {
    if (client->get()->connection() == connection)
      break;
  }
  CHECK(client != clients_.end());
  client->get()->OnAuthorizationComplete(true);
}

void ChromotingHost::ShowDisconnectWindow(bool show,
                                          const std::string& username) {
  if (!context_->IsUIThread()) {
    context_->PostToUIThread(
        FROM_HERE,
        NewRunnableMethod(this, &ChromotingHost::ShowDisconnectWindow,
                          show, username));
    return;
  }

  if (show) {
    desktop_environment_->disconnect_window()->Show(this, username);
  } else {
    desktop_environment_->disconnect_window()->Hide();
  }
}

void ChromotingHost::ShowContinueWindow(bool show) {
  if (context_->ui_message_loop() != MessageLoop::current()) {
    context_->ui_message_loop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromotingHost::ShowContinueWindow, show));
    return;
  }

  if (show) {
    desktop_environment_->continue_window()->Show(this);
  } else {
    desktop_environment_->continue_window()->Hide();
  }
}

void ChromotingHost::StartContinueWindowTimer(bool start) {
  if (context_->main_message_loop() != MessageLoop::current()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &ChromotingHost::StartContinueWindowTimer,
                          start));
    return;
  }
  if (continue_window_timer_.IsRunning() == start)
    return;
  if (start) {
    continue_window_timer_.Start(
        base::TimeDelta::FromSeconds(kContinueWindowTimeoutSecs),
        this, &ChromotingHost::ContinueWindowTimerFunc);
  } else {
    continue_window_timer_.Stop();
  }
}

void ChromotingHost::ContinueWindowTimerFunc() {
  PauseSession(true);
  ShowContinueWindow(true);
}

void ChromotingHost::ShutdownJingleClient() {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::ShutdownJingleClient, this));
    return;
  }

  // Disconnect from the talk network.
  if (jingle_client_) {
    jingle_client_->Close(NewRunnableMethod(
        this, &ChromotingHost::ShutdownSignallingDisconnected));
  } else {
    ShutdownRecorder();
  }
}

void ChromotingHost::ShutdownSignallingDisconnected() {
  if (MessageLoop::current() != context_->network_message_loop()) {
    context_->network_message_loop()->PostTask(
        FROM_HERE, base::Bind(
            &ChromotingHost::ShutdownSignallingDisconnected, this));
    return;
  }

  for (StatusObserverList::iterator it = status_observers_.begin();
       it != status_observers_.end(); ++it) {
    (*it)->OnSignallingDisconnected();
  }
  ShutdownRecorder();
}

void ChromotingHost::ShutdownRecorder() {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::ShutdownRecorder, this));
    return;
  }

  if (recorder_.get()) {
    recorder_->Stop(NewRunnableMethod(this, &ChromotingHost::ShutdownFinish));
  } else {
    ShutdownFinish();
  }
}

void ChromotingHost::ShutdownFinish() {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE, base::Bind(&ChromotingHost::ShutdownFinish, this));
    return;
  }

  {
    base::AutoLock auto_lock(lock_);
    state_ = kStopped;
  }

  // Notify observers.
  for (StatusObserverList::iterator it = status_observers_.begin();
       it != status_observers_.end(); ++it) {
    (*it)->OnShutdown();
  }

  for (std::vector<Task*>::iterator it = shutdown_tasks_.begin();
       it != shutdown_tasks_.end(); ++it) {
    (*it)->Run();
    delete *it;
  }
  shutdown_tasks_.clear();
}

}  // namespace remoting
