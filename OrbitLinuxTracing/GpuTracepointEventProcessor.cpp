#include "GpuTracepointEventProcessor.h"

#include <vector>
#include <tuple>
#include "Logging.h"
#include "PerfEventUtils.h"
#include "absl/container/flat_hash_map.h"

namespace LinuxTracing {

GpuTracepointEventProcessor::GpuTracepointEventProcessor() {
  amdgpu_cs_ioctl_common_type = LinuxTracing::get_tracepoint_id("amdgpu", "amdgpu_cs_ioctl");
  // TODO: Proper error handling.
  assert(amdgpu_cs_ioctl_common_type != -1);
  amdgpu_sched_run_job_common_type = get_tracepoint_id("amdgpu", "amdgpu_sched_run_job");
  // TODO: Proper error handling.
  assert(amdgpu_sched_run_job_common_type != -1);
  dma_fence_signaled_common_type = get_tracepoint_id("dma_fence", "dma_fence_signaled");
  // TODO: Proper error handling.
  assert(dma_fence_signaled_common_type != -1);
}

int GpuTracepointEventProcessor::ComputeDepthForEvent(const std::string& timeline,
                                                      uint64_t start_timestamp, uint64_t end_timestamp) {
  if (timeline_to_latest_timestamp_per_depth_.count(timeline) == 0) {
    std::vector<uint64_t> vec;
    timeline_to_latest_timestamp_per_depth_.emplace(timeline, vec);
  }
  auto it = timeline_to_latest_timestamp_per_depth_.find(timeline);
  std::vector<uint64_t>& vec = it->second;

  for (int d = 0; d < vec.size(); ++d) {
    if (start_timestamp >= (vec[d] + 20 * 1000000)) {
      vec[d] = end_timestamp;
      return d;
    }
  }
  vec.push_back(end_timestamp);
  return static_cast<int>(vec.size());
}

void GpuTracepointEventProcessor::HandleUserSchedulingEvent(const perf_sample_id& sample_id,
                                                            const AmdGpuCsIoctlFormat* format,
                                                            uint64_t timestamp_ns) {
  uint32_t context = format->context;
  uint32_t seqno = format->seqno;

  std::string timeline = ExtractTimelineString<AmdGpuCsIoctlFormat>(format);
  Key key = std::make_tuple(context, seqno, timeline);
  auto hw_finished_it = hw_finished_events_.find(key);
  auto hw_it = hw_scheduling_events_.find(key);

  if (hw_finished_it != hw_finished_events_.end() && hw_it != hw_scheduling_events_.end()) {
    if (timeline_to_latest_hw_finish_.count(timeline) == 0) {
      timeline_to_latest_hw_finish_.emplace(timeline, hw_it->second.sample_id.time);
    }
    auto it = timeline_to_latest_hw_finish_.find(timeline);
    uint64_t hw_start_time = hw_it->second.sample_id.time;
    if (hw_start_time < it->second) {
      hw_start_time = it->second;
    }


    int depth = ComputeDepthForEvent(timeline, timestamp_ns, hw_finished_it->second.sample_id.time);
    GpuExecutionEvent gpu_event(format->common_pid, timeline,
                                seqno, context, depth,
                                timestamp_ns,
                                hw_it->second.sample_id.time,
                                hw_start_time,
                                hw_finished_it->second.sample_id.time);
    listener_->OnGpuExecutionEvent(gpu_event);

    it->second = hw_finished_it->second.sample_id.time;

    hw_finished_events_.erase(key);
    hw_scheduling_events_.erase(key);
  } else {
    UserEvent event;
    event.sample_id = sample_id;
    event.tp_data = *format;
    user_scheduling_events_.emplace(key, event);
  }
}

void GpuTracepointEventProcessor::HandleHardwareSchedulingEvent(const perf_sample_id& sample_id,
                                                                const AmdGpuSchedRunJobFormat* format,
                                                                uint64_t timestamp_ns) {
  uint32_t context = format->context;
  uint32_t seqno = format->seqno;

  std::string timeline = ExtractTimelineString<AmdGpuSchedRunJobFormat>(format);
  Key key = std::make_tuple(context, seqno, timeline);
  auto user_it = user_scheduling_events_.find(key);
  auto hw_finished_it = hw_finished_events_.find(key);

  if (user_it != user_scheduling_events_.end() && hw_finished_it != hw_finished_events_.end()) {
    if (timeline_to_latest_hw_finish_.count(timeline) == 0) {
      timeline_to_latest_hw_finish_.emplace(timeline, timestamp_ns);
    }
    auto it = timeline_to_latest_hw_finish_.find(timeline);
    uint64_t hw_start_time = timestamp_ns;
    if (hw_start_time < it->second) {
      hw_start_time = it->second;
    }

    int depth = ComputeDepthForEvent(timeline, user_it->second.sample_id.time, hw_finished_it->second.sample_id.time);
    GpuExecutionEvent gpu_event(format->common_pid, timeline,
                                seqno, context, depth,
                                user_it->second.sample_id.time,
                                timestamp_ns,
                                hw_start_time,
                                hw_finished_it->second.sample_id.time);
    listener_->OnGpuExecutionEvent(gpu_event);

    it->second = hw_finished_it->second.sample_id.time;

    user_scheduling_events_.erase(key);
    hw_finished_events_.erase(key);
  } else {
    HwScheduleEvent event;
    event.sample_id = sample_id;
    event.tp_data = *format;
    hw_scheduling_events_.emplace(key, event);
  }
}
void GpuTracepointEventProcessor::HandleHardwareFinishedEvent(const perf_sample_id& sample_id,
                                                              const DmaFenceSignaledFormat* format,
                                                              uint64_t timestamp_ns) {
  uint32_t context = format->context;
  uint32_t seqno = format->seqno;

  std::string timeline = ExtractTimelineString<DmaFenceSignaledFormat>(format);
  Key key = std::make_tuple(context, seqno, timeline);
  auto user_it = user_scheduling_events_.find(key);
  auto hw_it = hw_scheduling_events_.find(key);

  if (user_it != user_scheduling_events_.end() && hw_it != hw_scheduling_events_.end()) {
    if (timeline_to_latest_hw_finish_.count(timeline) == 0) {
      timeline_to_latest_hw_finish_.emplace(timeline, hw_it->second.sample_id.time);
    }
    auto it = timeline_to_latest_hw_finish_.find(timeline);
    uint64_t hw_start_time = hw_it->second.sample_id.time;
    if (hw_start_time < it->second) {
      hw_start_time = it->second;
    }

    int depth = ComputeDepthForEvent(timeline, user_it->second.sample_id.time, timestamp_ns);
    GpuExecutionEvent gpu_event(format->common_pid, timeline,
                                seqno, context, depth,
                                user_it->second.sample_id.time,
                                hw_it->second.sample_id.time,
                                hw_start_time,
                                timestamp_ns);
    listener_->OnGpuExecutionEvent(gpu_event);

    it->second = timestamp_ns;
    user_scheduling_events_.erase(key);
    hw_scheduling_events_.erase(key);
  } else {
    HwFinishEvent event;
    event.sample_id = sample_id;
    event.tp_data = *format;

    hw_finished_events_.emplace(key, event);
  }
}

void GpuTracepointEventProcessor::AddTracepointEvent(const perf_event_header& header, const std::vector<uint8_t>& data) {
  // The samples we get from the ring buffer still contain the perf_event_header.
  // After the header we receive the perf_sample_id struct, which contains basic
  // information like tid, pid, and the timestamp.
  // Finally, after this, the size of the raw sample is given in the next 4 bytes.
  int offset = sizeof(header);

  perf_sample_id sample_id = *reinterpret_cast<const perf_sample_id*>(&data[0] + offset);
  uint32_t tid = sample_id.tid;
  uint64_t timestamp_ns = sample_id.time;
  offset += sizeof(perf_sample_id);

  uint32_t raw_sample_size = *reinterpret_cast<const uint32_t*>(&data[0] + offset);
  offset += sizeof(uint32_t);
  int32_t common_type = static_cast<int32_t>(*reinterpret_cast<const uint16_t*>(&data[0] + offset));
  if (common_type == amdgpu_cs_ioctl_common_type) {
    const AmdGpuCsIoctlFormat* format = reinterpret_cast<const AmdGpuCsIoctlFormat*>(&data[0] + offset);
    HandleUserSchedulingEvent(sample_id, format, timestamp_ns);
  } else if (common_type == amdgpu_sched_run_job_common_type) {
    const AmdGpuSchedRunJobFormat* format = reinterpret_cast<const AmdGpuSchedRunJobFormat*>(&data[0] + offset);
    HandleHardwareSchedulingEvent(sample_id, format, timestamp_ns);
  } else if (common_type == dma_fence_signaled_common_type) {
    const DmaFenceSignaledFormat* format = reinterpret_cast<const DmaFenceSignaledFormat*>(&data[0] + offset);
    HandleHardwareFinishedEvent(sample_id, format, timestamp_ns);
  } else {
    assert(false);
  }
}

void GpuTracepointEventProcessor::SetListener(TracerListener* listener) {
  listener_ = listener;
}

}  // namespace LinuxTracing
