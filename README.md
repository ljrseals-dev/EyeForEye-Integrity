# EyeForEye-Integrity

**A C++17 foundation for voice synthesis, phoneme-aware positioning, and audio integrity work.**

This repository is the sanitized public project baseline. It does not claim that external sample libraries, model assets, unpublished patent materials, or private archives have been imported.

## Current implementation

- C++17 core build with CMake and CTest
- Bounded PositionMap with synchronized access and versioned little-endian serialization
- Major, natural-minor, pentatonic, chromatic, and custom-mask scale snapping
- Sample-rate-aware ADSR envelope
- RIFF/WAVE PCM and IEEE-float reader with chunk-boundary and truncation checks
- SampleBank with deterministic path ordering and stable buffer addresses during append
- Fixed-polyphony sample renderer and functional artifact harness

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Requires CMake 3.20+ and a C++17 compiler.

## Safety and scope boundaries

- PositionMap uses a mutex to make concurrent access data-race-safe. Its methods may block and are **not real-time safe**; do not call record, reset, deserialize, or serialize from an audio callback.
- SampleBank append preserves existing AudioBuffer addresses through deque storage. Loading and clearing must still be externally serialized against rendering; clear invalidates returned pointers.
- Renderer uses caller-owned buffers and does not perform file I/O. No claim of complete host-level real-time safety, lock freedom, or production audio quality is made.
- The WAV reader supports RIFF/WAVE PCM 16/24/32-bit and IEEE float 32/64-bit mono-downmixed input. RIFX, WAVE64, compressed formats, and extensible subformat GUID validation are outside this milestone.
- Renderer currently selects the fixed `AH` phoneme. PositionMap-driven syllable selection, pitch shifting, cache prewarming, parser strategies, and AU integration are deferred.

## Sanitization boundary

Public source excludes private archives, credentials, unpublished disclosure material, personal data, and third-party model/sample assets without verified provenance and license terms. The repository does not include the `mouth-bottom` archive.

## Patent and provenance notice

The project includes discussion of the SPACE concept and its possible patent-related disclosure. Related technical records are subject to provenance and filing-status verification. This notice does not assert that a patent application has been filed, that any application is pending, or that a patent has been granted. See [PROVENANCE.md](docs/PROVENANCE.md).

## License

The repository's MIT license applies to project source files unless a file or directory states otherwise. It does not grant rights to third-party models, samples, datasets, trademarks, or separately licensed materials.
