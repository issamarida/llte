#include <catch2/catch_test_macros.hpp>
#include <llte/icc/seqlock.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

struct Tick {
    double price;
    std::uint64_t volume;
    std::uint64_t timestamp_ns;
};

}  // namespace

TEST_CASE("Seqlock store and load returns the same value", "[seqlock]") {
    llte::icc::Seqlock<Tick> seqlock;

    const Tick written{42.0, 100, 1000};
    seqlock.store(written);

    const auto read = seqlock.load();
    REQUIRE(read.price        == 42.0);
    REQUIRE(read.volume       == 100);
    REQUIRE(read.timestamp_ns == 1000);
}

TEST_CASE("Seqlock load reflects the latest store", "[seqlock]") {
    llte::icc::Seqlock<std::uint64_t> seqlock;

    seqlock.store(1);
    REQUIRE(seqlock.load() == 1);

    seqlock.store(2);
    REQUIRE(seqlock.load() == 2);

    seqlock.store(3);
    REQUIRE(seqlock.load() == 3);
}

TEST_CASE("Seqlock readers see consistent snapshots under writer pressure", "[seqlock][concurrent]") {
    llte::icc::Seqlock<Tick> seqlock;
    std::atomic<bool> stop_signal{false};
    std::atomic<std::uint64_t> torn_read_count{0};
    std::atomic<std::uint64_t> total_read_count{0};

    std::thread writer([&] {
        for (std::uint64_t counter = 1; !stop_signal.load(std::memory_order_relaxed); ++counter) {
            seqlock.store(Tick{static_cast<double>(counter), counter, counter});
        }
    });

    std::vector<std::thread> readers;
    constexpr int reader_count = 3;
    for (int reader_id = 0; reader_id < reader_count; ++reader_id) {
        readers.emplace_back([&] {
            while (!stop_signal.load(std::memory_order_relaxed)) {
                const auto snapshot = seqlock.load();
                total_read_count.fetch_add(1, std::memory_order_relaxed);
                const bool fields_match =
                    snapshot.price == static_cast<double>(snapshot.volume) &&
                    snapshot.volume == snapshot.timestamp_ns;
                if (!fields_match) {
                    torn_read_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    stop_signal.store(true, std::memory_order_relaxed);

    writer.join();
    for (auto& reader : readers) {
        reader.join();
    }

    REQUIRE(total_read_count.load() > 1'000);
    REQUIRE(torn_read_count.load() == 0);
}
