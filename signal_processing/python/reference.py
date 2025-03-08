import numpy as np
from scipy import signal
from typing import Tuple, Optional

def butter_filter(
    data: np.ndarray,
    cutoff: float,
    fs: float,
    order: int = 4,
    btype: str = 'low'
) -> np.ndarray:
    """
    Apply Butterworth filter to data.
    
    Args:
        data: Input signal
        cutoff: Cutoff frequency in Hz
        fs: Sampling frequency in Hz
        order: Filter order
        btype: Filter type ('low', 'high', 'band', 'bandstop')
    
    Returns:
        Filtered signal
    """
    nyq = 0.5 * fs
    normalized_cutoff = cutoff / nyq
    b, a = signal.butter(order, normalized_cutoff, btype=btype)
    return signal.filtfilt(b, a, data)

def find_peaks(
    data: np.ndarray,
    fs: float,
    height: Optional[float] = None,
    distance: Optional[float] = None
) -> Tuple[np.ndarray, dict]:
    """
    Find peaks in respiratory signal.
    
    Args:
        data: Input signal
        fs: Sampling frequency in Hz
        height: Minimum peak height
        distance: Minimum distance between peaks in seconds
    
    Returns:
        Tuple of (peak_indices, peak_properties)
    """
    # Convert distance from seconds to samples if provided
    min_distance = None if distance is None else int(distance * fs)
    
    return signal.find_peaks(
        data,
        height=height,
        distance=min_distance
    )

def resample_signal(
    data: np.ndarray,
    time: np.ndarray,
    target_fs: float
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Resample signal to target frequency using interpolation.
    
    Args:
        data: Input signal
        time: Time points of input signal
        target_fs: Target sampling frequency in Hz
    
    Returns:
        Tuple of (resampled_time, resampled_signal)
    """
    current_fs = 1 / np.mean(np.diff(time))
    num_samples = int(len(data) * target_fs / current_fs)
    
    new_time = np.linspace(time[0], time[-1], num_samples)
    new_signal = np.interp(new_time, time, data)
    
    return new_time, new_signal

def process_respiratory_signal(
    data: np.ndarray,
    time: np.ndarray,
    filter_cutoff: float = 1.0,
    peak_distance: float = 1.0
) -> dict:
    """
    Complete respiratory signal processing pipeline.
    
    Args:
        data: Input signal
        time: Time points
        filter_cutoff: Cutoff frequency for lowpass filter
        peak_distance: Minimum distance between peaks in seconds
    
    Returns:
        Dictionary containing processed results
    """
    fs = 1 / np.mean(np.diff(time))
    
    # Filter the signal
    filtered = butter_filter(data, filter_cutoff, fs)
    
    # Find peaks
    peak_idx, peak_props = find_peaks(
        filtered,
        fs=fs,
        distance=peak_distance
    )
    
    # Calculate breathing rate
    if len(peak_idx) > 1:
        peak_intervals = np.diff(time[peak_idx])
        breathing_rate = 60 / np.mean(peak_intervals)  # breaths per minute
    else:
        breathing_rate = 0
    
    return {
        'filtered_signal': filtered,
        'peak_indices': peak_idx,
        'peak_properties': peak_props,
        'breathing_rate': breathing_rate
    }

if __name__ == '__main__':
    # Example usage
    from generate import generate_respiratory_signal
    
    # Generate test signal
    t, sig = generate_respiratory_signal(duration=30.0)
    
    # Process it
    results = process_respiratory_signal(sig, t)
    print(f"Detected breathing rate: {results['breathing_rate']:.1f} breaths/min") 