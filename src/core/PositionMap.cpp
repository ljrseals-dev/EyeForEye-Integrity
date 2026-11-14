#include "PositionMap.hpp"
#include <cmath>
#include <cstring>
#include <limits>
namespace eye { namespace {
void put32(std::byte*p,std::uint32_t v){for(unsigned i=0;i<4;++i)p[i]=static_cast<std::byte>((v>>(8u*i))&255u);}
std::uint32_t get32(const std::byte*p){std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=static_cast<std::uint32_t>(p[i])<<(8u*i);return v;}
void put64(std::byte*p,std::uint64_t v){for(unsigned i=0;i<8;++i)p[i]=static_cast<std::byte>((v>>(8u*i))&255u);}
std::uint64_t get64(const std::byte*p){std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=static_cast<std::uint64_t>(p[i])<<(8u*i);return v;}
}
void PositionMap::record(double beat,std::uint32_t id){if(!std::isfinite(beat))return;std::lock_guard<std::mutex>l(mutex_);if(frozen_||count_==kCapacity)return;std::size_t p=count_;while(p&&entries_[p-1].beat>beat){entries_[p]=entries_[p-1];--p;}entries_[p]={beat,id};++count_;}
std::optional<std::uint32_t> PositionMap::lookup(double beat)const{if(!std::isfinite(beat))return std::nullopt;std::lock_guard<std::mutex>l(mutex_);std::size_t lo=0,hi=count_;while(lo<hi){auto m=lo+(hi-lo)/2;if(entries_[m].beat<=beat)lo=m+1;else hi=m;}if(!lo)return std::nullopt;return entries_[lo-1].syllable;}
void PositionMap::freeze(){std::lock_guard<std::mutex>l(mutex_);frozen_=true;}void PositionMap::unfreeze(){std::lock_guard<std::mutex>l(mutex_);frozen_=false;}bool PositionMap::isFrozen()const{std::lock_guard<std::mutex>l(mutex_);return frozen_;}void PositionMap::reset(){std::lock_guard<std::mutex>l(mutex_);count_=0;}std::size_t PositionMap::size()const{std::lock_guard<std::mutex>l(mutex_);return count_;}bool PositionMap::empty()const{return size()==0;}
std::vector<std::byte> PositionMap::serialize()const{std::lock_guard<std::mutex>l(mutex_);static_assert(sizeof(double)==8&&std::numeric_limits<double>::is_iec559,"IEEE binary64 required");std::vector<std::byte>o(12+count_*12);put32(o.data(),kMagic);put32(o.data()+4,kFormatVersion);put32(o.data()+8,static_cast<std::uint32_t>(count_));std::size_t p=12;for(std::size_t i=0;i<count_;++i){std::uint64_t bits=0;std::memcpy(&bits,&entries_[i].beat,8);put64(o.data()+p,bits);p+=8;put32(o.data()+p,entries_[i].syllable);p+=4;}return o;}
bool PositionMap::deserialize(const std::vector<std::byte>&d){static_assert(sizeof(double)==8&&std::numeric_limits<double>::is_iec559,"IEEE binary64 required");if(d.size()<12||get32(d.data())!=kMagic||get32(d.data()+4)!=kFormatVersion)return false;const auto n=get32(d.data()+8);if(n>kCapacity||d.size()!=12+static_cast<std::size_t>(n)*12)return false;std::array<Entry,kCapacity>tmp{};std::size_t p=12;for(std::uint32_t i=0;i<n;++i){auto bits=get64(d.data()+p);p+=8;std::memcpy(&tmp[i].beat,&bits,8);tmp[i].syllable=get32(d.data()+p);p+=4;if(!std::isfinite(tmp[i].beat)||(i&&tmp[i].beat<tmp[i-1].beat))return false;}std::lock_guard<std::mutex>l(mutex_);entries_=tmp;count_=n;return true;}
} // namespace eye
