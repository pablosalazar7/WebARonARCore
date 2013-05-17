/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "core/page/DOMWindowPagePopup.h"

#include "core/page/DOMWindow.h"
#include "core/page/PagePopupController.h"

namespace WebCore {

DOMWindowPagePopup::DOMWindowPagePopup(PagePopupClient* popupClient)
    : m_controller(PagePopupController::create(popupClient))
{
    ASSERT(popupClient);
}

DOMWindowPagePopup::~DOMWindowPagePopup()
{
    m_controller->clearPagePopupClient();
}

const char* DOMWindowPagePopup::supplementName()
{
    return "DOMWindowPagePopup";
}

PagePopupController* DOMWindowPagePopup::pagePopupController(DOMWindow* window)
{
    DOMWindowPagePopup* supplement = static_cast<DOMWindowPagePopup*>(from(window, supplementName()));
    ASSERT(supplement);
    return supplement->m_controller.get();
}

void DOMWindowPagePopup::install(DOMWindow* window, PagePopupClient* popupClient)
{
    ASSERT(window);
    ASSERT(popupClient);
    provideTo(window, supplementName(), adoptPtr(new DOMWindowPagePopup(popupClient)));
}

void DOMWindowPagePopup::uninstall(DOMWindow* window)
{
    ASSERT(window);
    window->removeSupplement(supplementName());
}

}
