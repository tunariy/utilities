#pragma once

#include <algorithm>
#include <array>
#include <bits/floatn-common.h>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <memory>
#include <mutex>

namespace utilities {
/*
 *  Thread-safe pool allocator
 */
template <std::size_t S> class PoolAllocator {
  static constexpr std::size_t N = S + 1;

public:
  PoolAllocator()
      : mPool(::operator new(N * sizeof(uint8_t)),
              [](void* p) { ::operator delete(p, N * sizeof(uint8_t)); }) {}

  PoolAllocator(PoolAllocator&& other) = delete;

  PoolAllocator& operator=(PoolAllocator&& other) = delete;

  [[nodiscard]] void* allocate(std::size_t n) {
    std::scoped_lock<std::mutex> guard(mLock);
    if (n > capacity()) {
      throw std::bad_alloc();
    }

    auto emptyBlock = getFirstEmptyBlock(offsets().begin(), n);
    if (emptyBlock >= offsets().end()) {
      throw std::bad_alloc();
    }

    std::fill(std::execution::par, emptyBlock, emptyBlock + n, 1);

    return static_cast<void*>(static_cast<uint8_t*>(mPool.get()) +
                              (emptyBlock - offsets().begin()));
  };

  void deallocate(uint8_t* at, std::size_t n) noexcept {
    std::scoped_lock<std::mutex> guard(mLock);
    const std::size_t offset = at - static_cast<uint8_t*>(mPool.get());

    for (std::size_t i{offset}; i < offset + n; i++) {
      offsets()[i] = 0;
    }
  }

  void deallocate(uint8_t* at) noexcept {
    std::scoped_lock<std::mutex> guard(mLock);
    const std::size_t offset = at - static_cast<uint8_t*>(mPool.get());
    offsets()[offset] = 0;
  }

  size_t capacity() { return offsets().size(); }

private:
  [[nodiscard]] std::array<bool, N>& offsets() { return *mOffsets.get(); }

  [[nodiscard]] std::array<bool, N>::iterator getFirstEmptyBlock(std::array<bool, N>::iterator from,
                                                                 std::size_t n) {
    if (from == offsets().end() || from + n >= offsets().end()) {
      return offsets().end();
    }
    auto res = std::find(std::execution::par, from, offsets().end(), 0);

    if (res + n >= offsets().end() && res != offsets().end()) {
      return offsets().end();
    }

    if (auto offset = std::find(std::execution::par, res, res + n, 1);
        res != offsets().end() && offset != res + n) {
      return getFirstEmptyBlock(std::find(offset, offsets().end(), 0), n);
    }
    return res;
  }

private:
  std::shared_ptr<void> mPool{nullptr};
  std::shared_ptr<std::array<bool, N>> mOffsets{std::make_shared<std::array<bool, N>>()};
  std::mutex mLock{};
};

/*
 *  Type specific, thread-safe pool allocator
 */
template <class Ty, std::size_t S> class TyPoolAllocator {
  static constexpr std::size_t N = S + 1;

public:
  using value_type = Ty;

  TyPoolAllocator()
      : mPool(static_cast<Ty*>(::operator new(N * sizeof(value_type))),
              [](Ty* p) { ::operator delete(p, sizeof(value_type) * N); }) {
    if (!mPool.get()) {
      throw std::bad_alloc();
    }
  }

  TyPoolAllocator(TyPoolAllocator&& other) = delete;

  TyPoolAllocator& operator=(TyPoolAllocator&& other) = delete;

public:
  [[nodiscard]] Ty* allocate(std::size_t n) {
    std::unique_lock<std::mutex> lock(mLock);
    if (n > capacity()) {
      throw std::bad_alloc();
    }

    auto emptyBlock = getFirstEmptyBlock(offsets().begin(), n);
    if (emptyBlock >= offsets().end()) {
      throw std::bad_alloc();
    }

    std::fill(std::execution::par, emptyBlock, emptyBlock + n, 1);

    return static_cast<Ty*>(mPool.get() + (emptyBlock - offsets().begin()));
  };

  void deallocate(Ty* at, std::size_t n) noexcept {
    std::scoped_lock<std::mutex> lock(mLock);
    const std::size_t offset = at - static_cast<Ty*>(mPool.get());

    for (std::size_t i{offset}; i < offset + n; i++) {
      offsets()[i] = 0;
    }
  }

  void deallocate(Ty* at) noexcept {
    std::scoped_lock<std::mutex> lock(mLock);
    const std::size_t offset = at - mPool.get();
    offsets()[offset] = 0;
  }

  size_t capacity() const { return mOffsets.get()->size(); }

private:
  std::array<bool, N>& offsets() { return *mOffsets.get(); }

  std::array<bool, N>::iterator getFirstEmptyBlock(std::array<bool, N>::iterator from,
                                                   std::size_t n) {
    if (from == offsets().end() || from + n >= offsets().end()) {
      return offsets().end();
    }

    auto res = std::find(std::execution::par, from, offsets().end(), 0);

    if (res + n >= offsets().end() && res != offsets().end()) {
      return offsets().end();
    }

    if (auto offset = std::find(std::execution::par, res, res + n, 1);
        res != offsets().end() && offset != res + n) {
      return getFirstEmptyBlock(std::find(offset, offsets().end(), 0), n);
    }
    return res;
  }

private:
  std::shared_ptr<Ty> mPool{nullptr};
  std::shared_ptr<std::array<bool, N>> mOffsets{std::make_shared<std::array<bool, N>>()};
  std::mutex mLock{};
};
}  // namespace utilities