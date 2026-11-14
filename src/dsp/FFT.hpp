#pragma once
#include <cstddef>

namespace eye {

// In-place radix-2 complex FFT. Buffers must contain size() floats each.
class FFT {
public:
    explicit FFT(std::size_t size);
    std::size_t size() const noexcept { return size_; }
    void forward(float* real, float* imag) const;
    void inverse(float* real, float* imag) const;

private:
    void transform(float* real, float* imag, bool inverse) const;
    std::size_t size_;
};

} // namespace eye
