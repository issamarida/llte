add_executable(test_timing test_timing.cpp)
target_link_libraries(test_timing PRIVATE
    llte
    Catch2::Catch2WithMain
)

add_executable(test_seqlock test_seqlock.cpp)
target_link_libraries(test_seqlock PRIVATE
    llte
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(test_timing)
catch_discover_tests(test_seqlock)
