#pragma once
#include <cstdint>
#include <vector>
namespace eye {
struct AudioBuffer { std::vector<float> samples; std::uint32_t sampleRate=0; int rootNote=60; };
} // namespace eye
