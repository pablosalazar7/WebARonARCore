/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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
#include "core/animation/AnimatableLengthPoint3D.h"

namespace WebCore {

PassRefPtrWillBeRawPtr<AnimatableValue> AnimatableLengthPoint3D::interpolateTo(const AnimatableValue* value, double fraction) const
{
    const AnimatableLengthPoint3D* lengthPoint = toAnimatableLengthPoint3D(value);
    return AnimatableLengthPoint3D::create(
        AnimatableValue::interpolate(this->x(), lengthPoint->x(), fraction),
        AnimatableValue::interpolate(this->y(), lengthPoint->y(), fraction),
        AnimatableValue::interpolate(this->z(), lengthPoint->z(), fraction));
}

PassRefPtrWillBeRawPtr<AnimatableValue> AnimatableLengthPoint3D::addWith(const AnimatableValue* value) const
{
    const AnimatableLengthPoint3D* lengthPoint = toAnimatableLengthPoint3D(value);
    return AnimatableLengthPoint3D::create(
        AnimatableValue::add(this->x(), lengthPoint->x()),
        AnimatableValue::add(this->y(), lengthPoint->y()),
        AnimatableValue::add(this->z(), lengthPoint->z()));
}

bool AnimatableLengthPoint3D::equalTo(const AnimatableValue* value) const
{
    const AnimatableLengthPoint3D* lengthPoint = toAnimatableLengthPoint3D(value);
    return x()->equals(lengthPoint->x()) && y()->equals(lengthPoint->y()) && z()->equals(lengthPoint->z());
}

void AnimatableLengthPoint3D::trace(Visitor* visitor)
{
    visitor->trace(m_x);
    visitor->trace(m_y);
    visitor->trace(m_z);
}

}
