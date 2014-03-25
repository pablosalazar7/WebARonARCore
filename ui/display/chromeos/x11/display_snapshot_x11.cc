// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/chromeos/x11/display_snapshot_x11.h"

#include "base/strings/stringprintf.h"
#include "ui/display/chromeos/x11/display_mode_x11.h"
#include "ui/display/x11/edid_parser_x11.h"

namespace ui {

DisplaySnapshotX11::DisplaySnapshotX11(
    int64_t display_id,
    bool has_proper_display_id,
    const gfx::Point& origin,
    const gfx::Size& physical_size,
    OutputType type,
    bool is_aspect_preserving_scaling,
    const std::vector<const DisplayMode*>& modes,
    const DisplayMode* current_mode,
    const DisplayMode* native_mode,
    RROutput output,
    RRCrtc crtc,
    int index)
    : DisplaySnapshot(display_id,
                      has_proper_display_id,
                      origin,
                      physical_size,
                      type,
                      is_aspect_preserving_scaling,
                      modes,
                      current_mode,
                      native_mode),
      output_(output),
      crtc_(crtc),
      index_(index) {}

DisplaySnapshotX11::~DisplaySnapshotX11() {}

std::string DisplaySnapshotX11::GetDisplayName() {
  return ui::GetDisplayName(output_);
}

bool DisplaySnapshotX11::GetOverscanFlag() {
  bool flag = false;
  GetOutputOverscanFlag(output_, &flag);

  return flag;
}

std::string DisplaySnapshotX11::ToString() const {
  return base::StringPrintf(
      "[type=%d, output=%ld, crtc=%ld, mode=%ld, dim=%dx%d]",
      type_,
      output_,
      crtc_,
      current_mode_
          ? static_cast<const DisplayModeX11*>(current_mode_)->mode_id()
          : 0,
      physical_size_.width(),
      physical_size_.height());
}

}  // namespace ui
