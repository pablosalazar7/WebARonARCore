/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "cc/CCTiledLayerImpl.h"

#include "ContentLayerChromium.h"

#include <wtf/text/WTFString.h>

namespace WebCore {

class LayerTexture;

CCTiledLayerImpl::CCTiledLayerImpl(LayerChromium* owner, int id)
    : CCLayerImpl(owner, id)
    , m_tiler(0)
{
}

CCTiledLayerImpl::~CCTiledLayerImpl()
{
}

void CCTiledLayerImpl::draw()
{
    const IntRect& layerRect = visibleLayerRect();
    if (!layerRect.isEmpty())
        m_tiler->draw(layerRect, m_tilingTransform, drawOpacity());
}

void CCTiledLayerImpl::bindContentsTexture()
{
    // This function is only valid for single texture layers, e.g. masks.
    ASSERT(m_tiler);

    LayerTexture* texture = m_tiler->getSingleTexture();
    ASSERT(texture);

    texture->bindTexture();
}

void CCTiledLayerImpl::dumpLayerProperties(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    CCLayerImpl::dumpLayerProperties(ts, indent);
}

}

#endif // USE(ACCELERATED_COMPOSITING)
