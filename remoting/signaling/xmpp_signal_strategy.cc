// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/xmpp_signal_strategy.h"

#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/observer_list.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "jingle/glue/proxy_resolving_client_socket.h"
#include "net/cert/cert_verifier.h"
#include "net/http/transport_security_state.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/base/buffered_socket_writer.h"
#include "remoting/signaling/xmpp_login_handler.h"
#include "remoting/signaling/xmpp_stream_parser.h"
#include "third_party/webrtc/libjingle/xmllite/xmlelement.h"

// Use 50 seconds keep-alive interval, in case routers terminate
// connections that are idle for more than a minute.
const int kKeepAliveIntervalSeconds = 50;

const int kReadBufferSize = 4096;

const int kDefaultXmppPort = 5222;
const int kDefaultHttpsPort = 443;

namespace remoting {

XmppSignalStrategy::XmppServerConfig::XmppServerConfig()
    : port(kDefaultXmppPort), use_tls(true) {
}

XmppSignalStrategy::XmppServerConfig::~XmppServerConfig() {
}

class XmppSignalStrategy::Core : public XmppLoginHandler::Delegate {
 public:
  Core(
      net::ClientSocketFactory* socket_factory,
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      const XmppServerConfig& xmpp_server_config);
  ~Core() override;

  void Connect();
  void Disconnect();
  State GetState() const;
  Error GetError() const;
  std::string GetLocalJid() const;
  void AddListener(Listener* listener);
  void RemoveListener(Listener* listener);
  bool SendStanza(scoped_ptr<buzz::XmlElement> stanza);

  void SetAuthInfo(const std::string& username,
                   const std::string& auth_token);

  void VerifyNoListeners();

 private:
  void OnSocketConnected(int result);
  void OnTlsConnected(int result);

  void ReadSocket();
  void OnReadResult(int result);
  void HandleReadResult(int result);

  // XmppLoginHandler::Delegate interface.
  void SendMessage(const std::string& message) override;
  void StartTls() override;
  void OnHandshakeDone(const std::string& jid,
                       scoped_ptr<XmppStreamParser> parser) override;
  void OnLoginHandlerError(SignalStrategy::Error error) override;

  // Event handlers for XmppStreamParser.
  void OnStanza(const scoped_ptr<buzz::XmlElement> stanza);
  void OnParserError();

  void OnNetworkError(int error);

  void SendKeepAlive();

  net::ClientSocketFactory* socket_factory_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  XmppServerConfig xmpp_server_config_;

  // Used by the |socket_|.
  scoped_ptr<net::CertVerifier> cert_verifier_;
  scoped_ptr<net::TransportSecurityState> transport_security_state_;

  scoped_ptr<net::StreamSocket> socket_;
  scoped_ptr<BufferedSocketWriter> writer_;
  scoped_refptr<net::IOBuffer> read_buffer_;
  bool read_pending_;
  bool tls_pending_;

  scoped_ptr<XmppLoginHandler> login_handler_;
  scoped_ptr<XmppStreamParser> stream_parser_;
  std::string jid_;

  Error error_;

  ObserverList<Listener, true> listeners_;

  base::Timer keep_alive_timer_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

XmppSignalStrategy::Core::Core(
    net::ClientSocketFactory* socket_factory,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const XmppSignalStrategy::XmppServerConfig& xmpp_server_config)
    : socket_factory_(socket_factory),
      request_context_getter_(request_context_getter),
      xmpp_server_config_(xmpp_server_config),
      read_pending_(false),
      tls_pending_(false),
      error_(OK),
      keep_alive_timer_(
          FROM_HERE,
          base::TimeDelta::FromSeconds(kKeepAliveIntervalSeconds),
          base::Bind(&Core::SendKeepAlive, base::Unretained(this)),
          true) {
#if defined(NDEBUG)
  // Non-secure connections are allowed only for debugging.
  CHECK(xmpp_server_config_.use_tls);
#endif
}

XmppSignalStrategy::Core::~Core() {
  Disconnect();
}

void XmppSignalStrategy::Core::Connect() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Disconnect first if we are currently connected.
  Disconnect();

  error_ = OK;

  FOR_EACH_OBSERVER(Listener, listeners_,
                    OnSignalStrategyStateChange(CONNECTING));

  socket_.reset(new jingle_glue::ProxyResolvingClientSocket(
      socket_factory_, request_context_getter_, net::SSLConfig(),
      net::HostPortPair(xmpp_server_config_.host, xmpp_server_config_.port)));

  int result = socket_->Connect(base::Bind(
      &Core::OnSocketConnected, base::Unretained(this)));
  if (result != net::ERR_IO_PENDING)
    OnSocketConnected(result);
}

void XmppSignalStrategy::Core::Disconnect() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (socket_) {
    login_handler_.reset();
    stream_parser_.reset();
    writer_.reset();
    socket_.reset();

    FOR_EACH_OBSERVER(Listener, listeners_,
                      OnSignalStrategyStateChange(DISCONNECTED));
  }
}

SignalStrategy::State XmppSignalStrategy::Core::GetState() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (stream_parser_) {
    DCHECK(socket_);
    return CONNECTED;
  } else if (socket_) {
    return CONNECTING;
  } else {
    return DISCONNECTED;
  }
}

SignalStrategy::Error XmppSignalStrategy::Core::GetError() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return error_;
}

std::string XmppSignalStrategy::Core::GetLocalJid() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return jid_;
}

void XmppSignalStrategy::Core::AddListener(Listener* listener) {
  DCHECK(thread_checker_.CalledOnValidThread());
  listeners_.AddObserver(listener);
}

void XmppSignalStrategy::Core::RemoveListener(Listener* listener) {
  DCHECK(thread_checker_.CalledOnValidThread());
  listeners_.RemoveObserver(listener);
}

bool XmppSignalStrategy::Core::SendStanza(scoped_ptr<buzz::XmlElement> stanza) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!stream_parser_) {
    VLOG(0) << "Dropping signalling message because XMPP "
               "connection has been terminated.";
    return false;
  }

  SendMessage(stanza->Str());
  return true;
}

void XmppSignalStrategy::Core::SetAuthInfo(const std::string& username,
                                           const std::string& auth_token) {
  DCHECK(thread_checker_.CalledOnValidThread());
  xmpp_server_config_.username = username;
  xmpp_server_config_.auth_token = auth_token;
}

void XmppSignalStrategy::Core::SendMessage(const std::string& message) {
  DCHECK(thread_checker_.CalledOnValidThread());
  scoped_refptr<net::IOBufferWithSize> buffer =
      new net::IOBufferWithSize(message.size());
  memcpy(buffer->data(), message.data(), message.size());
  writer_->Write(buffer, base::Closure());
}

void XmppSignalStrategy::Core::StartTls() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(login_handler_);

  // Reset the writer so we don't try to write to the raw socket anymore.
  DCHECK_EQ(writer_->GetBufferSize(), 0);
  writer_.reset();

  DCHECK(!read_pending_);

  scoped_ptr<net::ClientSocketHandle> socket_handle(
      new net::ClientSocketHandle());
  socket_handle->SetSocket(socket_.Pass());

  cert_verifier_.reset(net::CertVerifier::CreateDefault());
  transport_security_state_.reset(new net::TransportSecurityState());
  net::SSLClientSocketContext context;
  context.cert_verifier = cert_verifier_.get();
  context.transport_security_state = transport_security_state_.get();

  socket_ = socket_factory_->CreateSSLClientSocket(
      socket_handle.Pass(),
      net::HostPortPair(xmpp_server_config_.host, kDefaultHttpsPort),
      net::SSLConfig(), context);

  tls_pending_ = true;
  int result = socket_->Connect(
      base::Bind(&Core::OnTlsConnected, base::Unretained(this)));
  if (result != net::ERR_IO_PENDING)
    OnTlsConnected(result);
}

void XmppSignalStrategy::Core::OnHandshakeDone(
    const std::string& jid,
    scoped_ptr<XmppStreamParser> parser) {
  DCHECK(thread_checker_.CalledOnValidThread());

  jid_ = jid;
  stream_parser_ = parser.Pass();
  stream_parser_->SetCallbacks(
      base::Bind(&Core::OnStanza, base::Unretained(this)),
      base::Bind(&Core::OnParserError, base::Unretained(this)));

  // Don't need |login_handler_| anymore.
  login_handler_.reset();

  FOR_EACH_OBSERVER(Listener, listeners_,
                    OnSignalStrategyStateChange(CONNECTED));
}

void XmppSignalStrategy::Core::OnLoginHandlerError(
    SignalStrategy::Error error) {
  DCHECK(thread_checker_.CalledOnValidThread());

  error_ = error;
  Disconnect();
}

void XmppSignalStrategy::Core::OnStanza(
    const scoped_ptr<buzz::XmlElement> stanza) {
  DCHECK(thread_checker_.CalledOnValidThread());

  base::ObserverListBase<Listener>::Iterator it(&listeners_);
  for (Listener* listener = it.GetNext(); listener; listener = it.GetNext()) {
    if (listener->OnSignalStrategyIncomingStanza(stanza.get()))
      return;
  }
}

void XmppSignalStrategy::Core::OnParserError() {
  DCHECK(thread_checker_.CalledOnValidThread());

  error_ = NETWORK_ERROR;
  Disconnect();
}

void XmppSignalStrategy::Core::OnSocketConnected(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (result != net::OK) {
    OnNetworkError(result);
    return;
  }

  writer_.reset(new BufferedSocketWriter());
  writer_->Init(socket_.get(), base::Bind(&Core::OnNetworkError,
                                          base::Unretained(this)));

  XmppLoginHandler::TlsMode tls_mode;
  if (xmpp_server_config_.use_tls) {
    tls_mode = (xmpp_server_config_.port == kDefaultXmppPort)
                   ? XmppLoginHandler::TlsMode::WITH_HANDSHAKE
                   : XmppLoginHandler::TlsMode::WITHOUT_HANDSHAKE;
  } else {
    tls_mode = XmppLoginHandler::TlsMode::NO_TLS;
  }

  // The server name is passed as to attribute in the <stream>. When connecting
  // to talk.google.com it affects the certificate the server will use for TLS:
  // talk.google.com uses gmail certificate when specified server is gmail.com
  // or googlemail.com and google.com cert otherwise. In the same time it
  // doesn't accept talk.google.com as target server. Here we use google.com
  // server name when authenticating to talk.google.com. This ensures that the
  // server will use google.com cert which will be accepted by the TLS
  // implementation in Chrome (TLS API doesn't allow specifying domain other
  // than the one that was passed to connect()).
  std::string server = xmpp_server_config_.host;
  if (server == "talk.google.com")
    server = "google.com";

  login_handler_.reset(
      new XmppLoginHandler(server, xmpp_server_config_.username,
                           xmpp_server_config_.auth_token, tls_mode, this));
  login_handler_->Start();

  ReadSocket();
}

void XmppSignalStrategy::Core::OnTlsConnected(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tls_pending_);
  tls_pending_ = false;

  if (result != net::OK) {
    OnNetworkError(result);
    return;
  }

  writer_.reset(new BufferedSocketWriter());
  writer_->Init(socket_.get(), base::Bind(&Core::OnNetworkError,
                                          base::Unretained(this)));

  login_handler_->OnTlsStarted();

  ReadSocket();
}

void XmppSignalStrategy::Core::ReadSocket() {
  DCHECK(thread_checker_.CalledOnValidThread());

  while (socket_ && !read_pending_ && !tls_pending_) {
    read_buffer_ = new net::IOBuffer(kReadBufferSize);
    int result = socket_->Read(
        read_buffer_.get(), kReadBufferSize,
        base::Bind(&Core::OnReadResult, base::Unretained(this)));
    HandleReadResult(result);
  }
}

void XmppSignalStrategy::Core::OnReadResult(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(read_pending_);
  read_pending_ = false;
  HandleReadResult(result);
  ReadSocket();
}

void XmppSignalStrategy::Core::HandleReadResult(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (result == net::ERR_IO_PENDING) {
    read_pending_ = true;
    return;
  }

  if (result < 0) {
    OnNetworkError(result);
    return;
  }

  if (result == 0) {
    // Connection was closed by the server.
    error_ = OK;
    Disconnect();
    return;
  }

  if (stream_parser_) {
    stream_parser_->AppendData(std::string(read_buffer_->data(), result));
  } else {
    login_handler_->OnDataReceived(std::string(read_buffer_->data(), result));
  }
}

void XmppSignalStrategy::Core::OnNetworkError(int error) {
  DCHECK(thread_checker_.CalledOnValidThread());

  LOG(ERROR) << "XMPP socket error " << error;
  error_ = NETWORK_ERROR;
  Disconnect();
}

void XmppSignalStrategy::Core::SendKeepAlive() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (GetState() == CONNECTED)
    SendMessage(" ");
}

XmppSignalStrategy::XmppSignalStrategy(
    net::ClientSocketFactory* socket_factory,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const XmppServerConfig& xmpp_server_config)
    : core_(new Core(socket_factory,
                     request_context_getter,
                     xmpp_server_config)) {
}

XmppSignalStrategy::~XmppSignalStrategy() {
  // All listeners should be removed at this point, so it's safe to detach
  // |core_|.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, core_.release());
}

void XmppSignalStrategy::Connect() {
  core_->Connect();
}

void XmppSignalStrategy::Disconnect() {
  core_->Disconnect();
}

SignalStrategy::State XmppSignalStrategy::GetState() const {
  return core_->GetState();
}

SignalStrategy::Error XmppSignalStrategy::GetError() const {
  return core_->GetError();
}

std::string XmppSignalStrategy::GetLocalJid() const {
  return core_->GetLocalJid();
}

void XmppSignalStrategy::AddListener(Listener* listener) {
  core_->AddListener(listener);
}

void XmppSignalStrategy::RemoveListener(Listener* listener) {
  core_->RemoveListener(listener);
}
bool XmppSignalStrategy::SendStanza(scoped_ptr<buzz::XmlElement> stanza) {
  return core_->SendStanza(stanza.Pass());
}

std::string XmppSignalStrategy::GetNextId() {
  return base::Uint64ToString(base::RandUint64());
}

void XmppSignalStrategy::SetAuthInfo(const std::string& username,
                                     const std::string& auth_token) {
  core_->SetAuthInfo(username, auth_token);
}

}  // namespace remoting
