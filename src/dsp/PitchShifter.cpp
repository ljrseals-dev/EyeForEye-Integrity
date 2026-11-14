#include "PitchShifter.hpp"
#include "FFT.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace eye {
namespace {
constexpr double kPi = 3.1415926535897932384626433832795;

float interpolate(const std::vector<float>& data, double position) {
    if (data.empty()) return 0.0f;
    position = std::clamp(position, 0.0, static_cast<double>(data.size() - 1));
    const auto i = static_cast<std::size_t>(position);
    const auto j = std::min(i + 1, data.size() - 1);
    const float f = static_cast<float>(position - static_cast<double>(i));
    return data[i] + (data[j] - data[i]) * f;
}

AudioBuffer shortFallback(const AudioBuffer& input, double ratio) {
    AudioBuffer out;
    out.sampleRate = input.sampleRate;
    out.rootNote = input.rootNote;
    out.samples.resize(input.samples.size());
    const double center = input.samples.empty() ? 0.0 :
                          static_cast<double>(input.samples.size() - 1) * 0.5;
    for (std::size_t i = 0; i < out.samples.size(); ++i) {
        const double source = center +
            (static_cast<double>(i) - center) * ratio;
        out.samples[i] = interpolate(input.samples, source);
    }
    return out;
}
}

AudioBuffer PitchShifter::shift(const AudioBuffer& input, double ratio,
                                std::size_t fftSize, std::size_t analysisHop) {
    if (!std::isfinite(ratio) || ratio < 0.25 || ratio > 4.0)
        throw std::invalid_argument("pitch ratio must be finite and in [0.25, 4]");
    if (fftSize < 2 || (fftSize & (fftSize - 1)) != 0 ||
        analysisHop == 0 || analysisHop > fftSize)
        throw std::invalid_argument("invalid phase-vocoder configuration");

    if (input.samples.empty() || std::fabs(ratio - 1.0) < 1.0e-12)
        return input;
    if (input.samples.size() < fftSize)
        return shortFallback(input, ratio);

    const std::size_t n = fftSize;
    const std::size_t bins = n / 2 + 1;
    const std::size_t hopS = std::max<std::size_t>(
        1, static_cast<std::size_t>(std::llround(
            static_cast<double>(analysisHop) * ratio)));
    const std::size_t frames = 1 +
        (input.samples.size() - 1) / analysisHop;
    if (frames > (static_cast<std::size_t>(-1) - n) / hopS)
        throw std::length_error("phase-vocoder output is too large");
    const std::size_t stretchedSize = (frames - 1) * hopS + n;

    std::vector<float> window(n), real(n), imag(n);
    std::vector<double> previousPhase(bins, 0.0), synthesisPhase(bins, 0.0);
    std::vector<float> output(stretchedSize, 0.0f);
    std::vector<float> weight(stretchedSize, 0.0f);
    for (std::size_t i = 0; i < n; ++i)
        window[i] = static_cast<float>(0.5 - 0.5 *
            std::cos(2.0 * kPi * static_cast<double>(i) /
                     static_cast<double>(n - 1)));

    FFT fft(n);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const std::size_t inStart = frame * analysisHop;
        const std::size_t outStart = frame * hopS;
        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t sourceIndex = inStart + i;
            real[i] = (sourceIndex < input.samples.size()
                ? input.samples[sourceIndex] : 0.0f) * window[i];
            imag[i] = 0.0f;
        }
        fft.forward(real.data(), imag.data());

        for (std::size_t k = 0; k < bins; ++k) {
            const double phase = std::atan2(static_cast<double>(imag[k]),
                                            static_cast<double>(real[k]));
            if (frame == 0) {
                synthesisPhase[k] = phase;
            } else {
                const double expected = 2.0 * kPi *
                    static_cast<double>(k * analysisHop) / static_cast<double>(n);
                double delta = phase - previousPhase[k] - expected;
                delta -= 2.0 * kPi * std::round(delta / (2.0 * kPi));
                const double trueAdvance = expected + delta;
                synthesisPhase[k] += trueAdvance *
                    static_cast<double>(hopS) / static_cast<double>(analysisHop);
            }
            previousPhase[k] = phase;
            const double magnitude = std::hypot(static_cast<double>(real[k]),
                                                static_cast<double>(imag[k]));
            real[k] = static_cast<float>(magnitude * std::cos(synthesisPhase[k]));
            imag[k] = static_cast<float>(magnitude * std::sin(synthesisPhase[k]));
        }
        for (std::size_t k = bins; k < n; ++k) {
            const std::size_t mirror = n - k;
            real[k] = real[mirror];
            imag[k] = -imag[mirror];
        }
        fft.inverse(real.data(), imag.data());

        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t at = outStart + i;
            output[at] += real[i] * window[i];
            weight[at] += window[i] * window[i];
        }
    }

    for (std::size_t i = 0; i < output.size(); ++i)
        if (weight[i] > 1.0e-8f) output[i] /= weight[i];

    AudioBuffer result;
    result.sampleRate = input.sampleRate;
    result.rootNote = input.rootNote;
    result.samples.resize(input.samples.size());
    for (std::size_t i = 0; i < result.samples.size(); ++i)
        result.samples[i] = interpolate(output,
            static_cast<double>(i) * ratio);
    return result;
}

} // namespace eye
