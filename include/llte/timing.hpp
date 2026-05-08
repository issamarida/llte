#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

namespace llte::timing {

[[gnu::always_inline]] inline std::uint64_t now_cycles() noexcept {
    std::uint32_t tsc_low;
    std::uint32_t tsc_high;
    std::uint32_t processor_id;

    __asm__ __volatile__("rdtscp"
                         : "=a"(tsc_low), "=d"(tsc_high), "=c"(processor_id));

    return (static_cast<std::uint64_t>(tsc_high) << 32) | tsc_low;
}

struct TscCalibration {
    double ns_per_cycle;

    [[nodiscard]] static TscCalibration measure(std::chrono::milliseconds duration) {
        const auto chrono_start = std::chrono::steady_clock::now();
        const auto cycles_start = now_cycles();

        std::this_thread::sleep_for(duration);

        const auto cycles_end = now_cycles();
        const auto chrono_end = std::chrono::steady_clock::now();

        const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    chrono_end - chrono_start)
                                    .count();
        const auto elapsed_cycles = cycles_end - cycles_start;

        return TscCalibration{
            .ns_per_cycle = static_cast<double>(elapsed_ns) /
                            static_cast<double>(elapsed_cycles),
        };
    }

    [[nodiscard]] std::uint64_t cycles_to_ns(std::uint64_t cycles) const noexcept {
        return static_cast<std::uint64_t>(static_cast<double>(cycles) * ns_per_cycle);
    }
};

}  // namespace llte::timing
