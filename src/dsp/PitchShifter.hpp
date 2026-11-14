#pragma once
#include "../audio/AudioBuffer.hpp"
#include <cstddef>

namespace eye {

// Offline duration-preserving phase-vocoder pitch shifter.
// ratio > 1 raises pitch; ratio < 1 lowers pitch.
class PitchShifter {
public:
    static AudioBuffer shift(const AudioBuffer& input, double ratio,
                             std::size_t fftSize = 2048,
                             std::size_t analysisHop = 512);
};

} // namespace eye
