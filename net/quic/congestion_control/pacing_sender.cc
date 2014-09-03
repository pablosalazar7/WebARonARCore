// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/congestion_control/pacing_sender.h"

namespace net {

PacingSender::PacingSender(SendAlgorithmInterface* sender,
                           QuicTime::Delta alarm_granularity,
                           uint32 initial_packet_burst)
    : sender_(sender),
      alarm_granularity_(alarm_granularity),
      initial_packet_burst_(initial_packet_burst),
      burst_tokens_(initial_packet_burst),
      last_delayed_packet_sent_time_(QuicTime::Zero()),
      next_packet_send_time_(QuicTime::Zero()),
      was_last_send_delayed_(false),
      has_valid_rtt_(false) {
}

PacingSender::~PacingSender() {}

void PacingSender::SetFromConfig(const QuicConfig& config, bool is_server) {
  // TODO(ianswett): Consider using the suggested RTT for pacing an initial
  // response.
  sender_->SetFromConfig(config, is_server);
}

void PacingSender::OnIncomingQuicCongestionFeedbackFrame(
      const QuicCongestionFeedbackFrame& feedback,
      QuicTime feedback_receive_time) {
  sender_->OnIncomingQuicCongestionFeedbackFrame(
      feedback, feedback_receive_time);
}

void PacingSender::OnCongestionEvent(bool rtt_updated,
                                     QuicByteCount bytes_in_flight,
                                     const CongestionVector& acked_packets,
                                     const CongestionVector& lost_packets) {
  if (rtt_updated) {
    has_valid_rtt_ = true;
  }
  sender_->OnCongestionEvent(
      rtt_updated, bytes_in_flight, acked_packets, lost_packets);
}

bool PacingSender::OnPacketSent(
    QuicTime sent_time,
    QuicByteCount bytes_in_flight,
    QuicPacketSequenceNumber sequence_number,
    QuicByteCount bytes,
    HasRetransmittableData has_retransmittable_data) {
  // Only pace data packets once we have an updated RTT.
  const bool in_flight =
      sender_->OnPacketSent(sent_time, bytes_in_flight, sequence_number,
                            bytes, has_retransmittable_data);
  if (has_retransmittable_data != HAS_RETRANSMITTABLE_DATA || !has_valid_rtt_) {
    return in_flight;
  }
  if (burst_tokens_ > 0) {
    --burst_tokens_;
    was_last_send_delayed_ = false;
    last_delayed_packet_sent_time_ = QuicTime::Zero();
    next_packet_send_time_ = QuicTime::Zero();
    return in_flight;
  }
  // The next packet should be sent as soon as the current packets has
  // been transferred.  We pace at twice the rate of the underlying
  // sender's bandwidth estimate during slow start and 1.25x during congestion
  // avoidance to ensure pacing doesn't prevent us from filling the window.
  const float kPacingAggression = sender_->InSlowStart() ? 2 : 1.25;
  QuicTime::Delta delay =
      BandwidthEstimate().Scale(kPacingAggression).TransferTime(bytes);
  // If the last send was delayed, and the alarm took a long time to get
  // invoked, allow the connection to make up for lost time.
  if (was_last_send_delayed_) {
    next_packet_send_time_ = next_packet_send_time_.Add(delay);
    // The send was application limited if it takes longer than the
    // pacing delay between sent packets.
    const bool application_limited =
        last_delayed_packet_sent_time_.IsInitialized() &&
        sent_time > last_delayed_packet_sent_time_.Add(delay);
    const bool making_up_for_lost_time = next_packet_send_time_ <= sent_time;
    // As long as we're making up time and not application limited,
    // continue to consider the packets delayed, allowing the packets to be
    // sent immediately.
    if (making_up_for_lost_time && !application_limited) {
      last_delayed_packet_sent_time_ = sent_time;
    } else {
      was_last_send_delayed_ = false;
      last_delayed_packet_sent_time_ = QuicTime::Zero();
    }
  } else {
    next_packet_send_time_ =
        QuicTime::Max(next_packet_send_time_.Add(delay),
                      sent_time.Add(delay).Subtract(alarm_granularity_));
  }
  return in_flight;
}

void PacingSender::OnRetransmissionTimeout(bool packets_retransmitted) {
  sender_->OnRetransmissionTimeout(packets_retransmitted);
}

void PacingSender::RevertRetransmissionTimeout() {
  sender_->RevertRetransmissionTimeout();
}

QuicTime::Delta PacingSender::TimeUntilSend(
      QuicTime now,
      QuicByteCount bytes_in_flight,
      HasRetransmittableData has_retransmittable_data) const {
  QuicTime::Delta time_until_send =
      sender_->TimeUntilSend(now, bytes_in_flight, has_retransmittable_data);
  if (!has_valid_rtt_) {
    // Don't pace if we don't have an updated RTT estimate.
    return time_until_send;
  }
  if (bytes_in_flight == 0) {
    // Add more burst tokens anytime the connection is entering quiescence.
    burst_tokens_ = initial_packet_burst_;
  }
  if (burst_tokens_ > 0) {
    // Don't pace if we have burst tokens available.
    return time_until_send;
  }

  if (!time_until_send.IsZero()) {
    DCHECK(time_until_send.IsInfinite());
    // The underlying sender prevents sending.
    return time_until_send;
  }

  if (has_retransmittable_data == NO_RETRANSMITTABLE_DATA) {
    // Don't pace ACK packets, since they do not count against CWND and do not
    // cause CWND to grow.
    return QuicTime::Delta::Zero();
  }

  // If the next send time is within the alarm granularity, send immediately.
  if (next_packet_send_time_ > now.Add(alarm_granularity_)) {
    DVLOG(1) << "Delaying packet: "
             << next_packet_send_time_.Subtract(now).ToMicroseconds();
    was_last_send_delayed_ = true;
    return next_packet_send_time_.Subtract(now);
  }

  DVLOG(1) << "Sending packet now";
  return QuicTime::Delta::Zero();
}

QuicBandwidth PacingSender::BandwidthEstimate() const {
  return sender_->BandwidthEstimate();
}

bool PacingSender::HasReliableBandwidthEstimate() const {
  return sender_->HasReliableBandwidthEstimate();
}

QuicTime::Delta PacingSender::RetransmissionDelay() const {
  return sender_->RetransmissionDelay();
}

QuicByteCount PacingSender::GetCongestionWindow() const {
  return sender_->GetCongestionWindow();
}

bool PacingSender::InSlowStart() const {
  return sender_->InSlowStart();
}

bool PacingSender::InRecovery() const {
  return sender_->InRecovery();
}

QuicByteCount PacingSender::GetSlowStartThreshold() const {
  return sender_->GetSlowStartThreshold();
}

CongestionControlType PacingSender::GetCongestionControlType() const {
  return sender_->GetCongestionControlType();
}

}  // namespace net
