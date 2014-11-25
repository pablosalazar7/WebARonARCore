// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_MANAGED_TILE_STATE_H_
#define CC_RESOURCES_MANAGED_TILE_STATE_H_

#include "base/memory/scoped_ptr.h"
#include "cc/resources/platform_color.h"
#include "cc/resources/rasterizer.h"
#include "cc/resources/resource_pool.h"
#include "cc/resources/resource_provider.h"
#include "cc/resources/scoped_resource.h"
#include "cc/resources/tile_priority.h"

namespace cc {

// This is state that is specific to a tile that is
// managed by the TileManager.
class CC_EXPORT ManagedTileState {
 public:
  // This class holds all the state relevant to drawing a tile.
  class CC_EXPORT DrawInfo {
   public:
    enum Mode { RESOURCE_MODE, SOLID_COLOR_MODE, PICTURE_PILE_MODE };

    DrawInfo();
    ~DrawInfo();

    Mode mode() const { return mode_; }

    bool IsReadyToDraw() const;

    ResourceProvider::ResourceId get_resource_id() const {
      DCHECK(mode_ == RESOURCE_MODE);
      DCHECK(resource_);

      return resource_->id();
    }

    SkColor get_solid_color() const {
      DCHECK(mode_ == SOLID_COLOR_MODE);

      return solid_color_;
    }

    bool contents_swizzled() const {
      DCHECK(resource_);
      return !PlatformColor::SameComponentOrder(resource_->format());
    }

    bool requires_resource() const {
      return mode_ == RESOURCE_MODE || mode_ == PICTURE_PILE_MODE;
    }

    inline bool has_resource() const { return !!resource_; }

    void SetSolidColorForTesting(SkColor color) { set_solid_color(color); }
    void SetResourceForTesting(scoped_ptr<ScopedResource> resource) {
      resource_ = resource.Pass();
    }

   private:
    friend class TileManager;
    friend class PrioritizedTileSet;
    friend class Tile;
    friend class ManagedTileState;

    void set_use_resource() { mode_ = RESOURCE_MODE; }

    void set_solid_color(const SkColor& color) {
      mode_ = SOLID_COLOR_MODE;
      solid_color_ = color;
    }

    void set_rasterize_on_demand() { mode_ = PICTURE_PILE_MODE; }

    Mode mode_;
    SkColor solid_color_;
    scoped_ptr<ScopedResource> resource_;
  };

  ManagedTileState();
  ~ManagedTileState();

  void AsValueInto(base::debug::TracedValue* dict) const;

  // Persisted state: valid all the time.
  DrawInfo draw_info;
  scoped_refptr<RasterTask> raster_task;

  TileResolution resolution;
  TilePriority::PriorityBin priority_bin;

  // Priority for this state from the last time we assigned memory.
  unsigned scheduled_priority;
};

}  // namespace cc

#endif  // CC_RESOURCES_MANAGED_TILE_STATE_H_
