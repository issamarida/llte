#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace llte::icc {

template <typename T>
class alignas(64) Seqlock {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Seqlock<T> requires T to be trivially copyable");
    static_assert(std::is_default_constructible_v<T>,
                  "Seqlock<T> requires T to be default constructible");

public:
    void store(const T& value) noexcept {
        const auto current_sequence = sequence_.load(std::memory_order_relaxed);
        sequence_.store(current_sequence + 1, std::memory_order_release);
        value_ = value;
        sequence_.store(current_sequence + 2, std::memory_order_release);
    }

    [[nodiscard]] T load() const noexcept {
        while (true) {
            const auto sequence_before = sequence_.load(std::memory_order_acquire);
            if ((sequence_before & 1) != 0) {
                continue;
            }
            const T snapshot = value_;
            const auto sequence_after = sequence_.load(std::memory_order_acquire);
            if (sequence_before == sequence_after) {
                return snapshot;
            }
        }
    }

private:
    std::atomic<std::uint64_t> sequence_{0};
    T value_{};
};

}  // namespace llte::icc
