// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/cryptauth/cryptauth_enrollment_utils.h"

#include <math.h>

#include "base/md5.h"
#include "base/sha1.h"
#include "components/proximity_auth/cryptauth/base64url.h"

namespace proximity_auth {

int64_t HashStringToInt64(const std::string& string) {
  base::MD5Context context;
  base::MD5Init(&context);
  base::MD5Update(&context, string);

  base::MD5Digest digest;
  base::MD5Final(&digest, &context);

  // Fold the digest into an int64 value. |digest.a| is a 16-byte array, so we
  // sum the two 8-byte halves of the digest to create the hash.
  int64_t hash = 0;
  for (size_t i = 0; i < sizeof(digest.a); ++i) {
    uint8_t byte = digest.a[i];
    hash += static_cast<int64_t>(byte) << (i % sizeof(int64_t));
  }

  return hash;
}

std::string CalculateDeviceUserId(const std::string& device_id,
                                  const std::string& user_id) {
  std::string device_user_id;
  Base64UrlEncode(base::SHA1HashString(device_id + "|" + user_id),
                  &device_user_id);
  return device_user_id;
}

}  // namespace proximity_auth
