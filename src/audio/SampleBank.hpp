#pragma once
#include "AudioBuffer.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>
namespace eye {
class SampleBank {
public:
 struct LoadReport{std::size_t loaded=0,skipped=0;};
 // Load/clear must be externally serialized against rendering. Appending keeps
 // existing AudioBuffer addresses stable; clear() invalidates every returned pointer.
 LoadReport loadDirectory(const std::filesystem::path&);
 const AudioBuffer* getPhoneme(const std::string&)const noexcept;
 const AudioBuffer* getNearest(const std::string&,int targetNote)const noexcept;
 const AudioBuffer* getPitched(const AudioBuffer* source, double pitchRatio);
 std::size_t size()const noexcept{return byPhoneme_.size();}
 void clear()noexcept;
private:
 struct PitchKey {
     const AudioBuffer* source=nullptr;
     std::uint64_t ratioBits=0;
     bool operator==(const PitchKey& other) const noexcept {
         return source==other.source && ratioBits==other.ratioBits;
     }
 };
 struct PitchKeyHash {
     std::size_t operator()(const PitchKey& key) const noexcept {
         const auto p=std::hash<const void*>{}(key.source);
         const auto r=std::hash<std::uint64_t>{}(key.ratioBits);
         return p ^ (r + static_cast<std::size_t>(0x9e3779b9u) + (p<<6) + (p>>2));
     }
 };
 std::unordered_map<std::string,std::unordered_map<int,std::size_t>>byPhoneme_;
 std::deque<AudioBuffer>buffers_;
 std::unordered_map<PitchKey,std::size_t,PitchKeyHash>pitchCache_;
 // deque preserves addresses returned to voices as later pitches are cached.
 std::deque<AudioBuffer>pitchedBuffers_;
};
} // namespace eye
