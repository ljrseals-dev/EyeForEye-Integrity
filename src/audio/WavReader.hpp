#pragma once
#include "AudioBuffer.hpp"
#include <filesystem>
namespace eye {
// Loads PCM 16/24/32-bit or IEEE float 32/64-bit RIFF/WAVE and downmixes to mono.
// Returns false for malformed, truncated, unsupported, or unreadable input.
bool loadWav(const std::filesystem::path& path,AudioBuffer& out,int rootNote=60) noexcept;
} // namespace eye
