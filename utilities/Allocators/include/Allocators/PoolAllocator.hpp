#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <execution>
#include <memory>
#include <mutex>

namespace utilities {

/*
 *  Type specific, thread-safe pool allocator
 */
template <class T, std::size_t Capacity>
class PoolAllocator {
    static constexpr std::size_t N = Capacity + 1;

  public:
    using value_type = T;

    PoolAllocator()
        : m_Pool(static_cast<T*>(::operator new(N * sizeof(value_type))),
                 [](T* p) { ::operator delete(p, sizeof(value_type) * N); }) {
        if (!m_Pool.get()) {
            throw std::bad_alloc();
        }
    }

    PoolAllocator(PoolAllocator&& other) = delete;

    PoolAllocator& operator=(PoolAllocator&& other) = delete;

  public:
    [[nodiscard]] T* allocate(std::size_t n) {
        std::unique_lock<std::mutex> lock(m_Lock);

        if (n > capacity()) {
            throw std::bad_alloc();
        }

        auto emptyBlock = getFirstEmptyBlock(offsets().begin(), n);
        if (emptyBlock >= offsets().end()) {
            throw std::bad_alloc();
        }

        std::fill(std::execution::par, emptyBlock, emptyBlock + n, 1);

        return static_cast<T*>(m_Pool.get() + (emptyBlock - offsets().begin()));
    };

    void deallocate(T* at, std::size_t n) noexcept {
        std::scoped_lock<std::mutex> lock(m_Lock);

        const std::size_t offset = at - static_cast<T*>(m_Pool.get());
        std::fill_n(offsets().begin() + offset + n, 0);
    }

    void deallocate(T* at) noexcept {
        std::scoped_lock<std::mutex> lock(m_Lock);

        const std::size_t offset = at - m_Pool.get();
        offsets()[offset] = 0;
    }

    size_t capacity() const { return m_Offsets.get()->size(); }

  private:
    [[nodiscard]] std::array<bool, N>& offsets() { return *m_Offsets.get(); }

    [[nodiscard]] std::array<bool, N>::iterator
    getFirstEmptyBlock(std::array<bool, N>::iterator from, std::size_t n) {
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
    std::shared_ptr<T> m_Pool{nullptr};
    std::shared_ptr<std::array<bool, N>> m_Offsets{
        std::make_shared<std::array<bool, N>>()};
    std::mutex m_Lock{};
};

/*
 *  Thread-safe byte pool allocator
 */
template <std::size_t S>
class PoolAllocator<std::byte, S> {
    static constexpr std::size_t N = S + 1;

  public:
    PoolAllocator()
        : m_Pool(::operator new(N * sizeof(std::byte)),
                 [](void* p) { ::operator delete(p, N * sizeof(std::byte)); }) {}

    PoolAllocator(PoolAllocator&& other) = delete;

    PoolAllocator& operator=(PoolAllocator&& other) = delete;

    [[nodiscard]] void* allocate(std::size_t n) {
        std::scoped_lock<std::mutex> guard(m_Lock);
        if (n > capacity()) {
            throw std::bad_alloc();
        }

        auto emptyBlock = getFirstEmptyBlock(offsets().begin(), n);
        if (emptyBlock >= offsets().end()) {
            throw std::bad_alloc();
        }

        std::fill(std::execution::par, emptyBlock, emptyBlock + n, 1);

        return static_cast<void*>(static_cast<std::byte*>(m_Pool.get()) +
                                  (emptyBlock - offsets().begin()));
    };

    void deallocate(std::byte* at, std::size_t n) noexcept {
        std::scoped_lock<std::mutex> guard(m_Lock);
        const std::size_t offset = at - static_cast<std::byte*>(m_Pool.get());

        std::fill_n(offsets().begin() + offset, n, 0);
    }

    void deallocate(std::byte* at) noexcept {
        std::scoped_lock<std::mutex> guard(m_Lock);
        const std::size_t offset = at - static_cast<std::byte*>(m_Pool.get());

        offsets()[offset] = 0;
    }

    size_t capacity() { return offsets().size(); }

  private:
    [[nodiscard]] std::array<bool, N>& offsets() { return *m_Offsets.get(); }

    [[nodiscard]] std::array<bool, N>::iterator
    getFirstEmptyBlock(std::array<bool, N>::iterator from, std::size_t n) {
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
    std::shared_ptr<void> m_Pool{nullptr};
    std::shared_ptr<std::array<bool, N>> m_Offsets{};
    std::mutex m_Lock{};
};

}  // namespace utilities