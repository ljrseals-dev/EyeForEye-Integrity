#pragma once
#include <array>
#include <cstdint>
namespace eye {
enum class ScaleType : std::uint8_t { Major=0, NaturalMinor, Pentatonic, Chromatic, Custom, Count };
class ScaleEngine {
public:
    ScaleEngine() noexcept { setScale(ScaleType::Major,60); }
    void setScale(ScaleType type,int rootNote=60) noexcept;
    void setCustomMask(std::uint16_t mask12bit) noexcept;
    ScaleType scale() const noexcept { return type_; }
    int rootNote() const noexcept { return rootNote_; }
    double getPitchRatio(int midiNote) const noexcept;
    float getCentsDeviation(int midiNote) const noexcept;
    std::uint8_t snapPitchClass(std::uint8_t pc) const noexcept;
    const std::array<bool,12>& mask() const noexcept { return mask_; }
private:
    static const std::array<bool,12>& builtinMask(ScaleType) noexcept;
    ScaleType type_=ScaleType::Major;
    int rootNote_=60; // absolute MIDI root, not pitch class
    std::array<bool,12> mask_{};
};
} // namespace eye
