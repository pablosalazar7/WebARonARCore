/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WorkerGlobalScopeProxyProviderImpl.h"

#include "DatabaseClientImpl.h"
#include "LocalFileSystemClient.h"
#include "WebFrameImpl.h"
#include "WebPermissionClient.h"
#include "WebViewImpl.h"
#include "WorkerPermissionClient.h"
#include "core/dom/Document.h"
#include "core/inspector/ScriptCallStack.h"
#include "core/workers/Worker.h"
#include "core/workers/WorkerClients.h"
#include "core/workers/WorkerMessagingProxy.h"
#include "modules/serviceworkers/ServiceWorkerContainerClient.h"
#include "public/platform/WebServiceWorkerProvider.h"
#include "public/platform/WebString.h"
#include "public/web/WebFrameClient.h"
#include "public/web/WebWorkerPermissionClientProxy.h"

using namespace WebCore;

namespace blink {

WebCore::WorkerGlobalScopeProxy* WorkerGlobalScopeProxyProviderImpl::createWorkerGlobalScopeProxy(Worker* worker)
{
    if (worker->executionContext()->isDocument()) {
        Document* document = toDocument(worker->executionContext());
        WebFrameImpl* webFrame = WebFrameImpl::fromFrame(document->frame());
        OwnPtrWillBeRawPtr<WorkerClients> workerClients = WorkerClients::create();
        provideLocalFileSystemToWorker(workerClients.get(), LocalFileSystemClient::create());
        provideDatabaseClientToWorker(workerClients.get(), DatabaseClientImpl::create());
        providePermissionClientToWorker(workerClients.get(), adoptPtr(webFrame->client()->createWorkerPermissionClientProxy(webFrame)));
        provideServiceWorkerContainerClientToWorker(workerClients.get(), adoptPtr(webFrame->client()->createServiceWorkerProvider(webFrame)));
        return new WorkerMessagingProxy(worker, workerClients.release());
    }
    ASSERT_NOT_REACHED();
    return 0;
}

} // namespace blink
