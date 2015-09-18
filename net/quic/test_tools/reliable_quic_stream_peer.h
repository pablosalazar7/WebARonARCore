// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_TEST_TOOLS_RELIABLE_QUIC_STREAM_PEER_H_
#define NET_QUIC_TEST_TOOLS_RELIABLE_QUIC_STREAM_PEER_H_

#include "base/basictypes.h"
#include "base/strings/string_piece.h"
#include "net/quic/quic_ack_notifier.h"
#include "net/quic/quic_protocol.h"

namespace net {

class ReliableQuicStream;

namespace test {

class ReliableQuicStreamPeer {
 public:
  static void SetWriteSideClosed(bool value, ReliableQuicStream* stream);
  static void SetStreamBytesWritten(QuicStreamOffset stream_bytes_written,
                                    ReliableQuicStream* stream);
  static void CloseReadSide(ReliableQuicStream* stream);

  static bool FinSent(ReliableQuicStream* stream);
  static bool RstSent(ReliableQuicStream* stream);

  static uint32 SizeOfQueuedData(ReliableQuicStream* stream);

  static void SetFecPolicy(ReliableQuicStream* stream, FecPolicy fec_policy);

  static bool StreamContributesToConnectionFlowControl(
      ReliableQuicStream* stream);

  static void WriteOrBufferData(
      ReliableQuicStream* stream,
      base::StringPiece data,
      bool fin,
      QuicAckNotifier::DelegateInterface* ack_notifier_delegate);

 private:
  DISALLOW_COPY_AND_ASSIGN(ReliableQuicStreamPeer);
};

}  // namespace test
}  // namespace net

#endif  // NET_QUIC_TEST_TOOLS_RELIABLE_QUIC_STREAM_PEER_H_
