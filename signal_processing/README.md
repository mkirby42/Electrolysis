# Signal Processing Library

Embedded-focused signal processing library with fixed-point arithmetic support. Optimized for ARM Cortex-M4F.

## Features
- Fixed-point arithmetic (Q1.31 format)
- FIR filter implementation
- Python tools for coefficient generation and testing

## Requirements
- CMake 3.20+
- ARM GCC toolchain (for embedded builds)
- Python 3.8+ (for tools)

## Building

### For ARM target:
```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../arm-none-eabi-toolchain.cmake ..
make
```

### For host (testing):
```bash
mkdir build && cd build
cmake -DBUILD_TESTING=ON ..
make
ctest
```

## Python Tools
```bash
cd python
pip install -r requirements.txt

# Generate filter coefficients
python generate.py

# Run benchmarks
python benchmark_workflow.py

# Compare implementations
python compare.py
```

## Project Structure
- `src/` - Core C implementation
- `tests/` - Unit tests
- `python/` - Python tools and reference implementations
- `benchmarks/` - Performance benchmarks
- `data/` - Test data and coefficients 