#ifndef ORBIT_LINUX_TRACING_GPU_TRACEPOINT_EVENT_PROCESSOR
#define ORBIT_LINUX_TRACING_GPU_TRACEPOINT_EVENT_PROCESSOR

#include <tuple>
#include "PerfEventUtils.h"
#include "OrbitLinuxTracing/TracerListener.h"
#include "absl/container/flat_hash_map.h"

namespace LinuxTracing {

class GpuTracepointEventProcessor {

  typedef std::tuple<uint32_t, uint32_t, std::string> Key;

 public:
  GpuTracepointEventProcessor();

  void AddTracepointEvent(const perf_event_header& header, const std::vector<uint8_t>& data);

  void SetListener(TracerListener* listener);

 private:
  void HandleUserSchedulingEvent(const perf_sample_id& sample_id,
                                 const AmdGpuCsIoctlFormat* data, uint64_t timestamp_ns);
  void HandleHardwareSchedulingEvent(const perf_sample_id& sample_id,
                                     const AmdGpuSchedRunJobFormat* data, uint64_t timestamp_ns);
  void HandleHardwareFinishedEvent(const perf_sample_id& sample_id,
                                   const DmaFenceSignaledFormat* data, uint64_t timestamp_ns);

  int32_t amdgpu_cs_ioctl_common_type = -1;
  int32_t amdgpu_sched_run_job_common_type = -1;
  int32_t dma_fence_signaled_common_type = -1;

  TracerListener* listener_;

  struct UserEvent {
    perf_sample_id sample_id;
    AmdGpuCsIoctlFormat tp_data;
  };

  struct HwScheduleEvent {
    perf_sample_id sample_id;
    AmdGpuSchedRunJobFormat tp_data;
  };

  struct HwFinishEvent {
    perf_sample_id sample_id;
    DmaFenceSignaledFormat tp_data;
  };

  absl::flat_hash_map<Key, UserEvent> user_scheduling_events_;
  absl::flat_hash_map<Key, HwScheduleEvent> hw_scheduling_events_;
  absl::flat_hash_map<Key, HwFinishEvent> hw_finished_events_;

  std::vector<uint64_t> latest_timestamp_for_depth_;
  int ComputeDepthForEvent(uint64_t start_timestamp, uint64_t end_timestamp);

  // TODO: Fix this.
  template<typename T> std::string ExtractTimelineString(const T* format) {
  int32_t data_loc = *reinterpret_cast<const int32_t*>(&format->timeline);
  int16_t data_loc_size = static_cast<int16_t>(data_loc >> 16);
  int16_t data_loc_offset = static_cast<int16_t>(data_loc & 0x00ff);

  std::vector<char> data_loc_data(data_loc_size);;
  std::memcpy(&data_loc_data[0], reinterpret_cast<const char*>(format) + data_loc_offset, data_loc_size);
  return std::string(&data_loc_data[0]);
  }

};

}

#endif
