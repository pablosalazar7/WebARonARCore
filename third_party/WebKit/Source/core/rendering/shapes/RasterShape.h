/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RasterShape_h
#define RasterShape_h

#include "core/rendering/shapes/Shape.h"
#include "core/rendering/shapes/ShapeInterval.h"
#include "platform/geometry/FloatRect.h"
#include "wtf/Assertions.h"
#include "wtf/Vector.h"

namespace WebCore {

class RasterShapeIntervals {
public:
    RasterShapeIntervals(unsigned size, int offset = 0)
        : m_offset(offset)
    {
        m_intervalLists.resize(size);
    }

    const IntRect& bounds() const { return m_bounds; }
    bool isEmpty() const { return m_bounds.isEmpty(); }
    void appendInterval(int y, int x1, int x2);

    void getExcludedIntervals(int y1, int y2, IntShapeIntervals& result) const;

    PassOwnPtr<RasterShapeIntervals> computeShapeMarginIntervals(int shapeMargin) const;

    void buildBoundsPath(Path&) const;

private:
    int size() const { return m_intervalLists.size(); }
    int offset() const { return m_offset; }
    int minY() const { return -m_offset; }
    int maxY() const { return -m_offset + m_intervalLists.size(); }

    IntShapeIntervals& intervalsAt(int y)
    {
        ASSERT(y + m_offset >= 0 && static_cast<unsigned>(y + m_offset) < m_intervalLists.size());
        return m_intervalLists[y + m_offset];
    }

    const IntShapeIntervals& intervalsAt(int y) const
    {
        ASSERT(y + m_offset >= 0 && static_cast<unsigned>(y + m_offset) < m_intervalLists.size());
        return m_intervalLists[y + m_offset];
    }

    IntShapeInterval limitIntervalAt(int y) const
    {
        const IntShapeIntervals& intervals = intervalsAt(y);
        return intervals.size() ? IntShapeInterval(intervals[0].x1(), intervals.last().x2()) : IntShapeInterval();
    }

    bool contains(const IntRect&) const;
    bool getIntervalX1Values(int minY, int maxY, int minIntervalWidth, Vector<int>& result) const;
    void uniteMarginInterval(int y, const IntShapeInterval&);
    IntRect m_bounds;
    Vector<IntShapeIntervals> m_intervalLists;
    int m_offset;
};

class RasterShape FINAL : public Shape {
    WTF_MAKE_NONCOPYABLE(RasterShape);
public:
    RasterShape(PassOwnPtr<RasterShapeIntervals> intervals, const IntSize& marginRectSize)
        : m_intervals(intervals)
        , m_marginRectSize(marginRectSize)
    {
    }

    virtual LayoutRect shapeMarginLogicalBoundingBox() const OVERRIDE { return static_cast<LayoutRect>(marginIntervals().bounds()); }
    virtual bool isEmpty() const OVERRIDE { return m_intervals->isEmpty(); }
    virtual void getExcludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList&) const OVERRIDE;

private:
    const RasterShapeIntervals& marginIntervals() const;

    OwnPtr<RasterShapeIntervals> m_intervals;
    mutable OwnPtr<RasterShapeIntervals> m_marginIntervals;
    IntSize m_marginRectSize;
};

} // namespace WebCore

#endif // RasterShape_h
