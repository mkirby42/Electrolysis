set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain paths
find_program(ARM_CC arm-none-eabi-gcc)
find_program(ARM_CXX arm-none-eabi-g++)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_SIZE arm-none-eabi-size)

set(CMAKE_C_COMPILER ${ARM_CC})
set(CMAKE_CXX_COMPILER ${ARM_CXX})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Processor specific
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -Wall -Wextra" CACHE STRING "C++ compiler flags")

# Remove any -arch flags that might be inherited from the environment
string(REPLACE "-arch" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-arch" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Disable system includes
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "")

# Find programs only on the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY) 