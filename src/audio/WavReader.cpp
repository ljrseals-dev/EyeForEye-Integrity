#include "WavReader.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>
namespace eye { namespace {
std::uint16_t u16(const std::uint8_t*p){return static_cast<std::uint16_t>(p[0])|static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1])<<8);}
std::uint32_t u32(const std::uint8_t*p){return static_cast<std::uint32_t>(p[0])|(static_cast<std::uint32_t>(p[1])<<8)|(static_cast<std::uint32_t>(p[2])<<16)|(static_cast<std::uint32_t>(p[3])<<24);}
float sample(const std::uint8_t*p,std::uint16_t fmt,std::uint16_t bits,bool&ok){
 if(fmt==3&&bits==32){float v;std::memcpy(&v,p,4);if(!std::isfinite(v)){ok=false;return 0;}return v;}
 if(fmt==3&&bits==64){double v;std::memcpy(&v,p,8);if(!std::isfinite(v)){ok=false;return 0;}return static_cast<float>(v);}
 if(fmt==1&&bits==16){const auto v=static_cast<std::int16_t>(u16(p));return static_cast<float>(v)/32768.0f;}
 if(fmt==1&&bits==24){std::int32_t v=static_cast<std::int32_t>(static_cast<std::uint32_t>(p[0])|(static_cast<std::uint32_t>(p[1])<<8)|(static_cast<std::uint32_t>(p[2])<<16));if(v&0x800000)v-=0x1000000;return static_cast<float>(v)/8388608.0f;}
 if(fmt==1&&bits==32){const auto u=u32(p);const std::int64_t v=(u&0x80000000u)?static_cast<std::int64_t>(u)-0x100000000LL:u;return static_cast<float>(static_cast<double>(v)/2147483648.0);}
 ok=false;return 0;
}
}
bool loadWav(const std::filesystem::path&path,AudioBuffer&out,int rootNote)noexcept{
 try{
  std::ifstream f(path,std::ios::binary);if(!f)return false;f.seekg(0,std::ios::end);const auto end=f.tellg();if(end<12)return false;const auto fileSize=static_cast<std::uint64_t>(end);f.seekg(0);
  std::uint8_t hdr[12]{};f.read(reinterpret_cast<char*>(hdr),12);if(!f||std::memcmp(hdr,"RIFF",4)||std::memcmp(hdr+8,"WAVE",4))return false;
  const std::uint64_t riffEnd=static_cast<std::uint64_t>(u32(hdr+4))+8u;if(riffEnd>fileSize||riffEnd<12)return false;
  std::uint16_t fmt=0,ch=0,bits=0;std::uint32_t sr=0;std::vector<std::uint8_t>pcm;bool haveFmt=false,haveData=false;std::uint64_t pos=12;
  while(pos+8<=riffEnd){f.seekg(static_cast<std::streamoff>(pos));std::uint8_t h[8]{};f.read(reinterpret_cast<char*>(h),8);if(!f)return false;const std::uint32_t n=u32(h+4);const std::uint64_t body=pos+8u;if(static_cast<std::uint64_t>(n)>riffEnd-body)return false;
   if(std::memcmp(h,"fmt ",4)==0){if(n<16||n>4096)return false;std::vector<std::uint8_t>b(n);f.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(n));if(!f)return false;fmt=u16(b.data());ch=u16(b.data()+2);sr=u32(b.data()+4);bits=u16(b.data()+14);if(fmt==0xfffe)return false; // Extensible GUIDs are intentionally unsupported until fully validated.
   haveFmt=true;
  }
   else if(std::memcmp(h,"data",4)==0){if(!n)return false;pcm.resize(n);f.read(reinterpret_cast<char*>(pcm.data()),static_cast<std::streamsize>(n));if(!f)return false;haveData=true;}
   pos=body+static_cast<std::uint64_t>(n)+(n&1u);if(pos>riffEnd)return false;
  }
  if(pos!=riffEnd||!haveFmt||!haveData||!ch||!sr||rootNote<0||rootNote>127)return false;
  const std::size_t bps=bits/8u;if((fmt!=1&&fmt!=3)||!(bits==16||bits==24||bits==32||(fmt==3&&bits==64))||!bps)return false;
  if(static_cast<std::size_t>(ch)>std::numeric_limits<std::size_t>::max()/bps)return false;const std::size_t frameBytes=bps*ch;if(pcm.size()%frameBytes)return false;const std::size_t frames=pcm.size()/frameBytes;if(!frames)return false;
  AudioBuffer temp;temp.samples.resize(frames);temp.sampleRate=sr;temp.rootNote=rootNote;
  for(std::size_t i=0;i<frames;++i){double acc=0;for(std::size_t c=0;c<ch;++c){bool ok=true;const float v=sample(pcm.data()+i*frameBytes+c*bps,fmt,bits,ok);if(!ok)return false;acc+=v;}temp.samples[i]=static_cast<float>(acc/static_cast<double>(ch));}
  out=std::move(temp);return true;
 }catch(...){return false;}
}
} // namespace eye
