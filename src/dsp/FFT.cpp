#include "FFT.hpp"
#include <cmath>
#include <stdexcept>

namespace eye {
namespace {
constexpr double kPi = 3.1415926535897932384626433832795;
}

FFT::FFT(std::size_t size) : size_(size) {
    if (size < 2 || (size & (size - 1)) != 0)
        throw std::invalid_argument("FFT size must be a power of two >= 2");
}

void FFT::forward(float* real, float* imag) const {
    transform(real, imag, false);
}

void FFT::inverse(float* real, float* imag) const {
    transform(real, imag, true);
}

void FFT::transform(float* real, float* imag, bool inverseTransform) const {
    if (!real || !imag) throw std::invalid_argument("FFT buffers must not be null");

    const std::size_t n = size_;
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; (j & bit) != 0; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            const float tr = real[i]; real[i] = real[j]; real[j] = tr;
            const float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }

    for (std::size_t len = 2; len <= n; len <<= 1) {
        const double angle = (inverseTransform ? 2.0 : -2.0) * kPi /
                             static_cast<double>(len);
        const float wLenR = static_cast<float>(std::cos(angle));
        const float wLenI = static_cast<float>(std::sin(angle));
        for (std::size_t base = 0; base < n; base += len) {
            float wr = 1.0f, wi = 0.0f;
            const std::size_t half = len >> 1;
            for (std::size_t j = 0; j < half; ++j) {
                const std::size_t even = base + j;
                const std::size_t odd = even + half;
                const float vr = real[odd] * wr - imag[odd] * wi;
                const float vi = real[odd] * wi + imag[odd] * wr;
                const float ur = real[even], ui = imag[even];
                real[even] = ur + vr; imag[even] = ui + vi;
                real[odd] = ur - vr; imag[odd] = ui - vi;
                const float nextWr = wr * wLenR - wi * wLenI;
                wi = wr * wLenI + wi * wLenR;
                wr = nextWr;
            }
        }
        if (len == n) break; // Avoid size_t overflow on the final shift.
    }

    if (inverseTransform) {
        const float scale = 1.0f / static_cast<float>(n);
        for (std::size_t i = 0; i < n; ++i) {
            real[i] *= scale;
            imag[i] *= scale;
        }
    }
}

} // namespace eye
