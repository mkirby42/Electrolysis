#!/usr/bin/env python3
import numpy as np
from pathlib import Path
import subprocess
import json
import matplotlib.pyplot as plt
from generate import generate_signal
from reference import apply_butterworth
import tempfile
import os

def run_workflow(signal_params, filter_params):
    # 1. Generate test signal
    t, signal = generate_signal(**signal_params)
    
    # 2. Run Python reference implementation
    filtered_py = apply_butterworth(signal, **filter_params)
    
    # 3. Save data for C implementation
    data_dir = Path("../data")
    data_dir.mkdir(exist_ok=True)
    
    np.savetxt(data_dir / "input_signal.txt", signal)
    np.savetxt(data_dir / "python_output.txt", filtered_py)
    
    # 4. Build C implementation for ARM
    build_dir = Path("../build_arm")
    build_dir.mkdir(exist_ok=True)
    
    subprocess.run([
        "cmake", "..",
        "-B", str(build_dir),
        "-DCMAKE_TOOLCHAIN_FILE=../arm-none-eabi-toolchain.cmake",
        "-DCMAKE_BUILD_TYPE=Release"
    ], check=True)
    
    subprocess.run(["cmake", "--build", str(build_dir)], check=True)
    
    # 5. Run under QEMU with profiling
    with tempfile.NamedTemporaryFile() as prof_file:
        qemu_cmd = [
            "qemu-arm",
            "-cpu", "cortex-m4",
            "-nographic",
            "-semihosting",
            f"-icount", f"shift=0,align=off,sleep=off",
            "-d", f"op,in_asm,exec,nochain",
            "-D", prof_file.name,
            str(build_dir / "benchmarks" / "filter_bench")
        ]
        
        result = subprocess.run(qemu_cmd, capture_output=True, text=True)
        
        # Parse profiling data
        prof_data = parse_qemu_profile(prof_file.name)
    
    # 6. Compare outputs
    c_output = np.loadtxt(data_dir / "c_output.txt")
    mse = np.mean((filtered_py - c_output) ** 2)
    max_diff = np.max(np.abs(filtered_py - c_output))
    
    results = {
        "mse": float(mse),
        "max_diff": float(max_diff),
        "profiling": prof_data
    }
    
    with open(data_dir / "benchmark_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    # 7. Plot comparison
    plt.figure(figsize=(12, 6))
    plt.plot(t, filtered_py, label="Python Reference")
    plt.plot(t, c_output, "--", label="C Implementation")
    plt.legend()
    plt.savefig(data_dir / "comparison.png")
    
    return results

def parse_qemu_profile(filename):
    # Basic profiling data parser - expand based on your needs
    with open(filename) as f:
        lines = f.readlines()
    
    # Extract relevant metrics (customize based on what you want to measure)
    return {
        "total_instructions": len(lines),
        # Add more metrics as needed
    }

if __name__ == "__main__":
    # Example usage
    signal_params = {
        "duration": 10,
        "sampling_rate": 100,
        "freq": 1.0,
        "noise_std": 0.1
    }
    
    filter_params = {
        "order": 4,
        "cutoff": 2.0,
        "fs": 100
    }
    
    results = run_workflow(signal_params, filter_params)
    print(json.dumps(results, indent=2)) 