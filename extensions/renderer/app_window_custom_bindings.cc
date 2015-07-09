// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/app_window_custom_bindings.h"

#include <string>

#include "base/command_line.h"
#include "content/public/child/v8_value_converter.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/switches.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "grit/extensions_renderer_resources.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8.h"

namespace extensions {

class DidCreateDocumentElementObserver : public content::RenderFrameObserver {
 public:
  DidCreateDocumentElementObserver(content::RenderFrame* frame,
                                   const ScriptContextSet* script_context_set)
      : content::RenderFrameObserver(frame),
        script_context_set_(script_context_set) {
    DCHECK(script_context_set_);
  }

  void DidCreateDocumentElement() override {
    blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
    // Don't attempt to inject the titlebar into iframes.
    if (web_frame->parent())
      return;
    ScriptContext* script_context = script_context_set_->GetByV8Context(
        web_frame->mainWorldScriptContext());
    if (!script_context)
      return;
    script_context->module_system()->CallModuleMethod(
        "injectAppTitlebar", "didCreateDocumentElement");
  }

 private:
  const ScriptContextSet* script_context_set_;

  DISALLOW_COPY_AND_ASSIGN(DidCreateDocumentElementObserver);
};

AppWindowCustomBindings::AppWindowCustomBindings(
    const ScriptContextSet* script_context_set,
    ScriptContext* context)
    : ObjectBackedNativeHandler(context),
      script_context_set_(script_context_set) {
  RouteFunction("GetFrame", base::Bind(&AppWindowCustomBindings::GetFrame,
                                       base::Unretained(this)));

  RouteFunction("GetWindowControlsHtmlTemplate",
      base::Bind(&AppWindowCustomBindings::GetWindowControlsHtmlTemplate,
                 base::Unretained(this)));
}

void AppWindowCustomBindings::GetFrame(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  // TODO(jeremya): convert this to IDL nocompile to get validation, and turn
  // these argument checks into CHECK().
  if (args.Length() != 2)
    return;

  if (!args[0]->IsInt32())
    return;

  if (!args[1]->IsBoolean())
    return;

  int frame_id = args[0]->Int32Value();

  bool inject_titlebar = args[1]->BooleanValue();

  if (frame_id == MSG_ROUTING_NONE)
    return;

  content::RenderFrame* app_frame =
      content::RenderFrame::FromRoutingID(frame_id);
  if (!app_frame)
    return;

  // TODO(jeremya): it doesn't really make sense to set the opener here, but we
  // need to make sure the security origin is set up before returning the DOM
  // reference. A better way to do this would be to have the browser pass the
  // opener through so opener_id is set in RenderViewImpl's constructor.
  content::RenderFrame* context_render_frame = context()->GetRenderFrame();
  if (!context_render_frame)
    return;

  if (inject_titlebar)
    new DidCreateDocumentElementObserver(app_frame, script_context_set_);

  blink::WebFrame* opener = context_render_frame->GetWebFrame();
  blink::WebLocalFrame* app_web_frame = app_frame->GetWebFrame();
  app_web_frame->setOpener(opener);
  content::RenderThread::Get()->Send(
      new ExtensionHostMsg_ResumeRequests(app_frame->GetRoutingID()));

  v8::Local<v8::Value> window =
      app_web_frame->mainWorldScriptContext()->Global();
  args.GetReturnValue().Set(window);
}

void AppWindowCustomBindings::GetWindowControlsHtmlTemplate(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 0);

  v8::Local<v8::Value> result = v8::String::Empty(args.GetIsolate());
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableAppWindowControls)) {
    base::StringValue value(
        ResourceBundle::GetSharedInstance()
            .GetRawDataResource(IDR_WINDOW_CONTROLS_TEMPLATE_HTML)
            .as_string());
    scoped_ptr<content::V8ValueConverter> converter(
        content::V8ValueConverter::create());
    result = converter->ToV8Value(&value, context()->v8_context());
  }
  args.GetReturnValue().Set(result);
}

}  // namespace extensions
