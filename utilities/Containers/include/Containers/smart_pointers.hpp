#include <mutex>
#include <utility>

namespace utilities {

template <typename T>
struct custom_deleter {
    void operator()(T* pointer) const { delete pointer; }
};

template <typename T, typename custom_deleter = custom_deleter<T>>
class unique_ptr {
  public:
    unique_ptr() = default;

    unique_ptr(T* pointer) : mData(pointer) {}

    unique_ptr(const unique_ptr&) = delete;

    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr(unique_ptr&& other) noexcept {
        mData = other.mData;
        other.mData = nullptr;
    }

    unique_ptr& operator=(unique_ptr&& other) noexcept {
        mData = other.mData;
        other.mData = nullptr;
        return *this;
    }

    ~unique_ptr() {
        custom_deleter deleter;
        deleter.operator()(mData);
    };

    T* release() {
        T* temp = mData;
        mData = nullptr;
        return temp;
    }

    void reset(T* pointer) {
        custom_deleter deleter;
        deleter.operator()(mData);
        mData = pointer;
    }

    bool is_owning() const { return mData != nullptr; }

    T& operator*() const { return *mData; }

    T* operator->() const { return mData; }

    operator bool() const { return mData != nullptr; }

  private:
    T* mData;
};

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T{std::forward<Args>(args)...});
}

template <typename T>
class shared_ptr {
  public:
    shared_ptr() : mData(nullptr), cBlock(new control_block()) { cBlock->count_ = 0; }

    shared_ptr(T* pointer) : mData(pointer), cBlock(new control_block()) {
        cBlock->count_ = 1;
    }

    shared_ptr(const shared_ptr& other) noexcept {
        other.cBlock->lock();
        mData = other.mData;
        cBlock = other.cBlock;
        cBlock->count_++;
        other.cBlock->unlock();
    }

    shared_ptr& operator=(const shared_ptr& other) noexcept {
        other.cBlock->lock();
        mData = other.mData;
        cBlock = other.cBlock;
        cBlock->count_++;
        other.cBlock->unlock();
        return *this;
    }

    shared_ptr(shared_ptr&& other) noexcept {
        if (other.cBlock) {
            cBlock = other.cBlock;
            mData = other.mData;
            other.cBlock = nullptr;
            other.mData = nullptr;
        } else {
            cBlock = nullptr;
            mData = nullptr;
        }
    }

    shared_ptr& operator=(shared_ptr&& other) noexcept {
        if (other.cBlock) {
            cBlock = other.cBlock;
            mData = other.mData;
            other.cBlock = nullptr;
            other.mData = nullptr;
        } else {
            cBlock = nullptr;
            mData = nullptr;
        }
        return *this;
    }

    ~shared_ptr() {
        if (!cBlock) {
            return;
        }
        cBlock->lock();
        if (cBlock->count_ <= 1) {
            cBlock->unlock();
            delete mData;
            delete cBlock;
            return;
        }
        cBlock->count_--;
        cBlock->unlock();
    }

    void reset(T* pointer) {
        cBlock->lock();
        delete mData;
        mData = pointer;
        cBlock->count_ = 0;
        cBlock->unlock();
    }

    std::size_t get_count() const { return cBlock ? cBlock->count_ : 0; }

    T* operator->() const { return mData; }

    T& operator*() const { return *mData; }

    operator bool() const noexcept { return cBlock->count_ > 0; }

  private:
    struct control_block {
        void lock() { mutex_.lock(); }

        void unlock() { mutex_.unlock(); }

        std::size_t count_{};
        mutable std::mutex mutex_;
    };

    T* mData{nullptr};
    control_block* cBlock{nullptr};
};

template <typename T>
constexpr decltype(auto) move(T&& t) noexcept {  // takes in a universal reference
    return static_cast<typename std::remove_reference<T>::type&&>(
        t);  // returns rvalue reference with the reference being removed
}
}  // namespace utilities