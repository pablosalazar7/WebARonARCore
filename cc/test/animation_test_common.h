// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_ANIMATION_TEST_COMMON_H_
#define CC_TEST_ANIMATION_TEST_COMMON_H_

#include "cc/animation/animation.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/layer_animation_controller.h"
#include "cc/animation/layer_animation_value_observer.h"
#include "cc/animation/layer_animation_value_provider.h"
#include "cc/output/filter_operations.h"
#include "cc/test/geometry_test_utils.h"

namespace cc {
class LayerImpl;
class Layer;
}

namespace cc {

class FakeFloatAnimationCurve : public FloatAnimationCurve {
 public:
  FakeFloatAnimationCurve();
  explicit FakeFloatAnimationCurve(double duration);
  virtual ~FakeFloatAnimationCurve();

  virtual double Duration() const override;
  virtual float GetValue(double now) const override;
  virtual scoped_ptr<AnimationCurve> Clone() const override;

 private:
  double duration_;
};

class FakeTransformTransition : public TransformAnimationCurve {
 public:
  explicit FakeTransformTransition(double duration);
  virtual ~FakeTransformTransition();

  virtual double Duration() const override;
  virtual gfx::Transform GetValue(double time) const override;
  virtual bool AnimatedBoundsForBox(const gfx::BoxF& box,
                                    gfx::BoxF* bounds) const override;
  virtual bool AffectsScale() const override;
  virtual bool IsTranslation() const override;
  virtual bool MaximumTargetScale(bool forward_direction,
                                  float* max_scale) const override;

  virtual scoped_ptr<AnimationCurve> Clone() const override;

 private:
  double duration_;
};

class FakeFloatTransition : public FloatAnimationCurve {
 public:
  FakeFloatTransition(double duration, float from, float to);
  virtual ~FakeFloatTransition();

  virtual double Duration() const override;
  virtual float GetValue(double time) const override;

  virtual scoped_ptr<AnimationCurve> Clone() const override;

 private:
  double duration_;
  float from_;
  float to_;
};

class FakeLayerAnimationValueObserver : public LayerAnimationValueObserver {
 public:
  FakeLayerAnimationValueObserver();
  virtual ~FakeLayerAnimationValueObserver();

  // LayerAnimationValueObserver implementation
  virtual void OnFilterAnimated(const FilterOperations& filters) override;
  virtual void OnOpacityAnimated(float opacity) override;
  virtual void OnTransformAnimated(const gfx::Transform& transform) override;
  virtual void OnScrollOffsetAnimated(
      const gfx::ScrollOffset& scroll_offset) override;
  virtual void OnAnimationWaitingForDeletion() override;
  virtual bool IsActive() const override;

  const FilterOperations& filters() const { return filters_; }
  float opacity() const  { return opacity_; }
  const gfx::Transform& transform() const { return transform_; }
  gfx::ScrollOffset scroll_offset() { return scroll_offset_; }

  bool animation_waiting_for_deletion() {
    return animation_waiting_for_deletion_;
  }

 private:
  FilterOperations filters_;
  float opacity_;
  gfx::Transform transform_;
  gfx::ScrollOffset scroll_offset_;
  bool animation_waiting_for_deletion_;
};

class FakeInactiveLayerAnimationValueObserver
    : public FakeLayerAnimationValueObserver {
 public:
  virtual bool IsActive() const override;
};

class FakeLayerAnimationValueProvider : public LayerAnimationValueProvider {
 public:
  virtual gfx::ScrollOffset ScrollOffsetForAnimation() const override;

  void set_scroll_offset(const gfx::ScrollOffset& scroll_offset) {
    scroll_offset_ = scroll_offset;
  }

 private:
  gfx::ScrollOffset scroll_offset_;
};

int AddOpacityTransitionToController(LayerAnimationController* controller,
                                     double duration,
                                     float start_opacity,
                                     float end_opacity,
                                     bool use_timing_function);

int AddAnimatedTransformToController(LayerAnimationController* controller,
                                     double duration,
                                     int delta_x,
                                     int delta_y);

int AddAnimatedFilterToController(LayerAnimationController* controller,
                                  double duration,
                                  float start_brightness,
                                  float end_brightness);

int AddOpacityTransitionToLayer(Layer* layer,
                                double duration,
                                float start_opacity,
                                float end_opacity,
                                bool use_timing_function);

int AddOpacityTransitionToLayer(LayerImpl* layer,
                                double duration,
                                float start_opacity,
                                float end_opacity,
                                bool use_timing_function);

int AddAnimatedTransformToLayer(Layer* layer,
                                double duration,
                                int delta_x,
                                int delta_y);

int AddAnimatedTransformToLayer(LayerImpl* layer,
                                double duration,
                                int delta_x,
                                int delta_y);

int AddAnimatedTransformToLayer(Layer* layer,
                                double duration,
                                TransformOperations start_operations,
                                TransformOperations operations);

int AddAnimatedTransformToLayer(LayerImpl* layer,
                                double duration,
                                TransformOperations start_operations,
                                TransformOperations operations);

int AddAnimatedFilterToLayer(Layer* layer,
                             double duration,
                             float start_brightness,
                             float end_brightness);

int AddAnimatedFilterToLayer(LayerImpl* layer,
                             double duration,
                             float start_brightness,
                             float end_brightness);

}  // namespace cc

#endif  // CC_TEST_ANIMATION_TEST_COMMON_H_
