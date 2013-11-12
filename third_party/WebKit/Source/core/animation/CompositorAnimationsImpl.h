/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
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

namespace WebCore {

class CompositorAnimationsKeyframeEffectHelper {
private:
    typedef Vector<std::pair<double, const AnimationEffect::CompositableValue*> > KeyframeValues;

    static PassOwnPtr<Vector<CSSPropertyID> > getProperties(const KeyframeAnimationEffect*);
    static PassOwnPtr<KeyframeValues> getKeyframeValuesForProperty(const KeyframeAnimationEffect*, CSSPropertyID, double zero, double scale, bool reverse = false);
    static PassOwnPtr<KeyframeValues> getKeyframeValuesForProperty(const KeyframeAnimationEffect::PropertySpecificKeyframeGroup*, double zero, double scale, bool reverse);

    friend class CompositorAnimationsImpl;
};

class CompositorAnimationsImpl {
private:


    static bool isCandidateForCompositor(const Keyframe&);
    static bool isCandidateForCompositor(const KeyframeAnimationEffect&);
    static bool isCandidateForCompositor(const Timing&, const KeyframeAnimationEffect::KeyframeVector&);
    static bool isCandidateForCompositor(const TimingFunction&, const KeyframeAnimationEffect::KeyframeVector&, double frameOffset = 0);
    static void getCompositorAnimations(const Timing&, const KeyframeAnimationEffect&, Vector<OwnPtr<blink::WebAnimation> >& animations);

    template<typename PlatformAnimationCurveType, typename PlatformAnimationKeyframeType>
    static void addKeyframesToCurve(PlatformAnimationCurveType&, const CompositorAnimationsKeyframeEffectHelper::KeyframeValues&, const TimingFunction&);

    friend class CompositorAnimations;
    friend class CoreAnimationCompositorAnimationsTest;
};

} // WebCore
