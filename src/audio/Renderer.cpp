#include "Renderer.hpp"
#include "SampleBank.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace eye {
void Renderer::prepare(double sr,std::size_t)noexcept{sampleRate_=(std::isfinite(sr)&&sr>0)?sr:48000;for(auto&v:voices_){v=Voice{};v.env.prepare(sampleRate_);}activeCount_=rrCursor_=0;}
Renderer::Voice*Renderer::allocVoice()noexcept{for(auto&v:voices_)if(!v.active)return &v;Voice*v=&voices_[rrCursor_];rrCursor_=(rrCursor_+1)%kMaxVoices;return v;}
void Renderer::noteOn(std::uint8_t note,std::uint8_t vel,double,const Params&p)noexcept{
 if(!bank_||vel==0)return;const auto*b=bank_->getNearest("AH",note);if(!b||b->samples.size()<2||!b->sampleRate)return;
 const double bend=std::isfinite(p.pitchBendSemis)?std::clamp(static_cast<double>(p.pitchBendSemis),-48.0,48.0):0.0;
 double pitchRatio=std::pow(2.0,(static_cast<int>(note)-b->rootNote)/12.0);
 if(p.scaleEnabled&&scale_){const double fromRoot=scale_->getPitchRatio(static_cast<int>(note));const double rootOffset=std::pow(2.0,(scale_->rootNote()-b->rootNote)/12.0);pitchRatio=fromRoot*rootOffset;}
 const double ratio=(static_cast<double>(b->sampleRate)/sampleRate_)*pitchRatio*std::pow(2.0,bend/12.0);if(!std::isfinite(ratio)||ratio<=0)return;
 Voice*v=allocVoice();v->sample=b;v->pos=0;v->midiNote=note;v->gain=static_cast<float>(vel)/127.0f;v->ratio=ratio;v->active=true;v->env.prepare(sampleRate_);v->env.setParams(.01f,.10f,.85f,.20f);v->env.noteOn();
}
void Renderer::noteOff(std::uint8_t n)noexcept{for(auto&v:voices_)if(v.active&&v.midiNote==static_cast<int>(n))v.env.noteOff();}
void Renderer::process(float*out,std::size_t count,const MidiEvent*events,std::size_t eventCount,double beat,const Params&p)noexcept{
 if(!out)return;if(events)for(std::size_t i=0;i<eventCount;++i){if(events[i].kind==MidiEvent::Kind::NoteOn){if(events[i].velocity==0)noteOff(events[i].note);else noteOn(events[i].note,events[i].velocity,beat,p);}else noteOff(events[i].note);}
 std::fill(out,out+count,0.0f);const float master=std::isfinite(p.masterGain)?std::clamp(p.masterGain,0.0f,4.0f):0.0f;
 for(auto&v:voices_){if(!v.active||!v.sample)continue;const auto&src=v.sample->samples;for(std::size_t i=0;i<count;++i){if(!v.env.isActive()){v.active=false;break;}const float env=v.env.tick();if(!v.env.isActive()){v.active=false;break;}if(v.pos<0||v.pos>=static_cast<double>(src.size()-1)){v.active=false;break;}const auto i0=static_cast<std::size_t>(v.pos);const float frac=static_cast<float>(v.pos-static_cast<double>(i0));out[i]+=(src[i0]+(src[i0+1]-src[i0])*frac)*env*v.gain*master;v.pos+=v.ratio;}}
 activeCount_=0;for(const auto&v:voices_)if(v.active)++activeCount_;for(std::size_t i=0;i<count;++i)out[i]=std::clamp(out[i],-1.0f,1.0f);
}
} // namespace eye
