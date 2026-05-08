#include <benchmark/benchmark.h>
#include <llte/icc/seqlock.hpp>

#include <pthread.h>
#include <sched.h>

#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <thread>

namespace {

struct Payload {
    std::uint64_t a;
    std::uint64_t b;
    std::uint64_t c;
    std::uint64_t d;
};

void pin_current_thread_to_cpu(int cpu_id) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_id, &cpu_set);
    const int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
    if (result != 0) {
        throw std::runtime_error("pthread_setaffinity_np failed");
    }
}

}  // namespace

static void BM_seqlock_store(benchmark::State& state) {
    llte::icc::Seqlock<Payload> seqlock;
    std::uint64_t counter = 0;
    for (auto _ : state) {
        ++counter;
        seqlock.store(Payload{counter, counter, counter, counter});
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_seqlock_store);

static void BM_seqlock_load_uncontended(benchmark::State& state) {
    llte::icc::Seqlock<Payload> seqlock;
    seqlock.store(Payload{1, 2, 3, 4});
    for (auto _ : state) {
        auto snapshot = seqlock.load();
        benchmark::DoNotOptimize(snapshot);
    }
}
BENCHMARK(BM_seqlock_load_uncontended);

static void BM_seqlock_load_pinned(benchmark::State& state) {
    constexpr int reader_cpu = 0;
    const int writer_cpu = static_cast<int>(state.range(0));

    pin_current_thread_to_cpu(reader_cpu);

    llte::icc::Seqlock<Payload> seqlock;
    seqlock.store(Payload{1, 2, 3, 4});

    std::atomic<bool> stop_signal{false};
    std::atomic<bool> writer_ready{false};

    std::thread writer([&] {
        pin_current_thread_to_cpu(writer_cpu);
        writer_ready.store(true, std::memory_order_release);
        std::uint64_t counter = 0;
        while (!stop_signal.load(std::memory_order_relaxed)) {
            ++counter;
            seqlock.store(Payload{counter, counter, counter, counter});
        }
    });

    while (!writer_ready.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    for (auto _ : state) {
        auto snapshot = seqlock.load();
        benchmark::DoNotOptimize(snapshot);
    }

    stop_signal.store(true, std::memory_order_relaxed);
    writer.join();
}
BENCHMARK(BM_seqlock_load_pinned)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->ArgName("writer_cpu");

static void BM_seqlock_pingpong(benchmark::State& state) {
    constexpr int requester_cpu = 0;
    const int responder_cpu = static_cast<int>(state.range(0));

    pin_current_thread_to_cpu(requester_cpu);

    alignas(64) llte::icc::Seqlock<std::uint64_t> request_channel;
    alignas(64) llte::icc::Seqlock<std::uint64_t> response_channel;
    request_channel.store(0);
    response_channel.store(0);

    std::atomic<bool> stop_signal{false};
    std::atomic<bool> responder_ready{false};

    std::thread responder([&] {
        pin_current_thread_to_cpu(responder_cpu);
        responder_ready.store(true, std::memory_order_release);
        std::uint64_t last_seen = 0;
        while (!stop_signal.load(std::memory_order_relaxed)) {
            const auto incoming = request_channel.load();
            if (incoming != last_seen) {
                last_seen = incoming;
                response_channel.store(incoming);
            }
        }
    });

    while (!responder_ready.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    std::uint64_t request_id = 0;
    for (auto _ : state) {
        ++request_id;
        request_channel.store(request_id);
        std::uint64_t echoed = 0;
        do {
            echoed = response_channel.load();
        } while (echoed != request_id);
        benchmark::DoNotOptimize(echoed);
    }

    stop_signal.store(true, std::memory_order_relaxed);
    request_channel.store(request_id + 1);
    responder.join();
}
BENCHMARK(BM_seqlock_pingpong)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->ArgName("responder_cpu");
