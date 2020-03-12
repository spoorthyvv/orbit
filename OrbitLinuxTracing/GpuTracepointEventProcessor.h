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

};

}

#endif
