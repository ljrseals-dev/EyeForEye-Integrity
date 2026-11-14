#include "Envelope.hpp"
#include <algorithm>
#include <cmath>
namespace eye {
void Envelope::prepare(double sr) noexcept{sampleRate_=(std::isfinite(sr)&&sr>0)?sr:48000.0;reset();}
void Envelope::setParams(float a,float d,float s,float r) noexcept{attackSec_=std::isfinite(a)?std::max(0.0f,a):0.0f;decaySec_=std::isfinite(d)?std::max(0.0f,d):0.0f;sustainLevel_=std::isfinite(s)?std::clamp(s,0.0f,1.0f):0.0f;releaseSec_=std::isfinite(r)?std::max(0.0f,r):0.0f;}
void Envelope::reset() noexcept{stage_=Stage::Idle;level_=0;releaseStep_=0;}
void Envelope::noteOn() noexcept{stage_=Stage::Attack;}
void Envelope::noteOff() noexcept{if(stage_!=Stage::Idle){stage_=Stage::Release;releaseStep_=(releaseSec_<=0.0f)?level_:level_/static_cast<float>(releaseSec_*sampleRate_);if(releaseStep_<=0.0f){level_=0;stage_=Stage::Idle;}}}
float Envelope::tick() noexcept{
 switch(stage_){
 case Stage::Attack: level_+=attackSec_<=0?1.0f:static_cast<float>(1.0/(attackSec_*sampleRate_));if(level_>=1){level_=1;stage_=Stage::Decay;}break;
 case Stage::Decay: level_-=decaySec_<=0?1.0f:static_cast<float>((1.0-sustainLevel_)/(decaySec_*sampleRate_));if(level_<=sustainLevel_){level_=sustainLevel_;stage_=Stage::Sustain;}break;
 case Stage::Sustain:level_=sustainLevel_;break;
 case Stage::Release:level_-=releaseStep_;if(level_<=0){level_=0;stage_=Stage::Idle;}break;
 case Stage::Idle:level_=0;break;
 } return level_;
}
} // namespace eye
