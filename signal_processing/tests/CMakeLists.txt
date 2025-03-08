add_executable(test_fir test_fir.c)
target_link_libraries(test_fir signal_processing)
target_link_libraries(test_fir m)  # Link math library for fabsf
add_test(NAME test_fir COMMAND test_fir)

# Add more test executables as we create them 