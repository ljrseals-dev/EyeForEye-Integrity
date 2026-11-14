"""Offline DSP helpers for locally synthesized vocals."""
from .processing import PitchTrack, CompressorSettings, track_f0, compress_vocal, process_local_vocal
__all__ = ["PitchTrack", "CompressorSettings", "track_f0", "compress_vocal", "process_local_vocal"]
