#pragma once
#include "BSIIAfxArchive.h"
#include <optional>

namespace bsii::afx
{
// Dedicated overlayIndex map from Novation Components kD/jX (1.72.5).
// Source/hash and display-unit limits: docs/afx-field-map-research.md.
enum class Kind { boolean, mask, signed7, signed8, unsignedValue, fixedPitch, output };
struct Field {const char* id;const char* group;const char* label;int offset;Kind kind;int bitOrMask;};
inline constexpr Field fields[]{
        {"voiceOscSync","Voice","Osc 1/2 Sync",1,Kind::boolean,0},
        {"osc1Wave","Osc 1","Wave",2,Kind::mask,3},
        {"osc1PulseWidth","Osc 1","Pulse Width",3,Kind::unsignedValue,0},
        {"osc1Octave","Osc 1","Octave",4,Kind::signed7,0},
        {"osc1TuneCoarse","Osc 1","Coarse",5,Kind::signed8,0},
        {"osc1TuneFine","Osc 1","Fine",6,Kind::signed8,0},
        {"osc2Wave","Osc 2","Wave",7,Kind::mask,3},
        {"osc2PulseWidth","Osc 2","Pulse Width",8,Kind::unsignedValue,0},
        {"osc2Octave","Osc 2","Octave",9,Kind::signed7,0},
        {"osc2TuneCoarse","Osc 2","Coarse",10,Kind::signed8,0},
        {"osc2TuneFine","Osc 2","Fine",11,Kind::signed8,0},
        {"oscSubWave","Sub","Wave",12,Kind::mask,3},
        {"oscSubMode","Sub","Sub mode",12,Kind::boolean,2},
        {"oscSubPulseWidth","Sub","Pulse Width",13,Kind::unsignedValue,0},
        {"oscSubOctave","Sub","Octave",14,Kind::signed7,0},
        {"oscSubTuneCoarse","Sub","Coarse",15,Kind::signed8,0},
        {"oscSubTuneFine","Sub","Fine",16,Kind::signed8,0},
        {"oscSubWaveSubMode","Sub","Sub-mode waveform",17,Kind::mask,3},
        {"oscSubOctaveSubMode","Sub","Sub-mode octave",18,Kind::signed7,0},
        {"mixerOsc1","Mixer","Osc1",19,Kind::unsignedValue,0},
        {"mixerOsc2","Mixer","Osc2",20,Kind::unsignedValue,0},
        {"mixerOscSub","Mixer","Osc Sub",21,Kind::unsignedValue,0},
        {"mixerNoise","Mixer","Noise",22,Kind::unsignedValue,0},
        {"mixerRingMod","Mixer","Ring Mod",23,Kind::unsignedValue,0},
        {"mixerExternal","Mixer","External",24,Kind::unsignedValue,0},
        {"filterFrequency","Filter","Frequency",25,Kind::unsignedValue,0},
        {"filterResonance","Filter","Resonance",26,Kind::unsignedValue,0},
        {"filterOverdrive","Filter","Overdrive",27,Kind::unsignedValue,0},
        {"filterShape","Filter","Shape",28,Kind::mask,3},
        {"filterCircuit","Filter","Circuit",28,Kind::boolean,2},
        {"filterSlope","Filter","Slope",28,Kind::boolean,3},
        {"envAmpVelocity","Amp Env","Velocity",29,Kind::signed7,0},
        {"envAmpAttack","Amp Env","Attack",30,Kind::unsignedValue,0},
        {"envAmpDecay","Amp Env","Decay",31,Kind::unsignedValue,0},
        {"envAmpSustain","Amp Env","Sustain",32,Kind::unsignedValue,0},
        {"envAmpRelease","Amp Env","Release",33,Kind::unsignedValue,0},
        {"envAmpTrigger","Amp Env","Trigger",34,Kind::mask,3},
        {"envModVelocity","Mod Env","Velocity",35,Kind::signed7,0},
        {"envModAttack","Mod Env","Attack",36,Kind::unsignedValue,0},
        {"envModDecay","Mod Env","Decay",37,Kind::unsignedValue,0},
        {"envModSustain","Mod Env","Sustain",38,Kind::unsignedValue,0},
        {"envModRelease","Mod Env","Release",39,Kind::unsignedValue,0},
        {"envModTrigger","Mod Env","Trigger",40,Kind::mask,3},
        {"lfo1Waveform","LFO 1","Waveform",41,Kind::unsignedValue,0},
        {"lfo1Delay","LFO 1","Delay",42,Kind::unsignedValue,0},
        {"lfo1LfoSlew","LFO 1","Slew",43,Kind::unsignedValue,0},
        {"lfo1Speed","LFO 1","Speed",44,Kind::unsignedValue,0},
        {"lfo1SpeedSync","LFO 1","Speed Sync",45,Kind::unsignedValue,0},
        {"lfo1SpeedMode","LFO 1","Speed Mode",46,Kind::boolean,0},
        {"lfo1KeySync","LFO 1","Key Sync",46,Kind::boolean,1},
        {"lfo2Waveform","LFO 2","Waveform",47,Kind::unsignedValue,0},
        {"lfo2Delay","LFO 2","Delay",48,Kind::unsignedValue,0},
        {"lfo2LfoSlew","LFO 2","Slew",49,Kind::unsignedValue,0},
        {"lfo2Speed","LFO 2","Speed",50,Kind::unsignedValue,0},
        {"lfo2SpeedSync","LFO 2","Speed Sync",51,Kind::unsignedValue,0},
        {"lfo2SpeedMode","LFO 2","Speed Mode",52,Kind::boolean,0},
        {"lfo2KeySync","LFO 2","Key Sync",52,Kind::boolean,1},
        {"modAftertouchFilterFreq","Aftertouch","Filter Freq",53,Kind::signed7,0},
        {"modAftertouchLfo1OscFreq","Aftertouch","Lfo1 Osc Freq",54,Kind::signed7,0},
        {"modAftertouchLfo2Speed","Aftertouch","Lfo2 Speed",55,Kind::signed7,0},
        {"modLfo1Osc1Pitch","LFO 1 routes","Osc1 Pitch",56,Kind::signed8,0},
        {"modLfo1Osc2Pitch","LFO 1 routes","Osc2 Pitch",57,Kind::signed8,0},
        {"modLfo1OscSubPitch","LFO 1 routes","Osc Sub Pitch",58,Kind::signed8,0},
        {"modLfo2Osc1PW","LFO 2 routes","Osc1 PW",59,Kind::signed7,0},
        {"modLfo2Osc2PW","LFO 2 routes","Osc2 PW",60,Kind::signed7,0},
        {"modLfo2OscSubPW","LFO 2 routes","Osc Sub PW",61,Kind::signed7,0},
        {"modLfo2FilterFreq","LFO 2 routes","Filter Freq",62,Kind::signed8,0},
        {"modEnv2Osc1Pitch","Mod Env routes","Osc1 Pitch",63,Kind::signed7,0},
        {"modEnv2Osc2Pitch","Mod Env routes","Osc2 Pitch",64,Kind::signed7,0},
        {"modEnv2OscSubPitch","Mod Env routes","Osc Sub Pitch",65,Kind::signed7,0},
        {"modEnv2Osc1PW","Mod Env routes","Osc1 PW",66,Kind::signed7,0},
        {"modEnv2Osc2PW","Mod Env routes","Osc2 PW",67,Kind::signed7,0},
        {"modEnv2OscSubPW","Mod Env routes","Osc Sub PW",68,Kind::signed7,0},
        {"modEnv2FilterFreq","Mod Env routes","Filter Freq",69,Kind::signed7,0},
        {"modOsc2FilterFreq","Effects","Osc filter modulation",70,Kind::unsignedValue,0},
        {"distortAmount","Effects","Amount",71,Kind::unsignedValue,0},
        {"compressAmount","Effects","VCA limit",72,Kind::unsignedValue,0},
        {"envAmpRetrigger","Amp Env","Retrigger",73,Kind::unsignedValue,0},
        {"envModRetrigger","Mod Env","Retrigger",74,Kind::unsignedValue,0},
        {"pitch","Tuning / Output","Fixed pitch",75,Kind::fixedPitch,0},
        {"outputLevel","Tuning / Output","Output level",76,Kind::output,0},
        {"envAmpTimedSustain","Amp Env","Timed Sustain",77,Kind::boolean,0},
        {"envModTimedSustain","Mod Env","Timed Sustain",78,Kind::boolean,0},
        {"envAmpRetriggerCount","Amp Env","Retrigger Count",79,Kind::unsignedValue,0},
        {"envModRetriggerCount","Mod Env","Retrigger Count",80,Kind::unsignedValue,0},
};
inline std::optional<int> value(const std::array<juce::uint8,84>& bytes,const Field& field)
{
    if(!juce::isPositiveAndBelow(field.offset,84))return {};
    const int raw=bytes[static_cast<size_t>(field.offset)];
    switch(field.kind)
    {
        case Kind::boolean:return (raw>>field.bitOrMask)&1;
        case Kind::mask:return raw&field.bitOrMask;
        case Kind::signed7:return raw-64;
        case Kind::signed8:return raw-128;
        case Kind::fixedPitch:case Kind::output:return raw>=128?std::optional<int>(raw&127):std::nullopt;
        case Kind::unsignedValue:return raw;
    }
    return {};
}
inline juce::String display(const std::array<juce::uint8,84>& bytes,const Field& field)
{
    const auto decoded=value(bytes,field);
    if(!decoded)return "Unspecified (stored byte "+juce::String(bytes[static_cast<size_t>(field.offset)])+")";
    if(field.kind==Kind::boolean)return *decoded?"On":"Off";
    if(field.kind==Kind::fixedPitch)
        return *decoded==0?"Normal key pitch":"MIDI note "+juce::String(*decoded);
    return juce::String(*decoded);
}
}
