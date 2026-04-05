#include <algorithm>
#include <array>
#include <exception>
#include <execution>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdint.h>

/*
 *  Type specific, thread-safe pool allocator
 */
template <class _Tp, std::size_t _N = 100> class TyPoolAllocator {
  template <class _Up, std::size_t _Nn> friend class TypedPoolAllocator;

public:
  using value_type = _Tp;

  template <typename _Up> struct rebind {
    using other = TyPoolAllocator<_Up, _N>;
  };

  TyPoolAllocator()
      : mPool(static_cast<_Tp *>(::operator new(_N * sizeof(value_type))),
              [](_Tp *p) { ::operator delete(p, sizeof(value_type) * _N); }) {
    if (!mPool.get()) {
      throw std::bad_alloc();
    }
  }

  ~TyPoolAllocator() {};

  TyPoolAllocator(const TyPoolAllocator &other)
      : mPool(other.mPool), mOffsets(other.mOffsets) {}

  TyPoolAllocator &operator=(const TyPoolAllocator &other) {
    mPool = other.mPool;
    mOffsets = other.mOffsets;
    return *this;
  }

  template <typename _Up>
  TyPoolAllocator(const TyPoolAllocator<_Up, _N> &other) {
    if (auto alignDiff =
            (other.capacity() * alignof(_Up)) % alignof(value_type);
        alignDiff) {
      throw std::runtime_error("Bad conversion construction. value_type's of "
                               "allocators are misaligned!");
    }
    mPool = other.mPool;
    std::fill(std::execution::par,
              offsets().begin() + other.mOffsets * sizeof(_Up) / sizeof(_Tp),
              offsets().end(), 1);
  }

  TyPoolAllocator(TyPoolAllocator &&other) = delete;

  TyPoolAllocator &operator=(TyPoolAllocator &&other) = delete;

public:
  _Tp *allocate(std::size_t n) {
    lock();
    if (n > capacity()) {
      std::cerr << "Not enough capacity!\n";
      unlock();
      throw std::bad_alloc();
    }
    auto emptyBlock = getFirstEmptyBlock(offsets().begin(), n);
    std::clog << emptyBlock - offsets().begin() << std::endl;
    if (emptyBlock >= offsets().end()) {
      std::cerr << "Not enough capacity?!!?\n";
      unlock();
      throw std::bad_alloc();
    }

    std::fill(std::execution::par, emptyBlock, emptyBlock + n, 1);
    unlock();
    return static_cast<_Tp *>(
        std::launder(mPool.get() + (emptyBlock - offsets().begin())));
  };

  void deallocate(_Tp *at, std::size_t n) noexcept {
    lock();
    const std::size_t offset = at - static_cast<_Tp *>(mPool.get());

    for (std::size_t i{offset}; i < offset + n; i++) {
      offsets()[i] = 0;
    }
    unlock();
  }

  void deallocate(_Tp *at) noexcept {
    lock();
    const std::size_t offset = at - static_cast<_Tp *>(mPool.get());
    offsets()[offset] = 0;
    unlock();
  }

  void printOffsets() {
    lock();
    for (bool x : offsets()) {
      std::clog << x << " ";
    }
    std::clog << std::endl;
    unlock();
  }

  size_t capacity() { return offsets().size(); }

private:
  std::array<bool, _N> &offsets() { return *mOffsets.get(); }

  std::array<bool, _N>::iterator
  getFirstEmptyBlock(std::array<bool, _N>::iterator _from, std::size_t size) {
    static int depthGuard = 10;
    if (_from == offsets().end() || _from + size >= offsets().end()) {
      return offsets().end();
    }
    auto res = std::find(std::execution::par, _from, offsets().end(), 0);
    std::clog << res - offsets().begin() << std::endl;
    if (depthGuard-- < 0) {
      return offsets().end();
    }
    if (res + size > offsets().end() && res != offsets().end()) {
      return getFirstEmptyBlock(res + size, size);
    }
    if (auto offset = std::find(std::execution::par, res, res + size, 1);
        res != offsets().end() && offset != res + size) {
      return getFirstEmptyBlock(std::find(offset, offsets().end(), 0), size);
    }
    return res;
  }

  void lock() { mLock.lock(); }

  void unlock() { mLock.unlock(); }

  bool try_lock() { return mLock.try_lock(); }

private:
  std::shared_ptr<_Tp> mPool{nullptr};
  std::shared_ptr<std::array<bool, _N>> mOffsets{
      std::make_shared<std::array<bool, _N>>()};
  static inline std::mutex mLock;
};

template <std::size_t _N> class PoolAllocator {
public:
  struct rebind { // NOTE: for the fuckers who will rebind this shit
    using other = PoolAllocator<_N>;
  };

  PoolAllocator()
      : mPool(::operator new(_N * sizeof(uint8_t)),
              [](void *p) { ::operator delete(p, _N * sizeof(uint8_t)); }) {}

  PoolAllocator(const PoolAllocator<_N> &other) {
    mPool = other.mPool;
    mOffsets = other.mOffsets;
  }

  PoolAllocator &operator=(const PoolAllocator<_N> &other) {
    mPool = other.mPool;
    mOffsets = other.mOffsets;
    return *this;
  }

  PoolAllocator(PoolAllocator &&other) = delete;

  PoolAllocator &operator=(PoolAllocator &&other) = delete;

  ~PoolAllocator() {}

  void *allocate(std::size_t n) {
    lock();
    if (n > capacity()) {
      std::cerr << "Not enough capacity!\n";
      unlock();
      throw std::bad_alloc();
    }
    auto emptyBlock = getFirstEmptyBlock(offsets().begin(), n);
    // std::clog << emptyBlock - offsets().begin() << std::endl;
    if (emptyBlock >= offsets().end()) {
      std::cerr << "Not enough capacity?!!?\n";
      unlock();
      throw std::bad_alloc();
    }

    std::fill(std::execution::par, emptyBlock, emptyBlock + n, 1);
    unlock();
    return static_cast<void *>(
        std::launder(static_cast<uint8_t *>(mPool.get()) +
                     (emptyBlock - offsets().begin())));
  };

  void deallocate(uint8_t *at, std::size_t n) noexcept {
    lock();
    const std::size_t offset = at - static_cast<uint8_t *>(mPool.get());

    for (std::size_t i{offset}; i < offset + n; i++) {
      offsets()[i] = 0;
    }
    unlock();
  }

  void deallocate(uint8_t *at) noexcept {
    lock();
    const std::size_t offset = at - static_cast<uint8_t *>(mPool.get());
    offsets()[offset] = 0;
    unlock();
  }

  void printOffsets() {
    lock();
    for (bool x : offsets()) {
      std::clog << x << " ";
    }
    std::clog << std::endl;
    unlock();
  }

  size_t capacity() { return offsets().size(); }

private:
  std::array<bool, _N> &offsets() { return *mOffsets.get(); }

  std::array<bool, _N>::iterator
  getFirstEmptyBlock(std::array<bool, _N>::iterator _from, std::size_t size) {
    static int depthGuard = 10;
    if (_from == offsets().end() || _from + size >= offsets().end()) {
      return offsets().end();
    }
    auto res = std::find(std::execution::par, _from, offsets().end(), 0);
    // std::clog << res - offsets().begin() << std::endl;
    if (depthGuard-- < 0) {
      return offsets().end();
    }
    // std::clog << "Depth: " << std::setw(2) << depthGuard << std::setw(2)
    //           << "||" << std::setw(2) << res._M_offset << std::endl;
    if (res + size > offsets().end() && res != offsets().end()) {
      return getFirstEmptyBlock(res + size, size);
    }
    if (auto offset = std::find(std::execution::par, res, res + size, 1);
        res != offsets().end() && offset != res + size) {
      return getFirstEmptyBlock(std::find(offset, offsets().end(), 0), size);
    }
    return res;
  }

  void lock() { mLock.lock(); }

  void unlock() { mLock.unlock(); }

  bool try_lock() { return mLock.try_lock(); }

private:
  std::shared_ptr<void> mPool{nullptr};
  std::shared_ptr<std::array<bool, _N>> mOffsets{
      std::make_shared<std::array<bool, _N>>()};
  static inline std::mutex mLock;
};
