/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "platform/graphics/filters/SourceAlpha.h"

#include "platform/graphics/Color.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/filters/Filter.h"
#include "platform/text/TextStream.h"
#include "third_party/skia/include/effects/SkColorFilterImageFilter.h"
#include "third_party/skia/include/effects/SkColorMatrixFilter.h"
#include "wtf/StdLibExtras.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

PassRefPtr<SourceAlpha> SourceAlpha::create(Filter* filter)
{
    return adoptRef(new SourceAlpha(filter));
}

const AtomicString& SourceAlpha::effectName()
{
    DEFINE_STATIC_LOCAL(const AtomicString, s_effectName, ("SourceAlpha", AtomicString::ConstructFromLiteral));
    return s_effectName;
}

static FloatRect sourceImageRectInResolution(Filter* filter)
{
    FloatRect srcRect = filter->sourceImageRect();
    srcRect.scale(filter->filterResolution().width(), filter->filterResolution().height());
    return srcRect;
}

FloatRect SourceAlpha::determineAbsolutePaintRect(const FloatRect& requestedRect)
{
    FloatRect srcRect = sourceImageRectInResolution(filter());
    srcRect.intersect(requestedRect);
    addAbsolutePaintRect(srcRect);
    return srcRect;
}

void SourceAlpha::applySoftware()
{
    ImageBuffer* resultImage = createImageBufferResult();
    Filter* filter = this->filter();
    if (!resultImage || !filter->sourceImage())
        return;

    setIsAlphaImage(true);

    FloatRect imageRect(FloatPoint(), absolutePaintRect().size());
    GraphicsContext* filterContext = resultImage->context();
    filterContext->fillRect(imageRect, Color::black);

    FloatRect srcRect = sourceImageRectInResolution(filter);
    IntRect srcIntRect = enclosingIntRect(srcRect);
    filterContext->drawImageBuffer(
        filter->sourceImage(),
        IntPoint(srcIntRect.location() - absolutePaintRect().location()),
        CompositeDestinationIn);
}

PassRefPtr<SkImageFilter> SourceAlpha::createImageFilter(SkiaImageFilterBuilder* builder)
{
    SkScalar matrix[20] = {
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, SK_Scalar1, 0
    };
    RefPtr<SkColorFilter> colorFilter(adoptRef(new SkColorMatrixFilter(matrix)));
    return adoptRef(SkColorFilterImageFilter::Create(colorFilter.get()));
}

TextStream& SourceAlpha::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[SourceAlpha]\n";
    return ts;
}

} // namespace WebCore
