#pragma once
#include "AudioBuffer.hpp"
#include <cstddef>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
namespace eye {
class SampleBank {
public:
 struct LoadReport{std::size_t loaded=0,skipped=0;};
 LoadReport loadDirectory(const std::filesystem::path& dir);
 const AudioBuffer* getPhoneme(const std::string&id)const noexcept;
 const AudioBuffer* getNearest(const std::string&id,int targetNote)const noexcept;
 std::size_t size()const noexcept{return byPhoneme_.size();}
 void clear()noexcept;
private:
 std::unordered_map<std::string,std::unordered_map<int,std::size_t>> byPhoneme_;
 // deque preserves references/pointers across append; clear() invalidates all.
 std::deque<AudioBuffer> buffers_;
};
} // namespace eye
