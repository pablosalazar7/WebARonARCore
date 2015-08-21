// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/chromoting_instance.h"

#include <string>
#include <vector>

#include <nacl_io/nacl_io.h>
#include <sys/mount.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "crypto/random.h"
#include "jingle/glue/thread_wrapper.h"
#include "net/socket/ssl_server_socket.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/dev/url_util_dev.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/private/uma_private.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/cpp/var_dictionary.h"
#include "remoting/base/constants.h"
#include "remoting/base/util.h"
#include "remoting/client/chromoting_client.h"
#include "remoting/client/normalizing_input_filter_cros.h"
#include "remoting/client/normalizing_input_filter_mac.h"
#include "remoting/client/plugin/delegating_signal_strategy.h"
#include "remoting/client/plugin/pepper_audio_player.h"
#include "remoting/client/plugin/pepper_mouse_locker.h"
#include "remoting/client/plugin/pepper_port_allocator.h"
#include "remoting/client/plugin/pepper_video_renderer_2d.h"
#include "remoting/client/plugin/pepper_video_renderer_3d.h"
#include "remoting/client/software_video_renderer.h"
#include "remoting/client/token_fetcher_proxy.h"
#include "remoting/protocol/connection_to_host.h"
#include "remoting/protocol/host_stub.h"
#include "remoting/protocol/libjingle_transport_factory.h"
#include "url/gurl.h"

namespace remoting {

namespace {

// Default DPI to assume for old clients that use notifyClientResolution.
const int kDefaultDPI = 96;

// Size of the random seed blob used to initialize RNG in libjingle. OpenSSL
// needs at least 32 bytes of entropy (see
// http://wiki.openssl.org/index.php/Random_Numbers), but stores 1039 bytes of
// state, so we initialize it with 1k or random data.
const int kRandomSeedSize = 1024;

// TODO(sergeyu): Ideally we should just pass ErrorCode to the webapp
// and let it handle it, but it would be hard to fix it now because
// client plugin and webapp versions may not be in sync. It should be
// easy to do after we are finished moving the client plugin to NaCl.
std::string ConnectionErrorToString(protocol::ErrorCode error) {
  // Values returned by this function must match the
  // remoting.ClientSession.Error enum in JS code.
  switch (error) {
    case protocol::OK:
      return "NONE";

    case protocol::PEER_IS_OFFLINE:
      return "HOST_IS_OFFLINE";

    case protocol::SESSION_REJECTED:
    case protocol::AUTHENTICATION_FAILED:
      return "SESSION_REJECTED";

    case protocol::INCOMPATIBLE_PROTOCOL:
      return "INCOMPATIBLE_PROTOCOL";

    case protocol::HOST_OVERLOAD:
      return "HOST_OVERLOAD";

    case protocol::CHANNEL_CONNECTION_ERROR:
    case protocol::SIGNALING_ERROR:
    case protocol::SIGNALING_TIMEOUT:
    case protocol::UNKNOWN_ERROR:
      return "NETWORK_FAILURE";
  }
  DLOG(FATAL) << "Unknown error code" << error;
  return std::string();
}

PP_Instance g_logging_instance = 0;
base::LazyInstance<base::Lock>::Leaky g_logging_lock =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

ChromotingInstance::ChromotingInstance(PP_Instance pp_instance)
    : pp::Instance(pp_instance),
      initialized_(false),
      plugin_task_runner_(new PluginThreadTaskRunner(&plugin_thread_delegate_)),
      context_(plugin_task_runner_.get()),
      input_tracker_(&mouse_input_filter_),
      touch_input_scaler_(&input_tracker_),
      key_mapper_(&touch_input_scaler_),
      input_handler_(&input_tracker_),
      cursor_setter_(this),
      empty_cursor_filter_(&cursor_setter_),
      text_input_controller_(this),
      use_async_pin_dialog_(false),
      weak_factory_(this) {
  // In NaCl global resources need to be initialized differently because they
  // are not shared with Chrome.
  thread_task_runner_handle_.reset(
      new base::ThreadTaskRunnerHandle(plugin_task_runner_));
  thread_wrapper_ =
      jingle_glue::JingleThreadWrapper::WrapTaskRunner(plugin_task_runner_);

  // Register a global log handler.
  ChromotingInstance::RegisterLogMessageHandler();

  nacl_io_init_ppapi(pp_instance, pp::Module::Get()->get_browser_interface());
  mount("", "/etc", "memfs", 0, "");
  mount("", "/usr", "memfs", 0, "");

  // Register for mouse, wheel and keyboard events.
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL);
  RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);

  // Disable the client-side IME in Chrome.
  text_input_controller_.SetTextInputType(PP_TEXTINPUT_TYPE_NONE);

  // Resister this instance to handle debug log messsages.
  RegisterLoggingInstance();

  // Initialize random seed for libjingle. It's necessary only with OpenSSL.
  char random_seed[kRandomSeedSize];
  crypto::RandBytes(random_seed, sizeof(random_seed));
  rtc::InitRandom(random_seed, sizeof(random_seed));

  // Send hello message.
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  PostLegacyJsonMessage("hello", data.Pass());
}

ChromotingInstance::~ChromotingInstance() {
  DCHECK(plugin_task_runner_->BelongsToCurrentThread());

  // Disconnect the client.
  Disconnect();

  // Unregister this instance so that debug log messages will no longer be sent
  // to it. This will stop all logging in all Chromoting instances.
  UnregisterLoggingInstance();

  plugin_task_runner_->Quit();

  // Ensure that nothing touches the plugin thread delegate after this point.
  plugin_task_runner_->DetachAndRunShutdownLoop();

  // Stopping the context shuts down all chromoting threads.
  context_.Stop();
}

bool ChromotingInstance::Init(uint32_t argc,
                              const char* argn[],
                              const char* argv[]) {
  CHECK(!initialized_);
  initialized_ = true;

  VLOG(1) << "Started ChromotingInstance::Init";

  // Start all the threads.
  context_.Start();

  return true;
}

void ChromotingInstance::HandleMessage(const pp::Var& message) {
  if (!message.is_string()) {
    LOG(ERROR) << "Received a message that is not a string.";
    return;
  }

  scoped_ptr<base::Value> json(base::JSONReader::DeprecatedRead(
      message.AsString(), base::JSON_ALLOW_TRAILING_COMMAS));
  base::DictionaryValue* message_dict = nullptr;
  std::string method;
  base::DictionaryValue* data = nullptr;
  if (!json.get() ||
      !json->GetAsDictionary(&message_dict) ||
      !message_dict->GetString("method", &method) ||
      !message_dict->GetDictionary("data", &data)) {
    LOG(ERROR) << "Received invalid message:" << message.AsString();
    return;
  }

  if (method == "connect") {
    HandleConnect(*data);
  } else if (method == "disconnect") {
    HandleDisconnect(*data);
  } else if (method == "incomingIq") {
    HandleOnIncomingIq(*data);
  } else if (method == "releaseAllKeys") {
    HandleReleaseAllKeys(*data);
  } else if (method == "injectKeyEvent") {
    HandleInjectKeyEvent(*data);
  } else if (method == "remapKey") {
    HandleRemapKey(*data);
  } else if (method == "trapKey") {
    HandleTrapKey(*data);
  } else if (method == "sendClipboardItem") {
    HandleSendClipboardItem(*data);
  } else if (method == "notifyClientResolution") {
    HandleNotifyClientResolution(*data);
  } else if (method == "videoControl") {
    HandleVideoControl(*data);
  } else if (method == "pauseAudio") {
    HandlePauseAudio(*data);
  } else if (method == "useAsyncPinDialog") {
    use_async_pin_dialog_ = true;
  } else if (method == "onPinFetched") {
    HandleOnPinFetched(*data);
  } else if (method == "onThirdPartyTokenFetched") {
    HandleOnThirdPartyTokenFetched(*data);
  } else if (method == "requestPairing") {
    HandleRequestPairing(*data);
  } else if (method == "extensionMessage") {
    HandleExtensionMessage(*data);
  } else if (method == "allowMouseLock") {
    HandleAllowMouseLockMessage();
  } else if (method == "sendMouseInputWhenUnfocused") {
    HandleSendMouseInputWhenUnfocused();
  } else if (method == "delegateLargeCursors") {
    HandleDelegateLargeCursors();
  } else if (method == "enableDebugRegion") {
    HandleEnableDebugRegion(*data);
  } else if (method == "enableTouchEvents") {
    HandleEnableTouchEvents(*data);
  }
}

void ChromotingInstance::DidChangeFocus(bool has_focus) {
  DCHECK(plugin_task_runner_->BelongsToCurrentThread());

  if (!IsConnected())
    return;

  input_handler_.DidChangeFocus(has_focus);
  if (mouse_locker_)
    mouse_locker_->DidChangeFocus(has_focus);
}

void ChromotingInstance::DidChangeView(const pp::View& view) {
  DCHECK(plugin_task_runner_->BelongsToCurrentThread());

  plugin_view_ = view;
  webrtc::DesktopSize size(
      webrtc::DesktopSize(view.GetRect().width(), view.GetRect().height()));
  mouse_input_filter_.set_input_size(size);
  touch_input_scaler_.set_input_size(size);

  if (video_renderer_)
    video_renderer_->OnViewChanged(view);
}

bool ChromotingInstance::HandleInputEvent(const pp::InputEvent& event) {
  DCHECK(plugin_task_runner_->BelongsToCurrentThread());

  if (!IsConnected())
    return false;

  return input_handler_.HandleInputEvent(event);
}

void ChromotingInstance::OnVideoDecodeError() {
  Disconnect();

  // Assume that the decoder failure was caused by the host not encoding video
  // correctly and report it as a protocol error.
  // TODO(sergeyu): Consider using a different error code in case the decoder
  // error was caused by some other problem.
  OnConnectionState(protocol::ConnectionToHost::FAILED,
                    protocol::INCOMPATIBLE_PROTOCOL);
}

void ChromotingInstance::OnVideoFirstFrameReceived() {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  PostLegacyJsonMessage("onFirstFrameReceived", data.Pass());
}

void ChromotingInstance::OnVideoSize(const webrtc::DesktopSize& size,
                                     const webrtc::DesktopVector& dpi) {
  mouse_input_filter_.set_output_size(size);
  touch_input_scaler_.set_output_size(size);

  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetInteger("width", size.width());
  data->SetInteger("height", size.height());
  if (dpi.x())
    data->SetInteger("x_dpi", dpi.x());
  if (dpi.y())
    data->SetInteger("y_dpi", dpi.y());
  PostLegacyJsonMessage("onDesktopSize", data.Pass());
}

void ChromotingInstance::OnVideoShape(const webrtc::DesktopRegion* shape) {
  if ((shape && desktop_shape_ && shape->Equals(*desktop_shape_)) ||
      (!shape && !desktop_shape_)) {
    return;
  }

  scoped_ptr<base::DictionaryValue> shape_message(new base::DictionaryValue());
  if (shape) {
    desktop_shape_ = make_scoped_ptr(new webrtc::DesktopRegion(*shape));
    scoped_ptr<base::ListValue> rects_value(new base::ListValue());
    for (webrtc::DesktopRegion::Iterator i(*shape); !i.IsAtEnd(); i.Advance()) {
      const webrtc::DesktopRect& rect = i.rect();
      scoped_ptr<base::ListValue> rect_value(new base::ListValue());
      rect_value->AppendInteger(rect.left());
      rect_value->AppendInteger(rect.top());
      rect_value->AppendInteger(rect.width());
      rect_value->AppendInteger(rect.height());
      rects_value->Append(rect_value.release());
    }
    shape_message->Set("rects", rects_value.release());
  }

  PostLegacyJsonMessage("onDesktopShape", shape_message.Pass());
}

void ChromotingInstance::OnVideoFrameDirtyRegion(
    const webrtc::DesktopRegion& dirty_region) {
  scoped_ptr<base::ListValue> rects_value(new base::ListValue());
  for (webrtc::DesktopRegion::Iterator i(dirty_region); !i.IsAtEnd();
       i.Advance()) {
    const webrtc::DesktopRect& rect = i.rect();
    scoped_ptr<base::ListValue> rect_value(new base::ListValue());
    rect_value->AppendInteger(rect.left());
    rect_value->AppendInteger(rect.top());
    rect_value->AppendInteger(rect.width());
    rect_value->AppendInteger(rect.height());
    rects_value->Append(rect_value.release());
  }

  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->Set("rects", rects_value.release());
  PostLegacyJsonMessage("onDebugRegion", data.Pass());
}

void ChromotingInstance::OnConnectionState(
    protocol::ConnectionToHost::State state,
    protocol::ErrorCode error) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("state", protocol::ConnectionToHost::StateToString(state));
  data->SetString("error", ConnectionErrorToString(error));
  PostLegacyJsonMessage("onConnectionStatus", data.Pass());
}

void ChromotingInstance::FetchThirdPartyToken(
    const GURL& token_url,
    const std::string& host_public_key,
    const std::string& scope,
    base::WeakPtr<TokenFetcherProxy> token_fetcher_proxy) {
  // Once the Session object calls this function, it won't continue the
  // authentication until the callback is called (or connection is canceled).
  // So, it's impossible to reach this with a callback already registered.
  DCHECK(!token_fetcher_proxy_.get());
  token_fetcher_proxy_ = token_fetcher_proxy;
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("tokenUrl", token_url.spec());
  data->SetString("hostPublicKey", host_public_key);
  data->SetString("scope", scope);
  PostLegacyJsonMessage("fetchThirdPartyToken", data.Pass());
}

void ChromotingInstance::OnConnectionReady(bool ready) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetBoolean("ready", ready);
  PostLegacyJsonMessage("onConnectionReady", data.Pass());
}

void ChromotingInstance::OnRouteChanged(const std::string& channel_name,
                                        const protocol::TransportRoute& route) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("channel", channel_name);
  data->SetString("connectionType",
                  protocol::TransportRoute::GetTypeString(route.type));
  PostLegacyJsonMessage("onRouteChanged", data.Pass());
}

void ChromotingInstance::SetCapabilities(const std::string& capabilities) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("capabilities", capabilities);
  PostLegacyJsonMessage("setCapabilities", data.Pass());
}

void ChromotingInstance::SetPairingResponse(
    const protocol::PairingResponse& pairing_response) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("clientId", pairing_response.client_id());
  data->SetString("sharedSecret", pairing_response.shared_secret());
  PostLegacyJsonMessage("pairingResponse", data.Pass());
}

void ChromotingInstance::DeliverHostMessage(
    const protocol::ExtensionMessage& message) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("type", message.type());
  data->SetString("data", message.data());
  PostLegacyJsonMessage("extensionMessage", data.Pass());
}

void ChromotingInstance::FetchSecretFromDialog(
    bool pairing_supported,
    const protocol::SecretFetchedCallback& secret_fetched_callback) {
  // Once the Session object calls this function, it won't continue the
  // authentication until the callback is called (or connection is canceled).
  // So, it's impossible to reach this with a callback already registered.
  DCHECK(secret_fetched_callback_.is_null());
  secret_fetched_callback_ = secret_fetched_callback;
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetBoolean("pairingSupported", pairing_supported);
  PostLegacyJsonMessage("fetchPin", data.Pass());
}

void ChromotingInstance::FetchSecretFromString(
    const std::string& shared_secret,
    bool pairing_supported,
    const protocol::SecretFetchedCallback& secret_fetched_callback) {
  secret_fetched_callback.Run(shared_secret);
}

protocol::ClipboardStub* ChromotingInstance::GetClipboardStub() {
  // TODO(sergeyu): Move clipboard handling to a separate class.
  // crbug.com/138108
  return this;
}

protocol::CursorShapeStub* ChromotingInstance::GetCursorShapeStub() {
  return &empty_cursor_filter_;
}

void ChromotingInstance::InjectClipboardEvent(
    const protocol::ClipboardEvent& event) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("mimeType", event.mime_type());
  data->SetString("item", event.data());
  PostLegacyJsonMessage("injectClipboardItem", data.Pass());
}

void ChromotingInstance::SetCursorShape(
    const protocol::CursorShapeInfo& cursor_shape) {
  // If the delegated cursor is empty then stop rendering a DOM cursor.
  if (IsCursorShapeEmpty(cursor_shape)) {
    PostChromotingMessage("unsetCursorShape", pp::VarDictionary());
    return;
  }

  // Cursor is not empty, so pass it to JS to render.
  const int kBytesPerPixel = sizeof(uint32_t);
  const size_t buffer_size =
      cursor_shape.height() * cursor_shape.width() * kBytesPerPixel;

  pp::VarArrayBuffer array_buffer(buffer_size);
  void* dst = array_buffer.Map();
  memcpy(dst, cursor_shape.data().data(), buffer_size);
  array_buffer.Unmap();

  pp::VarDictionary dictionary;
  dictionary.Set(pp::Var("width"), cursor_shape.width());
  dictionary.Set(pp::Var("height"), cursor_shape.height());
  dictionary.Set(pp::Var("hotspotX"), cursor_shape.hotspot_x());
  dictionary.Set(pp::Var("hotspotY"), cursor_shape.hotspot_y());
  dictionary.Set(pp::Var("data"), array_buffer);
  PostChromotingMessage("setCursorShape", dictionary);
}

void ChromotingInstance::HandleConnect(const base::DictionaryValue& data) {
  std::string local_jid;
  std::string host_jid;
  std::string host_public_key;
  std::string authentication_tag;
  if (!data.GetString("hostJid", &host_jid) ||
      !data.GetString("hostPublicKey", &host_public_key) ||
      !data.GetString("localJid", &local_jid) ||
      !data.GetString("authenticationTag", &authentication_tag)) {
    LOG(ERROR) << "Invalid connect() data.";
    return;
  }

  std::string client_pairing_id;
  data.GetString("clientPairingId", &client_pairing_id);
  std::string client_paired_secret;
  data.GetString("clientPairedSecret", &client_paired_secret);

  protocol::FetchSecretCallback fetch_secret_callback;
  if (use_async_pin_dialog_) {
    fetch_secret_callback = base::Bind(
        &ChromotingInstance::FetchSecretFromDialog, weak_factory_.GetWeakPtr());
  } else {
    std::string shared_secret;
    if (!data.GetString("sharedSecret", &shared_secret)) {
      LOG(ERROR) << "sharedSecret not specified in connect().";
      return;
    }
    fetch_secret_callback =
        base::Bind(&ChromotingInstance::FetchSecretFromString, shared_secret);
  }

  // Read the list of capabilities, if any.
  std::string capabilities;
  if (data.HasKey("capabilities")) {
    if (!data.GetString("capabilities", &capabilities)) {
      LOG(ERROR) << "Invalid connect() data.";
      return;
    }
  }

  // Read and parse list of experiments.
  std::string experiments;
  std::vector<std::string> experiments_list;
  if (data.GetString("experiments", &experiments)) {
    experiments_list = base::SplitString(
        experiments, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  }

  VLOG(0) << "Connecting to " << host_jid
          << ". Local jid: " << local_jid << ".";

  std::string key_filter;
  if (!data.GetString("keyFilter", &key_filter)) {
    NOTREACHED();
    normalizing_input_filter_.reset(new protocol::InputFilter(&key_mapper_));
  } else if (key_filter == "mac") {
    normalizing_input_filter_.reset(
        new NormalizingInputFilterMac(&key_mapper_));
  } else if (key_filter == "cros") {
    normalizing_input_filter_.reset(
        new NormalizingInputFilterCros(&key_mapper_));
  } else {
    DCHECK(key_filter.empty());
    normalizing_input_filter_.reset(new protocol::InputFilter(&key_mapper_));
  }
  input_handler_.set_input_stub(normalizing_input_filter_.get());

  // Try initializing 3D video renderer.
  video_renderer_.reset(new PepperVideoRenderer3D());
  if (!video_renderer_->Initialize(this, context_, this))
    video_renderer_.reset();

  // If we didn't initialize 3D renderer then use the 2D renderer.
  if (!video_renderer_) {
    LOG(WARNING)
        << "Failed to initialize 3D renderer. Using 2D renderer instead.";
    video_renderer_.reset(new PepperVideoRenderer2D());
    if (!video_renderer_->Initialize(this, context_, this))
      video_renderer_.reset();
  }

  CHECK(video_renderer_);

  video_renderer_->GetStats()->SetUpdateUmaCallbacks(
      base::Bind(&ChromotingInstance::UpdateUmaCustomHistogram,
                 weak_factory_.GetWeakPtr(), true),
      base::Bind(&ChromotingInstance::UpdateUmaCustomHistogram,
                 weak_factory_.GetWeakPtr(), false),
      base::Bind(&ChromotingInstance::UpdateUmaEnumHistogram,
                 weak_factory_.GetWeakPtr()));

  if (!plugin_view_.is_null())
    video_renderer_->OnViewChanged(plugin_view_);

  scoped_ptr<AudioPlayer> audio_player(new PepperAudioPlayer(this));
  client_.reset(new ChromotingClient(&context_, this, video_renderer_.get(),
                                     audio_player.Pass()));

  // Connect the input pipeline to the protocol stub & initialize components.
  mouse_input_filter_.set_input_stub(client_->input_stub());
  if (!plugin_view_.is_null()) {
    webrtc::DesktopSize size(plugin_view_.GetRect().width(),
                             plugin_view_.GetRect().height());
    mouse_input_filter_.set_input_size(size);
    touch_input_scaler_.set_input_size(size);
  }

  // Setup the signal strategy.
  signal_strategy_.reset(new DelegatingSignalStrategy(
      local_jid, base::Bind(&ChromotingInstance::SendOutgoingIq,
                            weak_factory_.GetWeakPtr())));

  // Create TransportFactory.
  scoped_ptr<protocol::TransportFactory> transport_factory(
      new protocol::LibjingleTransportFactory(
          signal_strategy_.get(), PepperPortAllocator::Create(this).Pass(),
          protocol::NetworkSettings(
              protocol::NetworkSettings::NAT_TRAVERSAL_FULL),
          protocol::TransportRole::CLIENT));

  // Create Authenticator.
  scoped_ptr<protocol::ThirdPartyClientAuthenticator::TokenFetcher>
      token_fetcher(new TokenFetcherProxy(
          base::Bind(&ChromotingInstance::FetchThirdPartyToken,
                     weak_factory_.GetWeakPtr()),
          host_public_key));

  std::vector<protocol::AuthenticationMethod> auth_methods;
  auth_methods.push_back(protocol::AuthenticationMethod::ThirdParty());
  auth_methods.push_back(protocol::AuthenticationMethod::Spake2Pair());
  auth_methods.push_back(protocol::AuthenticationMethod::Spake2(
      protocol::AuthenticationMethod::HMAC_SHA256));
  auth_methods.push_back(protocol::AuthenticationMethod::Spake2(
      protocol::AuthenticationMethod::NONE));

  scoped_ptr<protocol::Authenticator> authenticator(
      new protocol::NegotiatingClientAuthenticator(
          client_pairing_id, client_paired_secret, authentication_tag,
          fetch_secret_callback, token_fetcher.Pass(), auth_methods));

  scoped_ptr<protocol::CandidateSessionConfig> config =
      protocol::CandidateSessionConfig::CreateDefault();
  if (std::find(experiments_list.begin(), experiments_list.end(), "vp9") !=
      experiments_list.end()) {
    config->set_vp9_experiment_enabled(true);
  }
  if (std::find(experiments_list.begin(), experiments_list.end(), "quic") !=
      experiments_list.end()) {
    config->PreferTransport(protocol::ChannelConfig::TRANSPORT_QUIC_STREAM);
  }
  client_->set_protocol_config(config.Pass());

  // Kick off the connection.
  client_->Start(signal_strategy_.get(), authenticator.Pass(),
                 transport_factory.Pass(), host_jid, capabilities);

  // Start timer that periodically sends perf stats.
  plugin_task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&ChromotingInstance::SendPerfStats,
                            weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(
          ChromotingStats::kStatsUpdateFrequencyInSeconds));
}

void ChromotingInstance::HandleDisconnect(const base::DictionaryValue& data) {
  DCHECK(plugin_task_runner_->BelongsToCurrentThread());
  Disconnect();
}

void ChromotingInstance::HandleOnIncomingIq(const base::DictionaryValue& data) {
  std::string iq;
  if (!data.GetString("iq", &iq)) {
    LOG(ERROR) << "Invalid incomingIq() data.";
    return;
  }

  // Just ignore the message if it's received before Connect() is called. It's
  // likely to be a leftover from a previous session, so it's safe to ignore it.
  if (signal_strategy_)
    signal_strategy_->OnIncomingMessage(iq);
}

void ChromotingInstance::HandleReleaseAllKeys(
    const base::DictionaryValue& data) {
  if (IsConnected())
    input_tracker_.ReleaseAll();
}

void ChromotingInstance::HandleInjectKeyEvent(
      const base::DictionaryValue& data) {
  int usb_keycode = 0;
  bool is_pressed = false;
  if (!data.GetInteger("usbKeycode", &usb_keycode) ||
      !data.GetBoolean("pressed", &is_pressed)) {
    LOG(ERROR) << "Invalid injectKeyEvent.";
    return;
  }

  protocol::KeyEvent event;
  event.set_usb_keycode(usb_keycode);
  event.set_pressed(is_pressed);

  // Inject after the KeyEventMapper, so the event won't get mapped or trapped.
  if (IsConnected())
    input_tracker_.InjectKeyEvent(event);
}

void ChromotingInstance::HandleRemapKey(const base::DictionaryValue& data) {
  int from_keycode = 0;
  int to_keycode = 0;
  if (!data.GetInteger("fromKeycode", &from_keycode) ||
      !data.GetInteger("toKeycode", &to_keycode)) {
    LOG(ERROR) << "Invalid remapKey.";
    return;
  }

  key_mapper_.RemapKey(from_keycode, to_keycode);
}

void ChromotingInstance::HandleTrapKey(const base::DictionaryValue& data) {
  int keycode = 0;
  bool trap = false;
  if (!data.GetInteger("keycode", &keycode) ||
      !data.GetBoolean("trap", &trap)) {
    LOG(ERROR) << "Invalid trapKey.";
    return;
  }

  key_mapper_.TrapKey(keycode, trap);
}

void ChromotingInstance::HandleSendClipboardItem(
    const base::DictionaryValue& data) {
  std::string mime_type;
  std::string item;
  if (!data.GetString("mimeType", &mime_type) ||
      !data.GetString("item", &item)) {
    LOG(ERROR) << "Invalid sendClipboardItem data.";
    return;
  }
  if (!IsConnected()) {
    return;
  }
  protocol::ClipboardEvent event;
  event.set_mime_type(mime_type);
  event.set_data(item);
  client_->clipboard_forwarder()->InjectClipboardEvent(event);
}

void ChromotingInstance::HandleNotifyClientResolution(
    const base::DictionaryValue& data) {
  int width = 0;
  int height = 0;
  int x_dpi = kDefaultDPI;
  int y_dpi = kDefaultDPI;
  if (!data.GetInteger("width", &width) ||
      !data.GetInteger("height", &height) ||
      !data.GetInteger("x_dpi", &x_dpi) ||
      !data.GetInteger("y_dpi", &y_dpi) ||
      width <= 0 || height <= 0 ||
      x_dpi <= 0 || y_dpi <= 0) {
    LOG(ERROR) << "Invalid notifyClientResolution.";
    return;
  }

  if (!IsConnected()) {
    return;
  }

  protocol::ClientResolution client_resolution;
  client_resolution.set_width(width);
  client_resolution.set_height(height);
  client_resolution.set_x_dpi(x_dpi);
  client_resolution.set_y_dpi(y_dpi);

  // Include the legacy width & height in DIPs for use by older hosts.
  client_resolution.set_dips_width((width * kDefaultDPI) / x_dpi);
  client_resolution.set_dips_height((height * kDefaultDPI) / y_dpi);

  client_->host_stub()->NotifyClientResolution(client_resolution);
}

void ChromotingInstance::HandleVideoControl(const base::DictionaryValue& data) {
  protocol::VideoControl video_control;
  bool pause_video = false;
  if (data.GetBoolean("pause", &pause_video)) {
    video_control.set_enable(!pause_video);
  }
  bool lossless_encode = false;
  if (data.GetBoolean("losslessEncode", &lossless_encode)) {
    video_control.set_lossless_encode(lossless_encode);
  }
  bool lossless_color = false;
  if (data.GetBoolean("losslessColor", &lossless_color)) {
    video_control.set_lossless_color(lossless_color);
  }
  if (!IsConnected()) {
    return;
  }
  client_->host_stub()->ControlVideo(video_control);
}

void ChromotingInstance::HandlePauseAudio(const base::DictionaryValue& data) {
  bool pause = false;
  if (!data.GetBoolean("pause", &pause)) {
    LOG(ERROR) << "Invalid pauseAudio.";
    return;
  }
  if (!IsConnected()) {
    return;
  }
  protocol::AudioControl audio_control;
  audio_control.set_enable(!pause);
  client_->host_stub()->ControlAudio(audio_control);
}
void ChromotingInstance::HandleOnPinFetched(const base::DictionaryValue& data) {
  std::string pin;
  if (!data.GetString("pin", &pin)) {
    LOG(ERROR) << "Invalid onPinFetched.";
    return;
  }
  if (!secret_fetched_callback_.is_null()) {
    base::ResetAndReturn(&secret_fetched_callback_).Run(pin);
  } else {
    LOG(WARNING) << "Ignored OnPinFetched received without a pending fetch.";
  }
}

void ChromotingInstance::HandleOnThirdPartyTokenFetched(
    const base::DictionaryValue& data) {
  std::string token;
  std::string shared_secret;
  if (!data.GetString("token", &token) ||
      !data.GetString("sharedSecret", &shared_secret)) {
    LOG(ERROR) << "Invalid onThirdPartyTokenFetched data.";
    return;
  }
  if (token_fetcher_proxy_.get()) {
    token_fetcher_proxy_->OnTokenFetched(token, shared_secret);
    token_fetcher_proxy_.reset();
  } else {
    LOG(WARNING) << "Ignored OnThirdPartyTokenFetched without a pending fetch.";
  }
}

void ChromotingInstance::HandleRequestPairing(
    const base::DictionaryValue& data) {
  std::string client_name;
  if (!data.GetString("clientName", &client_name)) {
    LOG(ERROR) << "Invalid requestPairing";
    return;
  }
  if (!IsConnected()) {
    return;
  }
  protocol::PairingRequest pairing_request;
  pairing_request.set_client_name(client_name);
  client_->host_stub()->RequestPairing(pairing_request);
}

void ChromotingInstance::HandleExtensionMessage(
    const base::DictionaryValue& data) {
  std::string type;
  std::string message_data;
  if (!data.GetString("type", &type) ||
      !data.GetString("data", &message_data)) {
    LOG(ERROR) << "Invalid extensionMessage.";
    return;
  }
  if (!IsConnected()) {
    return;
  }
  protocol::ExtensionMessage message;
  message.set_type(type);
  message.set_data(message_data);
  client_->host_stub()->DeliverClientMessage(message);
}

void ChromotingInstance::HandleAllowMouseLockMessage() {
  // Create the mouse lock handler and route cursor shape messages through it.
  mouse_locker_.reset(new PepperMouseLocker(
      this, base::Bind(&PepperInputHandler::set_send_mouse_move_deltas,
                       base::Unretained(&input_handler_)),
      &cursor_setter_));
  empty_cursor_filter_.set_cursor_stub(mouse_locker_.get());
}

void ChromotingInstance::HandleSendMouseInputWhenUnfocused() {
  input_handler_.set_send_mouse_input_when_unfocused(true);
}

void ChromotingInstance::HandleDelegateLargeCursors() {
  cursor_setter_.set_delegate_stub(this);
}

void ChromotingInstance::HandleEnableDebugRegion(
    const base::DictionaryValue& data) {
  bool enable = false;
  if (!data.GetBoolean("enable", &enable)) {
    LOG(ERROR) << "Invalid enableDebugRegion.";
    return;
  }

  video_renderer_->EnableDebugDirtyRegion(enable);
}

void ChromotingInstance::HandleEnableTouchEvents(
    const base::DictionaryValue& data) {
  bool enable = false;
  if (!data.GetBoolean("enable", &enable)) {
    LOG(ERROR) << "Invalid handleTouchEvents.";
    return;
  }

  if (enable) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_TOUCH);
  } else {
    ClearInputEventRequest(PP_INPUTEVENT_CLASS_TOUCH);
  }
}

void ChromotingInstance::Disconnect() {
  DCHECK(plugin_task_runner_->BelongsToCurrentThread());

  VLOG(0) << "Disconnecting from host.";

  // Disconnect the input pipeline and teardown the connection.
  mouse_input_filter_.set_input_stub(nullptr);
  client_.reset();
  video_renderer_.reset();
}

void ChromotingInstance::PostChromotingMessage(const std::string& method,
                                               const pp::VarDictionary& data) {
  pp::VarDictionary message;
  message.Set(pp::Var("method"), pp::Var(method));
  message.Set(pp::Var("data"), data);
  PostMessage(message);
}

void ChromotingInstance::PostLegacyJsonMessage(
    const std::string& method,
    scoped_ptr<base::DictionaryValue> data) {
  base::DictionaryValue message;
  message.SetString("method", method);
  message.Set("data", data.release());

  std::string message_json;
  base::JSONWriter::Write(message, &message_json);
  PostMessage(pp::Var(message_json));
}

void ChromotingInstance::SendTrappedKey(uint32 usb_keycode, bool pressed) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetInteger("usbKeycode", usb_keycode);
  data->SetBoolean("pressed", pressed);
  PostLegacyJsonMessage("trappedKeyEvent", data.Pass());
}

void ChromotingInstance::SendOutgoingIq(const std::string& iq) {
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  data->SetString("iq", iq);
  PostLegacyJsonMessage("sendOutgoingIq", data.Pass());
}

void ChromotingInstance::SendPerfStats() {
  if (!video_renderer_.get()) {
    return;
  }

  plugin_task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&ChromotingInstance::SendPerfStats,
                            weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(
          ChromotingStats::kStatsUpdateFrequencyInSeconds));

  // Fetch performance stats from the VideoRenderer and send them to the client
  // for display to users.
  scoped_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  ChromotingStats* stats = video_renderer_->GetStats();
  data->SetDouble("videoBandwidth", stats->video_bandwidth());
  data->SetDouble("videoFrameRate", stats->video_frame_rate());
  data->SetDouble("captureLatency", stats->video_capture_ms());
  data->SetDouble("encodeLatency", stats->video_encode_ms());
  data->SetDouble("decodeLatency", stats->video_decode_ms());
  data->SetDouble("renderLatency", stats->video_paint_ms());
  data->SetDouble("roundtripLatency", stats->round_trip_ms());
  PostLegacyJsonMessage("onPerfStats", data.Pass());

  // Record the video frame-rate, packet-rate and bandwidth stats to UMA.
  // TODO(anandc): Create a timer in ChromotingStats to do this work.
  // See http://crbug/508602.
  stats->UploadRateStatsToUma();
}

// static
void ChromotingInstance::RegisterLogMessageHandler() {
  base::AutoLock lock(g_logging_lock.Get());

  // Set up log message handler.
  // This is not thread-safe so we need it within our lock.
  logging::SetLogMessageHandler(&LogToUI);
}

void ChromotingInstance::RegisterLoggingInstance() {
  base::AutoLock lock(g_logging_lock.Get());
  g_logging_instance = pp_instance();
}

void ChromotingInstance::UnregisterLoggingInstance() {
  base::AutoLock lock(g_logging_lock.Get());

  // Don't unregister unless we're the currently registered instance.
  if (pp_instance() != g_logging_instance)
    return;

  // Unregister this instance for logging.
  g_logging_instance = 0;
}

// static
bool ChromotingInstance::LogToUI(int severity, const char* file, int line,
                                 size_t message_start,
                                 const std::string& str) {
  PP_LogLevel log_level = PP_LOGLEVEL_ERROR;
  switch(severity) {
    case logging::LOG_INFO:
      log_level = PP_LOGLEVEL_TIP;
      break;
    case logging::LOG_WARNING:
      log_level = PP_LOGLEVEL_WARNING;
      break;
    case logging::LOG_ERROR:
    case logging::LOG_FATAL:
      log_level = PP_LOGLEVEL_ERROR;
      break;
  }

  PP_Instance pp_instance = 0;
  {
    base::AutoLock lock(g_logging_lock.Get());
    if (g_logging_instance)
      pp_instance = g_logging_instance;
  }
  if (pp_instance) {
    const PPB_Console* console = reinterpret_cast<const PPB_Console*>(
        pp::Module::Get()->GetBrowserInterface(PPB_CONSOLE_INTERFACE));
    if (console)
      console->Log(pp_instance, log_level, pp::Var(str).pp_var());
  }

  // If this is a fatal message the log handler is going to crash after this
  // function returns. In that case sleep for 1 second, Otherwise the plugin
  // may crash before the message is delivered to the console.
  if (severity == logging::LOG_FATAL)
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));

  return false;
}

bool ChromotingInstance::IsConnected() {
  return client_ &&
         (client_->connection_state() == protocol::ConnectionToHost::CONNECTED);
}

void ChromotingInstance::UpdateUmaEnumHistogram(
    const std::string& histogram_name,
    int64 value,
    int histogram_max) {
  pp::UMAPrivate uma(this);
  uma.HistogramEnumeration(histogram_name, value, histogram_max);
}

void ChromotingInstance::UpdateUmaCustomHistogram(
    bool is_custom_counts_histogram,
    const std::string& histogram_name,
    int64 value,
    int histogram_min,
    int histogram_max,
    int histogram_buckets) {
  pp::UMAPrivate uma(this);

  if (is_custom_counts_histogram)
    uma.HistogramCustomCounts(histogram_name, value, histogram_min,
                              histogram_max, histogram_buckets);
  else
    uma.HistogramCustomTimes(histogram_name, value, histogram_min,
                             histogram_max, histogram_buckets);
}

}  // namespace remoting
