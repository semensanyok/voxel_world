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
  std::vector<std::function<void()>> tasks;
};
#endif
