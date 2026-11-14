#include "audio/AudioBuffer.hpp"
#include "dsp/FFT.hpp"
#include "dsp/PitchShifter.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr double kPi = 3.1415926535897932384626433832795;

float estimateFrequency(const std::vector<float>& samples, double sampleRate) {
    if (samples.size() < 2) return 0.0f;
    std::size_t crossings = 0;
    for (std::size_t i = 1; i < samples.size(); ++i)
        if (samples[i - 1] < 0.0f && samples[i] >= 0.0f) ++crossings;
    return static_cast<float>(static_cast<double>(crossings) * sampleRate /
                              static_cast<double>(samples.size()));
}

bool finite(const std::vector<float>& samples) {
    for (float value : samples) if (!std::isfinite(value)) return false;
    return true;
}
}

int main() {
    eye::FFT fft(8);
    float real[8] = {1,0,0,0,0,0,0,0};
    float imag[8] = {};
    fft.forward(real, imag);
    fft.inverse(real, imag);
    if (std::fabs(real[0] - 1.0f) > 1e-5f) return 1;
    for (std::size_t i = 1; i < 8; ++i)
        if (std::fabs(real[i]) > 1e-5f) return 2;

    bool rejected = false;
    try { eye::FFT invalid(7); } catch (const std::invalid_argument&) { rejected = true; }
    if (!rejected) return 3;

    eye::AudioBuffer input;
    input.sampleRate = 48000;
    input.rootNote = 69;
    input.samples.resize(48000);
    for (std::size_t i = 0; i < input.samples.size(); ++i)
        input.samples[i] = 0.5f * static_cast<float>(std::sin(
            2.0 * kPi * 440.0 * static_cast<double>(i) / 48000.0));

    const auto identity = eye::PitchShifter::shift(input, 1.0);
    if (identity.samples != input.samples) return 4;

    const auto up = eye::PitchShifter::shift(input, 2.0);
    const auto down = eye::PitchShifter::shift(input, 0.5);
    const auto fourth = eye::PitchShifter::shift(input, std::pow(2.0, 4.0 / 12.0));
    if (up.samples.size() != input.samples.size() ||
        down.samples.size() != input.samples.size() ||
        fourth.samples.size() != input.samples.size()) return 5;
    const float fUp = estimateFrequency(up.samples, 48000.0);
    const float fDown = estimateFrequency(down.samples, 48000.0);
    if (std::fabs(fUp - 880.0f) > 45.0f ||
        std::fabs(fDown - 220.0f) > 25.0f) return 6;
    if (!finite(up.samples) || !finite(down.samples) || !finite(fourth.samples)) return 7;

    eye::AudioBuffer shortInput;
    shortInput.sampleRate = 48000;
    shortInput.samples = {0.0f, 1.0f, 0.0f, -1.0f};
    const auto shortResult = eye::PitchShifter::shift(shortInput, 1.5);
    if (shortResult.samples.size() != shortInput.samples.size() ||
        !finite(shortResult.samples)) return 8;

    std::cout << "PitchShifter tests passed (" << fUp << " Hz / "
              << fDown << " Hz)\n";
    return 0;
}
