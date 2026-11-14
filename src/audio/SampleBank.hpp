#pragma once
#include "AudioBuffer.hpp"
#include <cstddef>
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
 std::size_t size()const noexcept{return byPhoneme_.size();}
 void clear()noexcept;
private:
 std::unordered_map<std::string,std::unordered_map<int,std::size_t>>byPhoneme_;
 std::deque<AudioBuffer>buffers_;
};
} // namespace eye
