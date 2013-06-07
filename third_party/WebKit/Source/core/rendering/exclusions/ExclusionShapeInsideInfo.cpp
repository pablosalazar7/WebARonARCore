/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "core/rendering/exclusions/ExclusionShapeInsideInfo.h"

#include "core/rendering/InlineIterator.h"
#include "core/rendering/RenderBlock.h"

namespace WebCore {

LineSegmentRange::LineSegmentRange(const InlineIterator& start, const InlineIterator& end)
    : start(start.root(), start.object(), start.offset())
    , end(end.root(), end.object(), end.offset())
    {
    }

bool ExclusionShapeInsideInfo::isEnabledFor(const RenderBlock* renderer)
{
    ExclusionShapeValue* shapeValue = renderer->style()->resolvedShapeInside();
    if (!shapeValue || shapeValue->type() != ExclusionShapeValue::Shape)
        return false;

    BasicShape* shape = shapeValue->shape();
    return shape && shape->type() != BasicShape::BasicShapeInsetRectangleType;
}

bool ExclusionShapeInsideInfo::adjustLogicalLineTop(float minSegmentWidth)
{
    const ExclusionShape* shape = computedShape();
    if (!shape || m_lineHeight <= 0 || logicalLineTop() > shapeLogicalBottom())
        return false;

    LayoutUnit newLineTop;
    if (shape->firstIncludedIntervalLogicalTop(m_shapeLineTop, LayoutSize(minSegmentWidth, m_lineHeight), newLineTop)) {
        if (newLineTop > m_shapeLineTop) {
            m_shapeLineTop = newLineTop;
            return true;
        }
    }

    return false;
}

}
