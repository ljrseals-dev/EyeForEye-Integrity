#include "SampleBank.hpp"
#include "WavReader.hpp"
#include "../dsp/PitchShifter.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>
namespace eye { namespace {
std::string lower(std::string s){for(auto&c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
std::pair<std::string,int> parseStem(const std::string&s){
 const auto p=s.rfind('_');if(p==std::string::npos||p==0||p+1>=s.size())return{lower(s),60};const auto tail=s.substr(p+1);
 if(tail.size()<=3&&!tail.empty()&&std::all_of(tail.begin(),tail.end(),[](unsigned char c){return std::isdigit(c);})){int n=0;for(char c:tail){n=n*10+(c-'0');if(n>127)return{lower(s),60};}return{lower(s.substr(0,p)),n};}
 if(tail.size()>=2&&tail.size()<=4){const char l=static_cast<char>(std::tolower(static_cast<unsigned char>(tail[0])));if(l>='a'&&l<='g'){static const int b[]={9,11,0,2,4,5,7};int sem=b[l-'a'];std::size_t i=1;if(i<tail.size()&&(tail[i]=='#'||tail[i]=='b')){sem+=tail[i]=='#'?1:-1;++i;}if(i==tail.size()-1&&std::isdigit(static_cast<unsigned char>(tail[i]))){const int m=(tail[i]-'0'+1)*12+sem;if(m>=0&&m<=127)return{lower(s.substr(0,p)),m};}}}
 return{lower(s),60};
}
}
SampleBank::LoadReport SampleBank::loadDirectory(const std::filesystem::path&dir){LoadReport r;std::error_code ec;if(!std::filesystem::is_directory(dir,ec)||ec)return r;std::vector<std::filesystem::path>paths;std::filesystem::recursive_directory_iterator it(dir,ec),end;for(;!ec&&it!=end;it.increment(ec)){std::error_code se;if(it->is_regular_file(se)&&!se)paths.push_back(it->path());}if(ec){++r.skipped;return r;}std::sort(paths.begin(),paths.end(),[](const auto&a,const auto&b){return a.lexically_normal().generic_string()<b.lexically_normal().generic_string();});for(const auto&p:paths){if(lower(p.extension().string())!=".wav"){++r.skipped;continue;}auto meta=parseStem(p.stem().string());if(meta.first.empty()){++r.skipped;continue;}auto&notes=byPhoneme_[meta.first];if(notes.count(meta.second)){++r.skipped;continue;}AudioBuffer b;if(!loadWav(p,b,meta.second)){++r.skipped;continue;}const auto idx=buffers_.size();buffers_.push_back(std::move(b));notes.emplace(meta.second,idx);++r.loaded;}return r;}
const AudioBuffer*SampleBank::getPhoneme(const std::string&id)const noexcept{auto i=byPhoneme_.find(lower(id));if(i==byPhoneme_.end()||i->second.empty())return nullptr;auto best=i->second.begin();for(auto j=i->second.begin();j!=i->second.end();++j)if(j->first<best->first)best=j;return &buffers_[best->second];}
const AudioBuffer*SampleBank::getNearest(const std::string&id,int note)const noexcept{auto i=byPhoneme_.find(id);if(i==byPhoneme_.end()){const auto key=lower(id);i=byPhoneme_.find(key);}if(i==byPhoneme_.end()||i->second.empty())return nullptr;std::int64_t dist=std::numeric_limits<std::int64_t>::max();int bestNote=std::numeric_limits<int>::max();std::size_t idx=0;for(const auto&kv:i->second){const auto d=std::llabs(static_cast<long long>(kv.first)-static_cast<long long>(note));if(d<dist||(d==dist&&kv.first<bestNote)){dist=d;bestNote=kv.first;idx=kv.second;}}return &buffers_[idx];}
const AudioBuffer*SampleBank::getPitched(const AudioBuffer*source,double ratio){
 if(!source)return nullptr;
 if(!std::isfinite(ratio)||ratio<0.25||ratio>4.0)return nullptr;
 if(std::fabs(ratio-1.0)<1.0e-12)return source;
 std::uint64_t ratioBits=0;
 static_assert(sizeof(ratioBits)==sizeof(ratio),"double must be 64-bit");
 std::memcpy(&ratioBits,&ratio,sizeof(ratioBits));
 const PitchKey key{source,ratioBits};
 const auto found=pitchCache_.find(key);
 if(found!=pitchCache_.end())return &pitchedBuffers_[found->second];
 AudioBuffer pitched=PitchShifter::shift(*source,ratio);
 pitchedBuffers_.push_back(std::move(pitched));
 const std::size_t index=pitchedBuffers_.size()-1;
 try { pitchCache_.emplace(key,index); }
 catch (...) { pitchedBuffers_.pop_back(); throw; }
 return &pitchedBuffers_[index];
}
void SampleBank::clear()noexcept{pitchCache_.clear();pitchedBuffers_.clear();byPhoneme_.clear();buffers_.clear();}
} // namespace eye
