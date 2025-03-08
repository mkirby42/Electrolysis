add_executable(filter_bench filter_bench.c)
target_link_libraries(filter_bench PRIVATE signal_proc)
target_include_directories(filter_bench PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(benchmark_runner
    benchmark_runner.c
)

target_link_libraries(benchmark_runner
    PRIVATE
    signal_processing
)

# Add custom target for running benchmarks
add_custom_target(run_benchmarks
    COMMAND qemu-arm ${CMAKE_CURRENT_BINARY_DIR}/benchmark_runner
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running benchmarks under QEMU ARM emulator"
) 