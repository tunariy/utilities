#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

namespace utilities {

enum class execution_policy : uint8_t { seq, parallel };

template <std::size_t Capacity, class Signature>
    requires(std::is_invocable_v<Signature> && Capacity > 0)
class ThreadPool {
  public:
    ThreadPool() noexcept = default;

    ~ThreadPool() noexcept(0) = default;

    ThreadPool(const ThreadPool&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;

  public:
    template <class Sign>
        requires(std::is_invocable_v<Sign>)
    auto addTask(std::function<Sign>&& func) -> void {
        std::scoped_lock lock{mLock};
        mQueue.push(func);
    }

    auto dispatch() -> void {
        std::jthread([this]() {
            while (!mInterrupt) {
                std::scoped_lock lock{mLock};
                auto it = std::find_if(
                    std::begin(mThreadPool), std::end(mThreadPool),
                    [](const std::thread& thread) { return !thread.joinable(); });

                if (it == std::end(mThreadPool)) continue;

                if (mQueue.empty()) {
                    mInterrupt = true;
                    continue;
                }

                mThreadPool[it - mThreadPool.begin()] =
                    std::move(std::thread(mQueue.front()));
                mQueue.pop();

                mThreadPool[it - mThreadPool.begin()].detach();
            }
        }).detach();
    }

    void interrupt() { mInterrupt = true; }

    std::size_t poolSize() const { return mThreadPool.size(); }

    std::size_t queueSize() const { return mQueue.size(); }

  private:
    std::array<std::thread, Capacity> mThreadPool{};
    std::queue<std::function<Signature>> mQueue{};
    std::mutex mLock{};
    std::atomic<bool> mInterrupt{false};
};
}  // namespace utilities