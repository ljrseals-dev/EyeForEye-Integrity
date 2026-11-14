#pragma once
#include <cstdint>
namespace eye {
class Envelope {
public:
 enum class Stage:std::uint8_t{Idle,Attack,Decay,Sustain,Release};
 void prepare(double sampleRate) noexcept;
 void setParams(float attackSec,float decaySec,float sustainLevel,float releaseSec) noexcept;
 void noteOn() noexcept; void noteOff() noexcept; void reset() noexcept;
 float tick() noexcept;
 bool isActive() const noexcept{return stage_!=Stage::Idle;}
 float level() const noexcept{return level_;}
 Stage stage() const noexcept{return stage_;}
private:
 double sampleRate_=48000.0; float attackSec_=0.01f,decaySec_=0.1f,sustainLevel_=0.85f,releaseSec_=0.2f;
 float level_=0.0f,releaseStep_=0.0f; Stage stage_=Stage::Idle;
};
} // namespace eye
