// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_STATISTICS_ANALYZER_H_
#define CAST_STREAMING_STATISTICS_ANALYZER_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "cast/streaming/clock_offset_estimator.h"
#include "cast/streaming/statistics.h"
#include "cast/streaming/statistics_collector.h"
#include "platform/api/time.h"
#include "util/alarm.h"

namespace openscreen {
namespace cast {

class StatisticsAnalyzer {
 public:
  StatisticsAnalyzer(SenderStatsClient* stats_client,
                     ClockNowFunctionPtr now,
                     TaskRunner& task_runner,
                     std::unique_ptr<ClockOffsetEstimator> offset_estimator);
  ~StatisticsAnalyzer();

  void ScheduleAnalysis();

  // Get the statistics collector managed by this analyzer.
  StatisticsCollector* statistics_collector() {
    return statistics_collector_.get();
  }

 private:
  struct FrameStatsAggregate {
    int event_counter;
    uint32_t sum_size;
    Clock::duration sum_delay;
  };

  struct PacketStatsAggregate {
    int event_counter;
    uint32_t sum_size;
  };

  struct LatencyStatsAggregate {
    int data_point_counter;
    Clock::duration sum_latency;
  };

  struct FrameInfo {
    Clock::time_point capture_begin_time = Clock::time_point::min();
    Clock::time_point capture_end_time = Clock::time_point::min();
    Clock::time_point encode_end_time = Clock::time_point::min();
  };

  struct PacketInfo {
    Clock::time_point timestamp;
    StatisticsEventType type;
  };

  struct SessionStats {
    Clock::time_point first_event_time = Clock::time_point::max();
    Clock::time_point last_event_time = Clock::time_point::min();
    Clock::time_point last_response_received_time = Clock::time_point::min();
    int late_frame_counter = 0;
  };

  using FrameStatsMap = std::map<StatisticsEventType, FrameStatsAggregate>;
  using PacketStatsMap = std::map<StatisticsEventType, PacketStatsAggregate>;
  using LatencyStatsMap = std::map<StatisticType, LatencyStatsAggregate>;

  using FrameInfoMap = std::map<RtpTimeTicks, FrameInfo>;
  using PacketKey = std::pair<RtpTimeTicks, uint16_t>;
  using PacketInfoMap = std::map<PacketKey, PacketInfo>;

  // Initialize the stats histograms with the preferred min, max, and width.
  void InitHistograms();

  // Takes the Frame and Packet events from the `collector_`, and processes them
  // into a form expected by `stats_client_`. Then sends the stats, and
  // schedules a future analysis.
  void AnalyzeStatistics();

  // Constructs a stats list, and sends it to `stats_client_`;
  void SendStatistics();

  // Handles incoming stat events, and adds their infos to all of the proper
  // stats maps / aggregates.
  void ProcessFrameEvents(const std::vector<FrameEvent> frame_events);
  void ProcessPacketEvents(const std::vector<PacketEvent> packet_events);
  void RecordFrameLatencies(const FrameEvent frame_event);
  void RecordPacketLatencies(const PacketEvent packet_event);
  void RecordEventTimes(const Clock::time_point timestamp,
                        const StatisticsEventMediaType media_type,
                        const bool is_receiver_event);
  void ErasePacketInfo(const PacketEvent packet_event);
  void AddToLatencyAggregrate(const StatisticType latency_stat,
                              const Clock::duration latency_delta,
                              const StatisticsEventMediaType media_type);
  void AddToHistogram(const HistogramType histogram,
                      const StatisticsEventMediaType media_type,
                      const int64_t sample);

  // Gets a reference to the appropriate object based on `media_type`.
  FrameStatsMap* GetFrameStatsMapForMediaType(
      const StatisticsEventMediaType media_type);
  PacketStatsMap* GetPacketStatsMapForMediaType(
      const StatisticsEventMediaType media_type);
  LatencyStatsMap* GetLatencyStatsMapForMediaType(
      const StatisticsEventMediaType media_type);
  SessionStats* GetSessionStatsForMediaType(
      const StatisticsEventMediaType media_type);
  FrameInfoMap* GetRecentFrameInfosForMediaType(
      const StatisticsEventMediaType media_type);
  PacketInfoMap* GetRecentPacketInfosForMediaType(
      const StatisticsEventMediaType media_type);

  // Create copies of the stat histograms in their current stats, and return
  // them as a list.
  SenderStats::HistogramsList GetAudioHistograms();
  SenderStats::HistogramsList GetVideoHistograms();

  // Creates a stats list, and populates the entries based on stored stats info
  // / aggregates for each stat field.
  SenderStats::StatisticsList ConstructStatisticsList(
      const Clock::time_point end_time,
      const StatisticsEventMediaType media_type);
  SenderStats::StatisticsList PopulatePacketCountStat(
      const StatisticsEventType event,
      const StatisticType stat,
      SenderStats::StatisticsList stats_list,
      const StatisticsEventMediaType media_type);
  SenderStats::StatisticsList PopulateFrameCountStat(
      const StatisticsEventType event,
      const StatisticType stat,
      SenderStats::StatisticsList stats_list,
      const StatisticsEventMediaType media_type);
  SenderStats::StatisticsList PopulateFpsStat(
      const StatisticsEventType event,
      const StatisticType stat,
      SenderStats::StatisticsList stats_list,
      StatisticsEventMediaType media_type,
      const Clock::time_point end_time);
  SenderStats::StatisticsList PopulateAvgLatencyStat(
      const StatisticType stat,
      SenderStats::StatisticsList stats_list,
      const StatisticsEventMediaType media_type);
  SenderStats::StatisticsList PopulateFrameBitrateStat(
      const StatisticsEventType event,
      const StatisticType stat,
      SenderStats::StatisticsList stats_list,
      const StatisticsEventMediaType media_type,
      const Clock::time_point end_time);
  SenderStats::StatisticsList PopulatePacketBitrateStat(
      const StatisticsEventType event,
      const StatisticType stat,
      const SenderStats::StatisticsList stats_list,
      StatisticsEventMediaType media_type,
      const Clock::time_point end_time);
  SenderStats::StatisticsList PopulateSessionStats(
      SenderStats::StatisticsList stats_list,
      const StatisticsEventMediaType media_type,
      const Clock::time_point end_time);

  // Calculates the offset between the sender and receiver clocks and returns
  // the sender-side version of this receiver timestamp, if possible.
  absl::optional<Clock::time_point> ToSenderTimestamp(
      Clock::time_point receiver_timestamp,
      StatisticsEventMediaType media_type);

  // The statistics client to which we report analyzed statistics.
  SenderStatsClient* const stats_client_;

  // The statistics collector from which we take the un-analyzed stats packets.
  std::unique_ptr<StatisticsCollector> statistics_collector_;

  // Keeps track of the best-guess clock offset between the sender and receiver.
  std::unique_ptr<ClockOffsetEstimator> offset_estimator_;

  // Keep track of time and events for this analyzer.
  ClockNowFunctionPtr now_;
  Alarm alarm_;
  Clock::time_point start_time_;

  // Maps of frame / packet infos used for stats that rely on seeing multiple
  // events. For example, network latency is the calculated time difference
  // between went a packet is sent, and when it is received.
  FrameInfoMap audio_recent_frame_infos_;
  FrameInfoMap video_recent_frame_infos_;
  PacketInfoMap audio_recent_packet_infos_;
  PacketInfoMap video_recent_packet_infos_;

  // Aggregate stats for particular event types.
  FrameStatsMap audio_frame_stats_;
  FrameStatsMap video_frame_stats_;
  PacketStatsMap audio_packet_stats_;
  PacketStatsMap video_packet_stats_;

  // Aggregates related to latency-type stats.
  LatencyStatsMap audio_latency_stats_;
  LatencyStatsMap video_latency_stats_;

  // Stats that relate to the entirety of the session. For example, total late
  // frames, or time of last event.
  SessionStats audio_session_stats_;
  SessionStats video_session_stats_;

  // Histograms
  SenderStats::HistogramsList audio_histograms_;
  SenderStats::HistogramsList video_histograms_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_STATISTICS_ANALYZER_H_
