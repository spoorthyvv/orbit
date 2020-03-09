#ifndef ORBIT_LINUX_TRACING_GPU_TRACEPOINT_EVENT_PROCESSOR
#define ORBIT_LINUX_TRACING_GPU_TRACEPOINT_EVENT_PROCESSOR

#include <vector>
#include <tuple>
#include "Logging.h"
#include "PerfEventUtils.h"
#include "absl/container/flat_hash_map.h"

namespace LinuxTracing {

class GpuTracepointEventProcessor {

  typedef std::tuple<uint32_t, uint32_t, std::string> Key;

 public:
  GpuTracepointEventProcessor() {
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

  void AddTracepointEvent(const perf_event_header& header, const std::vector<uint8_t>& data) {
    // The samples we get from the ring buffer still contain the perf_event_header.
    // In addition, the next 4 bytes after the header are the size of the sample.
    // That is, the raw sample starts at sizeof(perf_event_header) + 4.
    int offset = sizeof(header);
    uint32_t size_of_raw_sample =
        *reinterpret_cast<const uint32_t*>(&data[0] + offset);
    offset += sizeof(uint32_t);
    int32_t common_type = static_cast<int32_t>(*reinterpret_cast<const uint16_t*>(&data[0] + offset));
    if (common_type == amdgpu_cs_ioctl_common_type) {
      const AmdGpuCsIoctlFormat* format = reinterpret_cast<const AmdGpuCsIoctlFormat*>(&data[0] + offset);
      uint32_t context = format->context;
      uint32_t seqno = format->seqno;
      std::string timeline_string("gfx");  // TODO

      Key key = std::make_tuple(context, seqno, timeline_string);
      auto hw_finished_it = hw_finished_events_.find(key);
      auto hw_it = hw_scheduling_events_.find(key);

      if (hw_finished_it != hw_finished_events_.end() && hw_it != hw_scheduling_events_.end()) {
        // TODO: Need timestamps!
        GpuExecutionEvent gpu_event(format->common_pid, "gfx", seqno, context, 0, 0, 0);
        listener_->OnGpuExecutionEvent(gpu_event);

        hw_finished_events_.erase(key);
        hw_scheduling_events_.erase(key);
      } else {
        user_scheduling_events_.emplace(key, *format);
      }
    } else if (common_type == amdgpu_sched_run_job_common_type) {
      const AmdGpuSchedRunJobFormat* format = reinterpret_cast<const AmdGpuSchedRunJobFormat*>(&data[0] + offset);
      uint32_t context = format->context;
      uint32_t seqno = format->seqno;
      std::string timeline_string("gfx");  // TODO
      Key key = std::make_tuple(context, seqno, timeline_string);
      auto user_it = user_scheduling_events_.find(key);
      auto hw_finished_it = hw_finished_events_.find(key);

      if (user_it != user_scheduling_events_.end() && hw_finished_it != hw_finished_events_.end()) {
        // TODO: Need timestamps!
        GpuExecutionEvent gpu_event(format->common_pid, "gfx", seqno, context, 0, 0, 0);
        listener_->OnGpuExecutionEvent(gpu_event);

        user_scheduling_events_.erase(key);
        hw_finished_events_.erase(key);
      } else {
        hw_scheduling_events_.emplace(key, *format);
      }
    } else if (common_type == dma_fence_signaled_common_type) {
      const DmaFenceSignaledFormat* format = reinterpret_cast<const DmaFenceSignaledFormat*>(&data[0] + offset);
      uint32_t context = format->context;
      uint32_t seqno = format->seqno;
      std::string timeline_string("gfx");  // TODO
      Key key = std::make_tuple(context, seqno, timeline_string);

      auto user_it = user_scheduling_events_.find(key);
      auto hw_it = hw_scheduling_events_.find(key);

      if (user_it != user_scheduling_events_.end() && hw_it != hw_scheduling_events_.end()) {
        // TODO: Need timestamps!
        GpuExecutionEvent gpu_event(format->common_pid, "gfx", seqno, context, 0, 0, 0);
        listener_->OnGpuExecutionEvent(gpu_event);

        user_scheduling_events_.erase(key);
        hw_scheduling_events_.erase(key);
      } else {
        hw_finished_events_.emplace(key, *format);
      }
    } else {
      assert(false);
    }
  }

  void SetListener(TracerListener* listener) {
    listener_ = listener;
  }

 private:
  int32_t amdgpu_cs_ioctl_common_type = -1;
  int32_t amdgpu_sched_run_job_common_type = -1;
  int32_t dma_fence_signaled_common_type = -1;

  TracerListener* listener_;

  absl::flat_hash_map<Key, AmdGpuCsIoctlFormat> user_scheduling_events_;
  absl::flat_hash_map<Key, AmdGpuSchedRunJobFormat> hw_scheduling_events_;
  absl::flat_hash_map<Key, DmaFenceSignaledFormat> hw_finished_events_;
  
};

}

#endif
