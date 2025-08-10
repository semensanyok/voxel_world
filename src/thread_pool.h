#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "renderer_context.h"
#include "vw_constants.h"
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <vulkan/vulkan_core.h>

enum class ThreadCapabilities { Transfer, Render, Compute };

// ????
// enum class TaskType {
//   Transfer,
//   Buffer,
//   Render
//   // ....... etc. first draft. TODO:
//   // ???
//   CullOctreeGatherGeometry
// };

class ThreadLocals {
  TransferWorker transfer_worker;
};

class ThreadPool {
public:
  // has all workers except main.
  std::vector<std::thread> worker_threads;

  void init() {
    worker_threads.resize(1);
    auto &worker_thread = worker_threads.at(0);
    // assign culling and vertex gathering tasks
    // worker_thread = std::thread();
  };
};
#endif
