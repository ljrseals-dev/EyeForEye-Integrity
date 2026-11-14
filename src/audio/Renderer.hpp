#pragma once
#include "AudioBuffer.hpp"
#include "Envelope.hpp"
#include "../core/PositionMap.hpp"
#include "../core/ScaleEngine.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace eye {
class SampleBank;
struct MidiEvent{enum class Kind:std::uint8_t{NoteOn,NoteOff}kind;std::uint8_t note,velocity;};
class Renderer{
public:
 static constexpr std::size_t kMaxVoices=16;
 struct Params{float pitchBendSemis=0.0f;float masterGain=1.0f;bool scaleEnabled=true;};
 void prepare(double sampleRate,std::size_t maxFrames)noexcept;
 void setSampleBank(const SampleBank*b)noexcept{bank_=b;}
 void setScaleEngine(const ScaleEngine*s)noexcept{scale_=s;}
 void setPositionMap(const PositionMap*m)noexcept{posMap_=m;}
 // C++17 interface. Caller owns buffers and guarantees event/output ranges.
 void process(float*out,std::size_t frameCount,const MidiEvent*events,std::size_t eventCount,double currentBeat,const Params&params)noexcept;
 std::size_t activeVoices()const noexcept{return activeCount_;}
private:
 struct Voice{const AudioBuffer*sample=nullptr;double pos=0,ratio=1;int midiNote=-1;float gain=1;Envelope env;bool active=false;};
 void noteOn(std::uint8_t,std::uint8_t,double,const Params&)noexcept;void noteOff(std::uint8_t)noexcept;Voice*allocVoice()noexcept;
 const SampleBank*bank_=nullptr;const ScaleEngine*scale_=nullptr;const PositionMap*posMap_=nullptr;
 double sampleRate_=48000;std::array<Voice,kMaxVoices>voices_{};std::size_t activeCount_=0,rrCursor_=0;
};
} // namespace eye
