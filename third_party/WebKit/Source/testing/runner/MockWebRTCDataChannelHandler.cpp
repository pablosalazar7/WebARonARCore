/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "MockWebRTCDataChannelHandler.h"

#include "public/platform/WebRTCDataChannelHandlerClient.h"
#include "public/platform/WebRTCDataChannelInit.h"
#include "public/testing/WebTestDelegate.h"
#include <assert.h>

using namespace blink;

namespace WebTestRunner {

class DataChannelReadyStateTask : public WebMethodTask<MockWebRTCDataChannelHandler> {
public:
    DataChannelReadyStateTask(MockWebRTCDataChannelHandler* object, WebRTCDataChannelHandlerClient* dataChannelClient, WebRTCDataChannelHandlerClient::ReadyState state)
        : WebMethodTask<MockWebRTCDataChannelHandler>(object)
        , m_dataChannelClient(dataChannelClient)
        , m_state(state)
    {
    }

    virtual void runIfValid() OVERRIDE
    {
        m_dataChannelClient->didChangeReadyState(m_state);
    }

private:
    WebRTCDataChannelHandlerClient* m_dataChannelClient;
    WebRTCDataChannelHandlerClient::ReadyState m_state;
};

/////////////////////

MockWebRTCDataChannelHandler::MockWebRTCDataChannelHandler(WebString label, const WebRTCDataChannelInit& init, WebTestDelegate* delegate)
    : m_client(0)
    , m_label(label)
    , m_init(init)
    , m_delegate(delegate)
{
    m_reliable = (init.ordered && init.maxRetransmits == -1 && init.maxRetransmitTime == -1);
}

void MockWebRTCDataChannelHandler::setClient(WebRTCDataChannelHandlerClient* client)
{
    m_client = client;
    if (m_client)
        m_delegate->postTask(new DataChannelReadyStateTask(this, m_client, WebRTCDataChannelHandlerClient::ReadyStateOpen));
}

bool MockWebRTCDataChannelHandler::ordered() const
{
    return m_init.ordered;
}

unsigned short MockWebRTCDataChannelHandler::maxRetransmitTime() const
{
    return m_init.maxRetransmitTime;
}

unsigned short MockWebRTCDataChannelHandler::maxRetransmits() const
{
    return m_init.maxRetransmits;
}

WebString MockWebRTCDataChannelHandler::protocol() const
{
    return m_init.protocol;
}

bool MockWebRTCDataChannelHandler::negotiated() const
{
    return m_init.negotiated;
}

unsigned short MockWebRTCDataChannelHandler::id() const
{
    return m_init.id;
}

unsigned long MockWebRTCDataChannelHandler::bufferedAmount()
{
    return 0;
}

bool MockWebRTCDataChannelHandler::sendStringData(const WebString& data)
{
    assert(m_client);
    m_client->didReceiveStringData(data);
    return true;
}

bool MockWebRTCDataChannelHandler::sendRawData(const char* data, size_t size)
{
    assert(m_client);
    m_client->didReceiveRawData(data, size);
    return true;
}

void MockWebRTCDataChannelHandler::close()
{
    assert(m_client);
    m_delegate->postTask(new DataChannelReadyStateTask(this, m_client, WebRTCDataChannelHandlerClient::ReadyStateClosed));
}

}
