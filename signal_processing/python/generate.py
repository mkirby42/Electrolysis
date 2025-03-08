import numpy as np
from typing import Tuple, Optional, Dict
from dataclasses import dataclass
import json

@dataclass
class SignalParams:
    duration: float = 60.0
    sample_rate: float = 100.0
    breathing_rate: float = 0.25
    noise_level: float = 0.1
    baseline_drift: bool = True
    drift_frequency: float = 0.02
    harmonics: bool = False
    motion_artifacts: bool = False
    snr_db: float = 20.0

def generate_respiratory_signal(params: SignalParams) -> Tuple[np.ndarray, np.ndarray, Dict]:
    """
    Generate synthetic respiratory signal with configurable parameters
    
    Returns:
        Tuple of (time_points, signal_values, metadata)
    """
    t = np.linspace(0, params.duration, int(params.duration * params.sample_rate))
    
    # Basic respiratory signal (sinusoidal)
    signal = np.sin(2 * np.pi * params.breathing_rate * t)
    
    if params.harmonics:
        # Add first harmonic
        signal += 0.3 * np.sin(4 * np.pi * params.breathing_rate * t)
        # Add second harmonic
        signal += 0.1 * np.sin(6 * np.pi * params.breathing_rate * t)
    
    if params.baseline_drift:
        drift = 0.5 * np.sin(2 * np.pi * params.drift_frequency * t)
        signal += drift
    
    if params.motion_artifacts:
        # Add random motion artifacts
        artifact_times = np.random.choice(len(t), size=int(len(t)*0.05), replace=False)
        artifacts = np.zeros_like(t)
        artifacts[artifact_times] = np.random.normal(0, 2, size=len(artifact_times))
        signal += artifacts
    
    # Add Gaussian noise
    if params.noise_level > 0:
        signal_power = np.mean(signal ** 2)
        noise_power = signal_power / (10 ** (params.snr_db / 10))
        noise = np.random.normal(0, np.sqrt(noise_power), size=len(t))
        signal += noise * params.noise_level
    
    metadata = {
        "params": params.__dict__,
        "signal_stats": {
            "mean": float(np.mean(signal)),
            "std": float(np.std(signal)),
            "min": float(np.min(signal)),
            "max": float(np.max(signal))
        }
    }
    
    return t, signal, metadata

def save_test_case(filename: str, t: np.ndarray, signal: np.ndarray, metadata: Dict) -> None:
    """Save test case to NPZ file with metadata"""
    np.savez_compressed(
        filename,
        time=t,
        signal=signal,
        metadata=json.dumps(metadata)
    )

def generate_test_cases(
    output_file: str,
    num_cases: int = 5,
    durations: Optional[list] = None,
    sample_rates: Optional[list] = None
) -> None:
    """
    Generate multiple test cases and save to NPZ file.
    
    Args:
        output_file: Path to save the test cases
        num_cases: Number of test cases to generate
        durations: List of durations to use (if None, will generate random ones)
        sample_rates: List of sample rates to use (if None, will use default)
    """
    if durations is None:
        durations = np.random.uniform(10, 120, num_cases)
    if sample_rates is None:
        sample_rates = [100.0] * num_cases
    
    test_cases = {}
    for i in range(num_cases):
        t, signal, metadata = generate_respiratory_signal(
            SignalParams(
                duration=durations[i],
                sample_rate=sample_rates[i],
                breathing_rate=np.random.uniform(0.2, 0.4),
                noise_level=np.random.uniform(0.05, 0.2),
                baseline_drift=np.random.choice([True, False]),
                drift_frequency=np.random.uniform(0.01, 0.05),
                harmonics=np.random.choice([True, False]),
                motion_artifacts=np.random.choice([True, False]),
                snr_db=np.random.uniform(10, 30)
            )
        )
        test_cases[f'case_{i}_time'] = t
        test_cases[f'case_{i}_signal'] = signal
        test_cases[f'case_{i}_metadata'] = metadata
    
    np.savez(output_file, **test_cases)

if __name__ == '__main__':
    # Example usage
    generate_test_cases('test_signals.npz', num_cases=3)
    print("Generated test signals saved to test_signals.npz") 