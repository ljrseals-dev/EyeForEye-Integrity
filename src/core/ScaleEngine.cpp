#include "ScaleEngine.hpp"
#include <cmath>
#include <algorithm>
namespace eye {
const std::array<bool,12>& ScaleEngine::builtinMask(ScaleType t) noexcept {
    static const std::array<bool,12> major{{true,false,true,false,true,true,false,true,false,true,false,true}};
    static const std::array<bool,12> minor{{true,false,true,true,false,true,false,true,true,false,true,false}};
    static const std::array<bool,12> penta{{true,false,true,false,true,false,false,true,false,true,false,false}};
    static const std::array<bool,12> chrom{{true,true,true,true,true,true,true,true,true,true,true,true}};
    static const std::array<bool,12> empty{};
    switch(t){case ScaleType::Major:return major;case ScaleType::NaturalMinor:return minor;case ScaleType::Pentatonic:return penta;case ScaleType::Chromatic:return chrom;default:return empty;}
}
void ScaleEngine::setScale(ScaleType t,int root) noexcept {
    type_=(static_cast<unsigned>(t)<static_cast<unsigned>(ScaleType::Count))?t:ScaleType::Major;
    rootNote_=std::clamp(root,0,127);
    if(type_!=ScaleType::Custom) mask_=builtinMask(type_);
}
void ScaleEngine::setCustomMask(std::uint16_t m) noexcept { for(int i=0;i<12;++i) mask_[static_cast<std::size_t>(i)]=(m&(1u<<i))!=0; type_=ScaleType::Custom; }
std::uint8_t ScaleEngine::snapPitchClass(std::uint8_t pc) const noexcept {
    pc=static_cast<std::uint8_t>(pc%12);
    if(std::none_of(mask_.begin(),mask_.end(),[](bool b){return b;})) return pc;
    for(int d=0;d<=6;++d){int below=(static_cast<int>(pc)-d+12)%12;if(mask_[static_cast<std::size_t>(below)])return static_cast<std::uint8_t>(below);int above=(static_cast<int>(pc)+d)%12;if(mask_[static_cast<std::size_t>(above)])return static_cast<std::uint8_t>(above);}
    return pc;
}
double ScaleEngine::getPitchRatio(int midi) const noexcept {
    const int rel=midi-rootNote_; const int octave=(rel>=0)?rel/12:-((-rel+11)/12);
    const int pc=(rel%12+12)%12; const int snapped=static_cast<int>(snapPitchClass(static_cast<std::uint8_t>(pc)));
    return std::pow(2.0,static_cast<double>(octave*12+snapped)/12.0);
}
float ScaleEngine::getCentsDeviation(int midi) const noexcept {
    const int rel=midi-rootNote_; const int octave=(rel>=0)?rel/12:-((-rel+11)/12);
    const int pc=(rel%12+12)%12; const int snapped=static_cast<int>(snapPitchClass(static_cast<std::uint8_t>(pc)));
    return static_cast<float>(rel-(octave*12+snapped))*100.0f;
}
} // namespace eye
