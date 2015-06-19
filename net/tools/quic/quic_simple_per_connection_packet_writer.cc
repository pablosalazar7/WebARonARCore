// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/quic_simple_per_connection_packet_writer.h"

#include "base/bind.h"
#include "net/tools/quic/quic_simple_server_packet_writer.h"

namespace net {
namespace tools {

QuicSimplePerConnectionPacketWriter::QuicSimplePerConnectionPacketWriter(
    QuicSimpleServerPacketWriter* shared_writer,
    QuicConnection* connection)
    : shared_writer_(shared_writer),
      connection_(connection),
      weak_factory_(this){
}

QuicSimplePerConnectionPacketWriter::~QuicSimplePerConnectionPacketWriter() {
}

QuicPacketWriter* QuicSimplePerConnectionPacketWriter::shared_writer() const {
  return shared_writer_;
}

WriteResult QuicSimplePerConnectionPacketWriter::WritePacket(
    const char* buffer,
    size_t buf_len,
    const IPAddressNumber& self_address,
    const IPEndPoint& peer_address) {
  return shared_writer_->WritePacketWithCallback(
      buffer,
      buf_len,
      self_address,
      peer_address,
      base::Bind(&QuicSimplePerConnectionPacketWriter::OnWriteComplete,
                 weak_factory_.GetWeakPtr()));
}

bool QuicSimplePerConnectionPacketWriter::IsWriteBlockedDataBuffered() const {
  return shared_writer_->IsWriteBlockedDataBuffered();
}

bool QuicSimplePerConnectionPacketWriter::IsWriteBlocked() const {
  return shared_writer_->IsWriteBlocked();
}

void QuicSimplePerConnectionPacketWriter::SetWritable() {
  shared_writer_->SetWritable();
}

void QuicSimplePerConnectionPacketWriter::OnWriteComplete(WriteResult result) {
  if (connection_ && result.status == WRITE_STATUS_ERROR) {
    connection_->OnWriteError(result.error_code);
  }
}

}  // namespace tools
}  // namespace net
