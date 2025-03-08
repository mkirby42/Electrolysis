import numpy as np
import subprocess
import json
from pathlib import Path
from typing import Dict, Any, Tuple

def run_c_implementation(
    signal_data: np.ndarray,
    time_data: np.ndarray,
    executable_path: str
) -> Dict[str, Any]:
    """
    Run the C implementation on the given data.
    
    Args:
        signal_data: Input signal
        time_data: Time points
        executable_path: Path to C executable
    
    Returns:
        Dictionary with results from C implementation
    """
    # Save data to temporary file
    temp_input = 'temp_input.bin'
    temp_output = 'temp_output.bin'
    
    # Save as binary file
    with open(temp_input, 'wb') as f:
        np.save(f, signal_data)
        np.save(f, time_data)
    
    # Run C executable
    try:
        subprocess.run(
            [executable_path, temp_input, temp_output],
            check=True
        )
        
        # Read results
        with open(temp_output, 'rb') as f:
            filtered_signal = np.load(f)
            peak_indices = np.load(f)
            breathing_rate = np.load(f)
            
        results = {
            'filtered_signal': filtered_signal,
            'peak_indices': peak_indices,
            'breathing_rate': breathing_rate
        }
        
    finally:
        # Cleanup
        Path(temp_input).unlink(missing_ok=True)
        Path(temp_output).unlink(missing_ok=True)
    
    return results

def compare_implementations(
    signal_data: np.ndarray,
    time_data: np.ndarray,
    c_executable: str,
    tolerance: float = 1e-6
) -> Tuple[bool, Dict[str, float]]:
    """
    Compare Python and C implementations.
    
    Args:
        signal_data: Input signal
        time_data: Time points
        c_executable: Path to C executable
        tolerance: Maximum allowed difference
    
    Returns:
        Tuple of (passed, differences)
    """
    from reference import process_respiratory_signal
    
    # Run both implementations
    py_results = process_respiratory_signal(signal_data, time_data)
    c_results = run_c_implementation(signal_data, time_data, c_executable)
    
    # Compare results
    differences = {
        'filtered_signal_max_diff': np.max(np.abs(
            py_results['filtered_signal'] - c_results['filtered_signal']
        )),
        'peak_indices_diff': np.sum(
            py_results['peak_indices'] != c_results['peak_indices']
        ),
        'breathing_rate_diff': abs(
            py_results['breathing_rate'] - c_results['breathing_rate']
        )
    }
    
    # Check if within tolerance
    passed = all(
        diff < tolerance for diff in differences.values()
    )
    
    return passed, differences

if __name__ == '__main__':
    from generate import generate_respiratory_signal
    
    # Generate test signal
    t, sig = generate_respiratory_signal(duration=30.0)
    
    # Compare implementations
    passed, diffs = compare_implementations(
        sig, t,
        c_executable='../build/signal_proc'
    )
    
    print(f"Tests {'PASSED' if passed else 'FAILED'}")
    print("Differences:")
    for k, v in diffs.items():
        print(f"  {k}: {v}") 