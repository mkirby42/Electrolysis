cmake_minimum_required(VERSION 3.20)
project(signal_processing C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT BUILD_TESTING)
    # Include ARM toolchain file only for non-testing builds
    include(${CMAKE_CURRENT_SOURCE_DIR}/arm-none-eabi-toolchain.cmake)
    
    # ARM-specific flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fdata-sections -ffunction-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
else()
    # Host compilation flags for testing
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")
endif()

# Create library
add_library(signal_processing
    src/fixed_point.c
    src/fir_filter.c
)

target_include_directories(signal_processing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Testing executable (for host)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarking executable (for ARM)
if(NOT BUILD_TESTING)
    add_subdirectory(benchmarks)
endif()

# Install rules
install(TARGETS signal_processing
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
) 