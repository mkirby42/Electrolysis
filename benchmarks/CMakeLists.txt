add_executable(bench_fir bench_fir.c)
target_link_libraries(bench_fir signal_proc)

# Add more benchmark executables as we create them 