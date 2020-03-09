#include "PerfEventUtils.h"

#include <linux/perf_event.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <fstream>
#include "absl/strings/str_format.h"
#include "Logging.h"

namespace LinuxTracing {
namespace {
perf_event_attr generic_event_attr() {
  perf_event_attr pe{};
  pe.size = sizeof(struct perf_event_attr);
  pe.sample_period = 1;
  pe.use_clockid = 1;
  pe.clockid = CLOCK_MONOTONIC;
  pe.sample_id_all = 1;  // also include timestamps for lost events
  pe.disabled = 1;

  // We can set these even if we do not do sampling, as without the
  // PERF_SAMPLE_STACK_USER or PERF_SAMPLE_REGS_USER flags being set in
  // perf_event_attr::sample_type they will not be used anyways.
  pe.sample_stack_user = SAMPLE_STACK_USER_SIZE;
  pe.sample_regs_user = SAMPLE_REGS_USER_ALL;

  pe.sample_type = SAMPLE_TYPE_BASIC_FLAGS;

  return pe;
}

int32_t generic_event_open(perf_event_attr* attr, pid_t pid, int32_t cpu) {
  int32_t fd = perf_event_open(attr, pid, cpu, -1, 0);
  if (fd == -1) {
    ERROR("perf_event_open: %s", strerror(errno));
  }
  return fd;
}

perf_event_attr uprobe_event_attr(const char* module,
                                  uint64_t function_offset) {
  perf_event_attr pe = generic_event_attr();

  pe.type = 7;  // TODO: should be read from
                //  "/sys/bus/event_source/devices/uprobe/type"
  pe.config1 =
      reinterpret_cast<uint64_t>(module);  // pe.config1 == pe.uprobe_path
  pe.config2 = function_offset;            // pe.config2 == pe.probe_offset

  return pe;
}
}  // namespace

int32_t task_event_open(int32_t cpu) {
  perf_event_attr pe = generic_event_attr();
  pe.type = PERF_TYPE_SOFTWARE;
  pe.config = PERF_COUNT_SW_DUMMY;
  pe.task = 1;

  return generic_event_open(&pe, -1, cpu);
}

int32_t pid_context_switch_event_open(pid_t pid) {
  perf_event_attr pe = generic_event_attr();
  pe.type = PERF_TYPE_SOFTWARE;
  pe.config = PERF_COUNT_SW_DUMMY;
  pe.context_switch = 1;

  return generic_event_open(&pe, pid, -1);
}

int32_t cpu_context_switch_event_open(int32_t cpu) {
  perf_event_attr pe = generic_event_attr();
  pe.type = PERF_TYPE_SOFTWARE;
  pe.config = PERF_COUNT_SW_DUMMY;
  pe.context_switch = 1;

  return generic_event_open(&pe, -1, cpu);
}

int32_t sample_mmap_task_event_open(pid_t pid, uint64_t period_ns) {
  perf_event_attr pe = generic_event_attr();
  pe.type = PERF_TYPE_SOFTWARE;
  pe.config = PERF_COUNT_SW_CPU_CLOCK;
  pe.sample_period = period_ns;
  pe.sample_type |= PERF_SAMPLE_STACK_USER | PERF_SAMPLE_REGS_USER;
  // Also record mmaps, ...
  pe.mmap = 1;
  // ... forks, and termination.
  pe.task = 1;

  return generic_event_open(&pe, pid, -1);
}

int32_t uprobe_event_open(const char* module, uint64_t function_offset,
                          pid_t pid, int32_t cpu) {
  perf_event_attr pe = uprobe_event_attr(module, function_offset);
  pe.config = 0;

  return generic_event_open(&pe, pid, cpu);
}

int32_t uprobe_stack_event_open(const char* module, uint64_t function_offset,
                                pid_t pid, int32_t cpu) {
  perf_event_attr pe = uprobe_event_attr(module, function_offset);
  pe.config = 0;
  pe.sample_type |= PERF_SAMPLE_STACK_USER | PERF_SAMPLE_REGS_USER;

  return generic_event_open(&pe, pid, cpu);
}

int32_t uretprobe_event_open(const char* module, uint64_t function_offset,
                             pid_t pid, int32_t cpu) {
  perf_event_attr pe = uprobe_event_attr(module, function_offset);
  pe.config = 1;  // Set bit 0 of config for uretprobe.

  return generic_event_open(&pe, pid, cpu);
}

int32_t uretprobe_stack_event_open(const char* module, uint64_t function_offset,
                                   pid_t pid, int32_t cpu) {
  perf_event_attr pe = uprobe_event_attr(module, function_offset);
  pe.config = 1;  // Set bit 0 of config for uretprobe.
  pe.sample_type |= PERF_SAMPLE_STACK_USER | PERF_SAMPLE_REGS_USER;

  return generic_event_open(&pe, pid, cpu);
}

int32_t get_tracepoint_id(const char* tracepoint_category,
                          const char* tracepoint_name) {
  std::string filename =
      absl::StrFormat("/sys/kernel/debug/tracing/events/%s/%s/id",
                      tracepoint_category, tracepoint_name);
  LOG("Tracepoint filename: %s\n", filename.c_str());
  std::ifstream id_file{filename};
  int32_t tp_id = -1;
  std::string line;
  if (!std::getline(id_file, line)) {
    return -1;
  }
  return std::atoi(line.c_str());
}

int32_t tracepoint_event_open(const char* tracepoint_category, const char* tracepoint_name,
                              pid_t pid, int32_t cpu) {
  int tp_id = get_tracepoint_id(tracepoint_category, tracepoint_name);
  LOG("tracepoint id: %d", tp_id);
  perf_event_attr pe = {};
  pe.type = PERF_TYPE_TRACEPOINT;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = tp_id;
  pe.sample_type = PERF_SAMPLE_RAW;
  pe.disabled = 1;
  pe.sample_period = 1;
  pe.use_clockid = 1;
  pe.clockid = CLOCK_MONOTONIC;

  return generic_event_open(&pe, pid, cpu);
}

void print_amdgpu_sched_run_job(const AmdGpuSchedRunJobFormat* format) {
  // Extract data from data_loc. This field is a 32-bit number that encodes the size
  // (high 16-bit) and offset (low 16-bit) of a dynamically sized string, in this
  // case the timeline string ("gfx", "sdma0", etc.) It seems that this string is
  // null terminated.
  int32_t data_loc = *reinterpret_cast<const int32_t*>(&format->timeline);
  int16_t data_loc_size = static_cast<int16_t>(data_loc >> 16);
  int16_t data_loc_offset = static_cast<int16_t>(data_loc & 0x00ff);

  std::vector<char> data_loc_data(data_loc_size);;
  std::memcpy(&data_loc_data[0], reinterpret_cast<const char*>(format) + data_loc_offset, data_loc_size);

  LOG("    common_type: %d\n", format->common_type);
  LOG("    common_flags: %d\n", format->common_flags);
  LOG("    common_preempt_count: %d\n", format->common_preempt_count);
  LOG("    common_pid: %d\n", format->common_pid);
  LOG("    sched_job_id: %d\n", format->sched_job_id);
  LOG("    timeline size: %d\n", data_loc_size);
  LOG("    timeline offset: %d\n", data_loc_offset);
  LOG("    timeline string: %s\n", &data_loc_data[0]);
  LOG("    context: %d\n", format->context);
  LOG("    seqno: %d\n", format->seqno);
  // The type of this is declared as char* in this event's format file (/sys/kernel/debug/...)
  // of size 8. Printing this as a hexadecimal value matches the output that 'perf script'
  // produces.
  LOG("    ring_name: %llx\n", format->ring_name);
  LOG("    num_ibs: %d\n", format->num_ibs);
}

void print_amdgpu_cs_ioctl(const AmdGpuCsIoctlFormat* format) {
  // Extract data from data_loc. This field is a 32-bit number that encodes the size
  // (high 16-bit) and offset (low 16-bit) of a dynamically sized string, in this
  // case the timeline string ("gfx", "sdma0", etc.) It seems that this string is
  // null terminated.
  int32_t data_loc = *reinterpret_cast<const int32_t*>(&format->timeline);
  int16_t data_loc_size = static_cast<int16_t>(data_loc >> 16);
  int16_t data_loc_offset = static_cast<int16_t>(data_loc & 0x00ff);

  std::vector<char> data_loc_data(data_loc_size);;
  std::memcpy(&data_loc_data[0], reinterpret_cast<const char*>(format) + data_loc_offset, data_loc_size);

  LOG("    common_type: %d\n", format->common_type);
  LOG("    common_flags: %d\n", format->common_flags);
  LOG("    common_preempt_count: %d\n", format->common_preempt_count);
  LOG("    common_pid: %d\n", format->common_pid);
  LOG("    sched_job_id: %d\n", format->sched_job_id);
  LOG("    timeline size: %d\n", data_loc_size);
  LOG("    timeline offset: %d\n", data_loc_offset);
  LOG("    timeline string: %s\n", &data_loc_data[0]);
  LOG("    context: %d\n", format->context);
  LOG("    seqno: %d\n", format->seqno);
  // This is a pointer to a dma_fence struct. This does not get printed when running 'perf script'
  // for this event, so we can probably ignore this. This is 0 in any experiments I've done.
  LOG("    dma_fence: %llx\n", format->dma_fence);
  // The type of this is declared as char* in this event's format file (/sys/kernel/debug/...)
  // of size 8. Printing this as a hexadecimal value matches the output that 'perf script'
  // produces.
  LOG("    ring_name: %llx\n", format->ring_name);
  LOG("    num_ibs: %d\n", format->num_ibs);
}

void print_dma_fence_signaled(const DmaFenceSignaledFormat* format) {
  int32_t driver = *reinterpret_cast<const int32_t*>(&format->driver);
  int16_t driver_size = static_cast<int16_t>(driver >> 16);
  int16_t driver_offset = static_cast<int16_t>(driver & 0x00ff);

  std::vector<char> driver_data(driver_size);
  std::memcpy(&driver_data[0], reinterpret_cast<const char*>(format) + driver_offset, driver_size);

  // Extract data from data_loc. This field is a 32-bit number that encodes the size
  // (high 16-bit) and offset (low 16-bit) of a dynamically sized string, in this
  // case the timeline string ("gfx", "sdma0", etc.) It seems that this string is
  // null terminated.
  int32_t timeline = *reinterpret_cast<const int32_t*>(&format->timeline);
  int16_t timeline_size = static_cast<int16_t>(timeline >> 16);
  int16_t timeline_offset = static_cast<int16_t>(timeline & 0x00ff);

  std::vector<char> timeline_data(timeline_size);
  std::memcpy(&timeline_data[0], reinterpret_cast<const char*>(format) + timeline_offset, timeline_size);

  LOG("    common_type: %d\n", format->common_type);
  LOG("    common_flags: %d\n", format->common_flags);
  LOG("    common_preempt_count: %d\n", format->common_preempt_count);
  LOG("    common_pid: %d\n", format->common_pid);
  LOG("    driver: %s\n", &driver_data[0]);
  LOG("    timeline: %s\n", &timeline_data[0]);
  LOG("    context: %d\n", format->context);
  LOG("    seqno: %d\n", format->seqno);
}

}  // namespace LinuxTracing
