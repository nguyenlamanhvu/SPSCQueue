#include <atomic>
#include <vector>
#include <memory>
#include <new>    // std::hardware_destructive_interference_size
#include <utility>

template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity) 
        : capacityMask_(capacity - 1)
        , buffer_(static_cast<T*>(operator new[](sizeof(T) * capacity)))    // Allocate raw memory for the buffer
    {
        // Ensure capacity is a power of 2 for efficient modulo operation
        if ((capacity & capacityMask_) != 0) {
            throw std::invalid_argument("Capacity must be a power of 2");
        }
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
        headCached_ = 0;
        tailCached_ = 0;
    }

    ~SPSCQueue() {
        T data;
        while (pop(data)); 
        operator delete[](buffer_);
    }

    // Producer: Push data
    void push(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        static_assert(std::is_copy_constructible<T>::value,
                      "T must be copy constructible");
        emplace(v);
    }

    // Producer: Emplace data (zero-copy construction)
    template <typename... Args>
    bool emplace(Args&&... args) {
        const size_t t = tail_.load(std::memory_order_relaxed);
        
        if (((t + 1) & capacityMask_) == headCached_) {
            headCached_ = head_.load(std::memory_order_acquire);
            if (((t + 1) & capacityMask_) == headCached_) return false;
        }

        // Initialize the object in-place using placement new
        new (&buffer_[t]) T(std::forward<Args>(args)...);

        tail_.store((t + 1) & capacityMask_, std::memory_order_release);
        return true;
    }

    // Consumer: Read data
    bool pop(T& data) {
        const size_t h = head_.load(std::memory_order_relaxed);

        if (h == tailCached_) {
            tailCached_ = tail_.load(std::memory_order_acquire); // Update tail cache
            if (h == tailCached_) {
                return false; // Buffer empty
            }
        }

        data = std::move(buffer_[h]);   // Move data out of the buffer
        buffer_[h].~T();                // Explicitly call destructor to clean up the slot

        head_.store((h + 1) & capacityMask_, std::memory_order_release);  // Release head after reading data
        return true;
    }

    size_t size() const noexcept {
        std::ptrdiff_t diff = tail_.load(std::memory_order_acquire) -
                              head_.load(std::memory_order_acquire);
        if (diff < 0) {
            diff += capacityMask_ + 1;
        }
        return static_cast<size_t>(diff);
    }
    
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }
    
    size_t capacity() const noexcept { return capacityMask_; }

private:
#ifdef __cpp_lib_hardware_interference_size
  static constexpr size_t kCacheLineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr size_t kCacheLineSize = 64;
#endif
    const size_t capacityMask_;
    T* const buffer_;
    alignas(kCacheLineSize) std::atomic<size_t> head_;
    alignas(kCacheLineSize) size_t headCached_ = 0;
    alignas(kCacheLineSize) std::atomic<size_t> tail_;
    alignas(kCacheLineSize) size_t tailCached_ = 0;
};