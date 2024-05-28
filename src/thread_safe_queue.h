// #ifndef VW_THREAD_SAFE_QUEUE
// #define VW_THREAD_SAFE_QUEUE
//
// #include <atomic>
// #include <thread>
//
// using namespace std;
//
// template <typename T>
// class ThreadSafeQueue {
// public:
//   queue<T> container;
//   inline ThreadSafeQueue() : container{ queue<T>() } {}
//   inline ~ThreadSafeQueue() {}
//   inline queue<T> PopAll()
//   {
//     lock_guard<mutex> mlock(mtx);
//     auto res = queue<T>(container);
//     container = queue<T>();
//     return res;
//   }
//   inline queue<T> WaitPopAll(bool& quit)
//   {
//     unique_lock<mutex> mlock(mtx);
//     while (container.empty() && !quit)
//     {
//       push_event.wait(mlock);
//     }
//     auto res = queue<T>(container);
//     container = queue<T>();
//     return res;
//   }
//   inline T WaitPop(bool& quit)
//   {
//     unique_lock<mutex> mlock(mtx);
//     while (container.empty() && !quit)
//     {
//       push_event.wait(mlock);
//     }
//     auto element = container.front();
//     container.pop();
//     return element;
//   }
//   inline void Push(const T& element)
//   {
//     unique_lock<mutex> mlock(mtx);
//     container.push(element);
//     mlock.unlock();
//     push_event.notify_all();
//   }
// };
//
// #endif
