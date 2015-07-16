// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/devices/device_util_linux.h"

#include <string>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"

namespace ui {

InputDeviceType GetInputDeviceTypeFromPath(const base::FilePath& path) {
  DCHECK(!base::MessageLoopForUI::IsCurrent());
  std::string event_node = path.BaseName().value();
  if (event_node.empty() ||
      !base::StartsWith(event_node, "event",
                        base::CompareCase::INSENSITIVE_ASCII))
    return InputDeviceType::INPUT_DEVICE_UNKNOWN;

  // Find sysfs device path for this device.
  base::FilePath sysfs_path =
      base::FilePath(FILE_PATH_LITERAL("/sys/class/input"));
  sysfs_path = sysfs_path.Append(path.BaseName());
  sysfs_path = base::MakeAbsoluteFilePath(sysfs_path);

  // Device does not exist.
  if (sysfs_path.empty())
    return InputDeviceType::INPUT_DEVICE_UNKNOWN;

  // Check ancestor devices for a known bus attachment.
  for (base::FilePath path = sysfs_path; path != base::FilePath("/");
       path = path.DirName()) {
    // Bluetooth LE devices are virtual "uhid" devices.
    if (path ==
        base::FilePath(FILE_PATH_LITERAL("/sys/devices/virtual/misc/uhid")))
      return InputDeviceType::INPUT_DEVICE_EXTERNAL;

    std::string subsystem_path =
        base::MakeAbsoluteFilePath(path.Append(FILE_PATH_LITERAL("subsystem")))
            .value();
    if (subsystem_path.empty())
      continue;

    // Internal bus attachments.
    if (subsystem_path == "/sys/bus/pci")
      return InputDeviceType::INPUT_DEVICE_INTERNAL;
    if (subsystem_path == "/sys/bus/i2c")
      return InputDeviceType::INPUT_DEVICE_INTERNAL;
    if (subsystem_path == "/sys/bus/acpi")
      return InputDeviceType::INPUT_DEVICE_INTERNAL;
    if (subsystem_path == "/sys/bus/serio")
      return InputDeviceType::INPUT_DEVICE_INTERNAL;
    if (subsystem_path == "/sys/bus/platform")
      return InputDeviceType::INPUT_DEVICE_INTERNAL;

    // External bus attachments.
    if (subsystem_path == "/sys/bus/usb")
      return InputDeviceType::INPUT_DEVICE_EXTERNAL;
    if (subsystem_path == "/sys/class/bluetooth")
      return InputDeviceType::INPUT_DEVICE_EXTERNAL;
  }

  return InputDeviceType::INPUT_DEVICE_UNKNOWN;
}

}  // namespace
