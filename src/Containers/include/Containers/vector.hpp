#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

namespace tuna {
template <typename Ty_, typename Ty_allocator = std::allocator<Ty_>> class vector {

public:
  vector() {
    Ty_allocator allocator;
    mData = allocator.allocate(1);
    mCapacity = 1;
    mSize = 0;
  }

  ~vector() {
    Ty_allocator allocator;
    if (mCapacity) {
      for (auto i{0}; i < mSize; i++) {
        std::destroy_at(mData + i);
      }
      allocator.deallocate(mData, mCapacity);
    }
  }

  vector(std::size_t n) { resize(n); }

  vector(vector& other) {
    resize(other.mCapacity);
    std::memcpy(mData, other.mData, sizeof(Ty_) * mSize);
    mSize = other.mSize;
  }

  vector(const vector& other) {
    resize(other.mCapacity);
    std::memcpy(mData, other.mData, sizeof(Ty_) * mSize);
    mSize = other.mSize;
  }

  vector(vector&& other) {
    resize(other.mCapacity);
    for (auto i{0}; i < other.mSize; i++) {
      mData[i] = std::move(other.mData[i]);
    }
    mSize = other.mSize;
  }

  void push_back(Ty_ element) {
    if (requiresRealloc()) {
      resize(mCapacity * 3);
    }
    mData[mSize] = element;
    mSize++;
    if (requiresRealloc()) {
      resize(mCapacity * 3);
    }
  }

  void pop_back() {
    std::destroy_at(mData + mSize);
    mSize--;
  };

  void shrink_to_fit() {
    if (!requiresRealloc()) {
      resize(mSize);
      mCapacity = mSize;
    }
  };

  Ty_& at(std::size_t index) const {
    if (index >= mSize || index < 0) {
      throw;
    }
    return mData[index];
  }

  std::size_t get_size() const { return mSize; };

  std::size_t get_capacity() const { return mCapacity; };

  void clear() {
    Ty_allocator alloc;
    for (uint32_t i{}; i < mSize; i++) {
      std::destroy_at(mData + i);
    }
    mSize = 0;
  }

  Ty_& operator[](std::size_t at) { return mData[at]; }

  void resize(std::size_t n) {
    if (!requiresRealloc()) {
      return;
    }
    Ty_allocator allocator;
    if (n > mCapacity) {
      Ty_* tempBuffer = reinterpret_cast<Ty_*>(allocator.allocate(n));
      std::memcpy(tempBuffer, mData, sizeof(Ty_) * mSize);
      if (mCapacity) {
        allocator.deallocate(mData, mCapacity);
      }
      mData = tempBuffer;
      mCapacity = n;
    }
  };

  void reserve(std::size_t n) {
    resize(n);
    for (auto i{mSize}; i < mCapacity; i++) {
      std::construct_at(mData + i);
    }
    mSize = mCapacity;
  };

private:
  bool requiresRealloc() const { return mSize >= mCapacity; }

  Ty_* data() { return mData; }

private:
  Ty_* mData;
  std::size_t mSize{};
  std::size_t mCapacity{};
};
}  // namespace tuna