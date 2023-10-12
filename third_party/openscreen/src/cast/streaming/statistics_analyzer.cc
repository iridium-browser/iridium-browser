// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/statistics_analyzer.h"

#include <algorithm>

#include "platform/base/trivial_clock_traits.h"
#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {

namespace {

constexpr Clock::duration kStatisticsAnalysisInverval =
    std::chrono::milliseconds(500);

constexpr size_t kMaxRecentPacketInfoMapSize = 1000;
constexpr size_t kMaxRecentFrameInfoMapSize = 200;

constexpr int kDefaultMaxLatencyBucketMs = 800;
constexpr int kDefaultBucketWidthMs = 20;

double InMilliseconds(const Clock::duration duration) {
  return static_cast<double>(to_milliseconds(duration).count());
}

bool IsReceiverEvent(const StatisticsEventType event) {
  return event == StatisticsEventType::kFrameAckSent ||
         event == StatisticsEventType::kFrameDecoded ||
         event == StatisticsEventType::kFramePlayedOut ||
         event == StatisticsEventType::kPacketReceived;
}

}  // namespace

StatisticsAnalyzer::StatisticsAnalyzer(
    SenderStatsClient* stats_client,
    ClockNowFunctionPtr now,
    TaskRunner& task_runner,
    std::unique_ptr<ClockOffsetEstimator> offset_estimator)
    : stats_client_(stats_client),
      offset_estimator_(std::move(offset_estimator)),
      now_(now),
      alarm_(now, task_runner),
      start_time_(now()) {
  statistics_collector_ = std::make_unique<StatisticsCollector>(now_);
  InitHistograms();
}

StatisticsAnalyzer::~StatisticsAnalyzer() = default;

void StatisticsAnalyzer::ScheduleAnalysis() {
  const Clock::time_point next_analysis_time =
      now_() + kStatisticsAnalysisInverval;
  alarm_.Schedule([this] { AnalyzeStatistics(); }, next_analysis_time);
}

void StatisticsAnalyzer::InitHistograms() {
  for (auto& histogram : audio_histograms_) {
    histogram =
        SimpleHistogram(0, kDefaultMaxLatencyBucketMs, kDefaultBucketWidthMs);
  }
  for (auto& histogram : video_histograms_) {
    histogram =
        SimpleHistogram(0, kDefaultMaxLatencyBucketMs, kDefaultBucketWidthMs);
  }
}

void StatisticsAnalyzer::AnalyzeStatistics() {
  ProcessFrameEvents(statistics_collector_->TakeRecentFrameEvents());
  ProcessPacketEvents(statistics_collector_->TakeRecentPacketEvents());
  SendStatistics();
  ScheduleAnalysis();
}

void StatisticsAnalyzer::SendStatistics() {
  if (!stats_client_) {
    return;
  }

  const Clock::time_point end_time = now_();
  stats_client_->OnStatisticsUpdated(SenderStats{
      .audio_statistics =
          ConstructStatisticsList(end_time, StatisticsEventMediaType::kAudio),
      .audio_histograms = GetAudioHistograms(),
      .video_statistics =
          ConstructStatisticsList(end_time, StatisticsEventMediaType::kVideo),
      .video_histograms = GetVideoHistograms()});
}

void StatisticsAnalyzer::ProcessFrameEvents(
    const std::vector<FrameEvent> frame_events) {
  for (FrameEvent frame_event : frame_events) {
    offset_estimator_->OnFrameEvent(frame_event);

    FrameStatsMap* frame_stats_map =
        GetFrameStatsMapForMediaType(frame_event.media_type);
    if (frame_stats_map) {
      auto it = frame_stats_map->find(frame_event.type);
      if (it == frame_stats_map->end()) {
        frame_stats_map->insert(std::make_pair(
            frame_event.type,
            FrameStatsAggregate{.event_counter = 1,
                                .sum_size = frame_event.size,
                                .sum_delay = frame_event.delay_delta}));
      } else {
        ++(it->second.event_counter);
        it->second.sum_size += frame_event.size;
        it->second.sum_delay += frame_event.delay_delta;
      }
    }

    RecordEventTimes(frame_event.timestamp, frame_event.media_type,
                     IsReceiverEvent(frame_event.type));

    RecordFrameLatencies(frame_event);
  }
}

void StatisticsAnalyzer::ProcessPacketEvents(
    const std::vector<PacketEvent> packet_events) {
  for (PacketEvent packet_event : packet_events) {
    offset_estimator_->OnPacketEvent(packet_event);

    PacketStatsMap* packet_stats_map =
        GetPacketStatsMapForMediaType(packet_event.media_type);
    if (packet_stats_map) {
      auto it = packet_stats_map->find(packet_event.type);
      if (it == packet_stats_map->end()) {
        packet_stats_map->insert(std::make_pair(
            packet_event.type,
            PacketStatsAggregate{.event_counter = 1,
                                 .sum_size = packet_event.size}));
      } else {
        ++(it->second.event_counter);
        it->second.sum_size += packet_event.size;
      }
    }

    RecordEventTimes(packet_event.timestamp, packet_event.media_type,
                     IsReceiverEvent(packet_event.type));

    if (packet_event.type == StatisticsEventType::kPacketSentToNetwork ||
        packet_event.type == StatisticsEventType::kPacketReceived) {
      RecordPacketLatencies(packet_event);
    } else if (packet_event.type == StatisticsEventType::kPacketRetransmitted) {
      // We only measure network latency for packets that are not retransmitted.
      ErasePacketInfo(packet_event);
    }
  }
}

void StatisticsAnalyzer::RecordFrameLatencies(const FrameEvent frame_event) {
  FrameInfoMap* frame_infos =
      GetRecentFrameInfosForMediaType(frame_event.media_type);
  if (!frame_infos) {
    return;
  }

  auto it = frame_infos->find(frame_event.rtp_timestamp);
  if (it == frame_infos->end()) {
    auto emplace_result =
        frame_infos->emplace(frame_event.rtp_timestamp, FrameInfo{});
    OSP_CHECK(emplace_result.second);
    it = emplace_result.first;

    if (frame_infos->size() >= kMaxRecentFrameInfoMapSize) {
      frame_infos->erase(frame_infos->begin());
    }
  }

  switch (frame_event.type) {
    case StatisticsEventType::kFrameCaptureBegin:
      it->second.capture_begin_time = frame_event.timestamp;
      break;

    case StatisticsEventType::kFrameCaptureEnd: {
      it->second.capture_end_time = frame_event.timestamp;
      if (it->second.capture_begin_time != Clock::time_point::min()) {
        const Clock::duration capture_latency =
            frame_event.timestamp - it->second.capture_begin_time;
        AddToLatencyAggregrate(StatisticType::kAvgCaptureLatencyMs,
                               capture_latency, frame_event.media_type);
        AddToHistogram(HistogramType::kCaptureLatencyMs, frame_event.media_type,
                       InMilliseconds(capture_latency));
      }
    } break;

    case StatisticsEventType::kFrameEncoded: {
      it->second.encode_end_time = frame_event.timestamp;
      if (it->second.capture_end_time != Clock::time_point::min()) {
        const Clock::duration encode_latency =
            frame_event.timestamp - it->second.capture_end_time;
        AddToLatencyAggregrate(StatisticType::kAvgEncodeTimeMs, encode_latency,
                               frame_event.media_type);
        AddToHistogram(HistogramType::kEncodeTimeMs, frame_event.media_type,
                       InMilliseconds(encode_latency));
      }
    } break;

    // Frame latency is the time from when the frame is encoded until the
    // receiver ack for the frame is sent.
    case StatisticsEventType::kFrameAckSent: {
      const auto adjusted_timestamp =
          ToSenderTimestamp(frame_event.timestamp, frame_event.media_type);
      if (!adjusted_timestamp) {
        return;
      }

      if (it->second.encode_end_time != Clock::time_point::min()) {
        const Clock::duration frame_latency =
            *adjusted_timestamp - it->second.encode_end_time;
        AddToLatencyAggregrate(StatisticType::kAvgFrameLatencyMs, frame_latency,
                               frame_event.media_type);
      }
    } break;

    case StatisticsEventType::kFramePlayedOut: {
      const auto adjusted_timestamp =
          ToSenderTimestamp(frame_event.timestamp, frame_event.media_type);
      if (!adjusted_timestamp) {
        return;
      }

      if (it->second.capture_begin_time != Clock::time_point::min()) {
        const Clock::duration e2e_latency =
            *adjusted_timestamp - it->second.capture_begin_time;
        AddToLatencyAggregrate(StatisticType::kAvgEndToEndLatencyMs,
                               e2e_latency, frame_event.media_type);
        AddToHistogram(HistogramType::kEndToEndLatencyMs,
                       frame_event.media_type, InMilliseconds(e2e_latency));
      }

      // Positive delay means the frame is late.
      if (frame_event.delay_delta > Clock::duration::zero()) {
        SessionStats* session_stats =
            GetSessionStatsForMediaType(frame_event.media_type);
        if (session_stats) {
          ++(session_stats->late_frame_counter);
        }
        AddToHistogram(HistogramType::kFrameLatenessMs, frame_event.media_type,
                       InMilliseconds(frame_event.delay_delta));
      }
    } break;

    default:
      break;
  }
}

void StatisticsAnalyzer::RecordPacketLatencies(const PacketEvent packet_event) {
  FrameInfoMap* frame_infos =
      GetRecentFrameInfosForMediaType(packet_event.media_type);

  // Queueing latency is the time from when a frame is encoded to when the
  // packet is first sent.
  if (frame_infos &&
      packet_event.type == StatisticsEventType::kPacketSentToNetwork) {
    auto it = frame_infos->find(packet_event.rtp_timestamp);

    // We have an encode end time for a frame associated with this packet.
    if (it != frame_infos->end()) {
      const Clock::duration queueing_latency =
          packet_event.timestamp - it->second.encode_end_time;
      AddToLatencyAggregrate(StatisticType::kAvgQueueingLatencyMs,
                             queueing_latency, packet_event.media_type);
      AddToHistogram(HistogramType::kQueueingLatencyMs, packet_event.media_type,
                     InMilliseconds(queueing_latency));
    }
  }

  StatisticsAnalyzer::PacketKey key =
      std::make_pair(packet_event.rtp_timestamp, packet_event.packet_id);
  PacketInfoMap* packet_infos =
      GetRecentPacketInfosForMediaType(packet_event.media_type);
  if (!packet_infos) {
    return;
  }

  auto it = packet_infos->find(key);
  if (it == packet_infos->end()) {
    packet_infos->insert(
        std::make_pair(key, PacketInfo{.timestamp = packet_event.timestamp,
                                       .type = packet_event.type}));
    if (packet_infos->size() > kMaxRecentPacketInfoMapSize) {
      packet_infos->erase(packet_infos->begin());
    }
  } else {  // We know when this packet was sent, and when it arrived.
    PacketInfo value = it->second;
    StatisticsEventType recorded_type = value.type;
    Clock::time_point packet_sent_time;
    Clock::time_point packet_received_time;
    if (recorded_type == StatisticsEventType::kPacketSentToNetwork &&
        packet_event.type == StatisticsEventType::kPacketReceived) {
      packet_sent_time = value.timestamp;
      packet_received_time = packet_event.timestamp;
    } else if (recorded_type == StatisticsEventType::kPacketReceived &&
               packet_event.type == StatisticsEventType::kPacketSentToNetwork) {
      packet_sent_time = packet_event.timestamp;
      packet_received_time = value.timestamp;
    } else {
      return;
    }

    packet_infos->erase(it);

    // Use the offset estimator directly since we are trying to calculate the
    // average network latency.
    const absl::optional<Clock::duration> receiver_offset =
        offset_estimator_->GetEstimatedOffset();
    if (!receiver_offset) {
      return;
    }
    packet_received_time -= *receiver_offset;

    // Network latency is the time between when a packet is sent and when it
    // is received.
    const Clock::duration network_latency =
        packet_received_time - packet_sent_time;
    AddToLatencyAggregrate(StatisticType::kAvgNetworkLatencyMs, network_latency,
                           packet_event.media_type);
    AddToHistogram(HistogramType::kNetworkLatencyMs, packet_event.media_type,
                   InMilliseconds(network_latency));

    // Packet latency is the time from when a frame is encoded until when the
    // packet is received.
    auto frame_it = frame_infos->find(packet_event.rtp_timestamp);
    if (frame_it != frame_infos->end()) {
      const Clock::duration packet_latency =
          packet_received_time - frame_it->second.encode_end_time;
      AddToLatencyAggregrate(StatisticType::kAvgPacketLatencyMs, packet_latency,
                             packet_event.media_type);
      AddToHistogram(HistogramType::kPacketLatencyMs, packet_event.media_type,
                     InMilliseconds(packet_latency));
    }
  }
}

void StatisticsAnalyzer::RecordEventTimes(
    const Clock::time_point timestamp,
    const StatisticsEventMediaType media_type,
    const bool is_receiver_event) {
  SessionStats* session_stats = GetSessionStatsForMediaType(media_type);
  if (!session_stats) {
    return;
  }

  Clock::time_point sender_timestamp = timestamp;
  if (is_receiver_event) {
    auto timestamp_result = ToSenderTimestamp(timestamp, media_type);
    if (!timestamp_result) {
      return;
    }
    sender_timestamp = *timestamp_result;

    session_stats->last_response_received_time =
        std::max(session_stats->last_response_received_time, sender_timestamp);
  }

  session_stats->first_event_time =
      std::min(session_stats->first_event_time, sender_timestamp);
  session_stats->last_event_time =
      std::max(session_stats->last_event_time, sender_timestamp);
}

void StatisticsAnalyzer::ErasePacketInfo(const PacketEvent packet_event) {
  StatisticsAnalyzer::PacketKey key =
      std::make_pair(packet_event.rtp_timestamp, packet_event.packet_id);
  PacketInfoMap* packet_infos =
      GetRecentPacketInfosForMediaType(packet_event.media_type);
  if (packet_infos) {
    packet_infos->erase(key);
  }
}

void StatisticsAnalyzer::AddToLatencyAggregrate(
    const StatisticType latency_stat,
    const Clock::duration latency_delta,
    const StatisticsEventMediaType media_type) {
  LatencyStatsMap* latency_stats = GetLatencyStatsMapForMediaType(media_type);
  if (!latency_stats) {
    return;
  }

  auto it = latency_stats->find(latency_stat);
  if (it == latency_stats->end()) {
    latency_stats->insert(std::make_pair(
        latency_stat, LatencyStatsAggregate{.data_point_counter = 1,
                                            .sum_latency = latency_delta}));
  } else {
    ++(it->second.data_point_counter);
    it->second.sum_latency += latency_delta;
  }
}

void StatisticsAnalyzer::AddToHistogram(
    const HistogramType histogram,
    const StatisticsEventMediaType media_type,
    const int64_t sample) {
  if (media_type == StatisticsEventMediaType::kAudio) {
    audio_histograms_[static_cast<int>(histogram)].Add(sample);
  } else if (media_type == StatisticsEventMediaType::kVideo) {
    video_histograms_[static_cast<int>(histogram)].Add(sample);
  }
}

StatisticsAnalyzer::FrameStatsMap*
StatisticsAnalyzer::GetFrameStatsMapForMediaType(
    const StatisticsEventMediaType media_type) {
  switch (media_type) {
    case StatisticsEventMediaType::kAudio:
      return &audio_frame_stats_;
    case StatisticsEventMediaType::kVideo:
      return &video_frame_stats_;
    default:
      return nullptr;
  }
}

StatisticsAnalyzer::PacketStatsMap*
StatisticsAnalyzer::GetPacketStatsMapForMediaType(
    const StatisticsEventMediaType media_type) {
  switch (media_type) {
    case StatisticsEventMediaType::kAudio:
      return &audio_packet_stats_;
    case StatisticsEventMediaType::kVideo:
      return &video_packet_stats_;
    default:
      return nullptr;
  }
}

StatisticsAnalyzer::LatencyStatsMap*
StatisticsAnalyzer::GetLatencyStatsMapForMediaType(
    const StatisticsEventMediaType media_type) {
  switch (media_type) {
    case StatisticsEventMediaType::kAudio:
      return &audio_latency_stats_;
    case StatisticsEventMediaType::kVideo:
      return &video_latency_stats_;
    default:
      return nullptr;
  }
}

StatisticsAnalyzer::SessionStats*
StatisticsAnalyzer::GetSessionStatsForMediaType(
    const StatisticsEventMediaType media_type) {
  switch (media_type) {
    case StatisticsEventMediaType::kAudio:
      return &audio_session_stats_;
    case StatisticsEventMediaType::kVideo:
      return &video_session_stats_;
    default:
      return nullptr;
  }
}

StatisticsAnalyzer::FrameInfoMap*
StatisticsAnalyzer::GetRecentFrameInfosForMediaType(
    const StatisticsEventMediaType media_type) {
  switch (media_type) {
    case StatisticsEventMediaType::kAudio:
      return &audio_recent_frame_infos_;
    case StatisticsEventMediaType::kVideo:
      return &video_recent_frame_infos_;
    default:
      return nullptr;
  }
}

StatisticsAnalyzer::PacketInfoMap*
StatisticsAnalyzer::GetRecentPacketInfosForMediaType(
    const StatisticsEventMediaType media_type) {
  switch (media_type) {
    case StatisticsEventMediaType::kAudio:
      return &audio_recent_packet_infos_;
    case StatisticsEventMediaType::kVideo:
      return &video_recent_packet_infos_;
    default:
      return nullptr;
  }
}

SenderStats::HistogramsList StatisticsAnalyzer::GetAudioHistograms() {
  SenderStats::HistogramsList histos_list;
  for (size_t i = 0; i < audio_histograms_.size(); i++) {
    histos_list[i] = audio_histograms_[i].Copy();
  }
  return histos_list;
}

SenderStats::HistogramsList StatisticsAnalyzer::GetVideoHistograms() {
  SenderStats::HistogramsList histos_list;
  for (size_t i = 0; i < video_histograms_.size(); i++) {
    histos_list[i] = video_histograms_[i].Copy();
  }
  return histos_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::ConstructStatisticsList(
    const Clock::time_point end_time,
    const StatisticsEventMediaType media_type) {
  SenderStats::StatisticsList stats_list;

  // TODO(b/298205111): Support kNumFramesDroppedByEncoder stat.
  stats_list = PopulateFrameCountStat(StatisticsEventType::kFrameCaptureEnd,
                                      StatisticType::kNumFramesCaptured,
                                      stats_list, media_type);

  // kEnqueueFps
  stats_list = PopulateFpsStat(StatisticsEventType::kFrameEncoded,
                               StatisticType::kEnqueueFps, stats_list,
                               media_type, end_time);

  constexpr StatisticType kSupportedLatencyStats[] = {
      StatisticType::kAvgEncodeTimeMs,      StatisticType::kAvgCaptureLatencyMs,
      StatisticType::kAvgQueueingLatencyMs, StatisticType::kAvgNetworkLatencyMs,
      StatisticType::kAvgPacketLatencyMs,   StatisticType::kAvgFrameLatencyMs,
      StatisticType::kAvgEndToEndLatencyMs,
  };
  for (StatisticType type : kSupportedLatencyStats) {
    stats_list = PopulateAvgLatencyStat(type, stats_list, media_type);
  }

  // kEncodeRateKbps
  stats_list = PopulateFrameBitrateStat(StatisticsEventType::kFrameEncoded,
                                        StatisticType::kEncodeRateKbps,
                                        stats_list, media_type, end_time);

  // kPacketTransmissionRateKbps
  stats_list =
      PopulatePacketBitrateStat(StatisticsEventType::kPacketSentToNetwork,
                                StatisticType::kPacketTransmissionRateKbps,
                                stats_list, media_type, end_time);

  // kNumPacketsSent
  stats_list = PopulatePacketCountStat(
      StatisticsEventType::kPacketSentToNetwork, StatisticType::kNumPacketsSent,
      stats_list, media_type);

  // kNumPacketsReceived
  stats_list = PopulatePacketCountStat(StatisticsEventType::kPacketReceived,
                                       StatisticType::kNumPacketsReceived,
                                       stats_list, media_type);

  // kTimeSinceLastReceiverResponseMs
  // kFirstEventTimeMs
  // kLastEventTimeMs
  // kNumLateFrames
  stats_list = PopulateSessionStats(stats_list, media_type, end_time);

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulatePacketCountStat(
    const StatisticsEventType event,
    const StatisticType stat,
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type) {
  PacketStatsMap* stats_map = GetPacketStatsMapForMediaType(media_type);
  if (!stats_map) {
    return stats_list;
  }

  auto it = stats_map->find(event);
  if (it != stats_map->end()) {
    stats_list[static_cast<int>(stat)] = it->second.event_counter;
  }

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulateFrameCountStat(
    const StatisticsEventType event,
    const StatisticType stat,
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type) {
  FrameStatsMap* stats_map = GetFrameStatsMapForMediaType(media_type);
  if (!stats_map) {
    return stats_list;
  }
  auto it = stats_map->find(event);
  if (it != stats_map->end()) {
    stats_list[static_cast<int>(stat)] = it->second.event_counter;
  }

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulateFpsStat(
    const StatisticsEventType event,
    const StatisticType stat,
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type,
    const Clock::time_point end_time) {
  FrameStatsMap* stats_map = GetFrameStatsMapForMediaType(media_type);
  if (!stats_map) {
    return stats_list;
  }

  auto it = stats_map->find(event);
  if (it != stats_map->end()) {
    const Clock::duration duration = end_time - start_time_;
    if (duration != Clock::duration::zero()) {
      const int count = it->second.event_counter;
      const double fps = (count / InMilliseconds(duration)) * 1000;
      stats_list[static_cast<int>(stat)] = fps;
    }
  }

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulateAvgLatencyStat(
    const StatisticType stat,
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type) {
  LatencyStatsMap* latency_map = GetLatencyStatsMapForMediaType(media_type);
  if (!latency_map) {
    return stats_list;
  }

  auto it = latency_map->find(stat);
  if (it != latency_map->end() && it->second.data_point_counter > 0) {
    const double avg_latency =
        InMilliseconds(it->second.sum_latency) / it->second.data_point_counter;
    stats_list[static_cast<int>(stat)] = avg_latency;
  }

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulateFrameBitrateStat(
    const StatisticsEventType event,
    const StatisticType stat,
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type,
    const Clock::time_point end_time) {
  FrameStatsMap* stats_map = GetFrameStatsMapForMediaType(media_type);
  if (!stats_map) {
    return stats_list;
  }

  auto it = stats_map->find(event);
  if (it != stats_map->end()) {
    const Clock::duration duration = end_time - start_time_;
    if (duration != Clock::duration::zero()) {
      const double kbps = it->second.sum_size / InMilliseconds(duration) * 8;
      stats_list[static_cast<int>(stat)] = kbps;
    }
  }

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulatePacketBitrateStat(
    const StatisticsEventType event,
    const StatisticType stat,
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type,
    const Clock::time_point end_time) {
  PacketStatsMap* stats_map = GetPacketStatsMapForMediaType(media_type);
  if (!stats_map) {
    return stats_list;
  }

  auto it = stats_map->find(event);
  if (it != stats_map->end()) {
    const Clock::duration duration = end_time - start_time_;
    if (duration != Clock::duration::zero()) {
      const double kbps = it->second.sum_size / InMilliseconds(duration) * 8;
      stats_list[static_cast<int>(stat)] = kbps;
    }
  }

  return stats_list;
}

SenderStats::StatisticsList StatisticsAnalyzer::PopulateSessionStats(
    SenderStats::StatisticsList stats_list,
    const StatisticsEventMediaType media_type,
    const Clock::time_point end_time) {
  SessionStats* session_stats = GetSessionStatsForMediaType(media_type);
  if (!session_stats) {
    return stats_list;
  }

  if (session_stats->first_event_time != Clock::time_point::min()) {
    stats_list[static_cast<int>(StatisticType::kFirstEventTimeMs)] =
        InMilliseconds(session_stats->first_event_time.time_since_epoch());
  }

  if (session_stats->last_event_time != Clock::time_point::min()) {
    stats_list[static_cast<int>(StatisticType::kLastEventTimeMs)] =
        InMilliseconds(session_stats->last_event_time.time_since_epoch());
  }

  if (session_stats->last_response_received_time != Clock::time_point::min()) {
    stats_list[static_cast<int>(
        StatisticType::kTimeSinceLastReceiverResponseMs)] =
        InMilliseconds(end_time - session_stats->last_response_received_time);
  }

  stats_list[static_cast<int>(StatisticType::kNumLateFrames)] =
      session_stats->late_frame_counter;

  return stats_list;
}

absl::optional<Clock::time_point> StatisticsAnalyzer::ToSenderTimestamp(
    Clock::time_point receiver_timestamp,
    StatisticsEventMediaType media_type) {
  const absl::optional<Clock::duration> receiver_offset =
      offset_estimator_->GetEstimatedOffset();
  if (!receiver_offset) {
    return {};
  }
  receiver_timestamp -= *receiver_offset;

  // Offset by the avg network latency, if available.
  LatencyStatsMap* latency_map = GetLatencyStatsMapForMediaType(media_type);
  if (latency_map) {
    auto it = latency_map->find(StatisticType::kAvgNetworkLatencyMs);
    if (it != latency_map->end() && it->second.data_point_counter > 0) {
      const Clock::duration avg_network_latency =
          it->second.sum_latency / it->second.data_point_counter;
      receiver_timestamp += avg_network_latency;
    }
  }

  return receiver_timestamp;
}
}  // namespace cast
}  // namespace openscreen
