#include "../audio/Envelope.hpp"
#include "../audio/Renderer.hpp"
#include "../audio/SampleBank.hpp"
#include "../audio/WavReader.hpp"
#include "../core/PositionMap.hpp"
#include "../core/ScaleEngine.hpp"
#include "../dsp/PitchShifter.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
static bool writeWav(const fs::path&p){const std::uint32_t frames=24000,bytes=frames*2u;std::ofstream f(p,std::ios::binary);if(!f)return false;auto w16=[&](std::uint16_t v){char b[2]={static_cast<char>(v&255),static_cast<char>((v>>8)&255)};f.write(b,2);};auto w32=[&](std::uint32_t v){char b[4]={static_cast<char>(v&255),static_cast<char>((v>>8)&255),static_cast<char>((v>>16)&255),static_cast<char>((v>>24)&255)};f.write(b,4);};f.write("RIFF",4);w32(36+bytes);f.write("WAVEfmt ",8);w32(16);w16(1);w16(1);w32(48000);w32(96000);w16(2);w16(16);f.write("data",4);w32(bytes);constexpr double pi=3.14159265358979323846;for(std::uint32_t i=0;i<frames;++i){const auto s=static_cast<std::int16_t>(std::sin(2*pi*261.625565*i/48000.0)*16000);w16(static_cast<std::uint16_t>(s));}return static_cast<bool>(f);}
int main(){int fail=0;auto check=[&](bool v,const char*m){if(!v){std::cerr<<"FAIL: "<<m<<"\n";++fail;}};
{eye::PositionMap m;m.record(0,1);m.record(2,3);m.record(1,2);auto blob=m.serialize();eye::PositionMap n;check(n.deserialize(blob)&&n.lookup(1.5).value_or(0)==2,"PositionMap serialize/lookup");}
{eye::ScaleEngine s;s.setScale(eye::ScaleType::Major,60);check(std::fabs(s.getPitchRatio(60)-1)<1e-9,"Scale root ratio");check(std::fabs(s.getPitchRatio(72)-2)<1e-9,"Scale octave ratio");}
{eye::Envelope e;e.prepare(48000);e.setParams(.1f,.1f,0,.01f);e.noteOn();for(int i=0;i<100;++i)e.tick();e.noteOff();for(int i=0;i<1000&&e.isActive();++i)e.tick();check(!e.isActive(),"zero-sustain release terminates");}
{const auto dir=fs::temp_directory_path()/"eye_for_eye_m2_harness";std::error_code ec;fs::remove_all(dir,ec);fs::create_directories(dir,ec);check(writeWav(dir/"AH_60.wav"),"write WAV");check(writeWav(dir/"AH_64.wav"),"write second root WAV");const auto bad=dir/"truncated.wav";{std::ofstream f(bad,std::ios::binary);f.write("RIFF",4);const char size[4]={0,16,0,0};f.write(size,4);f.write("WAVE",4);}eye::AudioBuffer malformed;check(!eye::loadWav(bad,malformed),"reject truncated RIFF chunk declaration");eye::SampleBank bank;auto r=bank.loadDirectory(dir);check(r.loaded==2,"SampleBank loads both roots");check(bank.getNearest("ah",62)&&bank.getNearest("ah",62)->rootNote==60,"nearest-note ties choose lower MIDI");const auto*ptr=bank.getPhoneme("ah");check(ptr&&ptr->samples.size()==24000,"SampleBank lookup");for(int i=0;i<12;++i)check(writeWav(dir/("OW_"+std::to_string(60+i)+".wav")),"write append WAV");bank.loadDirectory(dir);check(ptr&&ptr->samples.size()==24000,"SampleBank pointer stable after append");const auto*pitchA=bank.getPitched(ptr,1.5);const auto*pitchB=bank.getPitched(ptr,1.25);check(pitchA&&pitchB&&pitchA!=pitchB,"SampleBank creates distinct pitch variants");check(bank.getPitched(ptr,1.5)==pitchA,"SampleBank pitch cache hit is stable");eye::Renderer renderer;eye::ScaleEngine scale;scale.setScale(eye::ScaleType::Chromatic,60);renderer.prepare(48000,512);renderer.setSampleBank(&bank);renderer.setScaleEngine(&scale);float out[512]{};eye::MidiEvent ev{eye::MidiEvent::Kind::NoteOn,60,100};eye::Renderer::Params params;renderer.process(out,512,&ev,1,0,params);float peak=0;for(float v:out)peak=std::max(peak,std::fabs(v));check(peak>.001f,"Renderer produces audio");fs::remove_all(dir,ec);}
{eye::AudioBuffer source;source.sampleRate=48000;source.rootNote=69;source.samples.resize(24000);constexpr double pi=3.14159265358979323846;for(std::size_t i=0;i<source.samples.size();++i)source.samples[i]=0.5f*static_cast<float>(std::sin(2*pi*440.0*static_cast<double>(i)/48000.0));const auto shifted=eye::PitchShifter::shift(source,2.0);check(shifted.samples.size()==source.samples.size(),"pitch shifter preserves duration");bool allFinite=true;for(float v:shifted.samples)if(!std::isfinite(v)){allFinite=false;break;}check(allFinite,"pitch shifter output is finite");}
std::cout<<(fail?"ArtifactHarness FAIL":"ArtifactHarness PASS")<<" ("<<fail<<" failures)\n";return fail?1:0;}
