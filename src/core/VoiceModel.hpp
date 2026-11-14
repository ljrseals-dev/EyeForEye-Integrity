#pragma once
#include <cstddef>
#include <cstdint>
namespace eye {
enum class VoiceType:std::uint8_t{Human=0,Songbird,Seal,Alligator,Elephant,Count};
struct VoiceDefaults{VoiceType type;const char*libraryName;const char*defaultPhoneme;float attackSec,decaySec,sustainLevel,releaseSec,gain;};
inline constexpr VoiceDefaults kVoiceTable[]={
 {VoiceType::Human,"human","AH",.010f,.100f,.85f,.200f,.9f},
 {VoiceType::Songbird,"heka-phonemes","IH",.005f,.060f,.90f,.120f,.8f},
 {VoiceType::Seal,"ljrseals-dev","OW",.020f,.150f,.70f,.250f,1.0f},
 {VoiceType::Alligator,"alligator","ER",.030f,.200f,.65f,.350f,1.1f},
 {VoiceType::Elephant,"elephant","UW",.040f,.250f,.60f,.400f,1.2f}};
inline const VoiceDefaults& voiceDefaults(VoiceType t)noexcept{const auto i=static_cast<std::size_t>(t);return kVoiceTable[i<sizeof(kVoiceTable)/sizeof(kVoiceTable[0])?i:0];}
} // namespace eye
