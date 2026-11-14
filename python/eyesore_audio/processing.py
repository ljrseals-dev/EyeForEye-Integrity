"""Model-independent offline F0 analysis and vocal compression."""
from dataclasses import dataclass
import numpy as np

@dataclass(frozen=True)
class PitchTrack:
    f0_hz: np.ndarray
    confidence: np.ndarray
    voiced: np.ndarray
    frame_times_seconds: np.ndarray

@dataclass(frozen=True)
class CompressorSettings:
    threshold_db: float = -18.0
    ratio: float = 3.0
    attack_ms: float = 10.0
    release_ms: float = 100.0
    makeup_db: float = 0.0
    max_reduction_db: float = 12.0

    def validate(self):
        for name, value, low, high in (
            ("threshold_db", self.threshold_db, -60, 0),
            ("ratio", self.ratio, 1, 20),
            ("attack_ms", self.attack_ms, .1, 200),
            ("release_ms", self.release_ms, 1, 2000),
            ("makeup_db", self.makeup_db, -12, 12),
            ("max_reduction_db", self.max_reduction_db, 0, 24),
        ):
            if not np.isfinite(value) or not low <= value <= high:
                raise ValueError(f"{name} outside supported range [{low}, {high}]")

def _audio(audio):
    x = np.asarray(audio, dtype=np.float64).reshape(-1)
    if x.size == 0 or not np.isfinite(x).all():
        raise ValueError("audio must be non-empty and finite")
    return x

def track_f0(audio, sample_rate, min_f0_hz=65.0, max_f0_hz=500.0,
             frame_ms=40.0, hop_ms=10.0, voiced_threshold=.60):
    """Monophonic normalized-autocorrelation F0 estimate; zero means unvoiced."""
    x = _audio(audio)
    if sample_rate <= 0 or not 0 < min_f0_hz < max_f0_hz < sample_rate / 2:
        raise ValueError("invalid sample rate or F0 range")
    if frame_ms <= 0 or hop_ms <= 0 or not 0 < voiced_threshold <= 1:
        raise ValueError("invalid frame, hop, or voicing threshold")
    n = max(16, round(sample_rate * frame_ms / 1000))
    hop = max(1, round(sample_rate * hop_ms / 1000))
    lo, hi = max(1, int(sample_rate / max_f0_hz)), min(n - 2, int(sample_rate / min_f0_hz))
    starts = list(range(0, max(1, len(x) - n + 1), hop))
    if not starts or starts[-1] + n < len(x): starts.append(max(0, len(x) - n))
    window = np.hanning(n)
    f0, conf, voiced, times = [], [], [], []
    fft_n = 1 << (2 * n - 1).bit_length()
    for start in starts:
        frame = x[start:start+n]
        if len(frame) < n: frame = np.pad(frame, (0, n-len(frame)))
        frame = (frame - frame.mean()) * window
        times.append((start + n/2) / sample_rate)
        if np.dot(frame, frame) <= 1e-12:
            f0.append(0.); conf.append(0.); voiced.append(False); continue
        spec = np.fft.rfft(frame, n=fft_n)
        corr = np.fft.irfft(spec * spec.conjugate(), n=fft_n)[:n]
        corr /= max(float(corr[0]), 1e-12)
        lag = lo + int(np.argmax(corr[lo:hi+1]))
        peak = float(corr[lag])
        if 0 < lag < n-1:
            a,b,c = corr[lag-1:lag+2]; den = a - 2*b + c
            if abs(float(den)) > 1e-12: lag += .5 * (a-c) / den
        is_voiced = peak >= voiced_threshold
        f0.append(sample_rate / lag if is_voiced else 0.)
        conf.append(float(np.clip(peak, 0, 1))); voiced.append(bool(is_voiced))
    return PitchTrack(np.asarray(f0, np.float32), np.asarray(conf, np.float32),
                      np.asarray(voiced, bool), np.asarray(times, np.float32))

def compress_vocal(audio, sample_rate, settings=None):
    """Offline feed-forward compressor with sample-peak ceiling, not true-peak limiting."""
    x = _audio(audio).astype(np.float32)
    if sample_rate <= 0: raise ValueError("sample_rate must be positive")
    cfg = settings or CompressorSettings(); cfg.validate()
    if cfg.ratio == 1 and cfg.makeup_db == 0: return x.copy()
    attack = np.exp(-1 / (sample_rate * cfg.attack_ms / 1000))
    release = np.exp(-1 / (sample_rate * cfg.release_ms / 1000))
    makeup = 10 ** (cfg.makeup_db / 20)
    env, gain = 0., 1.
    out = np.empty_like(x)
    for i, sample in enumerate(x):
        level = abs(float(sample)); coeff = attack if level > env else release
        env = coeff * env + (1-coeff) * level
        over = max(0., 20*np.log10(max(env, 1e-8)) - cfg.threshold_db)
        reduction = min(cfg.max_reduction_db, over * (1 - 1/cfg.ratio))
        target = 10 ** (-reduction / 20)
        coeff = attack if target < gain else release
        gain = coeff * gain + (1-coeff) * target
        out[i] = sample * gain * makeup
    peak = float(np.max(np.abs(out)))
    if peak > .999: out *= np.float32(.999 / peak)
    return out

def process_local_vocal(audio, sample_rate, settings=None):
    """Analyze and compress an already-generated waveform from any local voice engine."""
    return track_f0(audio, sample_rate), compress_vocal(audio, sample_rate, settings)
