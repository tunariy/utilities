#include <cstddef>

namespace utilities {

template <std::size_t Capacity>
class StackAllocator {
  public:
    StackAllocator() : offset(0) {}

    void* alloc(std::size_t size, std::size_t align = alignof(std::max_align_t)) {
        std::size_t currentOffset{offset};

        if (currentOffset % align) {
            if (currentOffset < align) {
                currentOffset += (align - (currentOffset % align));
            } else {
                currentOffset += (currentOffset % align);
            }
        }
        if (int((size + currentOffset) - remaining() - 1) >= 0) {
            return nullptr;
        }

        currentOffset += size;
        offset = currentOffset;
        return (void*)(pool + currentOffset);
    }

    void reset() noexcept { offset = 0; }

    std::size_t remaining() const { return Capacity - offset; }

  private:
    char pool[Capacity];
    std::size_t offset{0};
};

}  // namespace utilities