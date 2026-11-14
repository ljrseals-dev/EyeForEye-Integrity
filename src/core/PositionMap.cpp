#include "PositionMap.hpp"
#include <cmath>
#include <cstring>
#include <limits>

namespace eye {
namespace {
void put32(std::byte* p, std::uint32_t v) noexcept {
    for (unsigned i=0;i<4;++i) p[i]=static_cast<std::byte>((v>>(8u*i))&0xffu);
}
std::uint32_t get32(const std::byte* p) noexcept {
    std::uint32_t v=0; for(unsigned i=0;i<4;++i) v|=static_cast<std::uint32_t>(p[i])<<(8u*i); return v;
}
void put64(std::byte* p, std::uint64_t v) noexcept {
    for(unsigned i=0;i<8;++i) p[i]=static_cast<std::byte>((v>>(8u*i))&0xffu);
}
std::uint64_t get64(const std::byte* p) noexcept {
    std::uint64_t v=0; for(unsigned i=0;i<8;++i) v|=static_cast<std::uint64_t>(p[i])<<(8u*i); return v;
}
}
void PositionMap::record(double beat,std::uint32_t syllable) noexcept {
    if(!std::isfinite(beat)) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if(frozen_ || count_==kCapacity) return;
    std::size_t pos=count_;
    while(pos>0 && entries_[pos-1].beat>beat) { entries_[pos]=entries_[pos-1]; --pos; }
    entries_[pos]={beat,syllable}; ++count_;
}
std::optional<std::uint32_t> PositionMap::lookup(double beat) const noexcept {
    if(!std::isfinite(beat)) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t lo=0,hi=count_;
    while(lo<hi) { const auto mid=lo+(hi-lo)/2; if(entries_[mid].beat<=beat) lo=mid+1; else hi=mid; }
    if(lo==0) return std::nullopt;
    return entries_[lo-1].syllable;
}
void PositionMap::freeze() noexcept { std::lock_guard<std::mutex> l(mutex_); frozen_=true; }
void PositionMap::unfreeze() noexcept { std::lock_guard<std::mutex> l(mutex_); frozen_=false; }
bool PositionMap::isFrozen() const noexcept { std::lock_guard<std::mutex> l(mutex_); return frozen_; }
void PositionMap::reset() noexcept { std::lock_guard<std::mutex> l(mutex_); count_=0; }
std::size_t PositionMap::size() const noexcept { std::lock_guard<std::mutex> l(mutex_); return count_; }
bool PositionMap::empty() const noexcept { return size()==0; }
std::vector<std::byte> PositionMap::serialize() const {
    std::lock_guard<std::mutex> l(mutex_);
    static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559,"IEEE-754 binary64 required");
    std::vector<std::byte> out(12+count_*12);
    put32(out.data(),kMagic); put32(out.data()+4,kFormatVersion); put32(out.data()+8,static_cast<std::uint32_t>(count_));
    std::size_t off=12;
    for(std::size_t i=0;i<count_;++i) {
        std::uint64_t bits=0; std::memcpy(&bits,&entries_[i].beat,8); put64(out.data()+off,bits); off+=8;
        put32(out.data()+off,entries_[i].syllable); off+=4;
    }
    return out;
}
bool PositionMap::deserialize(const std::vector<std::byte>& data) noexcept {
    static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559,"IEEE-754 binary64 required");
    if(data.size()<12) return false;
    if(get32(data.data())!=kMagic || get32(data.data()+4)!=kFormatVersion) return false;
    const auto n=get32(data.data()+8);
    if(n>kCapacity || data.size()!=12+static_cast<std::size_t>(n)*12) return false;
    std::array<Entry,kCapacity> temp{};
    std::size_t off=12;
    for(std::uint32_t i=0;i<n;++i) {
        const auto bits=get64(data.data()+off); off+=8;
        std::memcpy(&temp[i].beat,&bits,8);
        temp[i].syllable=get32(data.data()+off); off+=4;
        if(!std::isfinite(temp[i].beat) || (i && temp[i].beat<temp[i-1].beat)) return false;
    }
    std::lock_guard<std::mutex> l(mutex_);
    entries_=temp; count_=n; return true;
}
} // namespace eye
