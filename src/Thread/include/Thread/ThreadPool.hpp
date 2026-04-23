#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

namespace utilities {

template <std::size_t N>
  requires(N > 0 && N <= 100)
class ThreadPool {

public:
  ThreadPool() {}

  ~ThreadPool() {}

  template <typename _Signature> void addTask(std::function<_Signature> task) {
    lock();
    mQueue.push(task);
    unlock();
  }

  void dispatch() {
    std::cout << "dispatched!\n";
    while (!mInterrupt) {
      lock();
      auto it = std::find_if(mThreadPool.begin(), mThreadPool.end(),
                             [](const std::thread& thread) { return !thread.joinable(); });
      if (it == mThreadPool.end()) {
        std::clog << "Could not find an empty thread!\n";
        continue;
      }
      if (mQueue.empty()) {
        std::clog << "\nEmpty!\n";
        interrupt();
        unlock();
        return;
      }
      Task<void()> a{mQueue.front()};
      mQueue.pop();
      mThreadPool[it - mThreadPool.begin()] = std::move(std::thread([&a]() { a.task(); }));
      mThreadPool[it - mThreadPool.begin()].detach();
      for (std::thread& tr : mThreadPool) {
        if (tr.joinable()) {
          tr.join();
        };
      }
      // TODO a detached thread pool handler that handles the joining as well???
      unlock();
    }
  }

  void interrupt() { mInterrupt = true; }

  std::size_t poolSize() const { return mThreadPool.size(); }

  std::array<std::thread, N>& pool() { return mThreadPool; }

private:
  template <typename _Signature> struct Task {
    Task(std::function<_Signature> function) : task(function), isDone(false) {}
    std::function<_Signature> task;
    std::atomic<bool> isDone{false};
  };

private:
  void lock() { mLock.lock(); }

  void unlock() { mLock.unlock(); }

  std::mutex mLock;
  std::array<std::thread, N> mThreadPool;
  std::queue<std::function<void()>> mQueue{};
  std::atomic<bool> mInterrupt{false};
};
}  // namespace utilities