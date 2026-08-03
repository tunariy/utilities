#include <cstdint>
#include <cstring>
#include <memory>

namespace utilities {

template <typename T, typename allocator_t = std::allocator<T>>
class vector {
  public:
    vector() {
        allocator_t allocator;
        mData = allocator.allocate(1);
        mCapacity = 1;
        mSize = 0;
    }

    ~vector() {
        allocator_t allocator;
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
        std::memcpy(mData, other.mData, sizeof(T) * mSize);
        mSize = other.mSize;
    }

    vector(const vector& other) {
        resize(other.mCapacity);
        std::memcpy(mData, other.mData, sizeof(T) * mSize);
        mSize = other.mSize;
    }

    vector(vector&& other) {
        resize(other.mCapacity);
        for (auto i{0}; i < other.mSize; i++) {
            mData[i] = std::move(other.mData[i]);
        }
        mSize = other.mSize;
    }

    void push_back(T element) {
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

    T& at(std::size_t index) const {
        if (index >= mSize || index < 0) {
            throw;
        }
        return mData[index];
    }

    std::size_t get_size() const { return mSize; };

    std::size_t get_capacity() const { return mCapacity; };

    void clear() {
        allocator_t alloc;
        for (uint32_t i{}; i < mSize; i++) {
            std::destroy_at(mData + i);
        }
        mSize = 0;
    }

    T& operator[](std::size_t at) { return mData[at]; }

    void resize(std::size_t n) {
        if (!requiresRealloc()) {
            return;
        }
        allocator_t allocator;
        if (n > mCapacity) {
            T* tempBuffer = reinterpret_cast<T*>(allocator.allocate(n));
            std::memcpy(tempBuffer, mData, sizeof(T) * mSize);
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

    T* data() { return mData; }

  private:
    T* mData;
    std::size_t mSize{};
    std::size_t mCapacity{};
};

}  // namespace utilities