// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_SPDY_HEADERS_HANDLER_INTERFACE_H_
#define NET_SPDY_SPDY_HEADERS_HANDLER_INTERFACE_H_

#include "base/strings/string_piece.h"

namespace net {

// This interface defines how an object that accepts header data should behave.
// It is used by both SpdyHeadersBlockParser and HpackDecoder.
class SpdyHeadersHandlerInterface {
 public:
  virtual ~SpdyHeadersHandlerInterface() {}

  // A callback method which notifies when the parser starts handling a new
  // header block fragment.
  virtual void OnHeaderBlockStart() = 0;

  // A callback method which notifies on a header key value pair. Multiple
  // values for a given key will be emitted as multiple calls to OnHeader.
  virtual void OnHeader(base::StringPiece key, base::StringPiece value) = 0;

  // A callback method which notifies when the parser finishes handling a
  // header block fragment. Also indicates the total number of bytes in this
  // block.
  virtual void OnHeaderBlockEnd(size_t header_bytes_parsed) = 0;
};

}  // namespace net

#endif  // NET_SPDY_SPDY_HEADERS_HANDLER_INTERFACE_H_
