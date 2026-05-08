#include <catch2/catch_test_macros.hpp>
#include <llte/timing.hpp>

#include <chrono>
#include <cstdint>

TEST_CASE("now_cycles returns monotonically non-decreasing values", "[timing]") {
    using llte::timing::now_cycles;

    auto first_reading_cycles  = now_cycles();
    auto second_reading_cycles = now_cycles();
    auto third_reading_cycles  = now_cycles();

    REQUIRE(second_reading_cycles >= first_reading_cycles);
    REQUIRE(third_reading_cycles  >= second_reading_cycles);
}

TEST_CASE("now_cycles advances over real work", "[timing]") {
    using llte::timing::now_cycles;

    auto start_cycles = now_cycles();

    volatile std::uint64_t accumulator = 0;
    for (int iteration = 0; iteration < 10'000; ++iteration) {
        accumulator += static_cast<std::uint64_t>(iteration);
    }

    auto end_cycles     = now_cycles();
    auto elapsed_cycles = end_cycles - start_cycles;

    REQUIRE(end_cycles > start_cycles);
    REQUIRE(elapsed_cycles > 1'000);
}

TEST_CASE("TscCalibration produces a sensible cycle/ns ratio", "[timing][calibration]") {
    using llte::timing::TscCalibration;

    const auto calibration = TscCalibration::measure(std::chrono::milliseconds{50});

    REQUIRE(calibration.ns_per_cycle > 0.05);
    REQUIRE(calibration.ns_per_cycle < 10.0);
}

TEST_CASE("TscCalibration cycles_to_ns is linear and monotonic", "[timing][calibration]") {
    using llte::timing::TscCalibration;

    const TscCalibration calibration{.ns_per_cycle = 0.2};

    REQUIRE(calibration.cycles_to_ns(0)    == 0);
    REQUIRE(calibration.cycles_to_ns(5)    == 1);
    REQUIRE(calibration.cycles_to_ns(50)   == 10);
    REQUIRE(calibration.cycles_to_ns(5000) == 1000);
}
