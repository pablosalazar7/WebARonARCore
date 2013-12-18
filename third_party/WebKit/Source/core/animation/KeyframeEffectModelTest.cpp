/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
#include "core/animation/KeyframeEffectModel.h"

#include "core/animation/AnimatableLength.h"
#include "core/animation/AnimatableUnknown.h"
#include "core/css/CSSPrimitiveValue.h"
#include <gtest/gtest.h>

using namespace WebCore;

namespace {

AnimatableValue* unknownAnimatableValue(double n)
{
    return AnimatableUnknown::create(CSSPrimitiveValue::create(n, CSSPrimitiveValue::CSS_UNKNOWN).get()).leakRef();
}

AnimatableValue* pixelAnimatableValue(double n)
{
    return AnimatableLength::create(CSSPrimitiveValue::create(n, CSSPrimitiveValue::CSS_PX).get()).leakRef();
}

KeyframeEffectModel::KeyframeVector keyframesAtZeroAndOne(AnimatableValue* zeroValue, AnimatableValue* oneValue)
{
    KeyframeEffectModel::KeyframeVector keyframes(2);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, zeroValue);
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(1.0);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, oneValue);
    return keyframes;
}

void expectDoubleValue(double expectedValue, PassRefPtr<AnimatableValue> value)
{
    ASSERT_TRUE(value->isLength() || value->isUnknown());

    double actualValue;
    if (value->isLength())
        actualValue = toCSSPrimitiveValue(toAnimatableLength(value.get())->toCSSValue().get())->getDoubleValue();
    else
        actualValue = toCSSPrimitiveValue(toAnimatableUnknown(value.get())->toCSSValue().get())->getDoubleValue();

    EXPECT_FLOAT_EQ(static_cast<float>(expectedValue), actualValue);
}

const AnimationEffect::CompositableValue* findValue(const AnimationEffect::CompositableValueList& values, CSSPropertyID id)
{
    for (size_t i = 0; i < values.size(); ++i) {
        const std::pair<CSSPropertyID, RefPtr<AnimationEffect::CompositableValue> >& value = values.at(i);
        if (value.first == id)
            return value.second.get();
    }
    return 0;
}


TEST(AnimationKeyframeEffectModel, BasicOperation)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(unknownAnimatableValue(3.0), unknownAnimatableValue(5.0));
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    OwnPtr<AnimationEffect::CompositableValueList> values = effect->sample(0, 0.6);
    ASSERT_EQ(1UL, values->size());
    EXPECT_EQ(CSSPropertyLeft, values->at(0).first);
    expectDoubleValue(5.0, values->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, CompositeReplaceNonInterpolable)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(unknownAnimatableValue(3.0), unknownAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(5.0, effect->sample(0, 0.6)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, CompositeReplace)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0 * 0.4 + 5.0 * 0.6, effect->sample(0, 0.6)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, CompositeAdd)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue((7.0 + 3.0) * 0.4 + (7.0 + 5.0) * 0.6, effect->sample(0, 0.6)->at(0).second->compositeOnto(pixelAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, ExtrapolateReplaceNonInterpolable)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(unknownAnimatableValue(3.0), unknownAnimatableValue(5.0));
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    expectDoubleValue(5.0, effect->sample(0, 1.6)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, ExtrapolateReplace)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    expectDoubleValue(3.0 * -0.6 + 5.0 * 1.6, effect->sample(0, 1.6)->at(0).second->compositeOnto(pixelAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, ExtrapolateAdd)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue((7.0 + 3.0) * -0.6 + (7.0 + 5.0) * 1.6, effect->sample(0, 1.6)->at(0).second->compositeOnto(pixelAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, ZeroKeyframes)
{
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(KeyframeEffectModel::KeyframeVector());
    EXPECT_TRUE(effect->sample(0, 0.5)->isEmpty());
}

TEST(AnimationKeyframeEffectModel, SingleKeyframeAtOffsetZero)
{
    KeyframeEffectModel::KeyframeVector keyframes(1);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0, effect->sample(0, 0.6)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, SingleKeyframeAtOffsetOne)
{
    KeyframeEffectModel::KeyframeVector keyframes(1);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(1.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(5.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(7.0 * 0.4 + 5.0 * 0.6, effect->sample(0, 0.6)->at(0).second->compositeOnto(pixelAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, MoreThanTwoKeyframes)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0));
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0));
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(4.0, effect->sample(0, 0.3)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    expectDoubleValue(5.0, effect->sample(0, 0.8)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, EndKeyframeOffsetsUnspecified)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0));
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0));
    keyframes[2] = Keyframe::create();
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0, effect->sample(0, 0.1)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    expectDoubleValue(4.0, effect->sample(0, 0.6)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    expectDoubleValue(5.0, effect->sample(0, 0.9)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, SampleOnKeyframe)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0));
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0));
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0, effect->sample(0, 0.0)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    expectDoubleValue(4.0, effect->sample(0, 0.5)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    expectDoubleValue(5.0, effect->sample(0, 1.0)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

// Note that this tests an implementation detail, not behaviour defined by the spec.
TEST(AnimationKeyframeEffectModel, SampleReturnsSameAnimatableValueInstance)
{
    AnimatableValue* threePixelsValue = unknownAnimatableValue(3.0);
    AnimatableValue* fourPixelsValue = unknownAnimatableValue(4.0);
    AnimatableValue* fivePixelsValue = unknownAnimatableValue(5.0);

    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, threePixelsValue);
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, fourPixelsValue);
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, fivePixelsValue);

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    EXPECT_EQ(threePixelsValue, effect->sample(0, 0.0)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    EXPECT_EQ(threePixelsValue, effect->sample(0, 0.1)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    EXPECT_EQ(fourPixelsValue, effect->sample(0, 0.4)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    EXPECT_EQ(fourPixelsValue, effect->sample(0, 0.5)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    EXPECT_EQ(fourPixelsValue, effect->sample(0, 0.6)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    EXPECT_EQ(fivePixelsValue, effect->sample(0, 0.9)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
    EXPECT_EQ(fivePixelsValue, effect->sample(0, 1.0)->at(0).second->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, MultipleKeyframesWithSameOffset)
{
    KeyframeEffectModel::KeyframeVector keyframes(7);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.1);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(1.0));
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.1);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(2.0));
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(0.5);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0));
    keyframes[3] = Keyframe::create();
    keyframes[3]->setOffset(0.5);
    keyframes[3]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0));
    keyframes[4] = Keyframe::create();
    keyframes[4]->setOffset(0.5);
    keyframes[4]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0));
    keyframes[5] = Keyframe::create();
    keyframes[5]->setOffset(0.9);
    keyframes[5]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(6.0));
    keyframes[6] = Keyframe::create();
    keyframes[6]->setOffset(0.9);
    keyframes[6]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(7.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(2.0, effect->sample(0, 0.0)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
    expectDoubleValue(2.0, effect->sample(0, 0.2)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
    expectDoubleValue(3.0, effect->sample(0, 0.4)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
    expectDoubleValue(5.0, effect->sample(0, 0.5)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
    expectDoubleValue(5.0, effect->sample(0, 0.6)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
    expectDoubleValue(6.0, effect->sample(0, 0.8)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
    expectDoubleValue(6.0, effect->sample(0, 1.0)->at(0).second->compositeOnto(unknownAnimatableValue(8.0)));
}

TEST(AnimationKeyframeEffectModel, PerKeyframeComposite)
{
    KeyframeEffectModel::KeyframeVector keyframes(2);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(3.0));
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(1.0);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(5.0));
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0 * 0.4 + (7.0 + 5.0) * 0.6, effect->sample(0, 0.6)->at(0).second->compositeOnto(pixelAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, MultipleProperties)
{
    KeyframeEffectModel::KeyframeVector keyframes(2);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0));
    keyframes[0]->setPropertyValue(CSSPropertyRight, unknownAnimatableValue(4.0));
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(1.0);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0));
    keyframes[1]->setPropertyValue(CSSPropertyRight, unknownAnimatableValue(6.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    OwnPtr<AnimationEffect::CompositableValueList> values = effect->sample(0, 0.6);
    EXPECT_EQ(2UL, values->size());
    const AnimationEffect::CompositableValue* leftValue = findValue(*values.get(), CSSPropertyLeft);
    ASSERT_TRUE(leftValue);
    expectDoubleValue(5.0, leftValue->compositeOnto(unknownAnimatableValue(7.0)));
    const AnimationEffect::CompositableValue* rightValue = findValue(*values.get(), CSSPropertyRight);
    ASSERT_TRUE(rightValue);
    expectDoubleValue(6.0, rightValue->compositeOnto(unknownAnimatableValue(7.0)));
}

TEST(AnimationKeyframeEffectModel, RecompositeCompositableValue)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    OwnPtr<AnimationEffect::CompositableValueList> values = effect->sample(0, 0.6);
    expectDoubleValue((7.0 + 3.0) * 0.4 + (7.0 + 5.0) * 0.6, values->at(0).second->compositeOnto(pixelAnimatableValue(7.0)));
    expectDoubleValue((9.0 + 3.0) * 0.4 + (9.0 + 5.0) * 0.6, values->at(0).second->compositeOnto(pixelAnimatableValue(9.0)));
}

TEST(AnimationKeyframeEffectModel, MultipleIterations)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(1.0), pixelAnimatableValue(3.0));
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(2.0, effect->sample(0, 0.5)->at(0).second->compositeOnto(unknownAnimatableValue(0.0)));
    expectDoubleValue(2.0, effect->sample(1, 0.5)->at(0).second->compositeOnto(unknownAnimatableValue(0.0)));
    expectDoubleValue(2.0, effect->sample(2, 0.5)->at(0).second->compositeOnto(unknownAnimatableValue(0.0)));
}

TEST(AnimationKeyframeEffectModel, DependsOnUnderlyingValue)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(1.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(1.0));
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(1.0));

    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    EXPECT_TRUE(effect->sample(0, 0)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_TRUE(effect->sample(0, 0.1)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_TRUE(effect->sample(0, 0.25)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_TRUE(effect->sample(0, 0.4)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_FALSE(effect->sample(0, 0.5)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_FALSE(effect->sample(0, 0.6)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_FALSE(effect->sample(0, 0.75)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_FALSE(effect->sample(0, 0.8)->at(0).second->dependsOnUnderlyingValue());
    EXPECT_FALSE(effect->sample(0, 1)->at(0).second->dependsOnUnderlyingValue());
}

TEST(AnimationKeyframeEffectModel, ToKeyframeEffectModel)
{
    KeyframeEffectModel::KeyframeVector keyframes(0);
    RefPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);

    AnimationEffect* baseEffect = effect.get();
    EXPECT_TRUE(toKeyframeEffectModel(baseEffect));
}

} // namespace
