# Eyesore diagnostics

This document describes what the current automated checks exercise, what they do not prove, and a practical order for investigating failures. It is a diagnostic guide, not a claim that every listed test has been run on every platform or host.

## Verification map

| Layer | Command / mechanism | Current coverage | Important gap |
|---|---|---|---|
| Configure | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` | CMake project generation | Does not test runtime behavior |
| Compile | `cmake --build build --parallel` | C++17 compilation of configured targets | Does not prove host/plugin integration |
| Native tests | `ctest --test-dir build --output-on-failure` | Version/core setup, PositionMap, ScaleEngine, PitchShifter, artifact harness | Finite deterministic cases; not exhaustive fuzzing or stress testing |
| Python DSP | `python -m pytest -q` | F0 tone tracking, silence, invalid samples, compressor bypass, peak bound, combined pipeline | Monophonic synthetic tests; no broad corpus or perceptual evaluation |
| CI | GitHub Actions | Build/CTest and Python test jobs as configured in workflow | CI environment is not every target OS/compiler/audio host |

## Native C++ diagnostic path

1. **Configuration failure:** inspect CMake version, compiler identity, generator, and the first CMake error.
2. **Compilation failure:** retain the first compiler diagnostic and target name; distinguish source errors from missing toolchain or SDK dependencies.
3. **CTest failure:** run the failing executable directly from the build tree, then rerun CTest with `--output-on-failure`.
4. **Audio artifact:** preserve the input WAV, sample rate, channel layout, MIDI event sequence, render parameters, and expected output characteristics. Reproduce with the smallest possible sample set.
5. **Concurrency or callback issue:** do not infer real-time safety from passing functional tests. Use sanitizer/stress builds and a host-specific callback test in a controlled environment.

Useful commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Optional sanitizer build (compiler support required)
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

The sanitizer commands are a recommended local diagnostic configuration; they are not evidence that a sanitizer run has already passed.

## Python DSP diagnostic path

1. Confirm Python and NumPy versions and install the declared test dependencies.
2. Run the focused test module: `python -m pytest -q python_tests/test_audio_processing.py`.
3. For F0 errors, record sample rate, expected pitch range, frame/hop sizes, voiced threshold, signal level, and whether the source is monophonic.
4. For compressor errors, record threshold, ratio, attack/release, makeup gain, maximum reduction, input peak, and output peak.
5. For unexpected results, save a short reproducible waveform and verify that it contains finite samples and the expected channel shape.

The F0 tracker is normalized-autocorrelation classical DSP and assumes a monophonic signal. The compressor is offline feed-forward processing. Its output ceiling is sample-peak based; inter-sample true peaks are not measured.

## Known engineering limits

| Area | Known limit | Operational consequence |
|---|---|---|
| PositionMap | Mutex-backed methods can block | Keep record/reset/serialization operations off the audio callback |
| SampleBank | Directory loading and clearing mutate storage; clear invalidates returned pointers | Serialize loading/clearing against rendering and pointer use |
| Pitch cache | Cache misses allocate; mutable cache is not synchronized | Precompute/coordinate variants outside concurrent render use; do not assume hard-RT safety |
| PitchShifter | Offline phase-vocoder allocates working buffers | Do not call from a hard real-time callback |
| Renderer | Caller-owned output buffer and no file I/O do not imply complete callback safety | Validate worst-case callback behavior in the intended host |
| Python F0 | Monophonic estimator, synthetic test coverage | Polyphonic sources and difficult vocal mixtures may produce unreliable estimates |
| Python compressor | Offline, allocating, sample-peak ceiling | Not a true-peak limiter or real-time processor |
| Model integration | No neural compressor checkpoint or voice model runtime is bundled | Local voice engines are upstream audio sources, not embedded inference dependencies |

## Reporting a diagnostic result

Include:

- Commit SHA and branch.
- OS, compiler, CMake, Python, and NumPy versions as applicable.
- Exact command and exit code.
- Complete first failure message and relevant surrounding output.
- Input format, sample rate, channels, and a minimal reproduction when audio is involved.
- Whether the result came from CI, a local build, a sanitizer run, or a real audio host.

Do not describe an unrun sanitizer, host, race, latency, or perceptual test as passing.
