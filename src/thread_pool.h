#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "renderer_context.h"
#include <atomic>
#include <functional>
#include <future>
#include <thread>
#include <vector>
#include <vulkan/vulkan_core.h>

enum class ThreadCapabilities { Transfer, Render };

enum class TaskType {
  Transfer,
  Buffer,
  Render
  // ....... etc. first draft. TODO:
};

struct ThreadCommandBuffers {
  // TODO: https://developer.nvidia.com/blog/vulkan-dos-donts/#work_submission
  // 20/01/2025
  // Work Submission
  // Do
  // Build command buffers in parallel and evenly across several threads/cores.
  // Recording commands is a CPU intensive operation, however multi-threading is
  // a readily available solution that the API was designed for. Be aware of the
  // cost of setting up and resetting a command buffer. A reasonable number of
  // command buffers are required for efficient parallel work submission. Try to
  // minimize the number of queue submissions. Each vkQueueSubmit() has a
  // significant performance cost on CPU, so lower is generally better. If
  // command recording on the CPU is heavy, aggressive queue submission batching
  // may result in additional latency penalty, while performance may remain the
  // same. If latency is important for your application consider submitting GPU
  // workloads earlier.
  // !!!!!!! Functions such as vkAllocateCommandBuffers(),
  // vkBeginCommandBuffer(), and vkEndCommandBuffer() should be called
  // !!!! from the thread that fills the command buffer.
  // !!!! These calls take measurable time on CPU and therefore should not be
  // collected in a specific thread. Check for gaps in execution on GPU using
  // Nsight Systems, GPUView, or
  // NvAPI_GPU_ClientRegisterForUtilizationSampleUpdates. Reuse command buffers
  // when possible. Secondary command buffers can be helpful here, depending on
  // the workload – check carefully to determine if they are actually
  // advantageous. Use VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT for command
  // buffers that will be submitted only once. Use
  // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT only when it’s really
  // necessary, it may affect GPU performance.
  //

  // Initial idea was to signal main thread that it can ack read and submit
  // if transfer ahead and submission of prepared_transfer not finished - swap
  // next_transfer?
  // ....... too complex
  // VkCommandBuffer prepared_transfer;
  // VkCommandBuffer next_transfer;

  // Better to use godot approach. Shared queue. Thread that records Command
  // buffer locks it on submit.
  VkCommandBuffer transfer;

  // Command pools are externally synchronized, meaning that a command pool must
  // not be used concurrently in multiple threads. That includes use via
  // recording commands on any command buffers allocated from the pool, as well
  // as operations that allocate, free, and reset command buffers or the pool
  // itself.
  VkCommandPool pool;
};

class ThreadLocals {
  // non negative for threads which were asigned transfer queues
  // index into
  int transfer_queue_index = -1;
  bool is_render_thread;
  std::atomic_bool is_ready_to_submit;

  // pools for transfer and draw differ in init flags
  ThreadCommandBuffers transfer_command;
  ThreadCommandBuffers dynamic_draw_command;
};

static thread_local ThreadLocals;
class Task {
  TaskType type;
  std::function<void(unsigned int)> f;
}

/*
 * for starter keep simple, 4 cores 4 threads, no dedicated threads, each task
 * is self sufficient in terms of locking. is there requirement for having
 * graphics queues on dedicated thread though?
 *
 * 01.01.2025:
 *
 */
class ThreadPool {
  std::vector<std::thread> worker_threads;
  std::vector<Task> tasks;

  std::thread render;
  std::vector<std::thread> transfer;
};
#endif
