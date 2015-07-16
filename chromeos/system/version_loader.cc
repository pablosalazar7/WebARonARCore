// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/system/version_loader.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/time/time.h"

namespace chromeos {
namespace version_loader {

namespace {

// Beginning of line we look for that gives full version number.
// Format: x.x.xx.x (Developer|Official build extra info) board info
const char kFullVersionKey[] = "CHROMEOS_RELEASE_DESCRIPTION";

// Same but for short version (x.x.xx.x).
const char kVersionKey[] = "CHROMEOS_RELEASE_VERSION";

// Beginning of line we look for that gives the firmware version.
const char kFirmwarePrefix[] = "version";

// File to look for firmware number in.
const char kPathFirmware[] = "/var/log/bios_info.txt";

}  // namespace

std::string GetVersion(VersionFormat format) {
  std::string version;
  std::string key = (format == VERSION_FULL ?
                     kFullVersionKey : kVersionKey);
  if (!base::SysInfo::GetLsbReleaseValue(key, &version)) {
    LOG_IF(ERROR, base::SysInfo::IsRunningOnChromeOS())
        << "No LSB version key: " << key;
    version = "0.0.0.0";
  }
  if (format == VERSION_SHORT_WITH_DATE) {
    base::Time::Exploded ctime;
    base::SysInfo::GetLsbReleaseTime().UTCExplode(&ctime);
    version += base::StringPrintf("-%02u.%02u.%02u",
                                  ctime.year % 100,
                                  ctime.month,
                                  ctime.day_of_month);
  }

  return version;
}

std::string GetFirmware() {
  std::string firmware;
  std::string contents;
  const base::FilePath file_path(kPathFirmware);
  if (base::ReadFileToString(file_path, &contents)) {
    firmware = ParseFirmware(contents);
  }
  return firmware;
}

std::string ParseFirmware(const std::string& contents) {
  // The file contains lines such as:
  // vendor           | ...
  // version          | ...
  // release_date     | ...
  // We don't make any assumption that the spaces between "version" and "|" is
  //   fixed. So we just match kFirmwarePrefix at the start of the line and find
  //   the first character that is not "|" or space

  std::vector<std::string> lines;
  base::SplitString(contents, '\n', &lines);
  for (size_t i = 0; i < lines.size(); ++i) {
    if (base::StartsWith(lines[i], kFirmwarePrefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      std::string str = lines[i].substr(std::string(kFirmwarePrefix).size());
      size_t found = str.find_first_not_of("| ");
      if (found != std::string::npos)
        return str.substr(found);
    }
  }
  return std::string();
}

}  // namespace version_loader
}  // namespace chromeos
