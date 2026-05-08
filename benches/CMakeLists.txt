add_executable(bench_timing bench_timing.cpp)
target_link_libraries(bench_timing PRIVATE
    llte
    benchmark::benchmark_main
    Threads::Threads
)

add_executable(bench_seqlock bench_seqlock.cpp)
target_link_libraries(bench_seqlock PRIVATE
    llte
    benchmark::benchmark_main
    Threads::Threads
)
