# EyeForEye-Integrity

**A C++17 foundation for voice synthesis, phoneme-aware positioning, and audio integrity work.**

This repository is the sanitized public project baseline. It intentionally contains only the buildable foundation authored for this repository; it does not claim that the complete synthesis engine, sample library, patent materials, or any external model assets have been imported.

## Current scope

- Framework-free C++17 library foundation
- CMake build and CTest integration
- Small version/API smoke test
- No network access, telemetry, automatic downloads, or build-time dependency fetching
- No audio-thread file I/O or dynamic allocation is introduced by this baseline

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Requires CMake 3.20+ and a C++17 compiler.

## Project status

This is a **foundation build**, not a completed voice engine. DSP, MIDI/position-map behavior, sample-bank ingestion, AU wrappers, and host integration remain unimplemented until their source and interfaces are reviewed and imported. No performance, latency, similarity, or audio-quality claims are made.

## Sanitization boundary

Public source excludes private archives, credentials, unpublished disclosure material, personal data, and third-party model/sample assets without verified provenance and license terms. The repository does not include the `mouth-bottom` archive.

## Patent and provenance notice

The project includes discussion of the SPACE concept and its possible patent-related disclosure. Related technical records are subject to provenance and filing-status verification. This notice does not assert that a patent application has been filed, that any application is pending, or that a patent has been granted. See [PROVENANCE.md](docs/PROVENANCE.md).

## License

The repository's MIT license applies to project source files unless a file or directory states otherwise. It does not grant rights to third-party models, samples, datasets, trademarks, or separately licensed materials.
