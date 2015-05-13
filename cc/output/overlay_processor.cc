// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/overlay_processor.h"

#include "cc/output/output_surface.h"
#include "cc/output/overlay_strategy_single_on_top.h"
#include "cc/output/overlay_strategy_underlay.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/transform.h"

namespace cc {

OverlayProcessor::OverlayProcessor(OutputSurface* surface,
                                   ResourceProvider* resource_provider)
    : surface_(surface), resource_provider_(resource_provider) {}

void OverlayProcessor::Initialize() {
  DCHECK(surface_);
  if (!resource_provider_)
    return;

  OverlayCandidateValidator* candidates =
      surface_->GetOverlayCandidateValidator();
  if (candidates) {
    strategies_.push_back(scoped_ptr<Strategy>(
        new OverlayStrategySingleOnTop(candidates, resource_provider_)));
    strategies_.push_back(scoped_ptr<Strategy>(
        new OverlayStrategyUnderlay(candidates, resource_provider_)));
  }
}

OverlayProcessor::~OverlayProcessor() {}

void OverlayProcessor::ProcessForOverlays(
    RenderPassList* render_passes_in_draw_order,
    OverlayCandidateList* candidate_list) {
  for (StrategyList::iterator it = strategies_.begin(); it != strategies_.end();
       ++it) {
    if ((*it)->Attempt(render_passes_in_draw_order, candidate_list))
      return;
  }
}

}  // namespace cc
