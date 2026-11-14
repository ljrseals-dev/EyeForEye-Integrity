# Eyesore: local vocal pipeline integration

This document describes the model-independent F0/compressor stage integrated into Eyesore alongside its C++17 phase-vocoder work.

The F0 tracker and compressor are offline NumPy DSP, not neural inference. No trained neural-compressor checkpoint was verified in ElevenX, so neural inference is not imported or claimed. Local XTTS synthesis remains an optional upstream audio producer; Eyesore does not silently add a Python server or model-download path to the C++17 core.

The C++17 phase-vocoder implementation is included on the integration branch. Its pitch-cache miss path allocates and its mutable cache is not synchronized; it is not hard-real-time safe.

## Processing contract

- Input: an already-generated mono waveform and its sample rate.
- F0: monophonic normalized-autocorrelation estimate with voiced flags and confidence values.
- Compression: offline feed-forward compressor with bounded settings.
- Peak ceiling: 0.999 sample-peak only; this is not a true-peak limiter.
- Threading: offline, allocating processing; do not call from a hard real-time callback.

## References

The F0/compressor contracts were previously developed and tested in ElevenX PRs #25 and #26, then adapted for this project:

- https://github.com/ljrseals-dev/ElevenX/pull/25
- https://github.com/ljrseals-dev/ElevenX/pull/26
