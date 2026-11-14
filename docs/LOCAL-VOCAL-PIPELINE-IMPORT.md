# Local vocal pipeline import

This repository is the integration target for the C++17 phase-vocoder work and the model-independent F0/compressor stage documented in ElevenX PRs #25 and #26.

The F0 tracker and compressor are offline NumPy DSP, not neural inference. No trained neural-compressor checkpoint was verified in ElevenX, so neural inference is not imported or claimed. Local XTTS synthesis remains an optional upstream audio producer; this repository does not silently add a Python server or model download path to the C++17 core.

The C++17 phase-vocoder remains in the existing M3 pull request pending review and merge. Its cache-miss path allocates and its mutable cache is not synchronized; it is not hard-real-time safe.

References: https://github.com/ljrseals-dev/ElevenX/pull/25 and https://github.com/ljrseals-dev/ElevenX/pull/26.
