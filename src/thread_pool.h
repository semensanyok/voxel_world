#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <functional>
#include <thread>
#include <vector>

/*
 * for starter keep simple, 4 cores 4 threads, no dedicated threads, each task
 * is self sufficient in terms of locking. is there requirement for having
 * graphics queues on dedicated thread though?
 */
class ThreadPool {
  std::vector<std::thread> worker_threads;
  // hw thread index to fetch thread locals, such as Vulkan command pool. dont
  // want to use std::thread_local (unreasoned)
  std::vector<std::vector<std::function<unsigned int>>> tasks;
};
#endif
