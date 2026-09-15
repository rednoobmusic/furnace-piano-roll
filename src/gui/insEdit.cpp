/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2026 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _USE_MATH_DEFINES
#include "gui.h"
#include "../ta-log.h"
#include "imgui_internal.h"
#include "../engine/macroInt.h"
#include "../engine/platform/sound/yam10.h"
// i don't know whether this is the right thing to do
#include "../engine/platform/sound/sid3.h"
#include "IconsFontAwesome4.h"
#include "furIcons.h"
#include "misc/cpp/imgui_stdlib.h"
#include "guiConst.h"
#include "intConst.h"
#include <fmt/printf.h>
#include <imgui.h>
#include "plot_nolerp.h"
#include "util.h"

extern "C" {
#include "../../extern/Nuked-OPLL/opll.h"
}

const char* ssgEnvTypes[8]={
  _N("Down Down Down"),
  _N("Down."),
  _N("Down Up Down Up"),
  _N("Down UP"),
  _N("Up Up Up"),
  _N("Up."),
  _N("Up Down Up Down"),
  _N("Up DOWN")
};

const char* fmParamNames[3][35]={
  {_N("Algorithm"), _N("Feedback"), _N("LFO > Freq"), _N("LFO > Amp"), _N("Attack"), _N("Decay"), _N("Decay 2"), _N("Release"), _N("Sustain"), _N("Level"), _N("EnvScale"), _N("Multiplier"), _N("Detune"), _N("Detune 2"), _N("SSG-EG"), _N("AM"), _N("AM Depth"), _N("Vibrato Depth"), _N("Sustained"), _N("Sustained"), _N("Level Scaling"), _N("Sustain"), _N("Vibrato"), _N("Waveform"), _N("Scale Rate"), _N("OP2 Half Sine"), _N("OP1 Half Sine"), _N("EnvShift"), _N("Reverb"), _N("Fine"), _N("LFO2 > Freq"), _N("LFO2 > Amp"), _N("Octave"), _N("TL Ramp"), _N("Tremolo Sensitivity")},
  {"ALG", "FB", "FMS/PMS", "AMS", "AR", "DR", "SR", "RR", "SL", "TL", "KS", "MULT", "DT", "DT2", "SSG-EG", "AM", "AMD", "FMD", "EGT", "EGT", "KSL", "SUS", "VIB", "WS", "KSR", "DC", "DM", "EGS", "REV", "Fine", "FMS/PMS2", "AMS2", "Block", "TL Ramp", "Tremolo Sensitivity"},
  {"ALG", "FB", "FMS/PMS", "AMS", "AR", "DR", "D2R", "RR", "SL", "TL", "RS", "MULT", "DT", "DT2", "SSG-EG", "AM", "DAM", "DVB", "EGT", "EGS", "KSL", "SUS", "VIB", "WS", "KSR", "DC", "DM", "EGS", "REV", "Fine", "FMS/PMS2", "AMS2", "Block", "TL Ramp", "Tremolo Sensitivity"}
};

const char* esfmParamLongNames[9]={
  _N("OP4 Noise Mode"),
  _N("Envelope Delay"),
  _N("Output Level"),
  _N("Modulation Input Level"),
  _N("Left Output"),
  _N("Right Output"),
  _N("Coarse Tune (semitones)"),
  _N("Detune"),
  _N("Fixed Frequency Mode")
};

const char* esfmParamNames[9]={
  _N("OP4 Noise Mode"),
  _N("Env. Delay"),
  _N("Output Level"),
  _N("ModInput"),
  _N("Left"),
  _N("Right"),
  _N("Tune"),
  _N("Detune"),
  _N("Fixed")
};

const char* esfmParamShortNames[9]={
  "NOI", "DL", "OL", "MI", "L", "R", "CT", "DT", "FIX"
};

const char* fmParamShortNames[3][35]={
  {"ALG", "FB", "FMS", "AMS", "A", "D", "D2", "R", "S", "TL", "RS", "ML", "DT", "DT2", "SSG", "AM", "DAM", "DVB", "SUS", "SUS", "KSL", "SUS", "VIB", "WS", "KSR", "DC", "DM", "EGS", "REV", "Fine", "FMS2", "AMS2", "Blk", "TLR", "TS"},
  {"ALG", "FB", "FMS", "AMS", "A", "D", "SR", "R", "S", "TL", "KS", "ML", "DT", "DT2", "SSG", "AM", "AMD", "FMD", "EGT", "EGT", "KSL", "SUS", "VIB", "WS", "KSR", "DC", "DM", "EGS", "REV", "Fine", "FMS2", "AMS2", "Blk", "TLR", "TS"},
  {"ALG", "FB", "FMS", "AMS", "A", "D", "D2", "R", "S", "TL", "RS", "ML", "DT", "DT2", "SSG", "AM", "DAM", "DVB", "EGT", "EGS", "KSL", "SUS", "VIB", "WS", "KSR", "DC", "DM", "EGS", "REV", "Fine", "FMS2", "AMS2", "Blk", "TLR", "TS"}
};

const char* opllVariants[4]={
  "OPLL",
  "YMF281",
  "YM2423",
  "VRC7"
};

const char* opllInsNames[4][17]={
  /* YM2413 */ {
    _N("User"),
    _N("1. Violin"),
    _N("2. Guitar"),
    _N("3. Piano"),
    _N("4. Flute"),
    _N("5. Clarinet"),
    _N("6. Oboe"),
    _N("7. Trumpet"),
    _N("8. Organ"),
    _N("9. Horn"),
    _N("10. Synth"),
    _N("11. Harpsichord"),
    _N("12. Vibraphone"),
    _N("13. Synth Bass"),
    _N("14. Acoustic Bass"),
    _N("15. Electric Guitar"),
    _N("Drums")
  },
  /* YMF281 */ {
    _N("User"),
    _N("1. Electric String"),
    _N("2. Bow wow"),
    _N("3. Electric Guitar"),
    _N("4. Organ"),
    _N("5. Clarinet"),
    _N("6. Saxophone"),
    _N("7. Trumpet"),
    _N("8. Street Organ"),
    _N("9. Synth Brass"),
    _N("10. Electric Piano"),
    _N("11. Bass"),
    _N("12. Vibraphone"),
    _N("13. Chime"),
    _N("14. Tom Tom II"),
    _N("15. Noise"),
    _N("Drums")
  },
  /* YM2423 */ {
    _N("User"),
    _N("1. Strings"),
    _N("2. Guitar"),
    _N("3. Electric Guitar"),
    _N("4. Electric Piano"),
    _N("5. Flute"),
    _N("6. Marimba"),
    _N("7. Trumpet"),
    _N("8. Harmonica"),
    _N("9. Tuba"),
    _N("10. Synth Brass"),
    _N("11. Short Saw"),
    _N("12. Vibraphone"),
    _N("13. Electric Guitar 2"),
    _N("14. Synth Bass"),
    _N("15. Sitar"),
    _N("Drums")
  },
  // stolen from FamiTracker
  /* VRC7 */ {
    _N("User"),
    _N("1. Bell"),
    _N("2. Guitar"),
    _N("3. Piano"),
    _N("4. Flute"),
    _N("5. Clarinet"),
    _N("6. Rattling Bell"),
    _N("7. Trumpet"),
    _N("8. Reed Organ"),
    _N("9. Soft Bell"),
    _N("10. Xylophone"),
    _N("11. Vibraphone"),
    _N("12. Brass"),
    _N("13. Bass Guitar"),
    _N("14. Synth"),
    _N("15. Chorus"),
    _N("Drums")
  }
};

const char* oplWaveforms[8]={
  _N("Sine"),
  _N("Half Sine"),
  _N("Absolute Sine"),
  _N("Quarter Sine"),
  _N("Squished Sine"),
  _N("Squished AbsSine"),
  _N("Square"),
  _N("Derived Square")
};

const char* oplWaveformsStandard[8]={
  _N("Sine"),
  _N("Half Sine"),
  _N("Absolute Sine"),
  _N("Pulse Sine"),
  _N("Sine (Even Periods)"),
  _N("AbsSine (Even Periods)"),
  _N("Square"),
  _N("Derived Square")
};

const char* opzWaveforms[8]={
  _N("Sine"),
  _N("Triangle"),
  _N("Cut Sine"),
  _N("Half Triangle"),
  _N("Squished Sine"),
  _N("Squished Triangle"),
  _N("Squished AbsSine"),
  _N("Squished AbsTriangle")
};

const char* oplDrumNames[4]={
  _N("Snare"),
  _N("Tom"),
  _N("Top"),
  _N("HiHat")
};

const char* esfmNoiseModeNames[4]={
  _N("Normal"),
  _N("Snare"),
  _N("HiHat"),
  _N("Top")
};

const char* esfmNoiseModeDescriptions[4]={
  _N("Noise disabled"),
  _N("Square + noise"),
  _N("Ringmod from OP3 + noise"),
  _N("Ringmod from OP3 + double pitch ModInput\nWARNING - has emulation issues; subject to change")
};

const char* sid2WaveMixModes[5]={
  _N("8580 SID"),
  _N("Bitwise AND"),
  _N("Bitwise OR"),
  _N("Bitwise XOR"),
  NULL
};

const char* sid2ControlBits[4]={
  _N("gate"),
  _N("sync"),
  _N("ring"),
  NULL
};

const char* sid3ControlBits[4]={
  _N("phase"),
  _N("sync"),
  _N("ring"),
  NULL
};

const char* sid3WaveMixModes[6]={
  _N("8580 SID"),
  _N("Bitwise AND"),
  _N("Bitwise OR"),
  _N("Bitwise XOR"),
  _N("Sum of the signals"),
  NULL
};

const char* sid3SpecialWaveforms[]={
  _N("Sine"),
  _N("Rect. Sine"),
  _N("Abs. Sine"),
  _N("Quart. Sine"),
  _N("Squish. Sine"),
  _N("Abs. Squish. Sine"),

  _N("Rect. Saw"),
  _N("Abs. Saw"),

  _N("Cubed Saw"),
  _N("Rect. Cubed Saw"),
  _N("Abs. Cubed Saw"),

  _N("Cubed Sine"),
  _N("Rect. Cubed Sine"),
  _N("Abs. Cubed Sine"),
  _N("Quart. Cubed Sine"),
  _N("Squish. Cubed Sine"),
  _N("Squish. Abs. Cub. Sine"),

  _N("Rect. Triangle"),
  _N("Abs. Triangle"),
  _N("Quart. Triangle"),
  _N("Squish. Triangle"),
  _N("Abs. Squish. Triangle"),

  _N("Cubed Triangle"),
  _N("Rect. Cubed Triangle"),
  _N("Abs. Cubed Triangle"),
  _N("Quart. Cubed Triangle"),
  _N("Squish. Cubed Triangle"),
  _N("Squish. Abs. Cub. Triangle"),

  // clipped

  _N("Clipped Sine"),
  _N("Clipped Rect. Sine"),
  _N("Clipped Abs. Sine"),
  _N("Clipped Quart. Sine"),
  _N("Clipped Squish. Sine"),
  _N("Clipped Abs. Squish. Sine"),

  _N("Clipped Rect. Saw"),
  _N("Clipped Abs. Saw"),

  _N("Clipped Cubed Saw"),
  _N("Clipped Rect. Cubed Saw"),
  _N("Clipped Abs. Cubed Saw"),

  _N("Clipped Cubed Sine"),
  _N("Clipped Rect. Cubed Sine"),
  _N("Clipped Abs. Cubed Sine"),
  _N("Clipped Quart. Cubed Sine"),
  _N("Clipped Squish. Cubed Sine"),
  _N("Clipped Squish. Abs. Cub. Sine"),

  _N("Clipped Rect. Triangle"),
  _N("Clipped Abs. Triangle"),
  _N("Clipped Quart. Triangle"),
  _N("Clipped Squish. Triangle"),
  _N("Clipped Abs. Squish. Triangle"),

  _N("Clipped Cubed Triangle"),
  _N("Clipped Rect. Cubed Triangle"),
  _N("Clipped Abs. Cubed Triangle"),
  _N("Clipped Quart. Cubed Triangle"),
  _N("Clipped Squish. Cubed Triangle"),
  _N("Clipped Squish. Abs. Cub. Triangle"),

  // two clipped simple waves

  _N("Clipped Triangle"),
  _N("Clipped Saw")
};

const bool opIsOutput[8][4]={
  {false,false,false,true},
  {false,false,false,true},
  {false,false,false,true},
  {false,false,false,true},
  {false,true,false,true},
  {false,true,true,true},
  {false,true,true,true},
  {true,true,true,true}
};

const bool opIsOutputOPL[4][4]={
  {false,false,false,true},
  {true,false,false,true},
  {false,true,false,true},
  {true,false,true,true}
};

enum FMParams {
  FM_ALG=0,
  FM_FB=1,
  FM_FMS=2,
  FM_AMS=3,
  FM_AR=4,
  FM_DR=5,
  FM_D2R=6,
  FM_RR=7,
  FM_SL=8,
  FM_TL=9,
  FM_RS=10,
  FM_MULT=11,
  FM_DT=12,
  FM_DT2=13,
  FM_SSG=14,
  FM_AM=15,
  FM_DAM=16,
  FM_DVB=17,
  FM_EGT=18,
  FM_EGS=19,
  FM_KSL=20,
  FM_SUS=21,
  FM_VIB=22,
  FM_WS=23,
  FM_KSR=24,
  FM_DC=25,
  FM_DM=26,
  FM_EGSHIFT=27,
  FM_REV=28,
  FM_FINE=29,
  FM_FMS2=30,
  FM_AMS2=31,
  FM_BLOCK=32,
  FM_TLRAMP=33,
  FM_TS=34
};

enum ESFMParams {
  ESFM_NOISE=0,
  ESFM_DELAY=1,
  ESFM_OUTLVL=2,
  ESFM_MODIN=3,
  ESFM_LEFT=4,
  ESFM_RIGHT=5,
  ESFM_CT=6,
  ESFM_DT=7,
  ESFM_FIXED=8
};

#define FM_NAME(x) _(fmParamNames[settings.fmNames][x])
#define FM_SHORT_NAME(x) fmParamShortNames[settings.fmNames][x]
#define ESFM_LONG_NAME(x) _(esfmParamLongNames[x])
#define ESFM_NAME(x) _(esfmParamNames[x])
#define ESFM_SHORT_NAME(x) (esfmParamShortNames[x])

const char* macroTypeLabels[4]={
  ICON_FA_BAR_CHART "##IMacroType",
  ICON_FUR_ADSR "##IMacroType",
  ICON_FUR_TRI "##IMacroType",
  ICON_FA_SIGN_OUT "##IMacroType"
};

const char* macroLFOShapes[4]={
  _N("Triangle"),
  _N("Saw"),
  _N("Square"),
  _N("How did you even")
};

const char* fmOperatorBits[5]={
  "op1", "op2", "op3", "op4", NULL
};

const char* yam10OperatorBits[7]={
  "op1", "op2", "op3", "op4", "op5", "op6", NULL
};

const char* c64ShapeBits[5]={
  _N("triangle"),
  _N("saw"),
  _N("pulse"),
  _N("noise"),
  NULL
};

const char* ayShapeBits[4]={
  _N("tone"),
  _N("noise"),
  _N("envelope"),
  NULL
};

const char* sid3ShapeBits[6]={
  _N("triangle"),
  _N("saw"),
  _N("pulse"),
  _N("noise"),
  _N("special wave"),
  NULL
};

const char* sid3FilterMatrixBits[5]={
  _N("From filter 1"),
  _N("From filter 2"),
  _N("From filter 3"),
  _N("From filter 4"),
  NULL
};

const char* ayEnvBits[4]={
  _N("hold"),
  _N("alternate"),
  _N("direction"),
  _N("enable")
};

const char* ssgEnvBits[5]={
  "0", "1", "2", _N("enabled"), NULL
};

const char* saaEnvBits[9]={
  _N("mirror"),
  _N("loop"),
  _N("cut"),
  _N("direction"),
  _N("resolution"),
  _N("fixed"),
  _N("N/A"),
  _N("enabled"),
  NULL
};

const char* snesModeBits[6]={
  _N("noise"),
  _N("echo"),
  _N("pitch mod"),
  _N("invert right"),
  _N("invert left"),
  NULL
};

const char* filtModeBits[5]={
  _N("low"),
  _N("band"),
  _N("high"),
  _N("ch3off"),
  NULL
};

const char* c64TestGateBits[5]={
  _N("gate"),
  _N("sync"),
  _N("ring"),
  _N("test"),
  NULL
};

const char* pokeyCtlBits[9]={
  _N("15KHz"),
  _N("filter 2+4"),
  _N("filter 1+3"),
  _N("16-bit 3+4"),
  _N("16-bit 1+2"),
  _N("high3"),
  _N("high1"),
  _N("poly9"),
  NULL
};

const char* mikeyFeedbackBits[11]={
  "0", "1", "2", "3", "4", "5", "7", "10", "11", "int", NULL
};

const char* msm5232ControlBits[7]={
  _N("16'"),
  _N("8'"),
  _N("4'"),
  _N("2'"),
  _N("sustain"),
  NULL
};

const char* tedControlBits[3]={
  _N("square"),
  _N("noise"),
  NULL
};

const char* c219ControlBits[4]={
  _N("noise"),
  _N("invert"),
  _N("surround"),
  NULL
};

const char* x1_010EnvBits[8]={
  _N("enable"),
  _N("oneshot"),
  _N("split L/R"),
  _N("HinvR"),
  _N("VinvR"),
  _N("HinvL"),
  _N("VinvL"),
  NULL
};

const char* suControlBits[5]={
  _N("ring mod"),
  _N("low pass"),
  _N("high pass"),
  _N("band pass"),
  NULL
};

const char* es5506FilterModes[4]={
  "HP/K2, HP/K2", "HP/K2, LP/K1", "LP/K2, LP/K2", "LP/K2, LP/K1",
};

const char* powerNoiseControlBits[3]={
  _N("enable tap B"),
  _N("AM with slope"),
  NULL
};

const char* powerNoiseSlopeControlBits[7]={
  _N("invert B"),
  _N("invert A"),
  _N("reset B"),
  _N("reset A"),
  _N("clip B"),
  _N("clip A"),
  NULL
};

const char* daveControlBits[5]={
  _N("high pass"),
  _N("ring mod"),
  _N("swap counters (noise)"),
  _N("low pass (noise)"),
  NULL
};

const char* panBits[5]={
  _N("right"),
  _N("left"),
  _N("rear right"),
  _N("rear left"),
  NULL
};

const char* oneBit[2]={
  _N("on"),
  NULL
};

const char* es5506EnvelopeModes[3]={
  _N("k1 slowdown"),
  _N("k2 slowdown"),
  NULL
};

const char* es5506ControlModes[3]={
  _N("pause"),
  _N("reverse"),
  NULL
};

const char* minModModeBits[3]={
  _N("invert right"),
  _N("invert left"),
  NULL
};

const int orderedOps[4]={
  0, 2, 1, 3
};

const char* singleWSEffects[7]={
  _N("None"),
  _N("Invert"),
  _N("Add"),
  _N("Subtract"),
  _N("Average"),
  _N("Phase"),
  _N("Chorus")
};

const char* dualWSEffects[9]={
  _N("None (dual)"),
  _N("Wipe"),
  _N("Fade"),
  _N("Fade (ping-pong)"),
  _N("Overlay"),
  _N("Negative Overlay"),
  _N("Slide"),
  _N("Mix Chorus"),
  _N("Phase Modulation")
};

const char* gbHWSeqCmdTypes[6]={
  _N("Envelope"),
  _N("Sweep"),
  _N("Wait"),
  _N("Wait for Release"),
  _N("Loop"),
  _N("Loop until Release")
};

const char* suHWSeqCmdTypes[7]={
  _N("Volume Sweep"),
  _N("Frequency Sweep"),
  _N("Cutoff Sweep"),
  _N("Wait"),
  _N("Wait for Release"),
  _N("Loop"),
  _N("Loop until Release")
};

const char* snesGainModes[5]={
  _N("Direct"),
  _N("Decrease (linear)"),
  _N("Decrease (logarithmic)"),
  _N("Increase (linear)"),
  _N("Increase (bent line)")
};

const int detuneMap[2][8]={
  {-3, -2, -1, 0, 1, 2, 3, 4},
  { 7,  6,  5, 0, 1, 2, 3, 4}
};

const int detuneUnmap[2][11]={
  {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0},
  {0, 0, 0, 3, 4, 5, 6, 7, 2, 1, 0}
};

const int kslMap[4]={
  0, 2, 1, 3
};

const int _SID3_SPECIAL_WAVES=SID3_NUM_SPECIAL_WAVES-1;
const int _SID3_NUM_CHANNELS=SID3_NUM_CHANNELS;
const int _SID3_NUM_CHANNELS_MINUS_ONE=SID3_NUM_CHANNELS-1;

// do not change these!
// anything other than a checkbox will look ugly!
//
// if you really need to, and have a good rationale (and by good I mean a VERY
// good one), please tell me and we'll sort it out.
const char* macroAbsoluteMode="Fixed";
const char* macroRelativeMode="Relative";
const char* macroQSoundMode="QSound";
const char* macroDummyMode="Bug";

String macroHoverNote(int id, float val, void* u) {
  int* macroVal=(int*)u;
  if ((macroVal[id]&0xc0000000)==0x40000000 || (macroVal[id]&0xc0000000)==0x80000000) {
    if (val<-60 || val>=120) return "???";
    return fmt::sprintf("%d: %s",id,noteNames[(int)val+60]);
  }
  return fmt::sprintf("%d: %d",id,(int)val);
}

String macroHover(int id, float val, void* u) {
  return fmt::sprintf("%d: %d",id,(int)val);
}

String macroHoverLoop(int id, float val, void* u) {
  if (val>1) return _("Release");
  if (val>0) return _("Loop");
  return "";
}

String macroHoverBit30(int id, float val, void* u) {
  if (val>0) return _("Fixed");
  return _("Relative");
}

String macroHoverGain(int id, float val, void* u) {
  if (val>=224.0f) {
    return fmt::sprintf(_("%d: +%d (exponential)"),id,(int)(val-224));
  }
  if (val>=192.0f) {
    return fmt::sprintf(_("%d: +%d (linear)"),id,(int)(val-192));
  }
  if (val>=160.0f) {
    return fmt::sprintf(_("%d: -%d (exponential)"),id,(int)(val-160));
  }
  if (val>=128.0f) {
    return fmt::sprintf(_("%d: -%d (linear)"),id,(int)(val-128));
  }
  return fmt::sprintf(_("%d: %d (direct)"),id,(int)val);
}

String macroHoverES5506FilterMode(int id, float val, void* u) {
  String mode="???";
  switch (((int)val)&3) {
    case 0:
      mode=_("HP/K2, HP/K2");
      break;
    case 1:
      mode=_("HP/K2, LP/K1");
      break;
    case 2:
      mode=_("LP/K2, LP/K2");
      break;
    case 3:
      mode=_("LP/K2, LP/K1");
      break;
    default:
      break;
  }
  return fmt::sprintf("%d: %s",id,mode);
}

String macroLFOWaves(int id, float val, void* u) {
  const char* label="???";
  switch (((int)val)&3) {
    case 0:
      label=_("Saw");
      break;
    case 1:
      label=_("Square");
      break;
    case 2:
      label=_("Triangle");
      break;
    case 3:
      label=_("Random");
      break;
    default: break;
  }
  return fmt::sprintf("%d: %s",id,label);
}

String macroVERAWaves(int id, float val, void* u) {
  const char* label="???";
  switch (((int)val)&3) {
    case 0:
      label=_("Pulse");
      break;
    case 1:
      label=_("Saw");
      break;
    case 2:
      label=_("Triangle");
      break;
    case 3:
      label=_("Noise");
      break;
    default: break;
  }
  return fmt::sprintf("%d: %s",id,label);
}

String macroSoundUnitWaves(int id, float val, void* u) {
  const char* label="???";
  switch (((int)val)&7) {
    case 0:
      label=_("Square");
      break;
    case 1:
      label=_("Saw");
      break;
    case 2:
      label=_("Sine");
      break;
    case 3:
      label=_("Triangle");
      break;
    case 4:
      label=_("Noise");
      break;
    case 5:
      label=_("Short Noise");
      break;
    case 6:
      label=_("XOR Sine");
      break;
    case 7:
      label=_("XOR Triangle");
      break;
    default: break;
  }
  return fmt::sprintf("%d: %s",id,label);
}

String macroTFXModes(int id, float val, void* u) {
  switch (((int)val)&3) {
    case 0:
      return _("Disabled");
    case 1:
      return _("PWM");
    case 2:
      return _("SyncBuzzer");
    case 3:
      return _("Reserved");
    default:
      return "???";
  }
  return "???";
}

String macroSID3SpecialWaves(int id, float val, void* u) {
  if ((int)val<0 || (int)val>=SID3_NUM_SPECIAL_WAVES) return "???";

  return fmt::sprintf("%d: %s",id,_(sid3SpecialWaveforms[(int)val%SID3_NUM_SPECIAL_WAVES]));
}

String macroSID3SourceChan(int id, float val, void* u) {
  if ((int)val>SID3_NUM_CHANNELS) return "???";

  if ((int)val==SID3_NUM_CHANNELS) {
    return _("Self");
  } else if ((int)val==SID3_NUM_CHANNELS-1) {
    return _("PCM/Wave channel");
  } else {
    return fmt::sprintf(_("Channel %d"),(int)val+1);
  }
}

String macroSID3NoiseLFSR(int id, float val, void* u) {
  return _(
    "values close to SID2 noise modes:\n\n"
    "Mode 1: 524288\n"
    "Mode 2: 66\n"
    "Mode 3: 541065280"
  );
}

String macroSID2WaveMixMode(int id, float val, void* u) {
  if ((int)val<0 || (int)val>3) return "???";

  return fmt::sprintf("%d: %s",id,_(sid2WaveMixModes[(int)val]));
}

String macroSID3WaveMixMode(int id, float val, void* u) {
  if ((int)val<0 || (int)val>4) return "???";

  return fmt::sprintf("%d: %s",id,_(sid3WaveMixModes[(int)val]));
}

void addAALine(ImDrawList* dl, const ImVec2& p1, const ImVec2& p2, const ImU32 color, float thickness=1.0f) {
  ImVec2 pt[2];
  pt[0]=p1;
  pt[1]=p2;
  dl->AddPolyline(pt,2,color,thickness,ImDrawFlags_None);
}

void FurnaceGUI::drawSSGEnv(unsigned char type, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_SSG]);
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("ssgEnvDisplay"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);
    switch (type) {
      case 0:
        for (int i=0; i<4; i++) {
          ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2((float)i/4.0f,0.2));
          ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2((float)(i+1)/4.0f,0.8));
          addAALine(dl,pos1,pos2,color);
          pos1.x=pos2.x;
          if (i<3) addAALine(dl,pos1,pos2,color);
        }
        break;
      case 1: {
        ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,0.2));
        ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.8));
        addAALine(dl,pos1,pos2,color);

        pos1=ImLerp(rect.Min,rect.Max,ImVec2(1.0,0.8));
        addAALine(dl,pos1,pos2,color);
        break;
      }
      case 2: {
        ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,0.2));
        ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.8));
        addAALine(dl,pos1,pos2,color);

        pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.2));
        addAALine(dl,pos1,pos2,color);

        pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.8));
        addAALine(dl,pos1,pos2,color);

        pos1=ImLerp(rect.Min,rect.Max,ImVec2(1.0,0.2));
        addAALine(dl,pos1,pos2,color);
        break;
      }
      case 3: {
        ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,0.2));
        ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.8));
        addAALine(dl,pos1,pos2,color);

        pos1.x=pos2.x;
        addAALine(dl,pos1,pos2,color);

        pos2=ImLerp(rect.Min,rect.Max,ImVec2(1.0,0.2));
        addAALine(dl,pos1,pos2,color);
        break;
      }
      case 4:
        for (int i=0; i<4; i++) {
          ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2((float)i/4.0f,0.8));
          ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2((float)(i+1)/4.0f,0.2));
          addAALine(dl,pos1,pos2,color);
          pos1.x=pos2.x;
          if (i<3) addAALine(dl,pos1,pos2,color);
        }
        break;
      case 5: {
        ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,0.8));
        ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.2));
        addAALine(dl,pos1,pos2,color);

        pos1=ImLerp(rect.Min,rect.Max,ImVec2(1.0,0.2));
        addAALine(dl,pos1,pos2,color);
        break;
      }
      case 6: {
        ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,0.8));
        ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.2));
        addAALine(dl,pos1,pos2,color);

        pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.8));
        addAALine(dl,pos1,pos2,color);

        pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.2));
        addAALine(dl,pos1,pos2,color);

        pos1=ImLerp(rect.Min,rect.Max,ImVec2(1.0,0.8));
        addAALine(dl,pos1,pos2,color);
        break;
      }
      case 7: {
        ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,0.8));
        ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.2));
        addAALine(dl,pos1,pos2,color);

        pos1.x=pos2.x;
        addAALine(dl,pos1,pos2,color);

        pos2=ImLerp(rect.Min,rect.Max,ImVec2(1.0,0.8));
        addAALine(dl,pos1,pos2,color);
        break;
      }
    }
  }
}

void FurnaceGUI::drawWaveform(unsigned char type, bool opz, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 waveform[65];
  const size_t waveformLen=64;

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_WAVE]);
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("wsDisplay"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);
    if (opz) {
      switch (type) {
        case 0:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=sin(x*2.0*M_PI);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 1:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=pow(sin(x*2.0*M_PI),2.0);
            if (x>=0.5) y=-y;
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 2:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=MAX(0.0,sin(x*2.0*M_PI));
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 3:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=pow(MAX(0.0,sin(x*2.0*M_PI)),2.0);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 4:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?0.0:sin(x*4.0*M_PI);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 5:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?0.0:pow(sin(x*4.0*M_PI),2.0);
            if (x>=0.25) y=-y;
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 6:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?0.0:fabs(sin(x*4.0*M_PI));
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 7:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?0.0:pow(sin(x*4.0*M_PI),2.0);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
      }
    } else {
      switch (type) {
        case 0:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=sin(x*2.0*M_PI);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 1:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=MAX(0.0,sin(x*2.0*M_PI));
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 2:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=fabs(sin(x*2.0*M_PI));
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 3:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=fabs((tan(x*2.0*M_PI)>=0.0)?sin(x*2.0*M_PI):0.0);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 4:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?0.0:sin(x*4.0*M_PI);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 5:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?0.0:fabs(sin(x*4.0*M_PI));
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 6:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=(x>=0.5)?-1.0:1.0;
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
        case 7:
          for (size_t i=0; i<=waveformLen; i++) {
            float x=(float)i/(float)waveformLen;
            float y=pow(2.0*(x-0.5),3.0);
            waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
          }
          break;
      }
    }
    dl->AddPolyline(waveform,waveformLen+1,color,dpiScale,ImDrawFlags_None);
  }
}

typedef double (*WaveFunc) (double a);

WaveFunc waveFuncsIns[]={
  sinus,
  rectSin,
  absSin,
  quartSin,
  squiSin,
  squiAbsSin,
  
  rectSaw,
  absSaw,
  
  cubSaw,
  rectCubSaw,
  absCubSaw,
  
  cubSine,
  rectCubSin,
  absCubSin,
  quartCubSin,
  squishCubSin,
  squishAbsCubSin,

  rectTri,
  absTri,
  quartTri,
  squiTri,
  absSquiTri,

  cubTriangle,
  cubRectTri,
  cubAbsTri,
  cubQuartTri,
  cubSquiTri,
  absCubSquiTri
};

void FurnaceGUI::drawWaveformSID3(unsigned char type, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 waveform[65];
  const size_t waveformLen=64;

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_WAVE]);
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("SID3wsDisplay"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);

    if (type<SID3_NUM_UNIQUE_SPECIAL_WAVES) {
      for (size_t i=0; i<=waveformLen; i++) {
        float x=(float)i/(float)waveformLen;
        float y=waveFuncsIns[type](x*2.0*M_PI);
        waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
      }
    } else if (type>=SID3_NUM_UNIQUE_SPECIAL_WAVES && type<SID3_NUM_UNIQUE_SPECIAL_WAVES*2) {
      for (size_t i=0; i<=waveformLen; i++) {
        float x=(float)i/(float)waveformLen;
        float y=waveFuncsIns[type-SID3_NUM_UNIQUE_SPECIAL_WAVES](x*2.0*M_PI);

        y*=2.0f; // clipping

        if (y>1.0f) y=1.0f;
        if (y<-1.0f) y=-1.0f;

        waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.48));
      }
    } else {
      if (type==SID3_NUM_UNIQUE_SPECIAL_WAVES*2) {
        for (size_t i=0; i<=waveformLen; i++) {
          float x=(float)i/(float)waveformLen;
          float y=triangle(x*2.0*M_PI);

          y*=2.0f; // clipping

          if (y>1.0f) y=1.0f;
          if (y<-1.0f) y=-1.0f;

          waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
        }
      }
      if (type==SID3_NUM_UNIQUE_SPECIAL_WAVES*2+1) {
        for (size_t i=0; i<=waveformLen; i++) {
          float x=(float)i/(float)waveformLen;
          float y=saw(x*2.0*M_PI);

          y*=2.0f; // clipping

          if (y>1.0f) y=1.0f;
          if (y<-1.0f) y=-1.0f;

          waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5-y*0.4));
        }
      }
    }

    dl->AddPolyline(waveform,waveformLen+1,color,dpiScale,ImDrawFlags_None);
  }
}

void FurnaceGUI::drawAlgorithm(unsigned char alg, FurnaceGUIFMAlgs algType, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 colorM=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_MOD]);
  ImU32 colorC=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_CAR]);
  ImU32 colorL=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_LINE]);
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("alg"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_BG]),true,style.FrameRounding);
    const float circleRadius=6.0f*dpiScale+1.0f;
    switch (algType) {
      case FM_ALGS_4OP:
        switch (alg) {
          case 0: { // 1 > 2 > 3 > 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.2,0.5));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.4,0.5));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.6,0.5));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.8,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos2,pos3,colorL);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);

            pos1.x-=ImGui::CalcTextSize("1").x*0.5;
            pos2.x-=ImGui::CalcTextSize("2").x*0.5;
            pos3.x-=ImGui::CalcTextSize("3").x*0.5;
            pos4.x-=ImGui::CalcTextSize("4").x*0.5;
            pos1.y-=ImGui::CalcTextSize("1").y+circleRadius;
            pos2.y-=ImGui::CalcTextSize("2").y+circleRadius;
            pos3.y-=ImGui::CalcTextSize("3").y+circleRadius;
            pos4.y-=ImGui::CalcTextSize("4").y+circleRadius;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 1: { // (1+2) > 3 > 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.3));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.7));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.5));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos3,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos2,pos3,colorL);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);

            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos1.x=pos2.x;
            pos3.x-=ImGui::CalcTextSize("3").x*0.5;
            pos4.x-=ImGui::CalcTextSize("4").x*0.5;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y+circleRadius;
            pos4.y-=ImGui::CalcTextSize("4").y+circleRadius;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 2: { // 1+(2>3) > 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.3));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.7));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.7));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos4,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos2,pos3,colorL);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x*0.5;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y+circleRadius;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 3: { // (1>2)+3 > 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.3));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.3));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.7));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos2,pos4,colorL);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x*0.5;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y+circleRadius;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 4: { // (1>2) + (3>4)
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.3));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.3));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.7));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.7));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos2,pos5,colorL);
            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y*0.5;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorC,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 5: { // 1 > (2+3+4)
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.5));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.25));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.5));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.75));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            addAALine(dl,pos1,pos3,colorL);
            addAALine(dl,pos1,pos4,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos2,pos5,colorL);
            addAALine(dl,pos3,pos5,colorL);
            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y*0.5;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorC,"2");
            dl->AddText(pos3,colorC,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 6: { // (1>2) + 3 + 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.25));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.25));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.5));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.75));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos2,pos5,colorL);
            addAALine(dl,pos3,pos5,colorL);
            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y*0.5;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorC,"2");
            dl->AddText(pos3,colorC,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 7: { // 1 + 2 + 3 + 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.2));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.35,0.4));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.45,0.6));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.55,0.8));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos1,pos5,colorL);
            addAALine(dl,pos2,pos5,colorL);
            addAALine(dl,pos3,pos5,colorL);
            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y*0.5;
            dl->AddText(pos1,colorC,"1");
            dl->AddText(pos2,colorC,"2");
            dl->AddText(pos3,colorC,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
        }
        break;
      case FM_ALGS_2OP_OPL:
        switch (alg) {
          case 0: { // 1 > 2
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.33,0.5));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.67,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x+=circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorC,"2");
            break;
          }
          case 1: { // 1 + 2
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.33,0.5));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.67,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x+=circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            dl->AddText(pos1,colorC,"1");
            dl->AddText(pos2,colorC,"2");
            break;
          }
        }
        break;
      case FM_ALGS_4OP_OPL:
        switch (alg) {
          case 0: { // 1 > 2 > 3 > 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.2,0.5));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.4,0.5));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.6,0.5));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.8,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos2,pos3,colorL);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);

            pos1.x-=ImGui::CalcTextSize("1").x*0.5;
            pos2.x-=ImGui::CalcTextSize("2").x*0.5;
            pos3.x-=ImGui::CalcTextSize("3").x*0.5;
            pos4.x-=ImGui::CalcTextSize("4").x*0.5;
            pos1.y-=ImGui::CalcTextSize("1").y+circleRadius;
            pos2.y-=ImGui::CalcTextSize("2").y+circleRadius;
            pos3.y-=ImGui::CalcTextSize("3").y+circleRadius;
            pos4.y-=ImGui::CalcTextSize("4").y+circleRadius;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 1: { // 1 + (2 > 3 > 4)
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.4,0.3));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.2,0.7));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.4,0.7));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.6,0.7));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.8,0.7));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos1,pos5,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos2,pos3,colorL);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);

            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x*0.5;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y+circleRadius;
            dl->AddText(pos1,colorC,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 2: { // (1>2) + (3>4)
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.3));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.3));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.7));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.7));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos1,pos2,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorM);
            addAALine(dl,pos3,pos4,colorL);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos2,pos5,colorL);
            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y*0.5;
            dl->AddText(pos1,colorM,"1");
            dl->AddText(pos2,colorC,"2");
            dl->AddText(pos3,colorM,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
          case 3: { // 1 + (2 > 3) + 4
            ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.25));
            ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(0.25,0.5));
            ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.5));
            ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(0.5,0.75));
            ImVec2 pos5=ImLerp(rect.Min,rect.Max,ImVec2(0.75,0.5));
            dl->AddCircleFilled(pos1,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircle(pos1,6.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos2,pos3,colorL);
            dl->AddCircleFilled(pos2,4.0f*dpiScale+1.0f,colorM);
            dl->AddCircleFilled(pos3,4.0f*dpiScale+1.0f,colorC);
            dl->AddCircleFilled(pos4,4.0f*dpiScale+1.0f,colorC);
            addAALine(dl,pos1,pos5,colorL);
            addAALine(dl,pos3,pos5,colorL);
            addAALine(dl,pos4,pos5,colorL);

            pos1.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0*dpiScale;
            pos3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0*dpiScale;
            pos4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0*dpiScale;
            pos1.y-=ImGui::CalcTextSize("1").y*0.5;
            pos2.y-=ImGui::CalcTextSize("2").y*0.5;
            pos3.y-=ImGui::CalcTextSize("3").y*0.5;
            pos4.y-=ImGui::CalcTextSize("4").y*0.5;
            dl->AddText(pos1,colorC,"1");
            dl->AddText(pos2,colorM,"2");
            dl->AddText(pos3,colorC,"3");
            dl->AddText(pos4,colorC,"4");
            break;
          }
        }
        break;
      default:
        break;
    }
  }
}

void FurnaceGUI::drawESFMAlgorithm(DivInstrumentESFM& esfm, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 colorM=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_MOD]);
  ImU32 colorC=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_CAR]);
  ImU32 colorsL[8];
  for (int i=0; i<8; i++){
    float alpha=(float)i/7.0f;
    ImVec4 color=uiColors[GUI_COLOR_FM_ALG_LINE];
    color.w *= alpha;
    colorsL[i]=ImGui::GetColorU32(color);
  }
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("alg"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_BG]),true,style.FrameRounding);
    const float circleRadius=6.0f*dpiScale+1.0f;
    //int modFb = esfm.op[0].modIn&7;
    int mod12 = esfm.op[1].modIn&7;
    int mod23 = esfm.op[2].modIn&7;
    int mod34 = esfm.op[3].modIn&7;
    int out1 = esfm.op[0].outLvl&7;
    int out2 = esfm.op[1].outLvl&7;
    int out3 = esfm.op[2].outLvl&7;
    int out4 = esfm.op[3].outLvl&7;
    bool isMod[4];
    for (int i=0; i<4; i++) {
      DivInstrumentESFM::Operator& opE=esfm.op[i];
      isMod[i]=true;
      if (opE.outLvl==7) isMod[i]=false;
      else if (opE.outLvl>0) {
        if (i==3) isMod[i]=false;
        else {
          DivInstrumentESFM::Operator& opENext=esfm.op[i+1];
          if (opENext.modIn==0) isMod[i]=false;
          else if ((opE.outLvl-opENext.modIn)>=2) isMod[i]=false;
        }
      }
    }

    ImVec2 posOp1=ImLerp(rect.Min,rect.Max,ImVec2(0.2f,0.25f));
    ImVec2 posOp2=ImLerp(rect.Min,rect.Max,ImVec2(0.4f,0.25f));
    ImVec2 posOp3=ImLerp(rect.Min,rect.Max,ImVec2(0.6f,0.25f));
    ImVec2 posOp4=ImLerp(rect.Min,rect.Max,ImVec2(0.8f,0.25f));
    ImVec2 posBusbarOp1=ImLerp(rect.Min,rect.Max,ImVec2(0.2f,0.75f));
    ImVec2 posBusbarOp2=ImLerp(rect.Min,rect.Max,ImVec2(0.4f,0.75f));
    ImVec2 posBusbarOp3=ImLerp(rect.Min,rect.Max,ImVec2(0.6f,0.75f));
    ImVec2 posBusbarOp4=ImLerp(rect.Min,rect.Max,ImVec2(0.8f,0.75f));

    ImVec2 posBusbarStart=ImLerp(rect.Min,rect.Max,ImVec2(0.15f,0.75f));
    ImVec2 posBusbarEnd=ImLerp(rect.Min,rect.Max,ImVec2(0.85f,0.75f));
    bool busbarStartSeen=false;
    for (int i=0; i<4; i++) {
      DivInstrumentESFM::Operator& opE=esfm.op[i];
      if (opE.outLvl>0) {
        if (!busbarStartSeen) {
          busbarStartSeen=true;
          posBusbarStart=ImLerp(rect.Min,rect.Max,ImVec2(0.15f+0.2f*i,0.75f));
        }
        posBusbarEnd=ImLerp(rect.Min,rect.Max,ImVec2(0.25f+0.2f*i,0.75f));
      }
    }

    if (mod12) addAALine(dl,posOp1,posOp2,colorsL[mod12]);
    if (mod23) addAALine(dl,posOp2,posOp3,colorsL[mod23]);
    if (mod34) addAALine(dl,posOp3,posOp4,colorsL[mod34]);
    if (out1) addAALine(dl,posOp1,posBusbarOp1,colorsL[out1]);
    if (out2) addAALine(dl,posOp2,posBusbarOp2,colorsL[out2]);
    if (out3) addAALine(dl,posOp3,posBusbarOp3,colorsL[out3]);
    if (out4) addAALine(dl,posOp4,posBusbarOp4,colorsL[out4]);

    addAALine(dl,posBusbarStart,posBusbarEnd,colorsL[2]);
    // addAALine(dl,posBusbarEnd,posBusbarEnd+ImVec2(-8.0f*dpiScale,-4.0f*dpiScale),colorsL[2]);
    // addAALine(dl,posBusbarEnd,posBusbarEnd+ImVec2(-8.0f*dpiScale,4.0f*dpiScale),colorsL[2]);
    // addAALine(dl,posBusbarStart+ImVec2(4.0f*dpiScale,-4.0f*dpiScale),posBusbarStart+ImVec2(4.0f*dpiScale,4.0f*dpiScale),colorsL[2]);

    dl->AddCircle(posOp1,6.0f*dpiScale+1.0f,isMod[0]?colorM:colorC);
    dl->AddCircleFilled(posOp1,4.0f*dpiScale+1.0f,isMod[0]?colorM:colorC);
    dl->AddCircleFilled(posOp2,4.0f*dpiScale+1.0f,isMod[1]?colorM:colorC);
    dl->AddCircleFilled(posOp3,4.0f*dpiScale+1.0f,isMod[2]?colorM:colorC);
    dl->AddCircleFilled(posOp4,4.0f*dpiScale+1.0f,isMod[3]?colorM:colorC);

    posOp1.x-=ImGui::CalcTextSize("1").x+circleRadius+3.0f*dpiScale;
    posOp2.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0f*dpiScale;
    posOp3.x-=ImGui::CalcTextSize("3").x+circleRadius+3.0f*dpiScale;
    posOp4.x-=ImGui::CalcTextSize("4").x+circleRadius+3.0f*dpiScale;
    posOp1.y-=ImGui::CalcTextSize("1").y*0.5f;
    posOp2.y-=ImGui::CalcTextSize("2").y*0.5f;
    posOp3.y-=ImGui::CalcTextSize("3").y*0.5f;
    posOp4.y-=ImGui::CalcTextSize("4").y*0.5f;
    dl->AddText(posOp1,isMod[0]?colorM:colorC,"1");
    dl->AddText(posOp2,isMod[1]?colorM:colorC,"2");
    dl->AddText(posOp3,isMod[2]?colorM:colorC,"3");
    dl->AddText(posOp4,isMod[3]?colorM:colorC,"4");
  }
}

void FurnaceGUI::drawFMEnv(unsigned char tl, unsigned char ar, unsigned char dr, unsigned char d2r, unsigned char rr, unsigned char sl, unsigned char sus, unsigned char egt, unsigned char algOrGlobalSus, float maxTl, float maxArDr, float maxRr, const ImVec2& size, unsigned short instType) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE]);
  ImU32 colorR=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_RELEASE]); // Relsease triangle
  ImU32 colorS=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_SUS_GUIDE]); // Sustain horiz/vert line color
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("fmEnv"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);

    //Adjust for OPLL global sustain setting
    if (instType==DIV_INS_OPLL && algOrGlobalSus==1.0){
      rr = 5.0;
    }
    //calculate x positions
    float arPos=float(maxArDr-ar)/maxArDr; //peak of AR, start of DR
    float drPos=arPos+((sl/15.0)*(float(maxArDr-dr)/maxArDr)); //end of DR, start of D2R
    float d2rPos=drPos+(((15.0-sl)/15.0)*(float(31.0-d2r)/31.0)); //End of D2R
    float rrPos=(float(maxRr-rr)/float(maxRr)); //end of RR

    //shrink all the x positions horizontally
    arPos/=2.0;
    drPos/=2.0;
    d2rPos/=2.0;
    rrPos/=1.0;

    ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,1.0)); //the bottom corner
    ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(arPos,(tl/maxTl))); //peak of AR, start of DR
    ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(drPos,(float)((tl/maxTl)+(sl/15.0)-((tl/maxTl)*(sl/15.0))))); //end of DR, start of D2R
    ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(d2rPos,1.0)); //end of D2R
    ImVec2 posRStart=ImLerp(rect.Min,rect.Max,ImVec2(0.0,(tl/maxTl))); //release start
    ImVec2 posREnd=ImLerp(rect.Min,rect.Max,ImVec2(rrPos,1.0));//release end
    ImVec2 posSLineHEnd=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(float)((tl/maxTl)+(sl/15.0)-((tl/maxTl)*(sl/15.0))))); //sustain horizontal line end
    ImVec2 posSLineVEnd=ImLerp(rect.Min,rect.Max,ImVec2(drPos,1.0)); //sustain vertical line end
    ImVec2 posDecayRate0Pt=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(tl/maxTl))); //Height of the peak of AR, forever
    ImVec2 posDecay2Rate0Pt=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(float)((tl/maxTl)+(sl/15.0)-((tl/maxTl)*(sl/15.0))))); //Height of the peak of SR, forever

    //dl->Flags=ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedLinesUseTex;
    if (ar==0.0) { //if AR = 0, the envelope never starts
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); //draw release as shaded triangle behind everything
      addAALine(dl,pos1,pos4,color); //draw line on ground
    } else if (dr==0.0 && sl!=0.0) { //if DR = 0 and SL is not 0, then the envelope stays at max volume forever
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); //draw release as shaded triangle behind everything
      //addAALine(dl,pos3,posSLineHEnd,colorS); //draw horiz line through sustain level
      //addAALine(dl,pos3,posSLineVEnd,colorS); //draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); //A
      addAALine(dl,pos2,posDecayRate0Pt,color); //Line from A to end of graph
    } else if (d2r==0.0 || ((instType==DIV_INS_OPL || instType==DIV_INS_SNES || instType==DIV_INS_ESFM) && sus==1.0) || (instType==DIV_INS_OPLL && egt!=0.0)) { //envelope stays at the sustain level forever
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); //draw release as shaded triangle behind everything
      addAALine(dl,pos3,posSLineHEnd,colorR); //draw horiz line through sustain level
      addAALine(dl,pos3,posSLineVEnd,colorR); //draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); //A
      addAALine(dl,pos2,pos3,color); //D
      addAALine(dl,pos3,posDecay2Rate0Pt,color); //Line from D to end of graph
    } else { //draw graph normally
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); //draw release as shaded triangle behind everything
      addAALine(dl,pos3,posSLineHEnd,colorR); //draw horiz line through sustain level
      addAALine(dl,pos3,posSLineVEnd,colorR); //draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); //A
      addAALine(dl,pos2,pos3,color); //D
      addAALine(dl,pos3,pos4,color); //D2
    }
    //dl->Flags^=ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedLinesUseTex;
  }
}

void FurnaceGUI::drawSID3Env(unsigned char tl, unsigned char ar, unsigned char dr, unsigned char d2r, unsigned char rr, unsigned char sl, unsigned char sus, unsigned char egt, unsigned char algOrGlobalSus, float maxTl, float maxArDr, float maxRr, const ImVec2& size, unsigned short instType) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE]);
  ImU32 colorR=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_RELEASE]); // Relsease triangle
  ImU32 colorS=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_SUS_GUIDE]); // Sustain horiz/vert line color
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("fmEnv"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);

    // Adjust for OPLL global sustain setting
    if (instType==DIV_INS_OPLL && algOrGlobalSus==1.0) {
      rr=5.0;
    }
    // calculate x positions
    float arPos=float(maxArDr-(float)ar)/maxArDr; // peak of AR, start of DR
    float drPos=arPos+(((float)sl/255.0)*(float(maxArDr-(float)dr)/maxArDr)); // end of DR, start of D2R
    float d2rPos=drPos+(((255.0-(float)sl)/255.0)*(float(255.0-(float)d2r)/255.0)); // End of D2R
    float rrPos=(float(maxRr-(float)rr)/float(maxRr)); // end of RR

    // shrink all the x positions horizontally
    arPos/=2.0;
    drPos/=2.0;
    d2rPos/=2.0;
    rrPos/=1.0;

    ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,1.0)); // the bottom corner
    ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(arPos,((float)tl/maxTl))); // peak of AR, start of DR
    ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(drPos,(float)(((float)tl/maxTl)+((float)sl/255.0)-(((float)tl/maxTl)*((float)sl/255.0))))); // end of DR, start of D2R
    ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(d2rPos,1.0)); // end of D2R
    ImVec2 posRStart=ImLerp(rect.Min,rect.Max,ImVec2(0.0,((float)tl/maxTl))); // release start
    ImVec2 posREnd=ImLerp(rect.Min,rect.Max,ImVec2(rrPos,1.0));// release end
    ImVec2 posSLineHEnd=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(float)(((float)tl/maxTl)+((float)sl/255.0)-(((float)tl/maxTl)*((float)sl/255.0))))); // sustain horizontal line end
    ImVec2 posSLineVEnd=ImLerp(rect.Min,rect.Max,ImVec2(drPos,1.0)); // sustain vertical line end
    ImVec2 posDecayRate0Pt=ImLerp(rect.Min,rect.Max,ImVec2(1.0,((float)tl/maxTl))); // Height of the peak of AR, forever
    ImVec2 posDecay2Rate0Pt=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(float)(((float)tl/maxTl)+((float)sl/255.0)-(((float)tl/maxTl)*((float)sl/255.0))))); // Height of the peak of SR, forever

    // dl->Flags=ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedLinesUseTex;
    if ((float)ar==0.0) { // if AR = 0, the envelope never starts
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      addAALine(dl,pos1,pos4,color); // draw line on ground
    } else if ((float)dr==0.0 && (float)sl!=0.0) { // if DR = 0 and SL is not 0, then the envelope stays at max volume forever
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      // addAALine(dl,pos3,posSLineHEnd,colorS); // draw horiz line through sustain level
      // addAALine(dl,pos3,posSLineVEnd,colorS); // draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); // A
      addAALine(dl,pos2,posDecayRate0Pt,color); // Line from A to end of graph
    } else if ((float)d2r==0.0 || ((instType==DIV_INS_OPL || instType==DIV_INS_SNES || instType == DIV_INS_ESFM) && sus==1.0) || (instType==DIV_INS_OPLL && egt!=0.0)) { // envelope stays at the sustain level forever
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      addAALine(dl,pos3,posSLineHEnd,colorR); // draw horiz line through sustain level
      addAALine(dl,pos3,posSLineVEnd,colorR); // draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); // A
      addAALine(dl,pos2,pos3,color); // D
      addAALine(dl,pos3,posDecay2Rate0Pt,color); // Line from D to end of graph
    } else { // draw graph normally
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      addAALine(dl,pos3,posSLineHEnd,colorR); // draw horiz line through sustain level
      addAALine(dl,pos3,posSLineVEnd,colorR); // draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); // A
      addAALine(dl,pos2,pos3,color); // D
      addAALine(dl,pos3,pos4,color); // D2
    }
    //dl->Flags^=ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedLinesUseTex;
  }
}

void FurnaceGUI::drawGBEnv(unsigned char vol, unsigned char len, unsigned char sLen, bool dir, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE]);
  //ImU32 colorS=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_SUS_GUIDE]); // Sustain horiz/vert line color
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("gbEnv"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);
    
    float volY=1.0-((float)vol/15.0);
    float lenPos=(sLen>63)?1.0:((float)sLen/384.0);
    float envEndPoint=((float)len/7.0)*((float)(dir?(15-vol):vol)/15.0);

    ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,volY));
    ImVec2 pos2;
    if (dir) {
      if (len>0) {
        if (lenPos<envEndPoint) {
          pos2=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,volY*(1.0-(lenPos/envEndPoint))));
        } else {
          pos2=ImLerp(rect.Min,rect.Max,ImVec2(envEndPoint,0.0));
        }
      } else {
        pos2=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,volY));
      }
    } else {
      if (len>0) {
        if (lenPos<envEndPoint) {
          pos2=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,volY+(1.0-volY)*(lenPos/envEndPoint)));
        } else {
          pos2=ImLerp(rect.Min,rect.Max,ImVec2(envEndPoint,1.0));
        }
      } else {
        pos2=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,volY));
      }
    }
    ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,(len>0 || sLen<64)?((dir && sLen>62)?0.0:1.0):volY));

    addAALine(dl,pos1,pos2,color);
    if (lenPos>=envEndPoint && sLen<64 && dir) {
      pos3=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,0.0));
      addAALine(dl,pos2,pos3,color);
      ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(lenPos,1.0));
      addAALine(dl,pos3,pos4,color);
    } else {
      addAALine(dl,pos2,pos3,color);
    }
  }
}

#define P(x) if (x) { \
  MARK_MODIFIED; \
  e->notifyInsChange(curIns); \
  updateFMPreview=true; \
}

#define PARAMETER MARK_MODIFIED; e->notifyInsChange(curIns); updateFMPreview=true;

String genericGuide(float value) {
  return fmt::sprintf("%d",(int)value);
}

inline int deBit30(const int val) {
  if ((val&0xc0000000)==0x40000000 || (val&0xc0000000)==0x80000000) return val^0x40000000;
  return val;
}

inline bool enBit30(const int val) {
  if ((val&0xc0000000)==0x40000000 || (val&0xc0000000)==0x80000000) return true;
  return false;
}


void FurnaceGUI::kvsConfig(DivInstrument* ins, bool supportsKVS) {
  if (fmPreviewOn) {
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(_("left click to restart\nmiddle click to pause\nright click to see algorithm"));
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
      updateFMPreview=true;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
      fmPreviewPaused=!fmPreviewPaused;
    }
  } else if (supportsKVS) {
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(_("left click to configure TL scaling\nright click to see FM preview"));
    }
  } else {
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(_("right click to see FM preview"));
    }
  }
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
    fmPreviewOn=!fmPreviewOn;
  }
  if (ImGui::IsItemHovered() && CHECK_LONG_HOLD) {
    NOTIFY_LONG_HOLD;
    fmPreviewOn=!fmPreviewOn;
  }
  if (!fmPreviewOn && supportsKVS) {
    int opCount=4;
    if (ins->type==DIV_INS_OPLL) opCount=2;
    if (ins->type==DIV_INS_OPL) opCount=(ins->fm.ops==4)?4:2;
    if (ImGui::BeginPopupContextItem("IKVSOpt",ImGuiPopupFlags_MouseButtonLeft)) {
      ImGui::Text(_("operator level changes with volume?"));
      if (ImGui::BeginTable("KVSTable",4,ImGuiTableFlags_BordersInner)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch);
        for (int i=0; i<4; i++) {
          int o=(opCount==4 && ins->type!=DIV_INS_ESFM)?orderedOps[i]:i;
          if (!(i&1)) ImGui::TableNextRow();
          const char* label=_("AUTO##OPKVS");
          if (ins->fm.op[o].kvs==0) {
            label=_("NO##OPKVS");
          } else if (ins->fm.op[o].kvs==1) {
            label=_("YES##OPKVS");
          }
          ImGui::TableNextColumn();
          ImGui::Text("%d",i+1);
          ImGui::TableNextColumn();
          ImGui::PushID(o);
          if (ImGui::Button(label,ImVec2(ImGui::GetContentRegionAvail().x,0.0f))) {
            if (++ins->fm.op[o].kvs>2) ins->fm.op[o].kvs=0;
            PARAMETER;
          }
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
      ImGui::EndPopup();
    }
  }
}

void FurnaceGUI::drawFMPreview(const ImVec2& size) {
  float asFloat[FM_PREVIEW_SIZE];
  for (int i=0; i<FM_PREVIEW_SIZE; i++) {
    asFloat[i]=(float)fmPreview[i]/8192.0f;
  }
  ImGui::PlotLines("##DebugFMPreview",asFloat,FM_PREVIEW_SIZE,0,NULL,-1.0,1.0,size);
}

#define MACRO_VZOOM i.ins->temp.vZoom[i.macro->macroType]
#define MACRO_VSCROLL i.ins->temp.vScroll[i.macro->macroType]

#define ADSR_LENGTH_HINT(_x,_range) \
  if (_x<1) { \
    ImGui::SetTooltip(_("forever")); \
  } else { \
    const int lHint=((((_range)<<8)|0xff)+(_x)-1)/(_x); \
    ImGui::SetTooltip(ngettext("~%d tick","~%d ticks",lHint),lHint); \
  }

#define ADSR_LENGTH_HINT_REL(_x,_range) \
  if (_x<1) { \
    ImGui::SetTooltip(_("instant")); \
  } else { \
    const int lHint=((((_range)<<8)|0xff)+(_x)-1)/(_x); \
    ImGui::SetTooltip(ngettext("~%d tick","~%d ticks",lHint),lHint); \
  }

void FurnaceGUI::drawMacroEdit(FurnaceGUIMacroDesc& i, int totalFit, float availableWidth, int index) {
  static float asFloat[256];
  static int asInt[256];
  static float loopIndicator[256];
  static float bit30Indicator[256];
  static bool doHighlight[256];

  if ((i.macro->open&6)==0) {
    for (int j=0; j<256; j++) {
      bit30Indicator[j]=0;
      if (j+macroDragScroll>=i.macro->len) {
        asFloat[j]=0;
        asInt[j]=0;
      } else {
        asFloat[j]=deBit30(i.macro->val[j+macroDragScroll]);
        asInt[j]=deBit30(i.macro->val[j+macroDragScroll]);
        if (i.bit30) bit30Indicator[j]=enBit30(i.macro->val[j+macroDragScroll]);
      }
      if (j+macroDragScroll>=i.macro->len || (j+macroDragScroll>i.macro->rel && i.macro->loop<i.macro->rel)) {
        loopIndicator[j]=0;
      } else {
        loopIndicator[j]=((i.macro->loop!=255 && (j+macroDragScroll)>=i.macro->loop))|((i.macro->rel!=255 && (j+macroDragScroll)==i.macro->rel)<<1);
      }
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0.0f,0.0f));
    if (MACRO_VZOOM<1) {
      if (i.macro->macroType==DIV_MACRO_ARP || i.isArp) {
        MACRO_VZOOM=24;
        MACRO_VSCROLL=120-12;
      }
      else if ((i.macro->macroType == DIV_MACRO_PITCH || i.isPitch) || (i.macro->macroType == DIV_MACRO_EX7 && i.isPitch)) {
        MACRO_VZOOM=128;
        MACRO_VSCROLL=2048-64;
      } else {
        MACRO_VZOOM=i.max-i.min;
        MACRO_VSCROLL=0;
      }
    }
    if (MACRO_VZOOM>(i.max-i.min)) {
      MACRO_VZOOM=i.max-i.min;
    }

    memset(doHighlight,0,256*sizeof(bool));
    if (e->isRunning()) for (int j=0; j<e->getTotalChannelCount(); j++) {
      DivChannelState* chanState=e->getChanState(j);
      if (chanState==NULL) continue;

      if (chanState->keyOff) continue;
      if (chanState->lastIns!=curIns) continue;

      DivMacroInt* macroInt=e->getMacroInt(j);
      if (macroInt==NULL) continue;

      DivMacroStruct* macroStruct=macroInt->structByType(i.macro->macroType);
      if (macroStruct==NULL) continue;

      if (macroStruct->lastPos>i.macro->len) continue;
      if (macroStruct->lastPos<macroDragScroll) continue;
      if (macroStruct->lastPos>255) continue;
      if (!macroStruct->actualHad) continue;

      doHighlight[macroStruct->lastPos-macroDragScroll]=true;
    }

    if (i.isBitfield) {
      PlotBitfield("##IMacro",asInt,totalFit,0,i.bitfieldBits,i.max,ImVec2(availableWidth,(i.macro->open&1)?(i.height*dpiScale):(32.0f*dpiScale)),sizeof(float),doHighlight,uiColors[GUI_COLOR_MACRO_HIGHLIGHT],i.color,i.hoverFunc,i.hoverFuncUser);
    } else {
      PlotCustom("##IMacro",asFloat,totalFit,macroDragScroll,NULL,i.min+MACRO_VSCROLL,i.min+MACRO_VSCROLL+MACRO_VZOOM,ImVec2(availableWidth,(i.macro->open&1)?(i.height*dpiScale):(32.0f*dpiScale)),sizeof(float),i.color,i.macro->len-macroDragScroll,i.hoverFunc,i.hoverFuncUser,i.blockMode,(i.macro->open&1)?genericGuide:NULL,doHighlight,uiColors[GUI_COLOR_MACRO_HIGHLIGHT]);
    }
    if ((i.macro->open&1) && (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))) {
      ImGui::InhibitInertialScroll();
      macroDragStart=ImGui::GetItemRectMin();
      macroDragAreaSize=ImVec2(availableWidth,i.height*dpiScale);
      if (i.isBitfield) {
        macroDragMin=i.min;
        macroDragMax=i.max;
      } else {
        macroDragMin=i.min+MACRO_VSCROLL;
        macroDragMax=i.min+MACRO_VSCROLL+MACRO_VZOOM;
      }
      macroDragBitMode=i.isBitfield;
      macroDragInitialValueSet=false;
      macroDragInitialValue=false;
      macroDragLen=totalFit;
      macroDragActive=true;
      macroDragBit30=i.bit30;
      macroDragSettingBit30=false;
      macroDragTarget=i.macro->val;
      macroDragChar=false;
      macroDragLineMode=(i.isBitfield)?false:ImGui::IsItemClicked(ImGuiMouseButton_Right);
      macroDragLineInitial=ImVec2(0,0);
      lastMacroDesc=i;
      processDrags(ImGui::GetMousePos().x,ImGui::GetMousePos().y);
    }
    if ((i.macro->open&1)) {
      if (ImGui::IsItemHovered()) {
        if (ctrlWheeling) {
          if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
            MACRO_VZOOM+=wheelY*(1+(MACRO_VZOOM>>4));
            if (MACRO_VZOOM<1) MACRO_VZOOM=1;
            if (MACRO_VZOOM>(i.max-i.min)) MACRO_VZOOM=i.max-i.min;
            if ((MACRO_VSCROLL+MACRO_VZOOM)>(i.max-i.min)) {
              MACRO_VSCROLL=(i.max-i.min)-MACRO_VZOOM;
            }
          } else if (settings.autoMacroStepSize==0) {
            macroPointSize+=wheelY;
            if (macroPointSize<1) macroPointSize=1;
            if (macroPointSize>256) macroPointSize=256;
          }
        } else if ((ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) && wheelY!=0) {
          MACRO_VSCROLL+=wheelY*(1+(MACRO_VZOOM>>4));
          if (MACRO_VSCROLL<0) MACRO_VSCROLL=0;
          if (MACRO_VSCROLL>((i.max-i.min)-MACRO_VZOOM)) MACRO_VSCROLL=(i.max-i.min)-MACRO_VZOOM;
        }
      }

      // slider
      if (!i.isBitfield) {
        if (settings.oldMacroVSlider) {
          ImGui::SameLine(0.0f);
          if (ImGui::VSliderInt("##IMacroVScroll",ImVec2(20.0f*dpiScale,i.height*dpiScale),&MACRO_VSCROLL,0,(i.max-i.min)-MACRO_VZOOM,"",ImGuiSliderFlags_NoInput)) {
            if (MACRO_VSCROLL<0) MACRO_VSCROLL=0;
            if (MACRO_VSCROLL>((i.max-i.min)-MACRO_VZOOM)) MACRO_VSCROLL=(i.max-i.min)-MACRO_VZOOM;
          }
          if (ImGui::IsItemHovered() && ctrlWheeling) {
            MACRO_VSCROLL+=wheelY*(1+(MACRO_VZOOM>>4));
            if (MACRO_VSCROLL<0) MACRO_VSCROLL=0;
            if (MACRO_VSCROLL>((i.max-i.min)-MACRO_VZOOM)) MACRO_VSCROLL=(i.max-i.min)-MACRO_VZOOM;
          }
        } else {
          ImS64 scrollV=(i.max-i.min-MACRO_VZOOM)-MACRO_VSCROLL;
          ImS64 availV=MACRO_VZOOM;
          ImS64 contentsV=(i.max-i.min);

          ImGui::SameLine(0.0f);
          ImGui::SetCursorPosX(ImGui::GetCursorPosX()-ImGui::GetStyle().ItemSpacing.x);
          ImRect scrollbarPos=ImRect(ImGui::GetCursorScreenPos(),ImGui::GetCursorScreenPos());
          scrollbarPos.Max.x+=ImGui::GetStyle().ScrollbarSize;
          scrollbarPos.Max.y+=i.height*dpiScale;
          ImGui::Dummy(ImVec2(ImGui::GetStyle().ScrollbarSize,i.height*dpiScale));
          if (ImGui::IsItemHovered() && ctrlWheeling) {
            MACRO_VSCROLL+=wheelY*(1+(MACRO_VZOOM>>4));
            if (MACRO_VSCROLL<0) MACRO_VSCROLL=0;
            if (MACRO_VSCROLL>((i.max-i.min)-MACRO_VZOOM)) MACRO_VSCROLL=(i.max-i.min)-MACRO_VZOOM;
          }

          ImGuiID scrollbarID=ImGui::GetID("##IMacroVScroll");
          ImGui::KeepAliveID(scrollbarID);
          if (ImGui::ScrollbarEx(scrollbarPos,scrollbarID,ImGuiAxis_Y,&scrollV,availV,contentsV,0)) {
            MACRO_VSCROLL=(i.max-i.min-MACRO_VZOOM)-scrollV;
          }
        }
      }

      // bit 30 area
      if (i.bit30) {
        PlotCustom("##IMacroBit30",bit30Indicator,totalFit,macroDragScroll,NULL,0,1,ImVec2(availableWidth,12.0f*dpiScale),sizeof(float),i.color,i.macro->len-macroDragScroll,&macroHoverBit30);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
          ImGui::InhibitInertialScroll();
          macroDragStart=ImGui::GetItemRectMin();
          macroDragAreaSize=ImVec2(availableWidth,12.0f*dpiScale);
          macroDragInitialValueSet=false;
          macroDragInitialValue=false;
          macroDragLen=totalFit;
          macroDragActive=true;
          macroDragBit30=i.bit30;
          macroDragSettingBit30=true;
          macroDragTarget=i.macro->val;
          macroDragChar=false;
          macroDragLineMode=false;
          macroDragLineInitial=ImVec2(0,0);
          lastMacroDesc=i;
          processDrags(ImGui::GetMousePos().x,ImGui::GetMousePos().y);
        }
      }

      // loop area
      PlotCustom("##IMacroLoop",loopIndicator,totalFit,macroDragScroll,NULL,0,2,ImVec2(availableWidth,12.0f*dpiScale),sizeof(float),i.color,i.macro->len-macroDragScroll,&macroHoverLoop);
      if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImGui::InhibitInertialScroll();
        macroLoopDragStart=ImGui::GetItemRectMin();
        macroLoopDragAreaSize=ImVec2(availableWidth,12.0f*dpiScale);
        macroLoopDragLen=totalFit;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
          macroLoopDragTarget=&i.macro->rel;
        } else {
          macroLoopDragTarget=&i.macro->loop;
        }
        macroLoopDragActive=true;
        processDrags(ImGui::GetMousePos().x,ImGui::GetMousePos().y);
      }
      if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        ImGui::InhibitInertialScroll();
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
          i.macro->rel=255;
        } else {
          i.macro->loop=255;
        }
      }
      ImGui::SetNextItemWidth(availableWidth);
      String& mmlStr=mmlString[index];
      if (ImGui::InputText("##IMacroMML",&mmlStr)) {
        decodeMMLStr(mmlStr,i.macro->val,i.macro->len,i.macro->loop,i.min,(i.isBitfield)?((1<<(i.isBitfield?(i.max):0))-1):i.max,i.macro->rel,i.bit30);
      }
      if (!ImGui::IsItemActive()) {
        encodeMMLStr(mmlStr,i.macro->val,i.macro->len,i.macro->loop,i.macro->rel,false,i.bit30);
      }
    }
    ImGui::PopStyleVar();
  } else {
    const int actualMax=i.isBitfield?((1<<i.max)-1):i.max;
    if (i.macro->open&2) {
      const bool compact=(availableWidth<300.0f*dpiScale);
      bool adsrClamp=false;
      if (ImGui::BeginTable("MacroADSR",compact?2:4)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.3);
        if (!compact) {
          ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed);
          ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.3);
        }
        //ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthStretch,0.4);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Bottom"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##MABottom",&i.macro->val[0],1,16)) { PARAMETER
          if (i.macro->val[0]<i.min) i.macro->val[0]=i.min;
          if (i.macro->val[0]>actualMax) i.macro->val[0]=actualMax;

          // clamp parameters to new range
          adsrClamp=true;
        }

        if (compact) ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Top"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##MATop",&i.macro->val[1],1,16)) { PARAMETER
          if (i.macro->val[1]<i.min) i.macro->val[1]=i.min;
          if (i.macro->val[1]>actualMax) i.macro->val[1]=actualMax;

          // clamp parameters to new range
          adsrClamp=true;
        }

        const int adsrBottom=MIN(i.macro->val[0],i.macro->val[1]);
        const int adsrTop=MAX(i.macro->val[0],i.macro->val[1]);
        const int adsrRange=abs(adsrTop-adsrBottom);
        const int adsrParamMax=(adsrRange<<8)|0xff;

        // if the range has changed, we must confine all parameters to make
        // sure they're in range.
        if (adsrClamp) {
          // attack
          if (i.macro->val[2]<0) i.macro->val[2]=0;
          if (i.macro->val[2]>adsrParamMax) i.macro->val[2]=adsrParamMax;

          // decay
          if (i.macro->val[4]<0) i.macro->val[4]=0;
          if (i.macro->val[4]>adsrParamMax) i.macro->val[4]=adsrParamMax;

          // sustain decay
          if (i.macro->val[7]<0) i.macro->val[7]=0;
          if (i.macro->val[7]>adsrParamMax) i.macro->val[7]=adsrParamMax;

          // release
          if (i.macro->val[8]<0) i.macro->val[8]=0;
          if (i.macro->val[8]>adsrParamMax) i.macro->val[8]=adsrParamMax;

          // sustain level
          if (i.macro->val[5]<adsrBottom) i.macro->val[5]=adsrBottom;
          if (i.macro->val[5]>adsrTop) i.macro->val[5]=adsrTop;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Attack"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (CWSliderInt("##MAAR",&i.macro->val[2],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
          if (i.macro->val[2]<0) i.macro->val[2]=0;
          if (i.macro->val[2]>adsrParamMax) i.macro->val[2]=adsrParamMax;
        } rightClickable
        if (ImGui::IsItemHovered()) {
          ADSR_LENGTH_HINT(i.macro->val[2],adsrRange);
        }

        if (compact) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Hold"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MAHT",&i.macro->val[3],0,255)) { PARAMETER
            if (i.macro->val[3]<0) i.macro->val[3]=0;
            if (i.macro->val[3]>255) i.macro->val[3]=255;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(ngettext("%d tick","%d ticks",i.macro->val[3]),i.macro->val[3]);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Decay"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MADR",&i.macro->val[4],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
            if (i.macro->val[4]<0) i.macro->val[4]=0;
            if (i.macro->val[4]>adsrParamMax) i.macro->val[4]=adsrParamMax;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ADSR_LENGTH_HINT(i.macro->val[4],adsrTop-i.macro->val[5]);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Sustain"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MASL",&i.macro->val[5],adsrBottom,adsrTop)) { PARAMETER
            if (i.macro->val[5]<adsrBottom) i.macro->val[5]=adsrBottom;
            if (i.macro->val[5]>adsrTop) i.macro->val[5]=adsrTop;
          } rightClickable

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("SusTime"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MAST",&i.macro->val[6],0,255)) { PARAMETER
            if (i.macro->val[6]<0) i.macro->val[6]=0;
            if (i.macro->val[6]>255) i.macro->val[6]=255;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(ngettext("%d tick","%d ticks",i.macro->val[6]),i.macro->val[6]);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("SusDecay"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MASR",&i.macro->val[7],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
            if (i.macro->val[7]<0) i.macro->val[7]=0;
            if (i.macro->val[7]>adsrParamMax) i.macro->val[7]=adsrParamMax;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ADSR_LENGTH_HINT(i.macro->val[7],i.macro->val[5]-adsrBottom);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Release"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MARR",&i.macro->val[8],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
            if (i.macro->val[8]<0) i.macro->val[8]=0;
            if (i.macro->val[8]>adsrParamMax) i.macro->val[8]=adsrParamMax;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ADSR_LENGTH_HINT_REL(i.macro->val[8],i.macro->val[5]-adsrBottom);
          }
        } else {
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Sustain"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MASL",&i.macro->val[5],adsrBottom,adsrTop)) { PARAMETER
            if (i.macro->val[5]<adsrBottom) i.macro->val[5]=adsrBottom;
            if (i.macro->val[5]>adsrTop) i.macro->val[5]=adsrTop;
          } rightClickable

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Hold"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MAHT",&i.macro->val[3],0,255)) { PARAMETER
            if (i.macro->val[3]<0) i.macro->val[3]=0;
            if (i.macro->val[3]>255) i.macro->val[3]=255;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(ngettext("%d tick","%d ticks",i.macro->val[3]),i.macro->val[3]);
          }

          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("SusTime"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MAST",&i.macro->val[6],0,255)) { PARAMETER
            if (i.macro->val[6]<0) i.macro->val[6]=0;
            if (i.macro->val[6]>255) i.macro->val[6]=255;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(ngettext("%d tick","%d ticks",i.macro->val[6]),i.macro->val[6]);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Decay"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MADR",&i.macro->val[4],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
            if (i.macro->val[4]<0) i.macro->val[4]=0;
            if (i.macro->val[4]>adsrParamMax) i.macro->val[4]=adsrParamMax;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ADSR_LENGTH_HINT(i.macro->val[4],adsrTop-i.macro->val[5]);
          }

          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("SusDecay"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MASR",&i.macro->val[7],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
            if (i.macro->val[7]<0) i.macro->val[7]=0;
            if (i.macro->val[7]>adsrParamMax) i.macro->val[7]=adsrParamMax;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ADSR_LENGTH_HINT(i.macro->val[7],i.macro->val[5]-adsrBottom);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TableNextColumn();

          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Release"));
          ImGui::TableNextColumn();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (CWSliderInt("##MARR",&i.macro->val[8],0,adsrParamMax,"%d",ImGuiSliderFlags_Logarithmic)) { PARAMETER
            if (i.macro->val[8]<0) i.macro->val[8]=0;
            if (i.macro->val[8]>adsrParamMax) i.macro->val[8]=adsrParamMax;
          } rightClickable
          if (ImGui::IsItemHovered()) {
            ADSR_LENGTH_HINT_REL(i.macro->val[8],i.macro->val[5]-adsrBottom);
          }
        }

        ImGui::EndTable();
      }
    }
    if (i.macro->open&4) {
      const bool compact=(availableWidth<300.0f*dpiScale);
      bool lfoClamp=false;
      if (ImGui::BeginTable("MacroLFO",compact?2:4)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.3);
        if (!compact) {
          ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed);
          ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.3);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Bottom"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##MABottom",&i.macro->val[0],1,16)) { PARAMETER
          if (i.macro->val[0]<i.min) i.macro->val[0]=i.min;
          if (i.macro->val[0]>actualMax) i.macro->val[0]=actualMax;

          // clamp parameters to new range
          lfoClamp=true;
        }

        if (compact) ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Top"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##MATop",&i.macro->val[1],1,16)) { PARAMETER
          if (i.macro->val[1]<i.min) i.macro->val[1]=i.min;
          if (i.macro->val[1]>actualMax) i.macro->val[1]=actualMax;

          // clamp parameters to new range
          lfoClamp=true;
        }

        const int lfoBottom=i.macro->val[0];
        const int lfoTop=i.macro->val[1];
        const int lfoShape=i.macro->val[12];
        const int lfoRange=abs(lfoTop-lfoBottom);
        int lfoParamMax=(lfoShape==2)?65536:((lfoRange<<8)|0xff);

        // if the range has changed, we must confine all parameters to make
        // sure they're in range.
        if (lfoClamp) {
          // speed
          if (i.macro->val[11]<0) i.macro->val[11]=0;
          if (i.macro->val[11]>(lfoParamMax>>1)) i.macro->val[11]=lfoParamMax>>1;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Speed"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (CWSliderInt("##MLSpeed",&i.macro->val[11],0,lfoParamMax>>1)) { PARAMETER
          if (i.macro->val[11]<0) i.macro->val[11]=0;
          if (i.macro->val[11]>(lfoParamMax>>1)) i.macro->val[11]=lfoParamMax>>1;
        } rightClickable
        if (ImGui::IsItemHovered()) {
          if (i.macro->val[11]<1) {
            ImGui::SetTooltip(_("halted"));
          } else {
            const int lHint=(lfoParamMax/i.macro->val[11])*((lfoShape==0)?2:1);
            ImGui::SetTooltip(ngettext("~%d tick","~%d ticks",lHint),lHint);
          }
        }

        if (compact) ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Phase"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (CWSliderInt("##MLPhase",&i.macro->val[13],0,1023)) { PARAMETER
          if (i.macro->val[13]<0) i.macro->val[13]=0;
          if (i.macro->val[13]>1023) i.macro->val[13]=1023;
        } rightClickable

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Shape"));
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (CWSliderInt("##MLShape",&i.macro->val[12],0,2,macroLFOShapes[i.macro->val[12]&3])) { PARAMETER
          if (i.macro->val[12]<0) i.macro->val[12]=0;
          if (i.macro->val[12]>2) i.macro->val[12]=2;

          // we must clamp in case the shape becomes square or not
          lfoClamp=true;
          lfoParamMax=(lfoShape==2)?32768:((lfoRange<<8)|0xff);
        } rightClickable

        // a second clamp is performed here in case the user has changed the
        // LFO shape (square uses an accumulator from 0 to 65535 and disregards
        // the range)
        if (lfoClamp) {
          // speed
          if (i.macro->val[11]<0) i.macro->val[11]=0;
          if (i.macro->val[11]>(lfoParamMax>>1)) i.macro->val[11]=lfoParamMax>>1;
        }

        ImGui::EndTable();
      }
    }
  }
}

#define BUTTON_TO_SET_MODE(buttonType) \
  if (buttonType(macroTypeLabels[(i.macro->open>>1)&3])) { \
    unsigned char prevOpen=i.macro->open; \
    if (i.macro->open>=4) { \
      i.macro->open&=(~6); \
    } else { \
      i.macro->open+=2; \
    } \
\
    /* check whether macro type is now ADSR/LFO or sequence */ \
    if (((prevOpen&6)?1:0)!=((i.macro->open&6)?1:0)) { \
      /* swap memory */ \
      /* this way the macro isn't corrupted if the user decides to go */ \
      /* back to sequence mode */ \
      i.macro->len^=i.ins->temp.lenMemory[i.macro->macroType]; \
      i.ins->temp.lenMemory[i.macro->macroType]^=i.macro->len; \
      i.macro->len^=i.ins->temp.lenMemory[i.macro->macroType]; \
\
      for (int j=0; j<16; j++) { \
        i.macro->val[j]^=i.ins->temp.typeMemory[i.macro->macroType][j]; \
        i.ins->temp.typeMemory[i.macro->macroType][j]^=i.macro->val[j]; \
        i.macro->val[j]^=i.ins->temp.typeMemory[i.macro->macroType][j]; \
      } \
\
      /* if ADSR/LFO, populate min/max */ \
      if (i.macro->open&6) { \
        const int actualMax=i.isBitfield?((1<<i.max)-1):i.max; \
        if (i.macro->val[0]==0 && i.macro->val[1]==0) { \
          i.macro->val[0]=i.min; \
          i.macro->val[1]=actualMax; \
        } \
        i.macro->val[0]=CLAMP(i.macro->val[0],i.min,actualMax); \
        i.macro->val[1]=CLAMP(i.macro->val[1],i.min,actualMax); \
      } \
    } \
    PARAMETER; \
  } \
  if (ImGui::IsItemHovered()) { \
    switch (i.macro->open&6) { \
      case 0: \
        ImGui::SetTooltip(_("Macro type: Sequence")); \
        break; \
      case 2: \
        ImGui::SetTooltip(_("Macro type: ADSR")); \
        break; \
      case 4: \
        ImGui::SetTooltip(_("Macro type: LFO")); \
        break; \
      default: \
        ImGui::SetTooltip(_("Macro type: What's going on here?")); \
        break; \
    } \
  } \
  if (i.macro->open&6) { \
    i.macro->len=16; \
  }

#define BUTTON_TO_SET_PROPS(_x) \
  pushToggleColors(_x.macro->speed!=1 || _x.macro->delay); \
  ImGui::Button(ICON_FA_ELLIPSIS_H "##IMacroSet"); \
  popToggleColors(); \
  if (ImGui::IsItemHovered()) { \
    ImGui::SetTooltip(_("Delay/Step Length")); \
  } \
  if (ImGui::BeginPopupContextItem("IMacroSetP",ImGuiPopupFlags_MouseButtonLeft)) { \
    if (ImGui::InputScalar(_("Step Length (ticks)##IMacroSpeed"),ImGuiDataType_U8,&_x.macro->speed,&_ONE,&_THREE)) { \
      if (_x.macro->speed<1) _x.macro->speed=1; \
      MARK_MODIFIED; \
    } \
    if (ImGui::InputScalar(_("Delay##IMacroDelay"),ImGuiDataType_U8,&_x.macro->delay,&_ONE,&_THREE)) { \
      MARK_MODIFIED; \
    } \
    ImGui::EndPopup(); \
  }

#define BUTTON_TO_SET_RELEASE(buttonType) \
  pushToggleColors(i.macro->open&8); \
  if (buttonType(ICON_FA_BOLT "##IMacroRelMode")) { \
    i.macro->open^=8; \
  } \
  if (ImGui::IsItemHovered()) { \
    if (i.macro->open&8) { \
      ImGui::SetTooltip(_("Release mode: Active (jump to release pos)")); \
    } else { \
      ImGui::SetTooltip(_("Release mode: Passive (delayed release)")); \
    } \
  } \
  popToggleColors(); \

#define BUTTON_FOR_MACRO_MENU(buttonType) \
  if (mobileUI) { \
    if (buttonType(ICON_FA_PAGELINES "##IMacroMenu")) { \
      lastMacroDesc=i; \
      displayMacroMenu=true; \
    } \
  }

void FurnaceGUI::drawMacros(std::vector<FurnaceGUIMacroDesc>& macros, FurnaceGUIMacroEditState& state, DivInstrument* ins) {
  int index=0;
  int maxMacroLen=0;
  float reservedSpace=(settings.oldMacroVSlider)?(20.0f*dpiScale+ImGui::GetStyle().ItemSpacing.x):ImGui::GetStyle().ScrollbarSize;

  for (FurnaceGUIMacroDesc& m: macros) {
    m.ins=ins;
    if (m.macro->len>maxMacroLen) maxMacroLen=m.macro->len;
  }

  switch (settings.macroLayout) {
    case 0: {
      if (ImGui::BeginTable("MacroSpace",2)) {
        float precalcWidth=0.0f;
        for (FurnaceGUIMacroDesc& i: macros) {
          float next=ImGui::CalcTextSize(i.displayName).x+ImGui::GetStyle().ItemInnerSpacing.x*2.0f+ImGui::CalcTextSize(ICON_FA_CHEVRON_UP).x+ImGui::GetStyle().ItemSpacing.x*2.0f;
          if (next>precalcWidth) precalcWidth=next;
        }
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed,MAX(120.0f*dpiScale,precalcWidth));
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.0);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        float lenAvail=ImGui::GetContentRegionAvail().x;
        //ImGui::Dummy(ImVec2(120.0f*dpiScale,dpiScale));
        if (settings.autoMacroStepSize==0) {
          ImGui::SetNextItemWidth(120.0f*dpiScale);
          if (ImGui::InputInt("##MacroPointSize",&macroPointSize,1,4)) {
            if (macroPointSize<1) macroPointSize=1;
            if (macroPointSize>256) macroPointSize=256;
          }
        }
        ImGui::TableNextColumn();
        float availableWidth=ImGui::GetContentRegionAvail().x-reservedSpace;
        int totalFit=MIN(255,availableWidth/MAX(1,macroPointSize*dpiScale));
        int scrollMax=0;
        if (settings.autoMacroStepSize!=0) totalFit=1;
        for (FurnaceGUIMacroDesc& i: macros) {
          if (i.macro->len>scrollMax) scrollMax=i.macro->len;
          if (settings.autoMacroStepSize==1) {
            if ((i.macro->open&6)==0 && totalFit<i.macro->len) totalFit=i.macro->len;
          } else if (settings.autoMacroStepSize==2) {
            if ((i.macro->open&6)==0 && totalFit<maxMacroLen) totalFit=maxMacroLen;
          }
        }
        scrollMax-=totalFit;
        if (scrollMax<0) scrollMax=0;
        if (macroDragScroll>scrollMax) {
          macroDragScroll=scrollMax;
        }
        ImGui::BeginDisabled(scrollMax<1);
        ImGui::SetNextItemWidth(availableWidth);
        if (CWSliderInt("##MacroScrollTop",&macroDragScroll,0,scrollMax,"")) {
          if (macroDragScroll<0) macroDragScroll=0;
          if (macroDragScroll>scrollMax) macroDragScroll=scrollMax;
        }
        ImGui::EndDisabled();

        // draw macros
        for (FurnaceGUIMacroDesc& i: macros) {
          ImGui::PushID(index);
          ImGui::TableNextRow();

          // description
          ImGui::TableNextColumn();
          ImGui::Text("%s",i.displayName);
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("index: %.2X"),i.macro->macroType);
          }
          ImGui::SameLine();
          if (ImGui::SmallButton((i.macro->open&1)?(ICON_FA_CHEVRON_UP "##IMacroOpen"):(ICON_FA_CHEVRON_DOWN "##IMacroOpen"))) {
            i.macro->open^=1;
          }
          if (i.macro->open&1) {
            if ((i.macro->open&6)==0) {
              ImGui::SetNextItemWidth(lenAvail);
              int macroLen=i.macro->len;
              if (ImGui::InputScalar("##IMacroLen",ImGuiDataType_U8,&macroLen,&_ONE,&_THREE)) { MARK_MODIFIED
                if (macroLen<0) macroLen=0;
                if (macroLen>255) macroLen=255;
                i.macro->len=macroLen;
              }
            }
            BUTTON_TO_SET_MODE(ImGui::Button);
            ImGui::SameLine();
            BUTTON_TO_SET_PROPS(i);
            if ((i.macro->open&6)==0) {
              ImGui::SameLine();
              BUTTON_TO_SET_RELEASE(ImGui::Button);
            }
            BUTTON_FOR_MACRO_MENU(ImGui::Button);
            // do not change this!
            // anything other than a checkbox will look ugly!
            // if you really need more than two macro modes please tell me.
            if (i.modeName!=NULL) {
              bool modeVal=i.macro->mode;
              String modeName=fmt::sprintf("%s##IMacroMode",i.modeName);
              if (ImGui::Checkbox(modeName.c_str(),&modeVal)) {
                i.macro->mode=modeVal;
                i.ins->temp.vZoom[i.macro->macroType]=-1;
                i.ins->temp.vScroll[i.macro->macroType]=-1;
              }
            }
          }

          // macro area
          ImGui::TableNextColumn();
          drawMacroEdit(i,totalFit,availableWidth,index);
          ImGui::PopID();
          index++;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(scrollMax<1);
        ImGui::SetNextItemWidth(availableWidth);
        if (CWSliderInt("##MacroScrollBottom",&macroDragScroll,0,scrollMax,"")) {
          if (macroDragScroll<0) macroDragScroll=0;
          if (macroDragScroll>scrollMax) macroDragScroll=scrollMax;
        }
        ImGui::EndDisabled();
        ImGui::EndTable();
      }
      break;
    }
    case 1: {
      ImGui::Text("Tabs");
      break;
    }
    case 2: {
      int columns=round(ImGui::GetContentRegionAvail().x/(400.0*dpiScale));
      int curColumn=0;
      if (columns<1) columns=1;
      if (ImGui::BeginTable("MacroGrid",columns,ImGuiTableFlags_BordersInner)) {
        for (FurnaceGUIMacroDesc& i: macros) {
          if (curColumn==0) ImGui::TableNextRow();
          ImGui::TableNextColumn();

          if (++curColumn>=columns) curColumn=0;
          
          float availableWidth=ImGui::GetContentRegionAvail().x-reservedSpace;
          int totalFit=i.macro->len;
          if (totalFit<1) totalFit=1;

          ImGui::PushID(index);

          ImGui::TextUnformatted(i.displayName);
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("index: %.2X"),i.macro->macroType);
          }
          ImGui::SameLine();
          if (ImGui::SmallButton((i.macro->open&1)?(ICON_FA_CHEVRON_UP "##IMacroOpen"):(ICON_FA_CHEVRON_DOWN "##IMacroOpen"))) {
            i.macro->open^=1;
          }

          if (i.macro->open&1) {
            ImGui::SameLine();
            BUTTON_TO_SET_MODE(ImGui::SmallButton);
          }

          drawMacroEdit(i,totalFit,availableWidth,index);

          if (i.macro->open&1) {
            if ((i.macro->open&6)==0) {
              ImGui::Text(_("Length"));
              ImGui::SameLine();
              ImGui::SetNextItemWidth(120.0f*dpiScale);
              int macroLen=i.macro->len;
              if (ImGui::InputScalar("##IMacroLen",ImGuiDataType_U8,&macroLen,&_ONE,&_THREE)) { MARK_MODIFIED
                if (macroLen<0) macroLen=0;
                if (macroLen>255) macroLen=255;
                i.macro->len=macroLen;
              }
              ImGui::SameLine();
            }
            BUTTON_TO_SET_PROPS(i);
            if ((i.macro->open&6)==0) {
              ImGui::SameLine();
              BUTTON_TO_SET_RELEASE(ImGui::Button);
            }
            BUTTON_FOR_MACRO_MENU(ImGui::Button);
            if (i.modeName!=NULL) {
              bool modeVal=i.macro->mode;
              String modeName=fmt::sprintf("%s##IMacroMode",i.modeName);
              ImGui::SameLine();
              if (ImGui::Checkbox(modeName.c_str(),&modeVal)) {
                i.macro->mode=modeVal;
                i.ins->temp.vZoom[i.macro->macroType]=-1;
                i.ins->temp.vScroll[i.macro->macroType]=-1;
              }
            }
          }

          ImGui::PopID();
          index++;
        }
        ImGui::EndTable();
      }
      break;
    }
    case 3: {
      if (ImGui::BeginTable("MacroList",2,ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.2f);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.8f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        for (size_t i=0; i<macros.size(); i++) {
          // include macro len if non-zero, making particularly clear at-a-glance which macros
          // have non-zero len (i.e. are active). and calculate how big we need to be to leave some
          // extra space so the column doesn't change size when len is changed under typical
          // circumstances (really don't want to move buttons while mouse is being clicked or held).
          char buf[256];

          if (macros[i].macro->len>0) {
            snprintf(buf,255,"%s [%d]###%s_%d",macros[i].displayName,macros[i].macro->len,macros[i].displayName,(int)i);
          } else {
            snprintf(buf,255,"%s###%s_%d",macros[i].displayName,macros[i].displayName,(int)i);
          }

          if (ImGui::Selectable(buf,state.selectedMacro==(int)i)) {
            state.selectedMacro=i;
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("index: %.2X"),macros[i].macro->macroType);
          }
        }

        ImGui::TableNextColumn();
        float availableWidth=ImGui::GetContentRegionAvail().x-reservedSpace;
        int totalFit=MIN(255,availableWidth/MAX(1,macroPointSize*dpiScale));
        if (macroDragScroll>255-totalFit) {
          macroDragScroll=255-totalFit;
        }

        if (state.selectedMacro<0 || state.selectedMacro>=(int)macros.size()) {
          state.selectedMacro=0;
        }

        if (state.selectedMacro>=0 && state.selectedMacro<(int)macros.size()) {
          FurnaceGUIMacroDesc& m=macros[state.selectedMacro];
          m.macro->open|=1;

          float availableWidth=ImGui::GetContentRegionAvail().x-reservedSpace;
          int totalFit=MIN(255,availableWidth/MAX(1,macroPointSize*dpiScale));
          int scrollMax=0;
          for (FurnaceGUIMacroDesc& i: macros) {
            if (i.macro->len>scrollMax) scrollMax=i.macro->len;
          }
          if (settings.autoMacroStepSize==1) totalFit=MAX(1,m.macro->len);
          else if (settings.autoMacroStepSize==2) totalFit=MAX(1,maxMacroLen);
          scrollMax-=totalFit;
          if (scrollMax<0) scrollMax=0;
          if (macroDragScroll>scrollMax) {
            macroDragScroll=scrollMax;
          }
          ImGui::BeginDisabled(scrollMax<1);
          ImGui::SetNextItemWidth(availableWidth);
          if (CWSliderInt("##MacroScroll",&macroDragScroll,0,scrollMax,"")) {
            if (macroDragScroll<0) macroDragScroll=0;
            if (macroDragScroll>scrollMax) macroDragScroll=scrollMax;
          }
          ImGui::EndDisabled();

          if (settings.autoMacroStepSize==0) {
            ImGui::SameLine();
            ImGui::Button(ICON_FA_SEARCH_PLUS "##MacroZoomB");
            if (ImGui::BeginPopupContextItem("MacroZoomP",ImGuiPopupFlags_MouseButtonLeft)) {
              ImGui::SetNextItemWidth(120.0f*dpiScale);
              if (ImGui::InputInt("##MacroPointSize",&macroPointSize,1,4)) {
                if (macroPointSize<1) macroPointSize=1;
                if (macroPointSize>256) macroPointSize=256;
              }
              ImGui::EndPopup();
            }
          }

          m.height=ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()-ImGui::GetFrameHeightWithSpacing()-(m.bit30?28.0f:12.0f)*dpiScale-ImGui::GetStyle().ItemSpacing.y*3.0f;
          if (m.height<10.0f*dpiScale) m.height=10.0f*dpiScale;
          m.height/=dpiScale;
          drawMacroEdit(m,totalFit,availableWidth,index);

          if (m.macro->open&1) {
            bool showLen=((m.macro->open&6)==0);
            int colCount=showLen ? 4 : 3;
            float availX=ImGui::GetContentRegionAvail().x;

            // fairly arbitrary scaling logic
            bool shortLabels=(availX<600.0f*dpiScale);
            float scalarItemWidth=MIN((availX-90.0f*dpiScale)/colCount, 120.0f*dpiScale);
            if (ImGui::BeginTable("##MacroMetaData",colCount)) {
              if (showLen) ImGui::TableSetupColumn("len",ImGuiTableColumnFlags_WidthStretch,0.0);
              ImGui::TableSetupColumn("stepLen",ImGuiTableColumnFlags_WidthStretch,0.0);
              ImGui::TableSetupColumn("delay",ImGuiTableColumnFlags_WidthStretch,0.0);
              ImGui::TableSetupColumn("buttons",ImGuiTableColumnFlags_WidthFixed,0.0);

              ImGui::TableNextRow();
              if (showLen) {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text(shortLabels ? _("Len##macroEditLengthShortLabel") : _("Length"));
                if (shortLabels && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", _("Length"));
                ImGui::SameLine();
                ImGui::SetNextItemWidth(scalarItemWidth);
                int macroLen=m.macro->len;
                if (ImGui::InputScalar("##IMacroLen",ImGuiDataType_U8,&macroLen,&_ONE,&_THREE)) { MARK_MODIFIED
                  if (macroLen<0) macroLen=0;
                  if (macroLen>255) macroLen=255;
                  m.macro->len=macroLen;
                }
              }
              ImGui::TableNextColumn();
              ImGui::AlignTextToFramePadding();
              ImGui::Text(shortLabels ? _("SLen##macroEditStepLenShortLabel") : _("StepLen"));
              if (shortLabels && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", _("StepLen"));
              ImGui::SameLine();
              ImGui::SetNextItemWidth(scalarItemWidth);
              if (ImGui::InputScalar("##IMacroSpeed",ImGuiDataType_U8,&m.macro->speed,&_ONE,&_THREE)) {
                if (m.macro->speed<1) m.macro->speed=1;
                MARK_MODIFIED;
              }
              ImGui::TableNextColumn();
              ImGui::AlignTextToFramePadding();
              ImGui::Text(shortLabels ? _("Del##macroEditDelayShortLabel") : _("Delay"));
              if (shortLabels && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", _("Delay"));
              ImGui::SameLine();
              ImGui::SetNextItemWidth(scalarItemWidth);
              if (ImGui::InputScalar("##IMacroDelay",ImGuiDataType_U8,&m.macro->delay,&_ONE,&_THREE)) {
                MARK_MODIFIED;
              }
              ImGui::TableNextColumn();
              {
                FurnaceGUIMacroDesc& i=m;
                BUTTON_TO_SET_MODE(ImGui::Button);
                if ((i.macro->open&6)==0) {
                  ImGui::SameLine();
                  BUTTON_TO_SET_RELEASE(ImGui::Button);
                }
                BUTTON_FOR_MACRO_MENU(ImGui::Button);
              }
              if (m.modeName!=NULL) {
                bool modeVal=m.macro->mode;
                String modeName=fmt::sprintf("%s##IMacroMode",m.modeName);
                ImGui::SameLine();
                if (ImGui::Checkbox(modeName.c_str(),&modeVal)) {
                  m.macro->mode=modeVal;
                  m.ins->temp.vZoom[m.macro->macroType]=-1;
                  m.ins->temp.vScroll[m.macro->macroType]=-1;
                }
              }
              ImGui::EndTable();
            }
          } else {
            ImGui::Text(_("The heck? No, this isn't even working correctly..."));
          }
        } else {
          ImGui::Text(_("The only problem with that selectedMacro is that it's a bug..."));
        }

        // goes here
        ImGui::EndTable();
      }
      break;
    }
    case 4: {
      ImGui::Text("Single (combo box)");
      break;
    }
  }
}

void FurnaceGUI::alterSampleMap(int column, int val) {
  if (curIns<0 || curIns>=(int)e->song.ins.size()) return;
  DivInstrument* ins=e->song.ins[curIns];
  int sampleMapMin=sampleMapSelStart;
  int sampleMapMax=sampleMapSelEnd;
  if (sampleMapMin>sampleMapMax) {
    sampleMapMin^=sampleMapMax;
    sampleMapMax^=sampleMapMin;
    sampleMapMin^=sampleMapMax;
  }

  for (int i=sampleMapMin; i<=sampleMapMax; i++) {
    if (i<0 || i>=180) continue;

    if (sampleMapColumn==1 && column==1) {
      ins->amiga.noteMap[i].freq=val;
    } else if (sampleMapColumn==0 && column==0) {
      if (val<0) {
        ins->amiga.noteMap[i].map=-1;
      } else if (sampleMapDigit>0) {
        ins->amiga.noteMap[i].map*=10;
        ins->amiga.noteMap[i].map+=val;
      } else {
        ins->amiga.noteMap[i].map=val;
      }
      if (ins->amiga.noteMap[i].map>=(int)e->song.sample.size()) {
        ins->amiga.noteMap[i].map=((int)e->song.sample.size())-1;
      }
    } else if (sampleMapColumn==2 && column==2) {
      if (val<0) {
        ins->amiga.noteMap[i].dpcmFreq=-1;
      } else if (sampleMapDigit>0) {
        ins->amiga.noteMap[i].dpcmFreq*=10;
        ins->amiga.noteMap[i].dpcmFreq+=val;
      } else {
        ins->amiga.noteMap[i].dpcmFreq=val;
      }
      if (ins->amiga.noteMap[i].dpcmFreq>15) {
        ins->amiga.noteMap[i].dpcmFreq%=10;
      }
    } else if (sampleMapColumn==3 && column==3) {
      if (val<0) {
        ins->amiga.noteMap[i].dpcmDelta=-1;
      } else if (sampleMapDigit>0) {
        if (ins->amiga.noteMap[i].dpcmDelta>7) {

          ins->amiga.noteMap[i].dpcmDelta=val;
        } else {
          ins->amiga.noteMap[i].dpcmDelta<<=4;
          ins->amiga.noteMap[i].dpcmDelta+=val;
        }
      } else {
        ins->amiga.noteMap[i].dpcmDelta=val;
      }
    }
  }

  bool advance=false;
  if (sampleMapColumn==1 && column==1) {
    advance=true;
  } else if (sampleMapColumn==0 && column==0) {
    int digits=1;
    if (e->song.sample.size()>=10) digits=2;
    if (e->song.sample.size()>=100) digits=3;
    if (e->song.sample.size()>=1000) digits=4;
    if (e->song.sample.size()>=10000) digits=5;
    if (++sampleMapDigit>=digits) {
      sampleMapDigit=0;
      advance=true;
    }
  } else if (sampleMapColumn==2 && column==2) {
    if (++sampleMapDigit>=2) {
      sampleMapDigit=0;
      advance=true;
    }
  } else if (sampleMapColumn==3 && column==3) {
    if (++sampleMapDigit>=2) {
      sampleMapDigit=0;
      advance=true;
    }
  }

  if (advance && sampleMapMin==sampleMapMax) {
    sampleMapSelStart++;
    if (sampleMapSelStart>179) sampleMapSelStart=179;
    sampleMapSelEnd=sampleMapSelStart;
  }

  MARK_MODIFIED;
}

#define DRUM_FREQ(name,db,df,prop) \
  ImGui::TableNextRow(); \
  ImGui::TableNextColumn(); \
  if (ins->type==DIV_INS_OPLL) { \
    block=(prop>>9)&7; \
    fNum=prop&511; \
  } else { \
    block=(prop>>10)&7; \
    fNum=prop&1023; \
  } \
  ImGui::Text(name); \
  ImGui::TableNextColumn(); \
  if (ImGui::InputInt(db,&block,1,1)) { \
    if (block<0) block=0; \
    if (block>7) block=7; \
    if (ins->type==DIV_INS_OPLL) { \
      prop=(block<<9)|fNum; \
    } else { \
      prop=(block<<10)|fNum; \
    } \
  } \
  ImGui::TableNextColumn(); \
  if (ImGui::InputInt(df,&fNum,1,16)) { \
    if (fNum<0) fNum=0; \
    if (ins->type==DIV_INS_OPLL) { \
      if (fNum>511) fNum=511; \
      prop=(block<<9)|fNum; \
    } else { \
      if (fNum>1023) fNum=1023; \
      prop=(block<<10)|fNum; \
    } \
  }


#define CENTER_TEXT(text) \
  ImGui::SetCursorPosX(ImGui::GetCursorPosX()+0.5*(ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(text).x));

#define CENTER_VSLIDER \
  ImGui::SetCursorPosX(ImGui::GetCursorPosX()+0.5f*ImGui::GetContentRegionAvail().x-10.0f*dpiScale);

#define CENTER_TEXT_20(text) \
  ImGui::SetCursorPosX(ImGui::GetCursorPosX()+0.5*(20.0f*dpiScale-ImGui::CalcTextSize(text).x));

#define TOOLTIP_TEXT(text) \
  if (ImGui::IsItemHovered()) { \
    ImGui::SetTooltip("%s", text); \
  }

#define OP_DRAG_POINT \
  if (ImGui::Button(ICON_FA_ARROWS)) { \
  } \
  if (ImGui::BeginDragDropSource()) { \
    opToMove=i; \
    ImGui::SetDragDropPayload("FUR_OP",NULL,0,ImGuiCond_Once); \
    ImGui::Button(ICON_FA_ARROWS "##SysDrag"); \
    ImGui::SameLine(); \
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) { \
      ImGui::Text(_("(copying)")); \
    } else { \
      ImGui::Text(_("(swapping)")); \
    } \
    ImGui::EndDragDropSource(); \
  } else if (ImGui::IsItemHovered()) { \
    ImGui::SetTooltip(_("- drag to swap operator\n- shift-drag to copy operator")); \
  } \
  if (ImGui::BeginDragDropTarget()) { \
    const ImGuiPayload* dragItem=ImGui::AcceptDragDropPayload("FUR_OP"); \
    if (dragItem!=NULL) { \
      if (dragItem->IsDataType("FUR_OP")) { \
        if (opToMove!=i && opToMove>=0) { \
          int destOp=(opCount==4 && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_ESFM)?opOrder[i]:i; \
          int sourceOp=(opCount==4 && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_ESFM)?opOrder[opToMove]:opToMove; \
          if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) { \
            e->lockEngine([ins,destOp,sourceOp]() { \
              ins->fm.op[destOp]=ins->fm.op[sourceOp]; \
              ins->esfm.op[destOp]=ins->esfm.op[sourceOp]; \
            }); \
          } else { \
            e->lockEngine([ins,destOp,sourceOp]() { \
              DivInstrumentFM::Operator origOp=ins->fm.op[sourceOp]; \
              DivInstrumentESFM::Operator origOpE=ins->esfm.op[sourceOp]; \
              ins->fm.op[sourceOp]=ins->fm.op[destOp]; \
              ins->esfm.op[sourceOp]=ins->esfm.op[destOp]; \
              ins->fm.op[destOp]=origOp; \
              ins->esfm.op[destOp]=origOpE; \
            }); \
          } \
          PARAMETER; \
        } \
        opToMove=-1; \
      } \
    } \
    ImGui::EndDragDropTarget(); \
  }

void FurnaceGUI::insTabWavetable(DivInstrument* ins)
{
  if (ImGui::BeginTabItem(_("Wavetable"))) {
    switch (ins->type) {
      case DIV_INS_GB:
      case DIV_INS_NAMCO:
      case DIV_INS_SM8521:
      case DIV_INS_SWAN:
        wavePreviewLen=32;
        wavePreviewHeight=15;
        break;
      case DIV_INS_PCE:
        wavePreviewLen=32;
        wavePreviewHeight=31;
        break;
      case DIV_INS_VBOY:
        wavePreviewLen=32;
        wavePreviewHeight=63;
        break;
      case DIV_INS_SCC:
        wavePreviewLen=32;
        wavePreviewHeight=255;
        break;
      case DIV_INS_FDS:
        wavePreviewLen=64;
        wavePreviewHeight=63;
        break;
      case DIV_INS_N163:
        wavePreviewLen=ins->n163.waveLen;
        wavePreviewHeight=15;
        break;
      case DIV_INS_X1_010:
        wavePreviewLen=128;
        wavePreviewHeight=255;
        break;
      case DIV_INS_AMIGA:
      case DIV_INS_GBA_DMA:
        wavePreviewLen=ins->amiga.waveLen+1;
        wavePreviewHeight=255;
        break;
      case DIV_INS_SNES:
        wavePreviewLen=ins->amiga.waveLen+1;
        wavePreviewHeight=15;
        break;
      case DIV_INS_GBA_MINMOD:
        wavePreviewLen=ins->amiga.waveLen+1;
        wavePreviewHeight=255;
        break;
      case DIV_INS_SID3:
        wavePreviewLen=256;
        wavePreviewHeight=255;
        break;
      default:
        wavePreviewLen=32;
        wavePreviewHeight=31;
        break;
    }
    if (ImGui::Checkbox(_("Enable synthesizer"),&ins->ws.enabled)) { PARAMETER
      wavePreviewInit=true;
    }
    if (ins->ws.enabled) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      if (ins->ws.effect&0x80) {
        if ((ins->ws.effect&0x7f)>=DIV_WS_DUAL_MAX) {
          ins->ws.effect=0;
          wavePreviewInit=true;
        }
      } else {
        if ((ins->ws.effect&0x7f)>=DIV_WS_SINGLE_MAX) {
          ins->ws.effect=0;
          wavePreviewInit=true;
        }
      }
      if (ImGui::BeginCombo("##WSEffect",(ins->ws.effect&0x80)?dualWSEffects[ins->ws.effect&0x7f]:singleWSEffects[ins->ws.effect&0x7f])) {
        ImGui::Text(_("Single-waveform"));
        ImGui::Indent();
        for (int i=0; i<DIV_WS_SINGLE_MAX; i++) {
          if (ImGui::Selectable(_(singleWSEffects[i]),ins->ws.effect==i)) { PARAMETER
            ins->ws.effect=i;
            wavePreviewInit=true;
          }
          if (ins->ws.effect==i) ImGui::SetItemDefaultFocus();
        }
        ImGui::Unindent();
        ImGui::Text(_("Dual-waveform"));
        ImGui::Indent();
        for (int i=129; i<DIV_WS_DUAL_MAX; i++) {
          if (ImGui::Selectable(_(dualWSEffects[i-128]),ins->ws.effect==i)) { PARAMETER
            ins->ws.effect=i;
            wavePreviewInit=true;
          }
          if (ins->ws.effect==i) ImGui::SetItemDefaultFocus();
        }
        ImGui::Unindent();
        ImGui::EndCombo();
      }
      const bool isSingleWaveFX=(ins->ws.effect>=128);
      if (ImGui::BeginTable("WSPreview",isSingleWaveFX?3:2)) {
        DivWavetable* wave1=e->getWave(ins->ws.wave1);
        DivWavetable* wave2=e->getWave(ins->ws.wave2);
        if (wavePreviewInit) {
          wavePreview.init(ins,wavePreviewLen,wavePreviewHeight,true);
          wavePreviewAccum=0.0f;
          wavePreviewInit=false;
        }
        float wavePreview1[257];
        float wavePreview2[257];
        float wavePreview3[257];
        for (int i=0; i<wave1->len; i++) {
          if (wave1->data[i]>wave1->max) {
            wavePreview1[i]=wave1->max;
          } else {
            wavePreview1[i]=wave1->data[i];
          }
        }
        if (wave1->len>0) {
          wavePreview1[wave1->len]=wave1->data[wave1->len-1];
        }
        for (int i=0; i<wave2->len; i++) {
          if (wave2->data[i]>wave2->max) {
            wavePreview2[i]=wave2->max;
          } else {
            wavePreview2[i]=wave2->data[i];
          }
        }
        if (wave2->len>0) {
          wavePreview2[wave2->len]=wave2->data[wave2->len-1];
        }
        if (ins->ws.enabled && (!wavePreviewPaused || wavePreviewInit)) {
          if (wavePreviewInit) {
            wavePreview.tick(true);
          } else {
            wavePreviewAccum+=ImGui::GetIO().DeltaTime;
            double accumRate=e->getCurHz();
            if (accumRate<1.0) accumRate=1.0;
            if (accumRate>1023.0) accumRate=1023.0;
            accumRate=1.0/accumRate;
            while (wavePreviewAccum>=accumRate) {
              wavePreviewAccum-=accumRate;
              wavePreview.tick(true);
            }
          }
          WAKE_UP;
        }
        for (int i=0; i<wavePreviewLen; i++) {
          wavePreview3[i]=wavePreview.output[i];
        }
        if (wavePreviewLen>0) {
          wavePreview3[wavePreviewLen]=wavePreview3[wavePreviewLen-1];
        }

        float ySize=(isSingleWaveFX?96.0f:128.0f)*dpiScale;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImVec2 size1=ImVec2(ImGui::GetContentRegionAvail().x,ySize);
        PlotNoLerp("##WaveformP1",wavePreview1,wave1->len+1,0,"Wave 1",0,wave1->max,size1);
        if (isSingleWaveFX) {
          ImGui::TableNextColumn();
          ImVec2 size2=ImVec2(ImGui::GetContentRegionAvail().x,ySize);
          PlotNoLerp("##WaveformP2",wavePreview2,wave2->len+1,0,"Wave 2",0,wave2->max,size2);
        }
        ImGui::TableNextColumn();
        ImVec2 size3=ImVec2(ImGui::GetContentRegionAvail().x,ySize);
        PlotNoLerp("##WaveformP3",wavePreview3,wavePreviewLen+1,0,"Result",0,wavePreviewHeight,size3);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ins->std.waveMacro.len>0) {
          ImGui::PushStyleColor(ImGuiCol_Text,uiColors[GUI_COLOR_WARNING]);
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Wave 1"));
          ImGui::SameLine();
          ImGui::Text(ICON_FA_EXCLAMATION_TRIANGLE);
          ImGui::PopStyleColor();
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("waveform macro is controlling wave 1!\nthis value will be ineffective."));
          }
        } else {
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Wave 1"));
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##SelWave1",&ins->ws.wave1,1,4)) { PARAMETER
          if (ins->ws.wave1<0) ins->ws.wave1=0;
          if (ins->ws.wave1>=(int)e->song.wave.size()) ins->ws.wave1=e->song.wave.size()-1;
          wavePreviewInit=true;
        }
        if (ins->std.waveMacro.len>0) {
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("waveform macro is controlling wave 1!\nthis value will be ineffective."));
          }
        }
        if (isSingleWaveFX) {
          ImGui::TableNextColumn();
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Wave 2"));
          ImGui::SameLine();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (ImGui::InputInt("##SelWave2",&ins->ws.wave2,1,4)) { PARAMETER
            if (ins->ws.wave2<0) ins->ws.wave2=0;
            if (ins->ws.wave2>=(int)e->song.wave.size()) ins->ws.wave2=e->song.wave.size()-1;
            wavePreviewInit=true;
          }
        }
        ImGui::TableNextColumn();
        if (ImGui::Button(wavePreviewPaused?(ICON_FA_PLAY "##WSPause"):(ICON_FA_PAUSE "##WSPause"))) {
          wavePreviewPaused=!wavePreviewPaused;
        }
        if (ImGui::IsItemHovered()) {
          if (wavePreviewPaused) {
            ImGui::SetTooltip(_("Resume preview"));
          } else {
            ImGui::SetTooltip(_("Pause preview"));
          }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_REPEAT "##WSRestart")) {
          wavePreviewInit=true;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(_("Restart preview"));
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_UPLOAD "##WSCopy")) {
          curWave=e->addWave();
          if (curWave==-1) {
            showError(_("too many wavetables!"));
          } else {
            wantScrollListWave=true;
            MARK_MODIFIED;
            RESET_WAVE_MACRO_ZOOM;
            nextWindow=GUI_WINDOW_WAVE_EDIT;

            DivWavetable* copyWave=e->song.wave[curWave];
            copyWave->len=wavePreviewLen;
            copyWave->max=wavePreviewHeight;
            memcpy(copyWave->data,wavePreview.output,256*sizeof(int));
          }
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(_("Copy to new wavetable"));
        }
        ImGui::SameLine();
        ImGui::Text("(%d×%d)",wavePreviewLen,wavePreviewHeight+1);
        ImGui::EndTable();
      }

      if (ImGui::InputScalar(_("Update Rate"),ImGuiDataType_U8,&ins->ws.rateDivider,&_ONE,&_EIGHT)) { PARAMETER
        wavePreviewInit=true;
      }
      int speed=ins->ws.speed+1;
      if (ImGui::InputInt(_("Speed"),&speed,1,8)) { PARAMETER
        if (speed<1) speed=1;
        if (speed>256) speed=256;
        ins->ws.speed=speed-1;
        wavePreviewInit=true;
      }

      if (ImGui::InputScalar(_("Amount"),ImGuiDataType_U8,&ins->ws.param1,&_ONE,&_EIGHT)) { PARAMETER
        wavePreviewInit=true;
      }

      if (ins->ws.effect==DIV_WS_PHASE_MOD) {
        if (ImGui::InputScalar(_("Power"),ImGuiDataType_U8,&ins->ws.param2,&_ONE,&_EIGHT)) { PARAMETER
          wavePreviewInit=true;
        }
      }

      if (ImGui::Checkbox(_("Global"),&ins->ws.global)) { PARAMETER
        wavePreviewInit=true;
      }
    } else {
      ImGui::TextWrapped(_("wavetable synthesizer disabled.\nuse the Waveform macro to set the wave for this instrument."));
    }

    ImGui::EndTabItem();
  }
}

void FurnaceGUI::insTabSample(DivInstrument* ins) {
  const char* sampleTabName=_("Sample");
  if (ins->type==DIV_INS_NES) sampleTabName=_("DPCM");
  if (ImGui::BeginTabItem(sampleTabName)) {
    if (ins->type==DIV_INS_NES && e->song.compatFlags.oldDPCM) {
      ImGui::Text(_("new DPCM features disabled (compatibility)!"));
      if (ImGui::Button(_("click here to enable them."))) {
        e->song.compatFlags.oldDPCM=false;
        MARK_MODIFIED;
      }
      ImGui::EndTabItem();
      return;
    }

    String sName;
    bool wannaOpenSMPopup=false;
    if (ins->amiga.initSample<0 || ins->amiga.initSample>=e->song.sampleLen) {
      sName=_("none selected");
    } else {
      sName=e->song.sample[ins->amiga.initSample]->name;
    }
    if (ins->type==DIV_INS_PCE ||
        ins->type==DIV_INS_MIKEY ||
        ins->type==DIV_INS_X1_010 ||
        ins->type==DIV_INS_SWAN ||
        ins->type==DIV_INS_AY ||
        ins->type==DIV_INS_AY8930 ||
        ins->type==DIV_INS_VRC6 ||
        ins->type==DIV_INS_SU ||
        ins->type==DIV_INS_NDS ||
        ins->type==DIV_INS_SUPERVISION ||
        ins->type==DIV_INS_SID3) {
      P(ImGui::Checkbox(_("Use sample"),&ins->amiga.useSample));
      if (ins->type==DIV_INS_X1_010) {
        if (ImGui::InputInt(_("Sample bank slot##BANKSLOT"),&ins->x1_010.bankSlot,1,4)) { PARAMETER
          if (ins->x1_010.bankSlot<0) ins->x1_010.bankSlot=0;
          if (ins->x1_010.bankSlot>=7) ins->x1_010.bankSlot=7;
          notifySampleChange=true;
        }
      }
    }
    ImGui::AlignTextToFramePadding();
    ImGui::Text(_("Sample"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##ISample",sName.c_str())) {
      String id;
      for (int i=0; i<e->song.sampleLen; i++) {
        id=fmt::sprintf("%d: %s",i,e->song.sample[i]->name);
        if (ImGui::Selectable(id.c_str(),ins->amiga.initSample==i)) { PARAMETER
          ins->amiga.initSample=i;
          notifySampleChange=true;
        }
      }
      ImGui::EndCombo();
    }
    // Wavetable
    if (ins->type==DIV_INS_AMIGA || ins->type==DIV_INS_SNES || ins->type==DIV_INS_GBA_DMA || ins->type==DIV_INS_GBA_MINMOD) {
      const char* useWaveText=ins->type==DIV_INS_AMIGA?_("Use wavetable (Amiga/Generic DAC only)"):_("Use wavetable");
      ImGui::BeginDisabled(ins->amiga.useNoteMap);
      P(ImGui::Checkbox(useWaveText,&ins->amiga.useWave));
      if (ins->amiga.useWave) {
        int len=ins->amiga.waveLen+1;
        int origLen=len;
        if (ImGui::InputInt(_("Width"),&len,2,16)) {
          if (ins->type==DIV_INS_SNES || ins->type==DIV_INS_GBA_DMA) {
            if (len<16) len=16;
            if (len>256) len=256;
            if (len>origLen) {
              ins->amiga.waveLen=((len+15)&(~15))-1;
            } else {
              ins->amiga.waveLen=(len&(~15))-1;
            }
          } else {
            if (len<2) len=2;
            if (len>256) len=256;
            ins->amiga.waveLen=(len&(~1))-1;
          }
          PARAMETER
        }
      }
      ImGui::EndDisabled();
    } else {
      ins->amiga.useWave=false;
    }
    // Note map
    ImGui::BeginDisabled(ins->amiga.useWave);
    P(ImGui::Checkbox(_("Use sample map"),&ins->amiga.useNoteMap));
    if ((ins->type==DIV_INS_MULTIPCM) && ImGui::IsItemHovered()) {
      ImGui::SetTooltip(_("Only for OPL4 PCM."));
    }
    if (ins->amiga.useNoteMap) {
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) sampleMapFocused=false;
      if (curWindowLast!=GUI_WINDOW_INS_EDIT) sampleMapFocused=false;
      if (!sampleMapFocused) sampleMapDigit=0;
      if (ImGui::BeginTable("NoteMap",(ins->type==DIV_INS_NES)?5:4,ImGuiTableFlags_ScrollY|ImGuiTableFlags_Borders|ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed);
        if (ins->type==DIV_INS_NES) ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::Text("#");
        if (ins->type==DIV_INS_NES) {
          ImGui::TableNextColumn();
          ImGui::Text(_("pitch"));
          ImGui::TableNextColumn();
          ImGui::Text(_("delta"));
        } else {
          ImGui::TableNextColumn();
          ImGui::Text(_("note"));
        }
        ImGui::TableNextColumn();
        ImGui::Text(_("sample name"));
        int sampleMapMin=sampleMapSelStart;
        int sampleMapMax=sampleMapSelEnd;
        if (sampleMapMin>sampleMapMax) {
          sampleMapMin^=sampleMapMax;
          sampleMapMax^=sampleMapMin;
          sampleMapMin^=sampleMapMax;
        }

        ImGui::PushStyleColor(ImGuiCol_Header,ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        for (int i=0; i<180; i++) {
          DivInstrumentAmiga::SampleMap& sampleMap=ins->amiga.noteMap[i];
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,ImGui::GetColorU32(ImGuiCol_TableHeaderBg));
          ImGui::AlignTextToFramePadding();
          ImGui::Text("%s",noteNames[i]);
          ImGui::TableNextColumn();
          if (sampleMap.map<0 || sampleMap.map>=e->song.sampleLen) {
            sName=fmt::sprintf("-----##SM%d",i);
            sampleMap.map=-1;
          } else {
            sName=fmt::sprintf("%5d##SM%d",sampleMap.map,i);
          }
          ImGui::PushFont(patFont);
          ImGui::AlignTextToFramePadding();
          ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000").x);
          ImGui::Selectable(sName.c_str(),(sampleMapWaitingInput && sampleMapColumn==0 && i>=sampleMapMin && i<=sampleMapMax));
          if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            sampleMapFocused=true;
            sampleMapColumn=0;
            sampleMapDigit=0;
            sampleMapSelStart=i;
            sampleMapSelEnd=i;

            sampleMapMin=sampleMapSelStart;
            sampleMapMax=sampleMapSelEnd;
            if (sampleMapMin>sampleMapMax) {
              sampleMapMin^=sampleMapMax;
              sampleMapMax^=sampleMapMin;
              sampleMapMin^=sampleMapMax;
            }
            ImGui::InhibitInertialScroll();
          }
          if (sampleMapFocused && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            sampleMapSelEnd=i;
          }
          if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (sampleMapSelStart==sampleMapSelEnd) {
              sampleMapFocused=true;
              sampleMapColumn=0;
              sampleMapDigit=0;
              sampleMapSelStart=i;
              sampleMapSelEnd=i;

              sampleMapMin=sampleMapSelStart;
              sampleMapMax=sampleMapSelEnd;
              if (sampleMapMin>sampleMapMax) {
                sampleMapMin^=sampleMapMax;
                sampleMapMax^=sampleMapMin;
                sampleMapMin^=sampleMapMax;
              }
            }
            if (sampleMapFocused) {
              wannaOpenSMPopup=true;
            }
          }
          ImGui::PopFont();

          if (ins->type==DIV_INS_NES) {
            // pitch
            ImGui::TableNextColumn();
            ImGui::PushFont(patFont);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("0000").x);
            if (sampleMap.dpcmFreq<0) {
              sName=fmt::sprintf(" -- ##SD1%d",i);
            } else {
              sName=fmt::sprintf(" %2d ##SD1%d",sampleMap.dpcmFreq,i);
            }
            ImGui::Selectable(sName.c_str(),(sampleMapWaitingInput && sampleMapColumn==2 && i>=sampleMapMin && i<=sampleMapMax));

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
              sampleMapFocused=true;
              sampleMapColumn=2;
              sampleMapDigit=0;
              sampleMapSelStart=i;
              sampleMapSelEnd=i;

              sampleMapMin=sampleMapSelStart;
              sampleMapMax=sampleMapSelEnd;
              if (sampleMapMin>sampleMapMax) {
                sampleMapMin^=sampleMapMax;
                sampleMapMax^=sampleMapMin;
                sampleMapMin^=sampleMapMax;
              }
              ImGui::InhibitInertialScroll();
            }
            if (sampleMapFocused && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
              sampleMapSelEnd=i;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
              if (sampleMapSelStart==sampleMapSelEnd) {
                sampleMapFocused=true;
                sampleMapColumn=2;
                sampleMapDigit=0;
                sampleMapSelStart=i;
                sampleMapSelEnd=i;

                sampleMapMin=sampleMapSelStart;
                sampleMapMax=sampleMapSelEnd;
                if (sampleMapMin>sampleMapMax) {
                  sampleMapMin^=sampleMapMax;
                  sampleMapMax^=sampleMapMin;
                  sampleMapMin^=sampleMapMax;
                }
              }
              if (sampleMapFocused) {
                wannaOpenSMPopup=true;
              }
            }

            ImGui::PopFont();

            // delta
            ImGui::TableNextColumn();
            ImGui::PushFont(patFont);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("0000").x);
            if (sampleMap.dpcmDelta<0) {
              sName=fmt::sprintf(" -- ##SD2%d",i);
            } else {
              sName=fmt::sprintf(" %2X ##SD2%d",sampleMap.dpcmDelta,i);
            }
            ImGui::Selectable(sName.c_str(),(sampleMapWaitingInput && sampleMapColumn==3 && i>=sampleMapMin && i<=sampleMapMax));

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
              sampleMapFocused=true;
              sampleMapColumn=3;
              sampleMapDigit=0;
              sampleMapSelStart=i;
              sampleMapSelEnd=i;

              sampleMapMin=sampleMapSelStart;
              sampleMapMax=sampleMapSelEnd;
              if (sampleMapMin>sampleMapMax) {
                sampleMapMin^=sampleMapMax;
                sampleMapMax^=sampleMapMin;
                sampleMapMin^=sampleMapMax;
              }
              ImGui::InhibitInertialScroll();
            }
            if (sampleMapFocused && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
              sampleMapSelEnd=i;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
              if (sampleMapSelStart==sampleMapSelEnd) {
                sampleMapFocused=true;
                sampleMapColumn=3;
                sampleMapDigit=0;
                sampleMapSelStart=i;
                sampleMapSelEnd=i;

                sampleMapMin=sampleMapSelStart;
                sampleMapMax=sampleMapSelEnd;
                if (sampleMapMin>sampleMapMax) {
                  sampleMapMin^=sampleMapMax;
                  sampleMapMax^=sampleMapMin;
                  sampleMapMin^=sampleMapMax;
                }
              }
              if (sampleMapFocused) {
                wannaOpenSMPopup=true;
              }
            }

            ImGui::PopFont();
          } else {
            ImGui::TableNextColumn();
            sName="???";
            if ((sampleMap.freq)>=0 && (sampleMap.freq)<180) {
              sName=noteNames[sampleMap.freq];
            }
            sName+=fmt::sprintf("##SN%d",i);
            ImGui::PushFont(patFont);
            ImGui::AlignTextToFramePadding();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000").x);
            ImGui::Selectable(sName.c_str(),(sampleMapWaitingInput && sampleMapColumn==1 && i>=sampleMapMin && i<=sampleMapMax));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
              sampleMapFocused=true;
              sampleMapColumn=1;
              sampleMapDigit=0;
              sampleMapSelStart=i;
              sampleMapSelEnd=i;

              sampleMapMin=sampleMapSelStart;
              sampleMapMax=sampleMapSelEnd;
              if (sampleMapMin>sampleMapMax) {
                sampleMapMin^=sampleMapMax;
                sampleMapMax^=sampleMapMin;
                sampleMapMin^=sampleMapMax;
              }
              ImGui::InhibitInertialScroll();
            }
            if (sampleMapFocused && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
              sampleMapSelEnd=i;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
              if (sampleMapSelStart==sampleMapSelEnd) {
                sampleMapFocused=true;
                sampleMapColumn=1;
                sampleMapDigit=0;
                sampleMapSelStart=i;
                sampleMapSelEnd=i;

                sampleMapMin=sampleMapSelStart;
                sampleMapMax=sampleMapSelEnd;
                if (sampleMapMin>sampleMapMax) {
                  sampleMapMin^=sampleMapMax;
                  sampleMapMax^=sampleMapMin;
                  sampleMapMin^=sampleMapMax;
                }
              }
              if (sampleMapFocused) {
                wannaOpenSMPopup=true;
              }
            }
            ImGui::PopFont();
          }

          ImGui::TableNextColumn();
          String prevName="---";
          if (sampleMap.map>=0 && sampleMap.map<e->song.sampleLen) {
            prevName=e->song.sample[sampleMap.map]->name;
          }
          ImGui::PushID(i+2);
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (ImGui::BeginCombo("##SMSample",prevName.c_str())) {
            if (ImGui::Selectable("---",sampleMap.map==-1)) {
              sampleMap.map=-1;
            }
            for (int k=0; k<e->song.sampleLen; k++) {
              String itemName=fmt::sprintf("%d: %s",k,e->song.sample[k]->name);
              if (ImGui::Selectable(itemName.c_str(),sampleMap.map==k)) {
                sampleMap.map=k;
              }
            }
            ImGui::EndCombo();
          }
          ImGui::PopID();
        }
        ImGui::PopStyleColor(2);
        ImGui::EndTable();
      }
    } else {
      sampleMapFocused=false;
    }
    ImGui::EndDisabled();
    if (wannaOpenSMPopup) {
      ImGui::OpenPopup("SampleMapUtils");
    }
    if (ImGui::BeginPopup("SampleMapUtils",ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings)) {
      if (sampleMapSelStart==sampleMapSelEnd && sampleMapSelStart>=0 && sampleMapSelStart<180) {
        if (ins->type==DIV_INS_NES) {
          if (ImGui::MenuItem(_("set entire map to this pitch"))) {
            if (sampleMapSelStart>=0 && sampleMapSelStart<180) {
              for (int i=0; i<180; i++) {
                if (i==sampleMapSelStart) continue;
                ins->amiga.noteMap[i].dpcmFreq=ins->amiga.noteMap[sampleMapSelStart].dpcmFreq;
              }
            }
          }
          if (ImGui::MenuItem(_("set entire map to this delta counter value"))) {
            if (sampleMapSelStart>=0 && sampleMapSelStart<180) {
              for (int i=0; i<180; i++) {
                if (i==sampleMapSelStart) continue;
                ins->amiga.noteMap[i].dpcmDelta=ins->amiga.noteMap[sampleMapSelStart].dpcmDelta;
              }
            }
          }
        } else {
          if (ImGui::MenuItem(_("set entire map to this note"))) {
            if (sampleMapSelStart>=0 && sampleMapSelStart<180) {
              for (int i=0; i<180; i++) {
                if (i==sampleMapSelStart) continue;
                ins->amiga.noteMap[i].freq=ins->amiga.noteMap[sampleMapSelStart].freq;
              }
            }
          }
        }
        if (ImGui::MenuItem(_("set entire map to this sample"))) {
          if (sampleMapSelStart>=0 && sampleMapSelStart<180) {
            for (int i=0; i<180; i++) {
              if (i==sampleMapSelStart) continue;
              ins->amiga.noteMap[i].map=ins->amiga.noteMap[sampleMapSelStart].map;
            }
          }
        }
      }
      if (ins->type==DIV_INS_NES) {
        if (ImGui::MenuItem(_("reset pitches"))) {
          for (int i=0; i<180; i++) {
            ins->amiga.noteMap[i].dpcmFreq=15;
          }
        }
        if (ImGui::MenuItem(_("clear delta counter values"))) {
          for (int i=0; i<180; i++) {
            ins->amiga.noteMap[i].dpcmDelta=-1;
          }
        }
      } else {
        if (ImGui::MenuItem(_("reset notes"))) {
          for (int i=0; i<180; i++) {
            ins->amiga.noteMap[i].freq=i;
          }
        }
      }
      if (ImGui::MenuItem(_("clear map samples"))) {
        for (int i=0; i<180; i++) {
          ins->amiga.noteMap[i].map=-1;
        }
      }
      ImGui::EndPopup();
    }
    ImGui::EndTabItem();
  } else {
    sampleMapFocused=false;
  }
}

void FurnaceGUI::insTabFMModernHeader(DivInstrument* ins) {
  ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
  ImGui::TableNextColumn();
  if (ins->type==DIV_INS_ESFM) {
    ImGui::TableNextColumn();
    CENTER_TEXT(ESFM_SHORT_NAME(ESFM_MODIN));
    ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_MODIN));
    TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_MODIN));
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();
    CENTER_TEXT(ESFM_SHORT_NAME(ESFM_DELAY));
    ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_DELAY));
    TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_DELAY));
  }
  ImGui::TableNextColumn();
  CENTER_TEXT(FM_SHORT_NAME(FM_AR));
  ImGui::TextUnformatted(FM_SHORT_NAME(FM_AR));
  TOOLTIP_TEXT(FM_NAME(FM_AR));
  ImGui::TableNextColumn();
  CENTER_TEXT(FM_SHORT_NAME(FM_DR));
  ImGui::TextUnformatted(FM_SHORT_NAME(FM_DR));
  TOOLTIP_TEXT(FM_NAME(FM_DR));
  if (settings.susPosition==0) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_SL));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_SL));
    TOOLTIP_TEXT(FM_NAME(FM_SL));
  }
  if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_D2R));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_D2R));
    TOOLTIP_TEXT(FM_NAME(FM_D2R));
  }
  ImGui::TableNextColumn();
  CENTER_TEXT(FM_SHORT_NAME(FM_RR));
  ImGui::TextUnformatted(FM_SHORT_NAME(FM_RR));
  TOOLTIP_TEXT(FM_NAME(FM_RR));
  if (settings.susPosition==1) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_SL));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_SL));
    TOOLTIP_TEXT(FM_NAME(FM_SL));
  }
  ImGui::TableNextColumn();
  if (settings.susPosition==2) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_SL));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_SL));
    TOOLTIP_TEXT(FM_NAME(FM_SL));
  }
  ImGui::TableNextColumn();
  CENTER_TEXT(FM_SHORT_NAME(FM_TL));
  ImGui::TextUnformatted(FM_SHORT_NAME(FM_TL));
  TOOLTIP_TEXT(FM_NAME(FM_TL));
  if (settings.susPosition==3) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_SL));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_SL));
    TOOLTIP_TEXT(FM_NAME(FM_SL));
  }
  ImGui::TableNextColumn();
  if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
    CENTER_TEXT(FM_SHORT_NAME(FM_RS));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_RS));
    TOOLTIP_TEXT(FM_NAME(FM_RS));
  } else {
    CENTER_TEXT(FM_SHORT_NAME(FM_KSL));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_KSL));
    TOOLTIP_TEXT(FM_NAME(FM_KSL));
  }
  if (ins->type==DIV_INS_OPZ) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_EGSHIFT));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_EGSHIFT));
    TOOLTIP_TEXT(FM_NAME(FM_EGSHIFT));
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_REV));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_REV));
    TOOLTIP_TEXT(FM_NAME(FM_REV));
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_TS));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_TS));
    TOOLTIP_TEXT(FM_NAME(FM_TS));
  }
  if (ins->type==DIV_INS_ESFM) {
    ImGui::TableNextColumn();
    CENTER_TEXT(ESFM_SHORT_NAME(ESFM_OUTLVL));
    ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_OUTLVL));
    TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_OUTLVL));
  }
  ImGui::TableNextColumn();
  CENTER_TEXT(FM_SHORT_NAME(FM_MULT));
  ImGui::TextUnformatted(FM_SHORT_NAME(FM_MULT));
  TOOLTIP_TEXT(FM_NAME(FM_MULT));
  if (ins->type==DIV_INS_OPZ) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_SHORT_NAME(FM_FINE));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_FINE));
    TOOLTIP_TEXT(FM_NAME(FM_FINE));
  }
  if (ins->type==DIV_INS_ESFM) {
    ImGui::TableNextColumn();
    CENTER_TEXT(ESFM_SHORT_NAME(ESFM_CT));
    ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_CT));
    TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_CT));
  }
  ImGui::TableNextColumn();
  if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
    CENTER_TEXT(FM_SHORT_NAME(FM_DT));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_DT));
    TOOLTIP_TEXT(FM_NAME(FM_DT));
    ImGui::TableNextColumn();
  }
  if (ins->type==DIV_INS_ESFM) {
    CENTER_TEXT(ESFM_SHORT_NAME(ESFM_DT));
    ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_DT));
    TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_DT));
    ImGui::TableNextColumn();
  }
  if (ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
    CENTER_TEXT(FM_SHORT_NAME(FM_DT2));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_DT2));
    TOOLTIP_TEXT(FM_NAME(FM_DT2));
    ImGui::TableNextColumn();
  }
  if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPM) {
    CENTER_TEXT(FM_SHORT_NAME(FM_AM));
    ImGui::TextUnformatted(FM_SHORT_NAME(FM_AM));
    TOOLTIP_TEXT(FM_NAME(FM_AM));
  } else {
    CENTER_TEXT("Other");
    ImGui::TextUnformatted("Other");
  }
  ImGui::TableNextColumn();
  if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_ESFM) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_NAME(FM_WS));
    ImGui::TextUnformatted(FM_NAME(FM_WS));
  } else if (ins->type!=DIV_INS_OPLL && ins->type!=DIV_INS_OPM) {
    ImGui::TableNextColumn();
    CENTER_TEXT(FM_NAME(FM_SSG));
    ImGui::TextUnformatted(FM_NAME(FM_SSG));
  }
  ImGui::TableNextColumn();
  CENTER_TEXT(_("Envelope"));
  ImGui::TextUnformatted(_("Envelope"));
}

void FurnaceGUI::insTabFM(DivInstrument* ins) {
  int opCount=4;
  if (ins->type==DIV_INS_OPLL) opCount=2;
  if (ins->type==DIV_INS_OPL) opCount=(ins->fm.ops==4)?4:2;
  bool opsAreMutable=(ins->type==DIV_INS_FM || ins->type==DIV_INS_OPM || ins->type==DIV_INS_OPZ);

  // this determines which features are hidden from the OPL editor.
  int oplType=0;
  for (int i=0; i<e->song.systemLen; i++) {
    switch (e->song.system[i]) {
      case DIV_SYSTEM_OPL:
      case DIV_SYSTEM_OPL_DRUMS:
      case DIV_SYSTEM_Y8950:
      case DIV_SYSTEM_Y8950_DRUMS:
        if (oplType<1) oplType=1;
        break;
      case DIV_SYSTEM_OPL2:
      case DIV_SYSTEM_OPL2_DRUMS:
        if (oplType<2) oplType=2;
        break;
      case DIV_SYSTEM_OPL3:
      case DIV_SYSTEM_OPL3_DRUMS:
      case DIV_SYSTEM_OPL4:
      case DIV_SYSTEM_OPL4_DRUMS:
      case DIV_SYSTEM_YMU759:
      case DIV_SYSTEM_ESFM: // to assist in porting
        if (oplType<3) oplType=3;
        break;
      default:
        break;
    }
  }
  // expose all features if no OPL chips are present
  if (oplType==0) oplType=3;

  int wsMax=7;
  if (oplType==2) wsMax=3;
  if (oplType==1) wsMax=0;

  if (ImGui::BeginTabItem("FM")) {
    DivInstrumentFM& fmOrigin=(ins->type==DIV_INS_OPLL && ins->fm.opllPreset>0 && ins->fm.opllPreset<16)?opllPreview:ins->fm;

    bool isPresent[4];
    int isPresentCount=0;
    memset(isPresent,0,4*sizeof(bool));
    for (int i=0; i<e->song.systemLen; i++) {
      if (e->song.system[i]==DIV_SYSTEM_VRC7) {
        isPresent[3]=true;
      } else if (e->song.system[i]==DIV_SYSTEM_OPLL || e->song.system[i]==DIV_SYSTEM_OPLL_DRUMS) {
        isPresent[(e->song.systemFlags[i].getInt("patchSet",0))&3]=true;
      }
    }
    if (!isPresent[0] && !isPresent[1] && !isPresent[2] && !isPresent[3]) {
      isPresent[0]=true;
    }
    for (int i=0; i<4; i++) {
      if (isPresent[i]) isPresentCount++;
    }
    int presentWhich=0;
    for (int i=0; i<4; i++) {
      if (isPresent[i]) {
        presentWhich=i;
        break;
      }
    }

    if (ImGui::BeginTable("fmDetails",3,(ins->type==DIV_INS_ESFM)?ImGuiTableFlags_SizingStretchProp:ImGuiTableFlags_SizingStretchSame)) {
      String blockTxt=_("Automatic");
      if (ins->fm.block>=1) {
        blockTxt=fmt::sprintf("%d",ins->fm.block-1);
      }

      ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,((ins->type==DIV_INS_ESFM)?0.40f:0.0f));
      ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,((ins->type==DIV_INS_ESFM)?0.25f:0.0f));
      ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,((ins->type==DIV_INS_ESFM)?0.35f:0.0f));

      ImGui::TableNextRow();
      switch (ins->type) {
        case DIV_INS_FM:
        case DIV_INS_OPM:
          ImGui::TableNextColumn();
          P(CWSliderScalar(FM_NAME(FM_FB),ImGuiDataType_U8,&ins->fm.fb,&_ZERO,&_SEVEN)); rightClickable
          P(CWSliderScalar(FM_NAME(FM_FMS),ImGuiDataType_U8,&ins->fm.fms,&_ZERO,&_SEVEN)); rightClickable
          if (ins->type==DIV_INS_FM) {
            P(CWSliderScalar(FM_NAME(FM_BLOCK),ImGuiDataType_U8,&ins->fm.block,&_ZERO,&_EIGHT,blockTxt.c_str())); rightClickable
          }
          ImGui::TableNextColumn();
          P(CWSliderScalar(FM_NAME(FM_ALG),ImGuiDataType_U8,&ins->fm.alg,&_ZERO,&_SEVEN)); rightClickable
          P(CWSliderScalar(FM_NAME(FM_AMS),ImGuiDataType_U8,&ins->fm.ams,&_ZERO,&_THREE)); rightClickable
          ImGui::TableNextColumn();
          if (fmPreviewOn) {
            drawFMPreview(ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
            if (!fmPreviewPaused) {
              renderFMPreview(ins,1);
              WAKE_UP;
            }
          } else {
            drawAlgorithm(ins->fm.alg,FM_ALGS_4OP,ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
          }
          kvsConfig(ins);
          break;
        case DIV_INS_OPZ:
          ImGui::TableNextColumn();
          P(CWSliderScalar(FM_NAME(FM_FB),ImGuiDataType_U8,&ins->fm.fb,&_ZERO,&_SEVEN)); rightClickable
          P(CWSliderScalar(FM_NAME(FM_FMS),ImGuiDataType_U8,&ins->fm.fms,&_ZERO,&_SEVEN)); rightClickable
          P(ImGui::Checkbox("FMS LFO2##FMS",&ins->fm.fmsLFO));
          ImGui::SameLine();
          P(ImGui::Checkbox("AMS LFO2##AMS",&ins->fm.amsLFO));
          ImGui::TableNextColumn();
          P(CWSliderScalar(FM_NAME(FM_ALG),ImGuiDataType_U8,&ins->fm.alg,&_ZERO,&_SEVEN)); rightClickable
          P(CWSliderScalar(FM_NAME(FM_AMS),ImGuiDataType_U8,&ins->fm.ams,&_ZERO,&_THREE)); rightClickable
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted(_("Tremolo"));
          ImGui::SameLine();
          if (ImGui::Button(ins->fm.tremLFO?"LFO4###TLFO":"LFO3###TLFO")) {
            ins->fm.tremLFO=!ins->fm.tremLFO;
            PARAMETER;
          }
          ImGui::TableNextColumn();
          if (fmPreviewOn) {
            drawFMPreview(ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
            if (!fmPreviewPaused) {
              renderFMPreview(ins,1);
              WAKE_UP;
            }
          } else {
            drawAlgorithm(ins->fm.alg,FM_ALGS_4OP,ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
          }
          kvsConfig(ins);

          if (ImGui::Button(_("Request from TX81Z"))) {
            doAction(GUI_ACTION_TX81Z_REQUEST);
          }
          /* 
          ImGui::SameLine();
          if (ImGui::Button("Send to TX81Z")) {
            showError("Coming soon!");
          }
          */
          break;
        case DIV_INS_OPL:
        case DIV_INS_OPL_DRUMS: {
          bool fourOp=(ins->fm.ops==4 || ins->type==DIV_INS_OPL_DRUMS);
          bool drums=ins->fm.opllPreset==16;
          int algMax=fourOp?3:1;
          ImGui::TableNextColumn();
          ins->fm.alg&=algMax;
          P(CWSliderScalar(FM_NAME(FM_FB),ImGuiDataType_U8,&ins->fm.fb,&_ZERO,&_SEVEN)); rightClickable
          P(CWSliderScalar(FM_NAME(FM_BLOCK),ImGuiDataType_U8,&ins->fm.block,&_ZERO,&_EIGHT,blockTxt.c_str())); rightClickable
          ImGui::TableNextColumn();
          P(CWSliderScalar(FM_NAME(FM_ALG),ImGuiDataType_U8,&ins->fm.alg,&_ZERO,&algMax)); rightClickable
          if (ins->type==DIV_INS_OPL) {
            ImGui::BeginDisabled(ins->fm.opllPreset==16);
            if (fourOp || oplType>=3) {
              pushWarningColor(oplType<3);
              if (ImGui::Checkbox("4-op",&fourOp)) { PARAMETER
                ins->fm.ops=fourOp?4:2;
              }
              if (oplType<3 && ImGui::IsItemHovered()) {
                ImGui::SetTooltip(_("4-op mode is not available in OPL1/2!"));
              }
              popWarningColor();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Checkbox(_("Drums"),&drums)) { PARAMETER
              ins->fm.opllPreset=drums?16:0;
            }
          }
          ImGui::TableNextColumn();
          if (fmPreviewOn) {
            drawFMPreview(ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
            if (!fmPreviewPaused) {
              renderFMPreview(ins,1);
              WAKE_UP;
            }
          } else {
            drawAlgorithm(ins->fm.alg&algMax,fourOp?FM_ALGS_4OP_OPL:FM_ALGS_2OP_OPL,ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
          }
          kvsConfig(ins);
          break;
        }
        case DIV_INS_OPLL: {
          bool dc=fmOrigin.fms;
          bool dm=fmOrigin.ams;
          bool sus=ins->fm.alg;
          ImGui::TableNextColumn();
          ImGui::BeginDisabled(ins->fm.opllPreset!=0);
          P(CWSliderScalar(FM_NAME(FM_FB),ImGuiDataType_U8,&fmOrigin.fb,&_ZERO,&_SEVEN)); rightClickable
          ImGui::EndDisabled();
          P(CWSliderScalar(FM_NAME(FM_BLOCK),ImGuiDataType_U8,&ins->fm.block,&_ZERO,&_EIGHT,blockTxt.c_str())); rightClickable
          if (ins->fm.opllPreset!=0) {
            ins->fm.op[1].tl&=15;
            P(CWSliderScalar(_("Volume##TL"),ImGuiDataType_U8,&ins->fm.op[1].tl,&_FIFTEEN,&_ZERO)); rightClickable
          }
          ImGui::TableNextColumn();
          if (ImGui::Checkbox(FM_NAME(FM_SUS),&sus)) { PARAMETER
            ins->fm.alg=sus;
          }
          ImGui::BeginDisabled(ins->fm.opllPreset!=0);
          if (ImGui::Checkbox(FM_NAME(FM_DC),&dc)) { PARAMETER
            fmOrigin.fms=dc;
          }
          ImGui::SameLine();
          if (ImGui::Checkbox(FM_NAME(FM_DM),&dm)) { PARAMETER
            fmOrigin.ams=dm;
          }
          ImGui::EndDisabled();
          ImGui::TableNextColumn();
          if (fmPreviewOn) {
            drawFMPreview(ImVec2(ImGui::GetContentRegionAvail().x,24.0*dpiScale));
            if (!fmPreviewPaused) {
              renderFMPreview(ins,1);
              WAKE_UP;
            }
          } else {
            drawAlgorithm(0,FM_ALGS_2OP_OPL,ImVec2(ImGui::GetContentRegionAvail().x,24.0*dpiScale));
          }
          kvsConfig(ins,false);

          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

          if (ImGui::BeginCombo("##LLPreset",_(opllInsNames[presentWhich][ins->fm.opllPreset]))) {
            if (isPresentCount>1) {
              if (ImGui::BeginTable("LLPresetList",isPresentCount)) {
                ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                for (int i=0; i<4; i++) {
                  if (!isPresent[i]) continue;
                  ImGui::TableNextColumn();
                  ImGui::Text(_("%s name"),opllVariants[i]);
                }
                for (int i=0; i<17; i++) {
                  ImGui::TableNextRow();
                  for (int j=0; j<4; j++) {
                    if (!isPresent[j]) continue;
                    ImGui::TableNextColumn();
                    ImGui::PushID(j*17+i);
                    if (ImGui::Selectable(_(opllInsNames[j][i]),ins->fm.opllPreset==i)) { PARAMETER
                      ins->fm.opllPreset=i;
                    }
                    ImGui::PopID();
                    if (ins->fm.opllPreset==i) ImGui::SetItemDefaultFocus();
                  }
                }
                ImGui::EndTable();
              }
            } else {
              for (int i=0; i<17; i++) {
                if (ImGui::Selectable(_(opllInsNames[presentWhich][i]),ins->fm.opllPreset==i)) { PARAMETER
                  ins->fm.opllPreset=i;
                }
                if (ins->fm.opllPreset==i) ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }
          break;
        }
        case DIV_INS_ESFM: {
          ImGui::TableNextColumn();
          P(CWSliderScalar(ESFM_LONG_NAME(ESFM_NOISE),ImGuiDataType_U8,&ins->esfm.noise,&_ZERO,&_THREE,_(esfmNoiseModeNames[ins->esfm.noise&3]))); rightClickable
          ImGui::TextUnformatted(_(esfmNoiseModeDescriptions[ins->esfm.noise&3]));
          ImGui::TableNextColumn();
          P(CWSliderScalar(FM_NAME(FM_BLOCK),ImGuiDataType_U8,&ins->fm.block,&_ZERO,&_EIGHT,blockTxt.c_str())); rightClickable
          ImGui::TableNextColumn();
          if (fmPreviewOn) {
            drawFMPreview(ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
            if (!fmPreviewPaused) {
              renderFMPreview(ins,1);
              WAKE_UP;
            }
          } else {
            drawESFMAlgorithm(ins->esfm, ImVec2(ImGui::GetContentRegionAvail().x,48.0*dpiScale));
          }
          kvsConfig(ins);
        }
        default:
          break;
      }
      ImGui::EndTable();
    }
    if (ins->type==DIV_INS_OPLL && ins->fm.opllPreset==16) {
      ImGui::Text(_("this volume slider only works in compatibility (non-drums) system."));
    }

    if (((ins->type==DIV_INS_OPLL || ins->type==DIV_INS_OPL) && ins->fm.opllPreset==16) || ins->type==DIV_INS_OPL_DRUMS) {
      ins->fm.ops=2;
      P(ImGui::Checkbox(_("Fixed frequency mode"),&ins->fm.fixedDrums));
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_("when enabled, drums will be set to the specified frequencies, ignoring the note."));
      }
      if (ins->fm.fixedDrums) {
        int block=0;
        int fNum=0;
        if (ImGui::BeginTable("fixedDrumSettings",3)) {
          ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
          ImGui::TableNextColumn();
          ImGui::Text(_("Drum"));
          ImGui::TableNextColumn();
          ImGui::Text(_("Block"));
          ImGui::TableNextColumn();
          ImGui::Text(_("FreqNum"));

          DRUM_FREQ(_("Kick"),"##DBlock0","##DFreq0",ins->fm.kickFreq);
          DRUM_FREQ(_("Snare/Hi-hat"),"##DBlock1","##DFreq1",ins->fm.snareHatFreq);
          DRUM_FREQ(_("Tom/Top"),"##DBlock2","##DFreq2",ins->fm.tomTopFreq);
          ImGui::EndTable();
        }
      }
    }

    bool willDisplayOps=true;
    if (ins->type==DIV_INS_OPLL && ins->fm.opllPreset!=0) willDisplayOps=false;
    if (!willDisplayOps && ins->type==DIV_INS_OPLL) {
      // update OPLL preset preview
      if (ins->fm.opllPreset>0 && ins->fm.opllPreset<16) {
        const opll_patch_t* patchROM=NULL;

        switch (presentWhich) {
          case 1:
            patchROM=OPLL_GetPatchROM(opll_type_ymf281);
            break;
          case 2:
            patchROM=OPLL_GetPatchROM(opll_type_ym2423);
            break;
          case 3:
            patchROM=OPLL_GetPatchROM(opll_type_ds1001);
            break;
          default:
            patchROM=OPLL_GetPatchROM(opll_type_ym2413);
            break;
        }

        const opll_patch_t* patch=&patchROM[ins->fm.opllPreset-1];

        opllPreview.alg=ins->fm.alg;
        opllPreview.fb=patch->fb;
        opllPreview.fms=patch->dc;
        opllPreview.ams=patch->dm;

        opllPreview.op[0].tl=patch->tl;
        opllPreview.op[1].tl=ins->fm.op[1].tl;

        for (int i=0; i<2; i++) {
          opllPreview.op[i].am=patch->am[i];
          opllPreview.op[i].vib=patch->vib[i];
          opllPreview.op[i].ssgEnv=patch->et[i]?8:0;
          opllPreview.op[i].ksr=patch->ksr[i];
          opllPreview.op[i].ksl=patch->ksl[i];
          opllPreview.op[i].mult=patch->multi[i];
          opllPreview.op[i].ar=patch->ar[i];
          opllPreview.op[i].dr=patch->dr[i];
          opllPreview.op[i].sl=patch->sl[i];
          opllPreview.op[i].rr=patch->rr[i];
        }
      }
    }

    ImGui::BeginDisabled(!willDisplayOps);
    if (settings.fmLayout==0 || settings.fmLayout==7) { // modern (why didn't I comment this?!)
      int numCols=15;
      if (ins->type==DIV_INS_OPL ||ins->type==DIV_INS_OPL_DRUMS) numCols=13;
      if (ins->type==DIV_INS_OPLL) numCols=12;
      if (ins->type==DIV_INS_OPZ) numCols=20;
      if (ins->type==DIV_INS_ESFM) numCols=19;
      if (ImGui::BeginTable("FMOperators",numCols,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_BordersH|ImGuiTableFlags_BordersOuterV)) {
        // configure columns
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed); // op name
        if (ins->type==DIV_INS_ESFM) {
          ImGui::TableSetupColumn("c0e0",ImGuiTableColumnFlags_WidthStretch,0.05f); // outLvl
          ImGui::TableSetupColumn("c0e1",ImGuiTableColumnFlags_WidthFixed); // -separator-
          ImGui::TableSetupColumn("c0e2",ImGuiTableColumnFlags_WidthStretch,0.05f); // delay
        }
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.05f); // ar
        ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,0.05f); // dr
        if (settings.susPosition==0) {
          ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.05f); // sl
        }
        if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
          ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthStretch,0.05f); // d2r
        }
        ImGui::TableSetupColumn("c5",ImGuiTableColumnFlags_WidthStretch,0.05f); // rr
        if (settings.susPosition==1) {
          ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.05f); // sl
        }
        ImGui::TableSetupColumn("c6",ImGuiTableColumnFlags_WidthFixed); // -separator-
        if (settings.susPosition==2) {
          ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.05f); // sl
        }
        ImGui::TableSetupColumn("c7",ImGuiTableColumnFlags_WidthStretch,0.05f); // tl
        if (settings.susPosition==3) {
          ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthStretch,0.05f); // sl
        }
        ImGui::TableSetupColumn("c8",ImGuiTableColumnFlags_WidthStretch,0.05f); // rs/ksl
        if (ins->type==DIV_INS_OPZ) {
          ImGui::TableSetupColumn("c8z0",ImGuiTableColumnFlags_WidthStretch,0.05f); // egs
          ImGui::TableSetupColumn("c8z1",ImGuiTableColumnFlags_WidthStretch,0.05f); // rev
          ImGui::TableSetupColumn("c8z2",ImGuiTableColumnFlags_WidthStretch,0.05f); // ts
        }
        if (ins->type==DIV_INS_ESFM) {
          ImGui::TableSetupColumn("c8e0",ImGuiTableColumnFlags_WidthStretch,0.05f); // outLvl
        }
        ImGui::TableSetupColumn("c9",ImGuiTableColumnFlags_WidthStretch,0.05f); // mult

        if (ins->type==DIV_INS_OPZ) {
          ImGui::TableSetupColumn("c9z",ImGuiTableColumnFlags_WidthStretch,0.05f); // fine
        }

        if (ins->type==DIV_INS_ESFM) {
          ImGui::TableSetupColumn("c9e",ImGuiTableColumnFlags_WidthStretch,0.05f); // ct
        }

        if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM || ins->type==DIV_INS_ESFM) {
          ImGui::TableSetupColumn("c10",ImGuiTableColumnFlags_WidthStretch,0.05f); // dt
        }
        if (ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
          ImGui::TableSetupColumn("c11",ImGuiTableColumnFlags_WidthStretch,0.05f); // dt2
        }
        ImGui::TableSetupColumn("c15",ImGuiTableColumnFlags_WidthFixed); // am

        ImGui::TableSetupColumn("c12",ImGuiTableColumnFlags_WidthFixed); // -separator-
        if (ins->type!=DIV_INS_OPLL && ins->type!=DIV_INS_OPM) {
          ImGui::TableSetupColumn("c13",ImGuiTableColumnFlags_WidthStretch,0.2f); // ssg/waveform
        }
        ImGui::TableSetupColumn("c14",ImGuiTableColumnFlags_WidthStretch,0.3f); // env

        float sliderHeight=((ImGui::GetContentRegionAvail().y-ImGui::GetFrameHeightWithSpacing()*(settings.fmLayout==7?4.0f:1.0f))/opCount)-ImGui::GetStyle().ItemSpacing.y;

        // header
        if (settings.fmLayout==0) {
          insTabFMModernHeader(ins);
        }

        // main view
        for (int i=0; i<opCount; i++) {
          DivInstrumentFM::Operator& op=fmOrigin.op[(opCount==4 && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_ESFM)?opOrder[i]:i];
          DivInstrumentESFM::Operator& opE=ins->esfm.op[i];

          // modern with more labels
          if (settings.fmLayout==7) {
            insTabFMModernHeader(ins);
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();

          // push colors
          if (settings.separateFMColors) {
            bool mod=true;
            if (ins->type==DIV_INS_OPL_DRUMS) {
              mod=false;
            } else if (ins->type==DIV_INS_ESFM) {
              // this is the same as the KVS heuristic in platform/esfm.h
              if (opE.outLvl==7) {
                mod=false;
              } else if (opE.outLvl>0) {
                if (i==3) {
                  mod=false;
                } else {
                  DivInstrumentESFM::Operator& opENext=ins->esfm.op[i+1];
                  if (opENext.modIn==0) {
                    mod=false;
                  } else if ((opE.outLvl-opENext.modIn)>=2) {
                    mod=false;
                  }
                }
              }
            } else if (opCount==4) {
              if (ins->type==DIV_INS_OPL) {
                if (opIsOutputOPL[fmOrigin.alg&3][i]) mod=false;
              } else {
                if (opIsOutput[fmOrigin.alg&7][i]) mod=false;
              }
            } else {
              if (i==1 || (ins->type==DIV_INS_OPL && (fmOrigin.alg&1))) mod=false;
            }
            if (mod) {
              pushAccentColors(
                uiColors[GUI_COLOR_FM_PRIMARY_MOD],
                uiColors[GUI_COLOR_FM_SECONDARY_MOD],
                uiColors[GUI_COLOR_FM_BORDER_MOD],
                uiColors[GUI_COLOR_FM_BORDER_SHADOW_MOD]
              );
            } else {
              pushAccentColors(
                uiColors[GUI_COLOR_FM_PRIMARY_CAR],
                uiColors[GUI_COLOR_FM_SECONDARY_CAR],
                uiColors[GUI_COLOR_FM_BORDER_CAR],
                uiColors[GUI_COLOR_FM_BORDER_SHADOW_CAR]
              );
            }
          }

          if (i==0) {
            float sliderMinHeightOPL=ImGui::GetFrameHeight()*4.0+ImGui::GetStyle().ItemSpacing.y*3.0;
            float sliderMinHeightESFM=ImGui::GetFrameHeight()*5.0+ImGui::GetStyle().ItemSpacing.y*4.0;
            if ((ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPLL) && sliderHeight<sliderMinHeightOPL) {
              sliderHeight=sliderMinHeightOPL;
            }
            if (ins->type==DIV_INS_ESFM && sliderHeight<sliderMinHeightESFM) {
              sliderHeight=sliderMinHeightESFM;
            }
          }

          ImGui::PushID(fmt::sprintf("op%d",i).c_str());
          String opNameLabel;
          if (ins->type==DIV_INS_OPL_DRUMS) {
            opNameLabel=fmt::sprintf("%s",oplDrumNames[i]);
          } else if (ins->type==DIV_INS_OPL && fmOrigin.opllPreset==16) {
            if (i==1) {
              opNameLabel=_("Kick");
            } else {
              opNameLabel=_("Env");
            }
          } else {
            opNameLabel=fmt::sprintf("OP%d",i+1);
          }
          if (opsAreMutable) {
            pushToggleColors(op.enable);
            if (ImGui::Button(opNameLabel.c_str())) {
              op.enable=!op.enable;
              PARAMETER;
            }
            popToggleColors();
          } else {
            ImGui::TextUnformatted(opNameLabel.c_str());
          }

          // drag point
          OP_DRAG_POINT;

          int maxTl=127;
          if (ins->type==DIV_INS_OPLL) {
            if (i==1) {
              maxTl=15;
            } else {
              maxTl=63;
            }
          }
          if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_ESFM) {
            maxTl=63;
          }
          int maxArDr=(ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM)?31:15;
          bool ssgOn=op.ssgEnv&8;
          bool ksrOn=op.ksr;
          bool vibOn=op.vib;
          bool susOn=op.sus;
          bool fixedOn=opE.fixed;
          unsigned char ssgEnv=op.ssgEnv&7;

          if (ins->type==DIV_INS_ESFM) {
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##MODIN",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&opE.modIn,&_ZERO,&_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            ImGui::Dummy(ImVec2(4.0f*dpiScale,2.0f*dpiScale));
            ImGui::TableNextColumn();
            opE.delay&=7;
            CENTER_VSLIDER;
            P(CWVSliderScalar("##DELAY",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&opE.delay,&_ZERO,&_SEVEN)); rightClickable
          }

          ImGui::TableNextColumn();
          op.ar&=maxArDr;
          CENTER_VSLIDER;
          P(CWVSliderScalar("##AR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.ar,&maxArDr,&_ZERO)); rightClickable

          ImGui::TableNextColumn();
          op.dr&=maxArDr;
          CENTER_VSLIDER;
          P(CWVSliderScalar("##DR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.dr,&maxArDr,&_ZERO)); rightClickable

          if (settings.susPosition==0) {
            ImGui::TableNextColumn();
            op.sl&=15;
            CENTER_VSLIDER;
            P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
          }

          if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
            ImGui::TableNextColumn();
            op.d2r&=31;
            CENTER_VSLIDER;
            P(CWVSliderScalar("##D2R",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.d2r,&_THIRTY_ONE,&_ZERO)); rightClickable
          }

          ImGui::TableNextColumn();
          op.rr&=15;
          CENTER_VSLIDER;
          P(CWVSliderScalar("##RR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.rr,&_FIFTEEN,&_ZERO)); rightClickable

          if (settings.susPosition==1) {
            ImGui::TableNextColumn();
            op.sl&=15;
            CENTER_VSLIDER;
            P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
          }

          ImGui::TableNextColumn();
          ImGui::Dummy(ImVec2(4.0f*dpiScale,2.0f*dpiScale));

          if (settings.susPosition==2) {
            ImGui::TableNextColumn();
            op.sl&=15;
            CENTER_VSLIDER;
            P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
          }

          ImGui::TableNextColumn();
          op.tl&=maxTl;
          CENTER_VSLIDER;
          P(CWVSliderScalar("##TL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.tl,&maxTl,&_ZERO)); rightClickable

          if (settings.susPosition==3) {
            ImGui::TableNextColumn();
            op.sl&=15;
            CENTER_VSLIDER;
            P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
          }

          ImGui::TableNextColumn();
          CENTER_VSLIDER;
          if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
            P(CWVSliderScalar("##RS",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.rs,&_ZERO,&_THREE)); rightClickable
          } else {
            int ksl=ins->type==DIV_INS_OPLL?op.ksl:kslMap[op.ksl&3];
            if (CWVSliderInt("##KSL",ImVec2(20.0f*dpiScale,sliderHeight),&ksl,0,3)) {
              op.ksl=(ins->type==DIV_INS_OPLL?ksl:kslMap[ksl&3]);
              PARAMETER;
            } rightClickable
          }

          if (ins->type==DIV_INS_OPZ) {
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##EGS",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.ksl,&_ZERO,&_THREE)); rightClickable

            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##REV",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.dam,&_ZERO,&_SEVEN)); rightClickable

            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##TS",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.ssgEnv,&_ZERO,&_THREE)); rightClickable
          }

          if (ins->type==DIV_INS_ESFM) {
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##OUTLVL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&opE.outLvl,&_ZERO,&_SEVEN)); rightClickable
          }

          ImGui::TableNextColumn();
          pushWarningColor(ins->type==DIV_INS_OPL_DRUMS && i==0);
          CENTER_VSLIDER;
          P(CWVSliderScalar("##MULT",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN)); rightClickable
          if (ins->type==DIV_INS_OPL_DRUMS && i==0) {
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s",_("Snare's multiplier is determined by HiHat's."));
            }
          }
          popWarningColor();

          if (ins->type==DIV_INS_OPZ) {
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            bool egtOn=op.egt;
            if (!egtOn) {
              P(CWVSliderScalar("##FINE",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.dvb,&_ZERO,&_FIFTEEN)); rightClickable
            }
          }

          if (ins->type==DIV_INS_ESFM) {
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##CT",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_S8,&opE.ct,&_MINUS_TWENTY_FOUR,&_TWENTY_FOUR)); rightClickable
          }

          if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
            int detune=detuneMap[settings.unsignedDetune?1:0][op.dt&7];
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            if (CWVSliderInt("##DT",ImVec2(20.0f*dpiScale,sliderHeight),&detune,settings.unsignedDetune?0:-3,settings.unsignedDetune?7:4)) { PARAMETER
              if (detune<-3) detune=-3;
              if (detune>7) detune=7;
              op.dt=detuneUnmap[settings.unsignedDetune?1:0][detune+3];
            } rightClickable

            if (ins->type!=DIV_INS_FM) {
              ImGui::TableNextColumn();
              CENTER_VSLIDER;
              P(CWVSliderScalar("##DT2",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.dt2,&_ZERO,&_THREE)); rightClickable
            }

            ImGui::TableNextColumn();
            bool amOn=op.am;
            if (ins->type==DIV_INS_OPZ) {
              bool egtOn=op.egt;
              bool susOn=op.sus;
              if (egtOn) {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+0.5*(sliderHeight-ImGui::GetFrameHeight()*6.0-ImGui::GetStyle().ItemSpacing.y*3.5));
              } else {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+0.5*(sliderHeight-ImGui::GetFrameHeight()*4.0-ImGui::GetStyle().ItemSpacing.y*1.0));
              }
              if (ImGui::Checkbox("AM",&amOn)) { PARAMETER
                op.am=amOn;
              }
              if (ImGui::Checkbox(_("TL Ramp##TLRamp"),&ksrOn)) { PARAMETER
                op.ksr=ksrOn;
              }
              if (ImGui::Checkbox(_("Fixed"),&egtOn)) { PARAMETER
                op.egt=egtOn;
              }
              if (egtOn) {
                pushWarningColor(susOn && !e->song.compatFlags.linearPitch);
                if (ImGui::Checkbox(_("Pitch control"),&susOn)) { PARAMETER
                  op.sus=susOn;
                  // HACK: reset zoom and scroll in fixed pitch macros so that they draw correctly
                  ins->temp.vZoom[DIV_MACRO_OP_SSG+(i<<5)]=-1;
                  ins->temp.vZoom[DIV_MACRO_OP_SUS+(i<<5)]=-1;
                }
                popWarningColor();
                if (ImGui::IsItemHovered()) {
                  if (susOn && !e->song.compatFlags.linearPitch) {
                    ImGui::SetTooltip(_("only works on linear pitch! go to Compatibility Flags > Pitch/Playback and set Pitch linearity to Full."));
                  } else {
                    ImGui::SetTooltip(_("use op's arpeggio and pitch macros control instead of block/f-num macros"));
                  }
                }
              }
              if (egtOn && !susOn) {
                int block=op.dt;
                int freqNum=(op.mult<<4)|(op.dvb&15);
                if (ImGui::InputInt(_("Block"),&block,1,1)) {
                  if (block<0) block=0;
                  if (block>7) block=7;
                  op.dt=block;
                }
                if (ImGui::InputInt(_("FreqNum"),&freqNum,1,16)) {
                  if (freqNum<0) freqNum=0;
                  if (freqNum>255) freqNum=255;
                  op.mult=freqNum>>4;
                  op.dvb=freqNum&15;
                }
              }
            } else {
              // disappoint me!!!
              bool displayTLRamp=false;
              if (ins->type==DIV_INS_OPM) {
                displayTLRamp=false;
                for (int i=0; i<e->song.systemLen; i++) {
                  if (e->song.system[i]==DIV_SYSTEM_YM2151) {
                    // test for YM2164 (OPP)
                    if (e->song.systemFlags[i].getInt("chipType",0)==1) {
                      displayTLRamp=true;
                      break;
                    }
                  }
                }
              }
              if (displayTLRamp) {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+0.5*(sliderHeight-ImGui::GetFrameHeight()*2.0-ImGui::GetStyle().ItemSpacing.y*1.0));
                if (ImGui::Checkbox("AM##AM",&amOn)) { PARAMETER
                  op.am=amOn;
                }
                // why does the text run away?!
                if (ImGui::Checkbox(_("TL Ramp##TLRamp"),&ksrOn)) { PARAMETER
                  op.ksr=ksrOn;
                }
              } else {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+0.5*(sliderHeight-ImGui::GetFrameHeight()));
                if (ImGui::Checkbox("##AM",&amOn)) { PARAMETER
                  op.am=amOn;
                }
              }
            }

            if (ins->type!=DIV_INS_OPL && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_OPZ && ins->type!=DIV_INS_OPM) {
              ImGui::TableNextColumn();
              ImGui::Dummy(ImVec2(4.0f*dpiScale,2.0f*dpiScale));
              ImGui::TableNextColumn();
              ImGui::BeginDisabled(!ssgOn);
              drawSSGEnv(op.ssgEnv&7,ImVec2(ImGui::GetContentRegionAvail().x,sliderHeight-ImGui::GetFrameHeightWithSpacing()));
              ImGui::EndDisabled();
              if (ImGui::Checkbox("##SSGOn",&ssgOn)) { PARAMETER
                op.ssgEnv=(op.ssgEnv&7)|(ssgOn<<3);
              }

              ImGui::SameLine();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              if (CWSliderScalar("##SSG",ImGuiDataType_U8,&ssgEnv,&_ZERO,&_SEVEN,_(ssgEnvTypes[ssgEnv]))) { PARAMETER
                op.ssgEnv=(op.ssgEnv&8)|(ssgEnv&7);
              }
            }
          } else if (ins->type==DIV_INS_ESFM) {
            ImGui::TableNextColumn();
            CENTER_VSLIDER;
            P(CWVSliderScalar("##DT",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_S8,&opE.dt,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable

            ImGui::TableNextColumn();
            bool amOn=op.am;
            bool leftOn=opE.left;
            bool rightOn=opE.right;

            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+0.5*(sliderHeight-ImGui::GetFrameHeight()*5.0-ImGui::GetStyle().ItemSpacing.y*4.0));
            ImVec2 curPosBeforeDummy = ImGui::GetCursorPos();
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeightWithSpacing()*2.0f+ImGui::CalcTextSize(FM_SHORT_NAME(FM_DAM)).x*2.0f,1.0f));
            ImGui::SetCursorPos(curPosBeforeDummy);

            if (ImGui::BeginTable("panCheckboxes",(fixedOn)?3:2,ImGuiTableFlags_SizingStretchProp)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,1.0);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,1.0);
              if (fixedOn) {
                ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,1.2);
              }

              float yCoordBeforeTablePadding=ImGui::GetCursorPosY();
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetCursorPosY(yCoordBeforeTablePadding);
              if (ImGui::Checkbox(ESFM_SHORT_NAME(ESFM_LEFT),&leftOn)) { PARAMETER
                opE.left=leftOn;
              }
              ImGui::TableNextColumn();
              ImGui::SetCursorPosY(yCoordBeforeTablePadding);
              if (ImGui::Checkbox(ESFM_SHORT_NAME(ESFM_RIGHT),&rightOn)) { PARAMETER
                opE.right=rightOn;
              }
              if (fixedOn) {
                ImGui::TableNextColumn();
                ImGui::SetCursorPosY(yCoordBeforeTablePadding);
                if (ImGui::Checkbox(FM_SHORT_NAME(FM_AM),&amOn)) { PARAMETER
                  op.am=amOn;
                }
              }
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              if (ImGui::Checkbox(FM_SHORT_NAME(FM_KSR),&ksrOn)) { PARAMETER
                op.ksr=ksrOn;
              }
              ImGui::TableNextColumn();
              if (ImGui::Checkbox(FM_SHORT_NAME(FM_SUS),&susOn)) { PARAMETER
                op.sus=susOn;
              }
              if (fixedOn) {
                bool damOn=op.dam;
                ImGui::TableNextColumn();
                if (ImGui::Checkbox(FM_SHORT_NAME(FM_DAM),&damOn)) { PARAMETER
                  op.dam=damOn;
                }
              }
              ImGui::EndTable();
            }
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()-0.5*ImGui::GetStyle().ItemSpacing.y);
            if (ImGui::Checkbox(ESFM_NAME(ESFM_FIXED),&fixedOn)) { PARAMETER
              opE.fixed=fixedOn;
              // HACK: reset zoom and scroll in fixed pitch macros so that they draw correctly
              ins->temp.vZoom[DIV_MACRO_OP_SSG+(i<<5)]=-1;
              ins->temp.vZoom[DIV_MACRO_OP_DT+(i<<5)]=-1;
            }
            if (ins->type==DIV_INS_ESFM) {
              if (fixedOn) {
                int block=(opE.ct>>2)&7;
                int freqNum=((opE.ct&3)<<8)|((unsigned char)opE.dt);
                if (ImGui::InputInt(_("Block"),&block,1,1)) {
                  if (block<0) block=0;
                  if (block>7) block=7;
                  opE.ct=(opE.ct&(~(7<<2)))|(block<<2);
                }
                if (ImGui::InputInt(_("FreqNum"),&freqNum,1,16)) {
                  if (freqNum<0) freqNum=0;
                  if (freqNum>1023) freqNum=1023;
                  opE.dt=freqNum&0xff;
                  opE.ct=(opE.ct&(~3))|(freqNum>>8);
                }
              } else {
                if (ImGui::BeginTable("amVibCheckboxes",2,ImGuiTableFlags_SizingStretchSame)) {
                  ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0);
                  ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.0);

                  float yCoordBeforeTablePadding=ImGui::GetCursorPosY();
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  ImGui::SetCursorPosY(yCoordBeforeTablePadding);
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_AM),&amOn)) { PARAMETER
                    op.am=amOn;
                  }
                  ImGui::TableNextColumn();
                  ImGui::SetCursorPosY(yCoordBeforeTablePadding);
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_VIB),&vibOn)) { PARAMETER
                    op.vib=vibOn;
                  }
                  bool damOn=op.dam;
                  bool dvbOn=op.dvb;
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_DAM),&damOn)) { PARAMETER
                    op.dam=damOn;
                  }
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_DVB),&dvbOn)) { PARAMETER
                    op.dvb=dvbOn;
                  }
                  ImGui::EndTable();
                }
              }
            }
          } else if (ins->type!=DIV_INS_OPM) {
            ImGui::TableNextColumn();
            bool amOn=op.am;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+0.5*(sliderHeight-ImGui::GetFrameHeight()*4.0-ImGui::GetStyle().ItemSpacing.y*3.0));
            if (ImGui::Checkbox(FM_NAME(FM_AM),&amOn)) { PARAMETER
              op.am=amOn;
            }
            if (ImGui::Checkbox(FM_NAME(FM_VIB),&vibOn)) { PARAMETER
              op.vib=vibOn;
            }
            if (ImGui::Checkbox(FM_NAME(FM_KSR),&ksrOn)) { PARAMETER
              op.ksr=ksrOn;
            }
            if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS) {
              if (ImGui::Checkbox(FM_NAME(FM_SUS),&susOn)) { PARAMETER
                op.sus=susOn;
              }
            } else if (ins->type==DIV_INS_OPLL) {
              if (ImGui::Checkbox(FM_NAME(FM_EGS),&ssgOn)) { PARAMETER
                op.ssgEnv=(op.ssgEnv&7)|(ssgOn<<3);
              }
            }
          }

          if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_ESFM) {
            ImGui::TableNextColumn();
            ImGui::Dummy(ImVec2(4.0f*dpiScale,2.0f*dpiScale));
            ImGui::TableNextColumn();

            drawWaveform(op.ws&wsMax,ins->type==DIV_INS_OPZ,ImVec2(ImGui::GetContentRegionAvail().x,sliderHeight-ImGui::GetFrameHeightWithSpacing()*((ins->type==DIV_INS_ESFM && fixedOn)?3.0f:1.0f)));
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##WS",ImGuiDataType_U8,&op.ws,&_ZERO,&wsMax,(ins->type==DIV_INS_OPZ)?opzWaveforms[op.ws&wsMax]:(settings.oplStandardWaveNames?oplWaveformsStandard[op.ws&wsMax]:oplWaveforms[op.ws&wsMax]))); rightClickable
            if (ins->type==DIV_INS_ESFM && fixedOn) {
              if (ImGui::Checkbox(FM_SHORT_NAME(FM_VIB),&vibOn)) { PARAMETER
                op.vib=vibOn;
              }
              bool dvbOn=op.dvb;
              if (ImGui::Checkbox(FM_SHORT_NAME(FM_DVB),&dvbOn)) { PARAMETER
                op.dvb=dvbOn;
              }
            }
          } else if (ins->type==DIV_INS_OPLL || ins->type==DIV_INS_OPM) {
            ImGui::TableNextColumn();
            ImGui::Dummy(ImVec2(4.0f*dpiScale,2.0f*dpiScale));
          }

          ImGui::TableNextColumn();
          drawFMEnv(op.tl&maxTl,op.ar&maxArDr,op.dr&maxArDr,(ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPLL || ins->type==DIV_INS_ESFM)?((op.rr&15)*2):op.d2r&31,op.rr&15,op.sl&15,op.sus,op.ssgEnv&8,fmOrigin.alg,maxTl,maxArDr,15,ImVec2(ImGui::GetContentRegionAvail().x,sliderHeight),ins->type);

          if (settings.separateFMColors) {
            popAccentColors();
          }

          ImGui::PopID();
        }

        ImGui::EndTable();
      }
    } else if (settings.fmLayout>=4 && settings.fmLayout<=6) { // alternate
      int columns=2;
      switch (settings.fmLayout) {
        case 4: // 2x2
          columns=2;
          break;
        case 5: // 1x4
          columns=1;
          break;
        case 6: // 4x1
          columns=opCount;
          break;
      }
      char tempID[1024];
      ImVec2 oldPadding=ImGui::GetStyle().CellPadding;
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(8.0f*dpiScale,4.0f*dpiScale));
      if (ImGui::BeginTable("AltFMOperators",columns,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInner)) {
        for (int i=0; i<opCount; i++) {
          DivInstrumentFM::Operator& op=fmOrigin.op[(opCount==4 && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_ESFM)?opOrder[i]:i];
          DivInstrumentESFM::Operator& opE=ins->esfm.op[i];
          if ((settings.fmLayout!=6 && ((i+1)&1)) || i==0 || settings.fmLayout==5) ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::PushID(fmt::sprintf("op%d",i).c_str());

          // push colors
          if (settings.separateFMColors) {
            bool mod=true;
            if (ins->type==DIV_INS_OPL_DRUMS) {
              mod=false;
            } else if (ins->type==DIV_INS_ESFM) {
              // this is the same as the KVS heuristic in platform/esfm.h
              if (opE.outLvl==7) mod=false;
              else if (opE.outLvl>0) {
                if (i==3) mod=false;
                else {
                  DivInstrumentESFM::Operator& opENext=ins->esfm.op[i+1];
                  if (opENext.modIn==0) mod=false;
                  else if ((opE.outLvl-opENext.modIn)>=2) mod=false;
                }
              }
            } else if (opCount==4) {
              if (ins->type==DIV_INS_OPL) {
                if (opIsOutputOPL[fmOrigin.alg&3][i]) mod=false;
              } else {
                if (opIsOutput[fmOrigin.alg&7][i]) mod=false;
              }
            } else {
              if (i==1 || (ins->type==DIV_INS_OPL && (fmOrigin.alg&1))) mod=false;
            }
            if (mod) {
              pushAccentColors(
                uiColors[GUI_COLOR_FM_PRIMARY_MOD],
                uiColors[GUI_COLOR_FM_SECONDARY_MOD],
                uiColors[GUI_COLOR_FM_BORDER_MOD],
                uiColors[GUI_COLOR_FM_BORDER_SHADOW_MOD]
              );
            } else {
              pushAccentColors(
                uiColors[GUI_COLOR_FM_PRIMARY_CAR],
                uiColors[GUI_COLOR_FM_SECONDARY_CAR],
                uiColors[GUI_COLOR_FM_BORDER_CAR],
                uiColors[GUI_COLOR_FM_BORDER_SHADOW_CAR]
              );
            }
          }

          ImGui::Dummy(ImVec2(dpiScale,dpiScale));
          if (ins->type==DIV_INS_OPL_DRUMS) {
            snprintf(tempID,1024,"%s",oplDrumNames[i]);
          } else if (ins->type==DIV_INS_OPL && fmOrigin.opllPreset==16) {
            if (i==1) {
              snprintf(tempID,1024,_("Envelope 2 (kick only)"));
            } else {
              snprintf(tempID,1024,_("Envelope"));
            }
          } else {
            snprintf(tempID,1024,_("Operator %d"),i+1);
          }
          float nextCursorPosX=ImGui::GetCursorPosX()+0.5*(ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(tempID).x-(opsAreMutable?(ImGui::GetStyle().FramePadding.x*2.0f):0.0f));
          OP_DRAG_POINT;
          ImGui::SameLine();
          ImGui::SetCursorPosX(nextCursorPosX);
          if (opsAreMutable) {
            pushToggleColors(op.enable);
            if (ImGui::Button(tempID)) {
              op.enable=!op.enable;
              PARAMETER;
            }
            popToggleColors();
          } else {
            ImGui::TextUnformatted(tempID);
          }

          float sliderHeight=200.0f*dpiScale;
          float waveWidth=140.0*dpiScale*((ins->type==DIV_INS_ESFM)?0.85f:1.0f);
          float waveHeight=sliderHeight-ImGui::GetFrameHeightWithSpacing()*((ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPL || ins->type==DIV_INS_ESFM)?5.0f:4.5f);

          int maxTl=127;
          if (ins->type==DIV_INS_OPLL) {
            if (i==1) {
              maxTl=15;
            } else {
              maxTl=63;
            }
          }
          if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_ESFM) {
            maxTl=63;
          }
          int maxArDr=(ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM)?31:15;

          bool ssgOn=op.ssgEnv&8;
          bool ksrOn=op.ksr;
          bool vibOn=op.vib;
          bool egtOn=op.egt;
          bool susOn=op.sus; // yawn
          unsigned char ssgEnv=op.ssgEnv&7;

          ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,oldPadding);
          if (ImGui::BeginTable("opParams",4,ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,waveWidth);
            ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            float textY=ImGui::GetCursorPosY();
            if (ins->type==DIV_INS_ESFM) {
              CENTER_TEXT_20(ESFM_SHORT_NAME(ESFM_DELAY));
              ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_DELAY));
              TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_DELAY));
            } else {
              CENTER_TEXT_20(FM_SHORT_NAME(FM_AR));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_AR));
              TOOLTIP_TEXT(FM_NAME(FM_AR));
            }
            ImGui::TableNextColumn();
            if (ins->type==DIV_INS_FM) {
              ImGui::Text(_("SSG-EG"));
            } else if (ins->type!=DIV_INS_OPM) {
              ImGui::Text(_("Waveform"));
            }
            ImGui::TableNextColumn();
            ImGui::Text(_("Envelope"));
            ImGui::TableNextColumn();

            // A/D/S/R
            ImGui::TableNextColumn();

            if (ins->type==DIV_INS_ESFM) {
              opE.delay&=7;
              P(CWVSliderScalar("##DELAY",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&opE.delay,&_ZERO,&_SEVEN)); rightClickable
              ImGui::SameLine();
            }

            op.ar&=maxArDr;
            float textX_AR=ImGui::GetCursorPosX();
            P(CWVSliderScalar("##AR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.ar,&maxArDr,&_ZERO)); rightClickable

            ImGui::SameLine();
            op.dr&=maxArDr;
            float textX_DR=ImGui::GetCursorPosX();
            P(CWVSliderScalar("##DR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.dr,&maxArDr,&_ZERO)); rightClickable

            float textX_SL=0.0f;
            if (settings.susPosition==0) {
              ImGui::SameLine();
              op.sl&=15;
              textX_SL=ImGui::GetCursorPosX();
              P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
            }

            float textX_D2R=0.0f;
            if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
              ImGui::SameLine();
              op.d2r&=31;
              textX_D2R=ImGui::GetCursorPosX();
              P(CWVSliderScalar("##D2R",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.d2r,&_THIRTY_ONE,&_ZERO)); rightClickable
            }

            ImGui::SameLine();
            op.rr&=15;
            float textX_RR=ImGui::GetCursorPosX();
            P(CWVSliderScalar("##RR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.rr,&_FIFTEEN,&_ZERO)); rightClickable

            if (settings.susPosition>0) {
              ImGui::SameLine();
              op.sl&=15;
              textX_SL=ImGui::GetCursorPosX();
              P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
            }

            ImVec2 prevCurPos=ImGui::GetCursorPos();

            // labels
            if (ins->type==DIV_INS_ESFM) {
              ImGui::SetCursorPos(ImVec2(textX_AR,textY));
              CENTER_TEXT_20(FM_SHORT_NAME(FM_AR));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_AR));
              TOOLTIP_TEXT(FM_NAME(FM_AR));
            }

            ImGui::SetCursorPos(ImVec2(textX_DR,textY));
            CENTER_TEXT_20(FM_SHORT_NAME(FM_DR));
            ImGui::TextUnformatted(FM_SHORT_NAME(FM_DR));
            TOOLTIP_TEXT(FM_NAME(FM_DR));

            ImGui::SetCursorPos(ImVec2(textX_SL,textY));
            CENTER_TEXT_20(FM_SHORT_NAME(FM_SL));
            ImGui::TextUnformatted(FM_SHORT_NAME(FM_SL));
            TOOLTIP_TEXT(FM_NAME(FM_SL));

            ImGui::SetCursorPos(ImVec2(textX_RR,textY));
            CENTER_TEXT_20(FM_SHORT_NAME(FM_RR));
            ImGui::TextUnformatted(FM_SHORT_NAME(FM_RR));
            TOOLTIP_TEXT(FM_NAME(FM_RR));

            if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
              ImGui::SetCursorPos(ImVec2(textX_D2R,textY));
              CENTER_TEXT_20(FM_SHORT_NAME(FM_D2R));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_D2R));
              TOOLTIP_TEXT(FM_NAME(FM_D2R));
            }

            ImGui::SetCursorPos(prevCurPos);
            ImGui::Dummy(ImVec2(0,0));
            
            ImGui::TableNextColumn();
            switch (ins->type) {
              case DIV_INS_FM: {
                // SSG
                ImGui::BeginDisabled(!ssgOn);
                drawSSGEnv(op.ssgEnv&7,ImVec2(waveWidth,waveHeight));
                ImGui::EndDisabled();
                if (ImGui::Checkbox("##SSGOn",&ssgOn)) { PARAMETER
                  op.ssgEnv=(op.ssgEnv&7)|(ssgOn<<3);
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (CWSliderScalar("##SSG",ImGuiDataType_U8,&ssgEnv,&_ZERO,&_SEVEN,_(ssgEnvTypes[ssgEnv]))) { PARAMETER
                  op.ssgEnv=(op.ssgEnv&8)|(ssgEnv&7);
                }
                
                // params
                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
                P(CWSliderScalar("##MULT",ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN,tempID)); rightClickable

                int detune=detuneMap[settings.unsignedDetune?1:0][op.dt&7];
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_DT));
                if (CWSliderInt("##DT",&detune,settings.unsignedDetune?0:-3,settings.unsignedDetune?7:4,tempID)) { PARAMETER
                  if (detune<-3) detune=-3;
                  if (detune>7) detune=7;
                  op.dt=detuneUnmap[settings.unsignedDetune?1:0][detune+3];
                } rightClickable

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_RS));
                P(CWSliderScalar("##RS",ImGuiDataType_U8,&op.rs,&_ZERO,&_THREE,tempID)); rightClickable

                break;
              }
              case DIV_INS_OPM: {
                // params
                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
                P(CWSliderScalar("##MULT",ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN,tempID)); rightClickable

                int detune=detuneMap[settings.unsignedDetune?1:0][op.dt&7];
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_DT));
                if (CWSliderInt("##DT",&detune,settings.unsignedDetune?0:-3,settings.unsignedDetune?7:4,tempID)) { PARAMETER
                  if (detune<-3) detune=-3;
                  if (detune>7) detune=7;
                  op.dt=detuneUnmap[settings.unsignedDetune?1:0][detune+3];
                } rightClickable

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_DT2));
                P(CWSliderScalar("##DT2",ImGuiDataType_U8,&op.dt2,&_ZERO,&_THREE,tempID)); rightClickable

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_RS));
                P(CWSliderScalar("##RS",ImGuiDataType_U8,&op.rs,&_ZERO,&_THREE,tempID)); rightClickable

                // OPP TL ramp
                bool displayTLRamp=false;
                if (ins->type==DIV_INS_OPM) {
                  displayTLRamp=false;
                  for (int i=0; i<e->song.systemLen; i++) {
                    if (e->song.system[i]==DIV_SYSTEM_YM2151) {
                      // test for YM2164 (OPP)
                      if (e->song.systemFlags[i].getInt("chipType",0)==1) {
                        displayTLRamp=true;
                        break;
                      }
                    }
                  }
                }
                if (displayTLRamp) {
                  if (ImGui::Checkbox(_("TL Ramp##TLRamp"),&ksrOn)) { PARAMETER
                    op.ksr=ksrOn;
                  }
                }
                break;
              }
              case DIV_INS_OPLL:
                // waveform
                drawWaveform(i==0?(fmOrigin.ams&1):(fmOrigin.fms&1),ins->type==DIV_INS_OPZ,ImVec2(waveWidth,waveHeight));

                // params
                ImGui::Separator();
                if (ImGui::BeginTable("FMParamsInner",2)) {
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  bool amOn=op.am;
                  if (ImGui::Checkbox(FM_NAME(FM_AM),&amOn)) { PARAMETER
                    op.am=amOn;
                  }
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_NAME(FM_KSR),&ksrOn)) { PARAMETER
                    op.ksr=ksrOn;
                  }

                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_NAME(FM_VIB),&vibOn)) { PARAMETER
                    op.vib=vibOn;
                  }
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_NAME(FM_EGS),&ssgOn)) { PARAMETER
                    op.ssgEnv=(op.ssgEnv&7)|(ssgOn<<3);
                  }
                  
                  ImGui::EndTable();
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
                P(CWSliderScalar("##MULT",ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN,tempID)); rightClickable

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_KSL));
                P(CWSliderScalar("##KSL",ImGuiDataType_U8,&op.ksl,&_ZERO,&_THREE,tempID)); rightClickable

                break;
              case DIV_INS_OPL:
              case DIV_INS_OPL_DRUMS: {
                // waveform
                drawWaveform(op.ws&wsMax,ins->type==DIV_INS_OPZ,ImVec2(waveWidth,waveHeight));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar("##WS",ImGuiDataType_U8,&op.ws,&_ZERO,&wsMax,(ins->type==DIV_INS_OPZ)?opzWaveforms[op.ws&wsMax]:(settings.oplStandardWaveNames?oplWaveformsStandard[op.ws&wsMax]:oplWaveforms[op.ws&wsMax]))); rightClickable

                // params
                ImGui::Separator();
                if (ImGui::BeginTable("FMParamsInner",2)) {
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  bool amOn=op.am;
                  if (ImGui::Checkbox(FM_NAME(FM_AM),&amOn)) { PARAMETER
                    op.am=amOn;
                  }
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_NAME(FM_KSR),&ksrOn)) { PARAMETER
                    op.ksr=ksrOn;
                  }

                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_NAME(FM_VIB),&vibOn)) { PARAMETER
                    op.vib=vibOn;
                  }
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_NAME(FM_SUS),&susOn)) { PARAMETER
                    op.sus=susOn;
                  }
                  
                  ImGui::EndTable();
                }

                pushWarningColor(ins->type==DIV_INS_OPL_DRUMS && i==0);          
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
                P(CWSliderScalar("##MULT",ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN,tempID)); rightClickable
                if (ins->type==DIV_INS_OPL_DRUMS && i==0) {
                  if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s",_("Snare's multiplier is determined by HiHat's."));
                  }
                }
                popWarningColor();

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_KSL));
                int ksl=kslMap[op.ksl&3];
                if (CWSliderInt("##KSL",&ksl,0,3,tempID)) {
                  op.ksl=kslMap[ksl&3];
                  PARAMETER;
                } rightClickable

                break;
              }
              case DIV_INS_OPZ: {
                // waveform
                drawWaveform(op.ws&wsMax,ins->type==DIV_INS_OPZ,ImVec2(waveWidth,waveHeight));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar("##WS",ImGuiDataType_U8,&op.ws,&_ZERO,&wsMax,(ins->type==DIV_INS_OPZ)?opzWaveforms[op.ws&wsMax]:(settings.oplStandardWaveNames?oplWaveformsStandard[op.ws&wsMax]:oplWaveforms[op.ws&wsMax]))); rightClickable

                // params
                ImGui::Separator();
                if (egtOn) {
                  if (!op.sus) {
                    int block=op.dt;
                    int freqNum=(op.mult<<4)|(op.dvb&15);
                    ImGui::Text(_("Block"));
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImVec2 cursorAlign=ImGui::GetCursorPos();
                    if (ImGui::InputInt("##Block",&block,1,1)) {
                      if (block<0) block=0;
                      if (block>7) block=7;
                      op.dt=block;
                    }
                    
                    ImGui::Text(_("Freq"));
                    ImGui::SameLine();
                    ImGui::SetCursorPos(ImVec2(cursorAlign.x,ImGui::GetCursorPosY()));
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (ImGui::InputInt("##FreqNum",&freqNum,1,16)) {
                      if (freqNum<0) freqNum=0;
                      if (freqNum>255) freqNum=255;
                      op.mult=freqNum>>4;
                      op.dvb=freqNum&15;
                    }
                  }
                } else {
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
                  P(CWSliderScalar("##MULT",ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN,tempID)); rightClickable

                  int detune=detuneMap[settings.unsignedDetune?1:0][op.dt&7];
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_DT));
                  if (CWSliderInt("##DT",&detune,settings.unsignedDetune?0:-3,settings.unsignedDetune?7:4,tempID)) { PARAMETER
                    if (detune<-3) detune=-3;
                    if (detune>7) detune=7;
                    op.dt=detuneUnmap[settings.unsignedDetune?1:0][detune+3];
                  } rightClickable
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_DT2));
                P(CWSliderScalar("##DT2",ImGuiDataType_U8,&op.dt2,&_ZERO,&_THREE,tempID)); rightClickable
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Only on YM2151 and YM2414 (OPM and OPZ)"));
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_RS));
                P(CWSliderScalar("##RS",ImGuiDataType_U8,&op.rs,&_ZERO,&_THREE,tempID)); rightClickable
                break;
              }
              case DIV_INS_ESFM:
                // waveform
                drawWaveform(op.ws&wsMax,ins->type==DIV_INS_OPZ,ImVec2(waveWidth,waveHeight));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar("##WS",ImGuiDataType_U8,&op.ws,&_ZERO,&wsMax,(ins->type==DIV_INS_OPZ)?opzWaveforms[op.ws&wsMax]:(settings.oplStandardWaveNames?oplWaveformsStandard[op.ws&wsMax]:oplWaveforms[op.ws&wsMax]))); rightClickable

                // params
                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
                P(CWSliderScalar("##MULT",ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN,tempID)); rightClickable

                if (opE.fixed) {
                  int block=(opE.ct>>2)&7;
                  int freqNum=((opE.ct&3)<<8)|((unsigned char)opE.dt);
                  ImGui::Text(_("Blk"));
                  if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(_("Block"));
                  }
                  ImGui::SameLine();
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  //ImVec2 cursorAlign=ImGui::GetCursorPos();
                  if (ImGui::InputInt("##Block",&block,1,1)) {
                    if (block<0) block=0;
                    if (block>7) block=7;
                    opE.ct=(opE.ct&(~(7<<2)))|(block<<2);
                  }

                  ImGui::Text(_("F"));
                  if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(_("Frequency (F-Num)"));
                  }
                  ImGui::SameLine();
                  //ImGui::SetCursorPos(ImVec2(cursorAlign.x,ImGui::GetCursorPosY()));
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  if (ImGui::InputInt("##FreqNum",&freqNum,1,16)) {
                    if (freqNum<0) freqNum=0;
                    if (freqNum>1023) freqNum=1023;
                    opE.dt=freqNum&0xff;
                    opE.ct=(opE.ct&(~3))|(freqNum>>8);
                  }
                } else {
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  snprintf(tempID,1024,"%s: %%d",ESFM_NAME(ESFM_CT));
                  P(CWSliderScalar("##CT",ImGuiDataType_S8,&opE.ct,&_MINUS_TWENTY_FOUR,&_TWENTY_FOUR,tempID)); rightClickable

                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  snprintf(tempID,1024,"%s: %%d",ESFM_NAME(ESFM_DT));
                  P(CWSliderScalar("##DT",ImGuiDataType_S8,&opE.dt,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
                }

                if (ImGui::BeginTable("panCheckboxes",2,ImGuiTableFlags_SizingStretchSame)) {
                  ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0f);
                  ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.0f);

                  float yPosOutsideTablePadding=ImGui::GetCursorPosY();
                  bool leftOn=opE.left;
                  bool rightOn=opE.right;
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  ImGui::SetCursorPosY(yPosOutsideTablePadding);
                  if (ImGui::Checkbox(ESFM_SHORT_NAME(ESFM_LEFT),&leftOn)) { PARAMETER
                    opE.left=leftOn;
                  }
                  ImGui::TableNextColumn();
                  ImGui::SetCursorPosY(yPosOutsideTablePadding);
                  if (ImGui::Checkbox(ESFM_SHORT_NAME(ESFM_RIGHT),&rightOn)) { PARAMETER
                    opE.right=rightOn;
                  }
                  ImGui::EndTable();
                }
                break;
              default:
                break;
            }

            ImGui::TableNextColumn();
            float envHeight=sliderHeight;//-ImGui::GetStyle().ItemSpacing.y*2.0f;
            if (ins->type==DIV_INS_OPZ) {
              envHeight-=ImGui::GetFrameHeightWithSpacing()*2.0f;
            }
            if (ins->type==DIV_INS_ESFM) {
              envHeight-=ImGui::GetFrameHeightWithSpacing()*3.0f;
            }
            drawFMEnv(op.tl&maxTl,op.ar&maxArDr,op.dr&maxArDr,(ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPLL || ins->type==DIV_INS_ESFM)?((op.rr&15)*2):op.d2r&31,op.rr&15,op.sl&15,op.sus,op.ssgEnv&8,fmOrigin.alg,maxTl,maxArDr,15,ImVec2(ImGui::GetContentRegionAvail().x,envHeight),ins->type);

            if (ins->type==DIV_INS_OPZ) {
              ImGui::Separator();
              if (ImGui::BeginTable("FMParamsInnerOPZ",2)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (!egtOn) {
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_FINE));
                  P(CWSliderScalar("##FINE",ImGuiDataType_U8,&op.dvb,&_ZERO,&_FIFTEEN,tempID)); rightClickable
                } else {
                  bool susOn=op.sus;
                  pushWarningColor(susOn && !e->song.compatFlags.linearPitch);
                  if (ImGui::Checkbox(_("Pitch control"),&susOn)) { PARAMETER
                    op.sus=susOn;
                    // HACK: reset zoom and scroll in fixed pitch macros so that they draw correctly
                    ins->temp.vZoom[DIV_MACRO_OP_SSG+(i<<5)]=-1;
                    ins->temp.vZoom[DIV_MACRO_OP_SUS+(i<<5)]=-1;
                  }
                  popWarningColor();
                  if (ImGui::IsItemHovered()) {
                    if (susOn && !e->song.compatFlags.linearPitch) {
                      ImGui::SetTooltip(_("only works on linear pitch! go to Compatibility Flags > Pitch/Playback and set Pitch linearity to Full."));
                    } else {
                      ImGui::SetTooltip(_("use op's arpeggio and pitch macros control instead of block/f-num macros"));
                    }
                  }
                }

                ImGui::TableNextColumn();
                bool amOn=op.am;
                if (ImGui::Checkbox(FM_NAME(FM_AM),&amOn)) { PARAMETER
                  op.am=amOn;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox(_("Fixed"),&egtOn)) { PARAMETER
                  op.egt=egtOn;
                }

                // this chip has too many parameters. I can't think of a better placement for the tremolo setting.
                float oneTinySlider=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*0.5f;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(oneTinySlider);
                snprintf(tempID,1024,"%s: %%d",FM_SHORT_NAME(FM_EGSHIFT));
                P(CWSliderScalar("##EGShift",ImGuiDataType_U8,&op.ksl,&_ZERO,&_THREE,tempID)); rightClickable
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("%s",FM_NAME(FM_EGSHIFT));
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth(oneTinySlider);
                snprintf(tempID,1024,"%s: %%d",FM_SHORT_NAME(FM_TS));
                P(CWSliderScalar("##TS",ImGuiDataType_U8,&op.ssgEnv,&_ZERO,&_THREE,tempID)); rightClickable
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("%s",FM_NAME(FM_TS));
                }

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_REV));
                P(CWSliderScalar("##REV",ImGuiDataType_U8,&op.dam,&_ZERO,&_SEVEN,tempID)); rightClickable

                ImGui::TableNextColumn();


                ImGui::EndTable();
              }
            }

            if (ins->type==DIV_INS_ESFM) {
              ImGui::Separator();
              if (ImGui::BeginTable("FMParamsInnerESFM",2)) {
                ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.64f);
                ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.36f);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_KSL));
                int ksl=kslMap[op.ksl&3];
                if (CWSliderInt("##KSL",&ksl,0,3,tempID)) {
                  op.ksl=kslMap[ksl&3];
                  PARAMETER;
                } rightClickable

                bool amOn=op.am;
                bool fixedOn=opE.fixed;
                ImGui::TableNextColumn();
                if (ImGui::Checkbox(FM_SHORT_NAME(FM_KSR),&ksrOn)) { PARAMETER
                  op.ksr=ksrOn;
                }
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::BeginTable("vibAmCheckboxes",2)) {
                  ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0f);
                  ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.0f);

                  float yPosOutsideTablePadding=ImGui::GetCursorPosY();
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  ImGui::SetCursorPosY(yPosOutsideTablePadding);
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_VIB),&vibOn)) { PARAMETER
                    op.vib=vibOn;
                  }
                  ImGui::TableNextColumn();
                  ImGui::SetCursorPosY(yPosOutsideTablePadding);
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_AM),&amOn)) { PARAMETER
                    op.am=amOn;
                  }

                  bool damOn=op.dam;
                  bool dvbOn=op.dvb;
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_DVB),&dvbOn)) { PARAMETER
                    op.dvb=dvbOn;
                  }
                  ImGui::TableNextColumn();
                  if (ImGui::Checkbox(FM_SHORT_NAME(FM_DAM),&damOn)) { PARAMETER
                    op.dam=damOn;
                  }
                  ImGui::EndTable();
                }
                ImGui::TableNextColumn();
                if (ImGui::Checkbox(FM_SHORT_NAME(FM_SUS),&susOn)) { PARAMETER
                  op.sus=susOn;
                }
                if (ImGui::Checkbox(ESFM_NAME(ESFM_FIXED),&fixedOn)) { PARAMETER
                  opE.fixed=fixedOn;
                  // HACK: reset zoom and scroll in fixed pitch macros so that they draw correctly
                  ins->temp.vZoom[DIV_MACRO_OP_SSG+(i<<5)]=-1;
                  ins->temp.vZoom[DIV_MACRO_OP_DT+(i<<5)]=-1;
                }

                ImGui::EndTable();
              }
            }

            ImGui::TableNextColumn();
            op.tl&=maxTl;
            float tlSliderWidth=(ins->type==DIV_INS_ESFM)?20.0f*dpiScale:ImGui::GetFrameHeight();
            float tlSliderHeight=sliderHeight-((ins->type==DIV_INS_FM || ins->type==DIV_INS_OPM || ins->type==DIV_INS_OPZ)?(ImGui::GetFrameHeightWithSpacing()+ImGui::CalcTextSize(FM_SHORT_NAME(FM_AM)).y+ImGui::GetStyle().ItemSpacing.y):0.0f);
            float textX_tl=ImGui::GetCursorPosX();
            P(CWVSliderScalar("##TL",ImVec2(tlSliderWidth,tlSliderHeight),ImGuiDataType_U8,&op.tl,&maxTl,&_ZERO)); rightClickable

            if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPM) {
              CENTER_TEXT(FM_SHORT_NAME(FM_AM));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_AM));
              TOOLTIP_TEXT(FM_NAME(FM_AM));
              bool amOn=op.am;
              if (ImGui::Checkbox("##AM",&amOn)) { PARAMETER
                op.am=amOn;
              }
            } else if (ins->type==DIV_INS_OPZ) {
              CENTER_TEXT(FM_SHORT_NAME(FM_TLRAMP));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_TLRAMP));
              TOOLTIP_TEXT(FM_NAME(FM_TLRAMP));
              bool ksrOn=op.ksr;
              if (ImGui::Checkbox("##TLRamp",&ksrOn)) { PARAMETER
                op.ksr=ksrOn;
              }
            }

            if (ins->type==DIV_INS_ESFM) {
              ImGui::SameLine();
              float textX_outLvl=ImGui::GetCursorPosX();
              P(CWVSliderScalar("##OUTLVL",ImVec2(tlSliderWidth,tlSliderHeight),ImGuiDataType_U8,&opE.outLvl,&_ZERO,&_SEVEN)); rightClickable

              ImGui::SameLine();
              float textX_modIn=ImGui::GetCursorPosX();
              P(CWVSliderScalar("##MODIN",ImVec2(tlSliderWidth,tlSliderHeight),ImGuiDataType_U8,&opE.modIn,&_ZERO,&_SEVEN)); rightClickable

              prevCurPos=ImGui::GetCursorPos();
              ImGui::SetCursorPos(ImVec2(textX_tl,textY));
              CENTER_TEXT_20(FM_SHORT_NAME(FM_TL));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_TL));
              TOOLTIP_TEXT(FM_NAME(FM_TL));

              ImGui::SetCursorPos(ImVec2(textX_outLvl,textY));
              CENTER_TEXT_20(ESFM_SHORT_NAME(ESFM_OUTLVL));
              ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_OUTLVL));
              TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_OUTLVL));

              ImGui::SetCursorPos(ImVec2(textX_modIn,textY));
              CENTER_TEXT_20(ESFM_SHORT_NAME(ESFM_MODIN));
              ImGui::TextUnformatted(ESFM_SHORT_NAME(ESFM_MODIN));
              TOOLTIP_TEXT(ESFM_LONG_NAME(ESFM_MODIN));

              ImGui::SetCursorPos(prevCurPos);
            } else {
              prevCurPos=ImGui::GetCursorPos();
              ImGui::SetCursorPos(ImVec2(textX_tl,textY));
              CENTER_TEXT(FM_SHORT_NAME(FM_TL));
              ImGui::TextUnformatted(FM_SHORT_NAME(FM_TL));
              TOOLTIP_TEXT(FM_NAME(FM_TL));

              ImGui::SetCursorPos(prevCurPos);
            }

            ImGui::EndTable();
          }
          ImGui::PopStyleVar();

          if (settings.separateFMColors) {
            popAccentColors();
          }

          ImGui::PopID();
        }
        ImGui::EndTable();
      }
      ImGui::PopStyleVar();
    } else { // classic...... erm COMPACT
      int columns=2;
      switch (settings.fmLayout) {
        case 1: // 2x2
          columns=2;
          break;
        case 2: // 1x4
          columns=1;
          break;
        case 3: // 4x1
          columns=opCount;
          break;
      }
      if (ImGui::BeginTable("FMOperators",columns,ImGuiTableFlags_SizingStretchSame)) {
        for (int i=0; i<opCount; i++) {
          DivInstrumentFM::Operator& op=fmOrigin.op[(opCount==4 && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_ESFM)?opOrder[i]:i];
          DivInstrumentESFM::Operator& opE=ins->esfm.op[i];
          if ((settings.fmLayout!=3 && ((i+1)&1)) || i==0 || settings.fmLayout==2) ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Separator();
          ImGui::PushID(fmt::sprintf("op%d",i).c_str());

          // push colors
          if (settings.separateFMColors) {
            bool mod=true;
            if (ins->type==DIV_INS_OPL_DRUMS) {
              mod=false;
            } else if (ins->type==DIV_INS_ESFM) {
              // this is the same as the KVS heuristic in platform/esfm.h
              if (opE.outLvl==7) {
                mod=false;
              } else if (opE.outLvl>0) {
                if (i==3) {
                  mod=false;
                } else {
                  DivInstrumentESFM::Operator& opENext=ins->esfm.op[i+1];
                  if (opENext.modIn==0) {
                    mod=false;
                  } else if ((opE.outLvl-opENext.modIn)>=2) {
                    mod=false;
                  }
                }
              }
            } else if (opCount==4) {
              if (ins->type==DIV_INS_OPL) {
                if (opIsOutputOPL[fmOrigin.alg&3][i]) mod=false;
              } else {
                if (opIsOutput[fmOrigin.alg&7][i]) mod=false;
              }
            } else {
              if (i==1 || (ins->type==DIV_INS_OPL && (fmOrigin.alg&1))) mod=false;
            }
            if (mod) {
              pushAccentColors(
                uiColors[GUI_COLOR_FM_PRIMARY_MOD],
                uiColors[GUI_COLOR_FM_SECONDARY_MOD],
                uiColors[GUI_COLOR_FM_BORDER_MOD],
                uiColors[GUI_COLOR_FM_BORDER_SHADOW_MOD]
              );
            } else {
              pushAccentColors(
                uiColors[GUI_COLOR_FM_PRIMARY_CAR],
                uiColors[GUI_COLOR_FM_SECONDARY_CAR],
                uiColors[GUI_COLOR_FM_BORDER_CAR],
                uiColors[GUI_COLOR_FM_BORDER_SHADOW_CAR]
              );
            }
          }

          ImGui::Dummy(ImVec2(dpiScale,dpiScale));
          String opNameLabel;
          OP_DRAG_POINT;
          ImGui::SameLine();
          if (ins->type==DIV_INS_OPL_DRUMS) {
            opNameLabel=fmt::sprintf("%s",oplDrumNames[i]);
          } else if (ins->type==DIV_INS_OPL && fmOrigin.opllPreset==16) {
            if (i==1) {
              opNameLabel=_("Envelope 2 (kick only)");
            } else {
              opNameLabel=_("Envelope");
            }
          } else {
            opNameLabel=fmt::sprintf("OP%d",i+1);
          }
          if (opsAreMutable) {
            pushToggleColors(op.enable);
            if (ImGui::Button(opNameLabel.c_str())) {
              op.enable=!op.enable;
              PARAMETER;
            }
            popToggleColors();
          } else {
            ImGui::TextUnformatted(opNameLabel.c_str());
          }

          ImGui::SameLine();

          bool amOn=op.am;
          if (ImGui::Checkbox(FM_NAME(FM_AM),&amOn)) { PARAMETER
            op.am=amOn;
          }

          // OPP TL ramp
          bool displayTLRamp=false;
          if (ins->type==DIV_INS_OPZ) {
            displayTLRamp=true;
          } else if (ins->type==DIV_INS_OPM) {
            displayTLRamp=false;
            for (int i=0; i<e->song.systemLen; i++) {
              if (e->song.system[i]==DIV_SYSTEM_YM2151) {
                // test for YM2164 (OPP)
                if (e->song.systemFlags[i].getInt("chipType",0)==1) {
                  displayTLRamp=true;
                  break;
                }
              }
            }
          }
          if (displayTLRamp) {
            ImGui::SameLine();
            bool ksrOn=op.ksr; // why is this not defined?!?!?!?
            if (ImGui::Checkbox(_("TL Ramp##TLRamp"),&ksrOn)) { PARAMETER
              op.ksr=ksrOn;
            }
          }

          int maxTl=127;
          if (ins->type==DIV_INS_OPLL) {
            if (i==1) {
              maxTl=15;
            } else {
              maxTl=63;
            }
          }
          if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_ESFM) {
            maxTl=63;
          }
          int maxArDr=(ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM)?31:15;

          bool ssgOn=op.ssgEnv&8;
          bool ksrOn=op.ksr;
          bool vibOn=op.vib;
          bool susOn=op.sus; // don't you make fun of this one
          unsigned char ssgEnv=op.ssgEnv&7;
          if (ins->type!=DIV_INS_OPL && ins->type!=DIV_INS_OPL_DRUMS && ins->type!=DIV_INS_OPZ && ins->type!=DIV_INS_OPM && ins->type!=DIV_INS_ESFM) {
            ImGui::SameLine();
            if (ImGui::Checkbox((ins->type==DIV_INS_OPLL)?FM_NAME(FM_EGS):_("SSG On"),&ssgOn)) { PARAMETER
              op.ssgEnv=(op.ssgEnv&7)|(ssgOn<<3);
            }
          }

          if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_ESFM) {
            ImGui::SameLine();
            if (ImGui::Checkbox(FM_NAME(FM_SUS),&susOn)) { PARAMETER
              op.sus=susOn;
            }
          }

          if (ins->type==DIV_INS_OPZ) {
            ImGui::SameLine();
            bool fixedOn=op.egt;
            if (ImGui::Checkbox(_("Fixed"),&fixedOn)) { PARAMETER
              op.egt=fixedOn;
            }
            bool susOn=op.sus;
            if (fixedOn) {
              ImGui::SameLine();
              pushWarningColor(susOn && !e->song.compatFlags.linearPitch);
              if (ImGui::Checkbox(_("Pitch control"),&susOn)) { PARAMETER
                op.sus=susOn;
                // HACK: reset zoom and scroll in fixed pitch macros so that they draw correctly
                ins->temp.vZoom[DIV_MACRO_OP_SSG+(i<<5)]=-1;
                ins->temp.vZoom[DIV_MACRO_OP_SUS+(i<<5)]=-1;
              }
              popWarningColor();
              if (ImGui::IsItemHovered()) {
                if (susOn && !e->song.compatFlags.linearPitch) {
                  ImGui::SetTooltip(_("only works on linear pitch! go to Compatibility Flags > Pitch/Playback and set Pitch linearity to Full."));
                } else {
                  ImGui::SetTooltip(_("use op's arpeggio and pitch macros control instead of block/f-num macros"));
                }
              }
            }
          }

          //52.0 controls vert scaling; default 96
          drawFMEnv(op.tl&maxTl,op.ar&maxArDr,op.dr&maxArDr,(ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPLL || ins->type==DIV_INS_ESFM)?((op.rr&15)*2):op.d2r&31,op.rr&15,op.sl&15,op.sus,op.ssgEnv&8,fmOrigin.alg,maxTl,maxArDr,15,ImVec2(ImGui::GetContentRegionAvail().x,52.0*dpiScale),ins->type);
          //P(CWSliderScalar(FM_NAME(FM_AR),ImGuiDataType_U8,&op.ar,&_ZERO,&_THIRTY_ONE)); rightClickable
          if (ImGui::BeginTable("opParams",2,ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0); \
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0); \

            if (ins->type==DIV_INS_ESFM) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              opE.delay&=7;
              P(CWSliderScalar("##DELAY",ImGuiDataType_U8,&opE.delay,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",ESFM_NAME(ESFM_DELAY));
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            op.ar&=maxArDr;
            P(CWSliderScalar("##AR",ImGuiDataType_U8,&op.ar,&maxArDr,&_ZERO)); rightClickable
            ImGui::TableNextColumn();
            ImGui::Text("%s",FM_NAME(FM_AR));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            op.dr&=maxArDr;
            P(CWSliderScalar("##DR",ImGuiDataType_U8,&op.dr,&maxArDr,&_ZERO)); rightClickable
            ImGui::TableNextColumn();
            ImGui::Text("%s",FM_NAME(FM_DR));

            if (settings.susPosition==0) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##SL",ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_SL));
            }

            if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##D2R",ImGuiDataType_U8,&op.d2r,&_THIRTY_ONE,&_ZERO)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_D2R));
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            P(CWSliderScalar("##RR",ImGuiDataType_U8,&op.rr,&_FIFTEEN,&_ZERO)); rightClickable
            ImGui::TableNextColumn();
            ImGui::Text("%s",FM_NAME(FM_RR));

            if (settings.susPosition>0) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##SL",ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_SL));
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            op.tl&=maxTl;
            P(CWSliderScalar("##TL",ImGuiDataType_U8,&op.tl,&maxTl,&_ZERO)); rightClickable
            ImGui::TableNextColumn();
            ImGui::Text("%s",FM_NAME(FM_TL));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Separator();
            ImGui::TableNextColumn();
            ImGui::Separator();
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
              P(CWSliderScalar("##RS",ImGuiDataType_U8,&op.rs,&_ZERO,&_THREE)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_RS));
            } else {
              int ksl=ins->type==DIV_INS_OPLL?op.ksl:kslMap[op.ksl&3];
              if (CWSliderInt("##KSL",&ksl,0,3)) {
                op.ksl=(ins->type==DIV_INS_OPLL?ksl:kslMap[ksl&3]);
                PARAMETER;
              } rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_KSL));
            }

            if (ins->type==DIV_INS_OPZ) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar(FM_NAME(FM_EGSHIFT),ImGuiDataType_U8,&op.ksl,&_ZERO,&_THREE)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_EGSHIFT));

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar(FM_NAME(FM_REV),ImGuiDataType_U8,&op.dam,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_REV));

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar(FM_NAME(FM_TS),ImGuiDataType_U8,&op.ssgEnv,&_ZERO,&_THREE)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_SHORT_NAME(FM_TS));
              if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s",FM_NAME(FM_TS));
              }
            }

            if (ins->type==DIV_INS_OPZ) {
              if (op.egt) {
                bool susOn=op.sus;
                if (!susOn) {
                  int block=op.dt;
                  int freqNum=(op.mult<<4)|(op.dvb&15);

                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  if (CWSliderInt(FM_NAME(FM_MULT),&block,0,7)) { PARAMETER
                    if (block<0) block=0;
                    if (block>7) block=7;
                    op.dt=block;
                  } rightClickable
                  ImGui::TableNextColumn();
                  ImGui::Text("Block");

                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                  if (CWSliderInt(FM_NAME(FM_FINE),&freqNum,0,255)) { PARAMETER
                    if (freqNum<0) freqNum=0;
                    if (freqNum>255) freqNum=255;
                    op.mult=freqNum>>4;
                    op.dvb=freqNum&15;
                  } rightClickable
                  ImGui::TableNextColumn();
                  ImGui::Text(_("FreqNum"));
                }
              } else {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar(FM_NAME(FM_MULT),ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN)); rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",FM_NAME(FM_MULT));

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar(FM_NAME(FM_FINE),ImGuiDataType_U8,&op.dvb,&_ZERO,&_FIFTEEN)); rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",FM_NAME(FM_FINE));
              }
            } else {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              pushWarningColor(ins->type==DIV_INS_OPL_DRUMS && i==0);          
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar(FM_NAME(FM_MULT),ImGuiDataType_U8,&op.mult,&_ZERO,&_FIFTEEN)); rightClickable
              if (ins->type==DIV_INS_OPL_DRUMS && i==0) {
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("%s",_("Snare's multiplier is determined by HiHat's."));
                }
              }
              popWarningColor();
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_MULT));
            }
            
            if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
              if (!(ins->type==DIV_INS_OPZ && op.egt)) {
                int detune=detuneMap[settings.unsignedDetune?1:0][op.dt&7];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (CWSliderInt("##DT",&detune,settings.unsignedDetune?0:-3,settings.unsignedDetune?7:4)) { PARAMETER
                  if (detune<-3) detune=-3;
                  if (detune>7) detune=7;
                  op.dt=detuneUnmap[settings.unsignedDetune?1:0][detune+3];
                } rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",FM_NAME(FM_DT));
              }

              if (ins->type!=DIV_INS_FM) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar("##DT2",ImGuiDataType_U8,&op.dt2,&_ZERO,&_THREE)); rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",FM_NAME(FM_DT2));
              }

              if (ins->type==DIV_INS_FM) { // OPN only
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (CWSliderScalar("##SSG",ImGuiDataType_U8,&ssgEnv,&_ZERO,&_SEVEN,_(ssgEnvTypes[ssgEnv]))) { PARAMETER
                  op.ssgEnv=(op.ssgEnv&8)|(ssgEnv&7);
                } rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",FM_NAME(FM_SSG));
              }
            }

            if (ins->type==DIV_INS_ESFM) {
              bool fixedOn=opE.fixed;
              if (fixedOn) {
                int block=(opE.ct>>2)&7;
                int freqNum=((opE.ct&3)<<8)|((unsigned char)opE.dt);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputInt("##Block",&block,1,1)) {
                  if (block<0) block=0;
                  if (block>7) block=7;
                  opE.ct=(opE.ct&(~(7<<2)))|(block<<2);
                }
                ImGui::TableNextColumn();
                ImGui::Text(_("Block"));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputInt("##FreqNum",&freqNum,1,16)) {
                  if (freqNum<0) freqNum=0;
                  if (freqNum>1023) freqNum=1023;
                  opE.dt=freqNum&0xff;
                  opE.ct=(opE.ct&(~3))|(freqNum>>8);
                }
                ImGui::TableNextColumn();
                ImGui::Text(_("FreqNum"));
              } else {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar("##CT",ImGuiDataType_S8,&opE.ct,&_MINUS_TWENTY_FOUR,&_TWENTY_FOUR)); rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",ESFM_NAME(ESFM_CT));

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                P(CWSliderScalar("##DT",ImGuiDataType_S8,&opE.dt,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
                ImGui::TableNextColumn();
                ImGui::Text("%s",ESFM_NAME(ESFM_DT));
              }

            }

            if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_ESFM) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##WS",ImGuiDataType_U8,&op.ws,&_ZERO,&wsMax,(ins->type==DIV_INS_OPZ)?opzWaveforms[op.ws&wsMax]:(settings.oplStandardWaveNames?oplWaveformsStandard[op.ws&wsMax]:oplWaveforms[op.ws&wsMax]))); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",FM_NAME(FM_WS));
            }

            if (ins->type==DIV_INS_ESFM) {
              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::Separator();
              ImGui::TableNextColumn();
              ImGui::Separator();

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##OUTLVL",ImGuiDataType_U8,&opE.outLvl,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",ESFM_NAME(ESFM_OUTLVL));

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##MODIN",ImGuiDataType_U8,&opE.modIn,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              ImGui::Text("%s",ESFM_NAME(ESFM_MODIN));
            }

            ImGui::EndTable();
          }

          if (ins->type==DIV_INS_OPLL || ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_ESFM) {
            if (ImGui::Checkbox(FM_NAME(FM_VIB),&vibOn)) { PARAMETER
              op.vib=vibOn;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(FM_NAME(FM_KSR),&ksrOn)) { PARAMETER
              op.ksr=ksrOn;
            }
          }

          if (ins->type==DIV_INS_ESFM) {
            bool dvbOn=op.dvb;
            bool damOn=op.dam;
            bool leftOn=opE.left;
            bool rightOn=opE.right;
            bool fixedOn=opE.fixed;
            if (ImGui::Checkbox(FM_NAME(FM_DVB),&dvbOn)) { PARAMETER
              op.dvb=dvbOn;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(FM_NAME(FM_DAM),&damOn)) { PARAMETER
              op.dam=damOn;
            }
            if (ImGui::Checkbox(ESFM_NAME(ESFM_LEFT),&leftOn)) { PARAMETER
              opE.left=leftOn;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(ESFM_NAME(ESFM_RIGHT),&rightOn)) { PARAMETER
              opE.right=rightOn;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(ESFM_NAME(ESFM_FIXED),&fixedOn)) { PARAMETER
              opE.fixed=fixedOn;
              // HACK: reset zoom and scroll in fixed pitch macros so that they draw correctly
              ins->temp.vZoom[DIV_MACRO_OP_SSG+(i<<5)]=-1;
              ins->temp.vZoom[DIV_MACRO_OP_DT+(i<<5)]=-1;
            }
          }

          if (settings.separateFMColors) {
            popAccentColors();
          }

          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    }
    ImGui::EndDisabled();
    ImGui::EndTabItem();
  }
}

// operator drag and drop for YAM10. the stock OP_DRAG_POINT swaps the fm and
// esfm arrays, which are four operators long, so it would run off the end here.
// swapping also has to move the routing bits, since modIn refers to operators
// by index.
#define YAM10_OP_DRAG_POINT \
  if (ImGui::Button(ICON_FA_ARROWS)) { \
  } \
  if (ImGui::BeginDragDropSource()) { \
    opToMove=i; \
    ImGui::SetDragDropPayload("FUR_YAM10OP",NULL,0,ImGuiCond_Once); \
    ImGui::Button(ICON_FA_ARROWS "##YAM10Drag"); \
    ImGui::SameLine(); \
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) { \
      ImGui::Text("%s",_("(copying)")); \
    } else { \
      ImGui::Text("%s",_("(swapping)")); \
    } \
    ImGui::EndDragDropSource(); \
  } else if (ImGui::IsItemHovered()) { \
    ImGui::SetTooltip("%s",_("- drag to swap operator\n- shift-drag to copy operator")); \
  } \
  if (ImGui::BeginDragDropTarget()) { \
    const ImGuiPayload* dragItem=ImGui::AcceptDragDropPayload("FUR_YAM10OP"); \
    if (dragItem!=NULL) { \
      if (dragItem->IsDataType("FUR_YAM10OP")) { \
        if (opToMove!=i && opToMove>=0 && opToMove<6) { \
          int sourceOp=opToMove; \
          int destOp=i; \
          if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) { \
            e->lockEngine([ins,destOp,sourceOp]() { \
              ins->yam10.op[destOp]=ins->yam10.op[sourceOp]; \
            }); \
          } else { \
            e->lockEngine([ins,destOp,sourceOp]() { \
              DivInstrumentYAM10::Operator origOp=ins->yam10.op[sourceOp]; \
              ins->yam10.op[sourceOp]=ins->yam10.op[destOp]; \
              ins->yam10.op[destOp]=origOp; \
              for (int k=0; k<6; k++) { \
                unsigned char m=ins->yam10.op[k].modIn; \
                bool fromSource=m&(1<<sourceOp); \
                bool fromDest=m&(1<<destOp); \
                m&=~((1<<sourceOp)|(1<<destOp)); \
                if (fromSource) m|=(1<<destOp); \
                if (fromDest) m|=(1<<sourceOp); \
                ins->yam10.op[k].modIn=m; \
              } \
            }); \
          } \
          PARAMETER; \
        } \
        opToMove=-1; \
      } \
    } \
    ImGui::EndDragDropTarget(); \
  }

static const int _YAM10_MINUS_36=-36, _YAM10_36=36, _YAM10_MINUS_64=-64, _YAM10_63=63;
// the waveform slider needs pointers whose type actually matches the U8 slider
static const unsigned char _YAM10_WS_MIN=0, _YAM10_WS_MAX=YAM10_WAVES-1;
// the DSP bytes index curves that run far wider than anything useful, so
// the sliders only cover the part worth reaching
static const unsigned char _YAM10_XLO_MIN=49,  _YAM10_XLO_MAX=159; // 50 Hz to 730 Hz
static const unsigned char _YAM10_XHI_MIN=160, _YAM10_XHI_MAX=228; // 800 Hz to 11.5 kHz
static const unsigned char _YAM10_EQF_MIN=19,  _YAM10_EQF_MAX=236; // 20 Hz to 19.5 kHz
static const unsigned char _YAM10_EQQ_MIN=9,   _YAM10_EQQ_MAX=200; // Q 0.10 to 12.3
static const unsigned char _YAM10_GAIN_MIN=51, _YAM10_GAIN_MAX=205; // -12 dB to +12 dB
static const unsigned char _YAM10_U8_ZERO=0, _YAM10_U8_127=127;
static const unsigned char _YAM10_MULT_MAX=16;
static const unsigned char _YAM10_DELAY_MAX=7;

// the waveforms are grouped by family rather than by when they were added,
// so the names are given in one list rather than borrowed from elsewhere.
// the numbers are the order they were added in, which is what a saved song
// carries, and the picker below puts them back into family order.
static const char* yam10WaveNames[YAM10_WAVES]={
  _N("Sine"), _N("Half Sine"), _N("Absolute Sine"),
  _N("Pulse Sine"), _N("Squished Sine"), _N("Squished AbsSine"),
  _N("Quarter Squished Sine"), _N("Triangle"), _N("Absolute Triangle"),
  _N("Cut Triangle"), _N("Squished Triangle"), _N("Squished AbsTriangle"),
  _N("Squared Sine"), _N("Absolute Squared Sine"), _N("Half Squared Sine"),
  _N("Squished Squared Sine"), _N("Squished AbsSquared Sine"), _N("Saw"),
  _N("Half Saw"), _N("Square"), _N("Logarithmic Saw"),
  _N("Noise"), _N("Noise (1-bit)"), _N("Sample & Hold"),
  _N("Pulse Triangle"), _N("Quarter Squished Triangle"), _N("Pulse Squared Sine"),
  _N("Quarter Squished Squared Sine"), _N("Absolute Saw"), _N("Pulse Saw"),
  _N("Squished Saw"), _N("Squished AbsSaw"), _N("Quarter Squished Saw"),
  _N("Half Square"), _N("Pulse Square"), _N("Squished Square"),
  _N("Quarter Squished Square"), _N("Pulse"), _N("Pulse 25%"),
  _N("Pulse 75%"), _N("Half Log Saw"), _N("Absolute Log Saw"),
  _N("Pulse Log Saw"), _N("Squished Log Saw"), _N("Squished AbsLog Saw"),
  _N("Quarter Squished Log Saw"), _N("Cubed Sine"), _N("Half Cubed Sine"),
  _N("Absolute Cubed Sine"), _N("Pulse Cubed Sine"), _N("Squished Cubed Sine"),
  _N("Squished AbsCubed Sine"), _N("Quarter Squished Cubed Sine"), _N("Cubed Triangle"),
  _N("Cut Cubed Triangle"), _N("Absolute Cubed Triangle"), _N("Pulse Cubed Triangle"),
  _N("Squished Cubed Triangle"), _N("Squished AbsCubed Triangle"), _N("Quarter Squished Cubed Triangle"),
  _N("Cubed Saw"), _N("Half Cubed Saw"), _N("Absolute Cubed Saw"),
  _N("Pulse Cubed Saw"), _N("Squished Cubed Saw"), _N("Squished AbsCubed Saw"),
  _N("Quarter Squished Cubed Saw"), _N("Periodic Noise (7 step)"), _N("Periodic Noise (15 step)"),
  _N("Periodic Noise (63 step)"), _N("Periodic Noise (127 step)"), _N("POKEY Noise (4-bit)"),
  _N("Atari 2600 (5-bit poly)"), _N("POKEY Noise (9-bit)"), _N("NES Short Noise"),
  _N("Atari 2600 (poly 5 to div 6)"), _N("Atari 2600 (div 15 to poly 4)"),
  _N("Atari 2600 (poly 5 to poly 4)"), _N("Atari 2600 (white noise)")
};

// what the picker shows, in the order it shows it. every family carries the
// same derivations in the same order, so a row means the same thing wherever
// you are in the list.
#define YAM10_WAVE_GROUP_MAX 12

struct YAM10WaveGroup {
  const char* name;
  unsigned char ws[YAM10_WAVE_GROUP_MAX];
  int count;
};

static const YAM10WaveGroup yam10WaveGroups[]={
  {_N("Sine"),{0,1,2,3,4,5,6},7},
  {_N("Squared Sine"),{12,14,13,26,15,16,27},7},
  {_N("Cubed Sine"),{46,47,48,49,50,51,52},7},
  {_N("Triangle"),{7,9,8,24,10,11,25},7},
  {_N("Cubed Triangle"),{53,54,55,56,57,58,59},7},
  {_N("Sawtooth"),{17,18,28,29,30,31,32},7},
  {_N("Cubed Sawtooth"),{60,61,62,63,64,65,66},7},
  {_N("Logarithmic Saw"),{20,40,41,42,43,44,45},7},
  {_N("Square and Pulse"),{19,33,34,35,36,37},6},
  {_N("Noise"),{21,22,23},3},
  {_N("Periodic Noise"),{67,68,69,70},4},
  {_N("Atari 2600"),{72,75,76,77,78},5},
  {_N("POKEY and NES"),{71,73,74},3},
};
#define YAM10_WAVE_GROUP_COUNT ((int)(sizeof(yam10WaveGroups)/sizeof(yam10WaveGroups[0])))

static const char* yam10WaveName(unsigned char ws, bool oplStandard) {
  // the OPL standard names setting does not apply: this is the chip's own list
  (void)oplStandard;
  if (ws>=YAM10_WAVES) ws=0;
  return _(yam10WaveNames[ws]);
}

enum YAM10Params {
  YAM10_OL=0,
  YAM10_MI=1,
  YAM10_PR=2,
  YAM10_WT=3,
  YAM10_FINE=4,
  YAM10_PAN=5
};

static const char* yam10ParamShortNames[6]={"OL","MI","PR","WT","Fine","Pan"};
static const char* yam10ParamLongNames[6]={
  _N("Output Level"),
  _N("Modulation Input"),
  _N("Phase Reset"),
  _N("Wavetable"),
  _N("Fine Detune"),
  _N("Panning")
};

#define YAM10_SHORT_NAME(x) (yam10ParamShortNames[x])
#define YAM10_LONG_NAME(x) _(yam10ParamLongNames[x])

// reads the chip's own waveform bank, so the picture is always what plays
void FurnaceGUI::drawYAM10Waveform(unsigned char ws, bool custom, int waveIndex, unsigned char duty, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(minArea.x+size.x,minArea.y+size.y);
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_WAVE]);
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("yam10Wave"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);
    yam10_build_wf();

    DivWavetable* wt=NULL;
    if (custom && waveIndex>=0 && waveIndex<(int)e->song.wave.size()) wt=e->song.wave[waveIndex];
    if (custom && (wt==NULL || wt->len<2)) {
      dl->AddText(ImVec2(rect.Min.x+5.0f*dpiScale,rect.Min.y+5.0f*dpiScale),color,_("no wavetable"));
      return;
    }

    // one point per pixel so a long wavetable is not undersampled
    int waveformLen=(int)(size.x);
    if (waveformLen<64) waveformLen=64;
    if (waveformLen>512) waveformLen=512;
    ImVec2 waveform[513];

    // one cycle of the note for everything, so every picture is drawn to the
    // same scale and a long pattern does not collapse into hash. sample and
    // hold is the exception: it moves once a cycle, so one would be flat
    const unsigned char wsClamped=(ws>=YAM10_WAVES)?(YAM10_WAVES-1):ws;
    const unsigned char wkind=custom?YAM10_WK_WAVETABLE:yam10_wave_def[wsClamped].kind;
    const int cycles=(wkind==YAM10_WK_SH)?6:1;
    float held=0.0f;
    int lastCycle=-1;
    unsigned int noiseReg=0xACE1u;
    int noiseStep=0;

    for (int i=0; i<=waveformLen; i++) {
      float x=(float)i/(float)waveformLen;
      float yv=0.0f;
      if (custom) {
        // the last drawn point is the last sample, not a wrap back to the
        // first, which used to snap the trace shut at the right edge
        int wi=(int)((double)i*(double)(wt->len-1)/(double)waveformLen+0.5);
        if (wi<0) wi=0;
        if (wi>=wt->len) wi=wt->len-1;
        double half=(double)(wt->max>0?wt->max:255)*0.5;
        yv=(float)(((double)wt->data[wi]-half)/(half>0.0?half:1.0));
      } else if (wkind==YAM10_WK_NOISE || wkind==YAM10_WK_NOISE1) {
        // step the register the chip steps, at the 32 steps a cycle it uses,
        // rather than standing in for it with a different generator
        int want=(int)((double)i*32.0*(double)cycles/(double)waveformLen);
        while (noiseStep<want) {
          noiseReg=(noiseReg>>1)^((unsigned int)(-(int)(noiseReg&1))&0xB400u);
          noiseStep++;
        }
        if (wkind==YAM10_WK_NOISE1) {
          yv=(noiseReg&1)?1.0f:-1.0f;
        } else {
          int n=(int)(noiseReg&0x1fff)-0x1000;
          unsigned short lg=(unsigned short)((n<0)?-n:n);
          unsigned short neg=(n<0)?0x8000:0;
          lg=(lg<1)?0x1000:(unsigned short)(-(int)(log2((double)lg/4096.0)*256.0));
          yv=(lg>=0x1000)?0.0f:((float)yam10_exp(lg,neg)/4084.0f);
        }
      } else if (wkind==YAM10_WK_POLY) {
        // the same prebuilt pattern the chip reads, stepped the way the
        // oscillator steps it, so the picture cannot drift from what plays.
        // a pattern laid over several cycles shows the first of them
        const YAM10WaveDef& wd=yam10_wave_def[wsClamped];
        int step=(int)((double)i*(double)wd.polyPeriod/
                       ((double)waveformLen*(double)(1<<wd.polyShift)));
        if (step>=(int)wd.polyPeriod) step=(int)wd.polyPeriod-1;
        yv=yam10_poly_bits[wd.polyIndex][step]?1.0f:-1.0f;
      } else if (wkind==YAM10_WK_PULSE) {
        yv=(((unsigned int)(x*1024.0f))<((unsigned int)duty<<2))?1.0f:-1.0f;
      } else if (wkind==YAM10_WK_SH) {
        // the same register again, but clocked once a cycle as the chip does
        int cyc=(int)(x*(float)cycles);
        if (cyc!=lastCycle) {
          lastCycle=cyc;
          noiseReg=(noiseReg>>1)^((unsigned int)(-(int)(noiseReg&1))&0xB400u);
          int n=(int)(noiseReg&0x1fff)-0x1000;
          unsigned short lg=(unsigned short)((n<0)?-n:n);
          unsigned short neg=(n<0)?0x8000:0;
          lg=(lg<1)?0x1000:(unsigned short)(-(int)(log2((double)lg/4096.0)*256.0));
          held=(lg>=0x1000)?0.0f:((float)yam10_exp(lg,neg)/4084.0f);
        }
        yv=held;
      } else {
        // same endpoint rule: run to entry 1023 rather than wrapping to 0
        unsigned int p=(unsigned int)((double)i*1024.0*(double)cycles/(double)waveformLen);
        if (cycles>1) p&=0x3ff; else if (p>1023) p=1023;
        unsigned short entry=yam10_wf[wsClamped][p];
        unsigned short lg=entry&0x7fff, neg=entry&0x8000;
        yv=(lg>=0x1000)?0.0f:((float)yam10_exp(lg,neg)/4084.0f);
      }
      if (yv>1.0f) yv=1.0f;
      if (yv<-1.0f) yv=-1.0f;
      waveform[i]=ImLerp(rect.Min,rect.Max,ImVec2(x,0.5f-yv*0.4f));
    }
    dl->AddPolyline(waveform,waveformLen+1,color,dpiScale,ImDrawFlags_None);
  }
}

// the waveform picker. one row a waveform, grouped by family, and every row
// carries the picture the chip will actually play rather than a stock icon.
// the number is shown beside the name because that is what the waveform
// effects take.
bool FurnaceGUI::drawYAM10WaveSelect(const char* id, unsigned char& ws, unsigned char duty) {
  bool changed=false;
  if (ws>=YAM10_WAVES) ws=0;
  ImGuiStyle& style=ImGui::GetStyle();
  const float rowHeight=ImGui::GetFontSize()*1.5f;
  const float previewW=52.0f*dpiScale;

  // the popup has to hold the longest name outright, otherwise the list reads
  // as a column of truncated words. measuring beats guessing a width, and the
  // answer only changes when the font does.
  static float listWidth=0.0f;
  static float measuredAt=-1.0f;
  if (measuredAt!=ImGui::GetFontSize()) {
    float widest=0.0f;
    for (int w=0; w<YAM10_WAVES; w++) {
      String entry=fmt::sprintf("%s (%d)",_(yam10WaveNames[w]),w);
      float tw=ImGui::CalcTextSize(entry.c_str()).x;
      if (tw>widest) widest=tw;
    }
    listWidth=previewW+widest+style.ItemSpacing.x+style.WindowPadding.x*2.0f+style.ScrollbarSize;
    measuredAt=ImGui::GetFontSize();
  }

  // the popup does its own scrolling. putting a scrolling table inside it gave
  // two scrollbars and let the popup cap the table's height.
  ImGui::SetNextWindowSizeConstraints(ImVec2(listWidth,rowHeight*12.0f),ImVec2(canvasW,canvasH));

  String label=fmt::sprintf("%s (%d)",yam10WaveName(ws,false),(int)ws);
  if (ImGui::BeginCombo(id,label.c_str(),ImGuiComboFlags_HeightLargest)) {
    for (int g=0; g<YAM10_WAVE_GROUP_COUNT; g++) {
      const YAM10WaveGroup& grp=yam10WaveGroups[g];
      ImGui::SeparatorText(_(grp.name));
      for (int i=0; i<grp.count; i++) {
        const unsigned char w=grp.ws[i];
        ImGui::PushID((int)w);
        const ImVec2 rowStart=ImGui::GetCursorPos();
        if (ImGui::Selectable("##yam10WsRow",ws==w,ImGuiSelectableFlags_None,ImVec2(0.0f,rowHeight))) {
          ws=w;
          changed=true;
        }
        if (ws==w) {
          ImGui::SetItemDefaultFocus();
          // open on the current waveform rather than at the top of 75 of them
          if (ImGui::IsWindowAppearing()) ImGui::SetScrollHereY(0.5f);
        }
        ImGui::SetCursorPos(rowStart);
        drawYAM10Waveform(w,false,-1,duty,ImVec2(previewW,rowHeight));
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s (%d)",_(yam10WaveNames[w]),(int)w);
        ImGui::PopID();
      }
    }
    ImGui::EndCombo();
  }
  return changed;
}

// YAM10 has no algorithm list, so this draws the routing matrix itself:
// six operators and a line for every modulation input. it follows the same
// conventions as drawAlgorithm: nodes are filled circles at the stock
// radius, the digit sits outside the node in the node's own colour, and a
// concentric ring means feedback. a disabled operator is dimmed rather than
// hollowed out, since a hollow circle already means feedback.
void FurnaceGUI::drawYAM10Algorithm(DivInstrumentYAM10& y, const ImVec2& size) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(minArea.x+size.x,minArea.y+size.y);
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 colorM=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_MOD]);
  ImU32 colorC=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_CAR]);
  ImU32 colorL=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_LINE]);
  /* a disabled operator is faded, the way ESFM fades its weaker routes */
  ImVec4 offCol=uiColors[GUI_COLOR_FM_MOD];
  offCol.w*=0.3f;
  ImU32 colorOff=ImGui::GetColorU32(offCol);
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("yam10Alg"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_BG]),true,style.FrameRounding);
    const float circleRadius=6.0f*dpiScale+1.0f;
    ImVec2 pos[6];
    for (int i=0; i<6; i++) {
      pos[i]=ImLerp(rect.Min,rect.Max,ImVec2(0.17f+0.33f*(float)(i%3),0.30f+0.40f*(float)(i/3)));
    }
    static const char* const opLabel[6]={"1","2","3","4","5","6"};
    for (int i=0; i<6; i++) {
      for (int j=0; j<6; j++) {
        if (i==j) continue;                      /* the ring says this one */
        if (!(y.op[i].modIn&(1<<j))) continue;
        addAALine(dl,pos[j],pos[i],colorL);
      }
    }
    for (int i=0; i<6; i++) {
      ImU32 c=y.op[i].enable?((y.op[i].outLvl>0)?colorC:colorM):colorOff;
      dl->AddCircleFilled(pos[i],4.0f*dpiScale+1.0f,c);
      if (y.op[i].fb>0 || (y.op[i].modIn&(1<<i))) dl->AddCircle(pos[i],circleRadius,c);
      /* the offset measures "2" for every label so the column lines up */
      ImVec2 labelPos=pos[i];
      labelPos.x-=ImGui::CalcTextSize("2").x+circleRadius+3.0f*dpiScale;
      labelPos.y-=ImGui::CalcTextSize(opLabel[i]).y*0.5f;
      dl->AddText(labelPos,c,opLabel[i]);
    }
  }
}

void FurnaceGUI::drawInsYAM10(DivInstrument* ins) {
  if (!ImGui::BeginTabItem("YAM10")) return;
  DivInstrumentYAM10& y=ins->yam10;
  char tempID[1024];

  // follow whatever FM layout the user picked, widened for six operators
  int columns=2;
  switch (settings.fmLayout) {
    case 2: case 5: // 1xN
      columns=1;
      break;
    case 3: case 6: // as wide as six operators sensibly go
      columns=3;
      break;
    default: // 2x2, which is 2x3 here
      columns=2;
      break;
  }

  ImGui::Text("%s",_("Routing"));
  drawYAM10Algorithm(y,ImVec2(ImGui::GetContentRegionAvail().x,48.0f*dpiScale));
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s",_("carriers take the carrier colour\na ring is feedback, faded is off"));
  }

  ImVec2 oldPadding=ImGui::GetStyle().CellPadding;
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(8.0f*dpiScale,4.0f*dpiScale));
  if (ImGui::BeginTable("YAM10Operators",columns,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInner)) {
    for (int i=0; i<6; i++) {
      DivInstrumentYAM10::Operator& op=y.op[i];
      if (i%columns==0) ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::PushID(fmt::sprintf("yam10op%d",i).c_str());

      // outLvl above zero is what makes an operator a carrier
      if (settings.separateFMColors) {
        if (op.outLvl==0) {
          pushAccentColors(
            uiColors[GUI_COLOR_FM_PRIMARY_MOD],
            uiColors[GUI_COLOR_FM_SECONDARY_MOD],
            uiColors[GUI_COLOR_FM_BORDER_MOD],
            uiColors[GUI_COLOR_FM_BORDER_SHADOW_MOD]
          );
        } else {
          pushAccentColors(
            uiColors[GUI_COLOR_FM_PRIMARY_CAR],
            uiColors[GUI_COLOR_FM_SECONDARY_CAR],
            uiColors[GUI_COLOR_FM_BORDER_CAR],
            uiColors[GUI_COLOR_FM_BORDER_SHADOW_CAR]
          );
        }
      }

      ImGui::Dummy(ImVec2(dpiScale,dpiScale));
      snprintf(tempID,1024,_("Operator %d"),i+1);
      float nextCursorPosX=ImGui::GetCursorPosX()+0.5*(ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(tempID).x-ImGui::GetStyle().FramePadding.x*2.0f);
      YAM10_OP_DRAG_POINT;
      ImGui::SameLine();
      ImGui::SetCursorPosX(nextCursorPosX);
      pushToggleColors(op.enable);
      if (ImGui::Button(tempID)) {
        op.enable=!op.enable;
        PARAMETER;
      }
      popToggleColors();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("click to toggle operator"));
      }

      float sliderHeight=200.0f*dpiScale;
      float waveHeight=sliderHeight-ImGui::GetFrameHeightWithSpacing()*2.0f;
      if (waveHeight<40.0f*dpiScale) waveHeight=40.0f*dpiScale;

      if (op.ws>=YAM10_WAVES) op.ws=0;

      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,oldPadding);
      if (ImGui::BeginTable("yam10opParams",4,ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.45f);
        ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,0.55f);
        ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow();

        // ---- envelope rates ----
        // sustain sits where the user's susPosition setting puts it, like the
        // FM editor. sliders first, labels stamped over them afterwards.
        ImGui::TableNextColumn();
        int rateOrder[6];
        int rateCount=0;
        rateOrder[rateCount++]=FM_AR;
        rateOrder[rateCount++]=FM_DR;
        if (settings.susPosition==0) rateOrder[rateCount++]=FM_SL;
        rateOrder[rateCount++]=FM_D2R;
        rateOrder[rateCount++]=FM_RR;
        if (settings.susPosition>0) rateOrder[rateCount++]=FM_SL;

        float textY=ImGui::GetCursorPosY();
        // the delay comes before the attack, since that is the order the
        // envelope happens in
        CENTER_TEXT_20(_("Dl"));
        ImGui::TextUnformatted(_("Dl"));
        TOOLTIP_TEXT(_("Delay"));

        P(CWVSliderScalar("##YAM10DELAY",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.delay,&_YAM10_U8_ZERO,&_YAM10_DELAY_MAX)); rightClickable
        if (ImGui::IsItemHovered()) {
          if (op.delay>0) {
            ImGui::SetTooltip(_("delay: %d (%.0f ms)\nheld silent before the attack"),
              op.delay,(double)(256u<<((op.delay>7)?7:op.delay))/49716.0*1000.0);
          } else {
            ImGui::SetTooltip("%s",_("delay: off\nheld silent before the attack"));
          }
        }

        float rateX[6];
        for (int c=0; c<rateCount; c++) {
          ImGui::SameLine();
          rateX[c]=ImGui::GetCursorPosX();
          switch (rateOrder[c]) {
            case FM_AR:
              op.ar&=31;
              P(CWVSliderScalar("##AR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.ar,&_THIRTY_ONE,&_ZERO)); rightClickable
              break;
            case FM_DR:
              op.dr&=31;
              P(CWVSliderScalar("##DR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.dr,&_THIRTY_ONE,&_ZERO)); rightClickable
              break;
            case FM_D2R:
              op.d2r&=31;
              P(CWVSliderScalar("##D2R",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.d2r,&_THIRTY_ONE,&_ZERO)); rightClickable
              break;
            case FM_RR:
              op.rr&=15;
              P(CWVSliderScalar("##RR",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.rr,&_FIFTEEN,&_ZERO)); rightClickable
              break;
            default: // FM_SL
              op.sl&=15;
              P(CWVSliderScalar("##SL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.sl,&_FIFTEEN,&_ZERO)); rightClickable
              break;
          }
        }

        {
          ImVec2 prevCurPos=ImGui::GetCursorPos();
          for (int c=0; c<rateCount; c++) {
            ImGui::SetCursorPos(ImVec2(rateX[c],textY));
            CENTER_TEXT_20(FM_SHORT_NAME(rateOrder[c]));
            ImGui::TextUnformatted(FM_SHORT_NAME(rateOrder[c]));
            TOOLTIP_TEXT(FM_NAME(rateOrder[c]));
          }
          ImGui::SetCursorPos(prevCurPos);
          ImGui::Dummy(ImVec2(0,0));
        }

        bool ksr=op.ksr;
        if (ImGui::Checkbox(FM_NAME(FM_KSR),&ksr)) { PARAMETER
          op.ksr=ksr;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("higher notes run envelopes faster"));
        }

        // ---- waveform ----
        ImGui::TableNextColumn();
        ImGui::Text("%s",_("Waveform"));
        drawYAM10Waveform(op.ws,op.customWave,op.customWaveIndex,op.duty,ImVec2(ImGui::GetContentRegionAvail().x,waveHeight));

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (drawYAM10WaveSelect("##YAM10WS",op.ws,op.duty)) { PARAMETER }

        // only the pulse has a width to set, and the row is left in place
        // disabled so picking it does not shuffle everything below
        ImGui::BeginDisabled(op.customWave || op.ws!=YAM10_WF_PULSE);
        float dutyPercent=(float)op.duty*100.0f/256.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (CWSliderFloat("##YAM10Duty",&dutyPercent,0.0f,99.6f,_("width: %.1f%%"))) { PARAMETER
          int d=(int)(dutyPercent*256.0f/100.0f+0.5f);
          if (d<0) d=0;
          if (d>255) d=255;
          op.duty=(unsigned char)d;
        } rightClickable
        if (ImGui::IsItemHovered() && op.ws==YAM10_WF_PULSE && !op.customWave) {
          ImGui::SetTooltip("%s",_("pulse width\n12.5, 25, 50 and 75 are the usual ones"));
        }
        ImGui::EndDisabled();

        bool customWave=op.customWave;
        if (ImGui::Checkbox(YAM10_SHORT_NAME(YAM10_WT),&customWave)) { PARAMETER
          op.customWave=customWave;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("use a wavetable instead of a built in shape"));
        }
        ImGui::SameLine();
        // stays visible but disabled so the panel height never jumps
        ImGui::BeginDisabled(!op.customWave);
        int wi=op.customWaveIndex;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##YAM10WT",&wi)) {
          if (wi<0) wi=0;
          op.customWaveIndex=wi;
          PARAMETER
        } rightClickable
        ImGui::EndDisabled();

        // ---- envelope shape ----
        ImGui::TableNextColumn();
        ImGui::Text("%s",_("Envelope"));
        drawFMEnv(op.tl&127,op.ar&31,op.dr&31,op.d2r&31,op.rr&15,op.sl&15,0,0,0,127.0f,31.0f,15.0f,ImVec2(ImGui::GetContentRegionAvail().x,waveHeight),DIV_INS_YAM10);

        // ---- levels ----
        ImGui::TableNextColumn();
        float lvlTextY=ImGui::GetCursorPosY();
        CENTER_TEXT_20(FM_SHORT_NAME(FM_TL));
        ImGui::TextUnformatted(FM_SHORT_NAME(FM_TL));
        TOOLTIP_TEXT(FM_NAME(FM_TL));

        op.tl&=127;
        P(CWVSliderScalar("##TL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.tl,&_ONE_HUNDRED_TWENTY_SEVEN,&_ZERO)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("total level\n0.75 dB per step"));
        }
        ImGui::SameLine();
        float textX_OL=ImGui::GetCursorPosX();
        op.outLvl&=127;
        P(CWVSliderScalar("##OUTLVL",ImVec2(20.0f*dpiScale,sliderHeight),ImGuiDataType_U8,&op.outLvl,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("output level\nabove zero makes it a carrier"));
        }

        {
          ImVec2 prevCurPos=ImGui::GetCursorPos();
          ImGui::SetCursorPos(ImVec2(textX_OL,lvlTextY));
          CENTER_TEXT_20(YAM10_SHORT_NAME(YAM10_OL));
          ImGui::TextUnformatted(YAM10_SHORT_NAME(YAM10_OL));
          TOOLTIP_TEXT(YAM10_LONG_NAME(YAM10_OL));
          ImGui::SetCursorPos(prevCurPos);
          ImGui::Dummy(ImVec2(0,0));
        }

        ImGui::EndTable();
      }

      if (ImGui::BeginTable("yam10opParams2",2,ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_MULT));
        P(CWSliderScalar("##YAM10MULT",ImGuiDataType_U8,&op.mult,&_YAM10_U8_ZERO,&_YAM10_MULT_MAX,tempID)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("frequency multiplier\n0 is half"));
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_DT));
        P(CWSliderScalar("##YAM10DTS",ImGuiDataType_S8,&op.dtSemi,&_YAM10_MINUS_36,&_YAM10_36,tempID)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("detune in semitones"));
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_FINE));
        P(CWSliderScalar("##YAM10DTF",ImGuiDataType_S8,&op.dtFine,&_YAM10_MINUS_64,&_YAM10_63,tempID)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("detune in cents"));
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_RS));
        P(CWSliderScalar("##YAM10RS",ImGuiDataType_U8,&op.rs,&_ZERO,&_THREE,tempID)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("how much the note affects envelope speed"));
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        snprintf(tempID,1024,"%s: %%d",YAM10_SHORT_NAME(YAM10_PAN));
        P(CWSliderScalar("##YAM10PAN",ImGuiDataType_U8,&op.pan,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,tempID)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("panning\n128 is centre"));
        }

        ImGui::TableNextColumn();
        ImGui::Text("%s",YAM10_LONG_NAME(YAM10_MI));
        {
          float yCoordBeforeTablePadding=ImGui::GetCursorPosY();
          if (ImGui::BeginTable("yam10ModIn",3,ImGuiTableFlags_SizingStretchSame)) {
            for (int j=0; j<6; j++) {
              if (j%3==0) ImGui::TableNextRow();
              ImGui::TableNextColumn();
              if (j<3) ImGui::SetCursorPosY(yCoordBeforeTablePadding);
              bool on=op.modIn&(1<<j);
              if (j==i) {
                snprintf(tempID,1024,"%s##YAM10MOD%d",_("SELF"),j);
              } else {
                snprintf(tempID,1024,"O%d##YAM10MOD%d",j+1,j);
              }
              if (ImGui::Checkbox(tempID,&on)) {
                if (on) op.modIn|=(1<<j); else op.modIn&=~(1<<j);
                PARAMETER
              }
              if (ImGui::IsItemHovered()) {
                if (j==i) {
                  ImGui::SetTooltip("%s",_("feed the operator back into itself"));
                } else {
                  ImGui::SetTooltip("%s",fmt::sprintf(_("modulate with operator %d"),j+1).c_str());
                }
              }
            }
            ImGui::EndTable();
          }
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        snprintf(tempID,1024,"%s: %%d",FM_NAME(FM_FB));
        P(CWSliderScalar("##YAM10FB",ImGuiDataType_U8,&op.fb,&_ZERO,&_SEVEN,tempID)); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("self feedback\non every operator"));
        }

        ImGui::Text("%s",YAM10_SHORT_NAME(YAM10_PR));
        TOOLTIP_TEXT(YAM10_LONG_NAME(YAM10_PR));
        ImGui::SameLine();
        int pr=op.phaseReset;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##YAM10PR",&pr)) {
          if (pr<0) pr=0;
          if (pr>255) pr=255;
          op.phaseReset=pr;
          PARAMETER
        } rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("restart the operator every N ticks\n0 is off"));
        }

        bool fixedMode=op.fixedMode;
        if (ImGui::Checkbox(_("Fixed"),&fixedMode)) { PARAMETER
          op.fixedMode=fixedMode;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("ignore the note and hold one frequency"));
        }
        ImGui::BeginDisabled(!op.fixedMode);
        {
          int block=(op.fixedFreq>>10)&7;
          int fnum=op.fixedFreq&1023;
          ImGui::Text("%s",_("Blk"));
          TOOLTIP_TEXT(_("Block: each step doubles the frequency"));
          ImGui::SameLine();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (ImGui::InputInt("##YAM10FBLK",&block,1,1)) {
            if (block<0) block=0;
            if (block>7) block=7;
            op.fixedFreq=(block<<10)|fnum;
            PARAMETER
          } rightClickable
          ImGui::Text("%s",_("F"));
          TOOLTIP_TEXT(_("Frequency (F-Num)"));
          ImGui::SameLine();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (ImGui::InputInt("##YAM10FNUM",&fnum,1,16)) {
            if (fnum<0) fnum=0;
            if (fnum>1023) fnum=1023;
            op.fixedFreq=(block<<10)|fnum;
            PARAMETER
          } rightClickable
          double hz=(double)fnum*(double)(1<<block)/8.0;
          if (hz<10.0) {
            ImGui::Text(_("%.3f Hz"),hz);
          } else {
            ImGui::Text(_("%.1f Hz"),hz);
          }
          TOOLTIP_TEXT(_("0 holds the phase still. Low values give a slow noise step rate."));
        }
        ImGui::EndDisabled();

        ImGui::EndTable();
      }

      ImGui::PopStyleVar();

      if (settings.separateFMColors) {
        popAccentColors();
      }

      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();

  ImGui::EndTabItem();
}

static const char* yam10FilterModes[4]={
  _N("Low pass"), _N("High pass"), _N("Band pass"), _N("Notch")
};

// index 0-255 into the same frequency and Q curves the YM2609 effects use
static float yam10CutoffHz(unsigned char v) {
  if (v<96) return (float)(v+1);
  if (v<160) return (float)((v-96)*10+100);
  if (v<224) return (float)((v-160)*100+800);
  return (float)((v-224)*1000+7500);
}

static float yam10ResQ(unsigned char v) {
  if (v<96) return (float)(1.0/96.0*(v+1));
  if (v<192) return (float)(10.0/96.0*(v+1-96)+1.0);
  return (float)(10.0/64.0*(v+1-192)+11.0);
}

// ---- the EQ graph ----
// every value the chip holds is a byte into one of its own curves, so the
// graph converts back and forth rather than storing floats. dragging a node
// moves frequency and gain, the wheel changes Q, and right click picks the
// band shape.

static const char* yam10EQTypeNames[6]={
  _N("Peak"), _N("Low Shelf"), _N("High Shelf"),
  _N("Low Pass"), _N("High Pass"), _N("Notch")
};

// nearest byte in a rising 256 entry curve
static unsigned char yam10NearestIn(const float* tab, float v) {
  int best=0;
  float bestD=fabsf(tab[0]-v);
  for (int i=1; i<256; i++) {
    float d=fabsf(tab[i]-v);
    if (d<bestD) { bestD=d; best=i; }
  }
  return (unsigned char)best;
}

// magnitude of one band at one frequency, in dB
static float yam10EQBandAt(int type, float freq, float gain, float q, float at, float sr) {
  YAM10Biquad b;
  b.setEQ(type,freq,q,gain,sr);
  double wt=2.0*M_PI*(double)at/(double)sr;
  double c1=cos(wt), s1=sin(wt);
  double c2=cos(2.0*wt), s2=sin(2.0*wt);
  double nr=(double)b.b0+(double)b.b1*c1+(double)b.b2*c2;
  double ni=(double)b.b1*s1+(double)b.b2*s2;
  double dr=(double)b.a0+(double)b.a1*c1+(double)b.a2*c2;
  double di=(double)b.a1*s1+(double)b.a2*s2;
  double den=dr*dr+di*di;
  if (den<1.0e-30) return 0.0f;
  return (float)(10.0*log10((nr*nr+ni*ni)/den+1.0e-30));
}

void FurnaceGUI::drawYAM10EQ(DivInstrumentYAM10& y, const ImVec2& size) {
  yam10_build_dsp();
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();
  ImGuiStyle& style=ImGui::GetStyle();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(minArea.x+size.x,minArea.y+size.y);
  ImRect rect=ImRect(minArea,maxArea);
  ImGui::ItemSize(size,style.FramePadding.y);
  ImGuiID id=window->GetID("##yam10EQ");
  if (!ImGui::ItemAdd(rect,id)) return;
  // the wheel sets a band's Q, so take it away from the panel that would
  // otherwise scroll under it
  ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

  const float cw=size.x, chh=size.y;
  const float minF=20.0f;
  float sr=(float)yam10EQPreviewRate;
  float maxF=sr*0.49f;
  if (maxF<1000.0f) maxF=1000.0f;
  if (maxF>20000.0f) maxF=20000.0f;
  const float maxDB=15.0f;
  const float nodeR=6.0f*dpiScale+1.0f;

  ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_BG]),true,style.FrameRounding);

  // the theme decides the colours, so the graph follows a custom palette
  ImVec4 lineV=uiColors[GUI_COLOR_FM_ALG_LINE];
  ImVec4 gridV=lineV; gridV.w*=0.35f;
  ImVec4 majorV=lineV; majorV.w*=0.8f;
  ImU32 gridCol=ImGui::GetColorU32(gridV);
  ImU32 majorCol=ImGui::GetColorU32(majorV);
  ImU32 curveCol=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_CAR]);
  ImVec4 fillV=uiColors[GUI_COLOR_FM_CAR]; fillV.w*=0.13f;
  ImU32 fillCol=ImGui::GetColorU32(fillV);
  ImU32 textCol=ImGui::GetColorU32(majorV);

  dl->PushClipRect(rect.Min,rect.Max,true);

  static const int fLines[]={
    20,30,40,50,60,70,80,90,100,200,300,400,500,600,700,800,900,
    1000,2000,3000,4000,5000,6000,7000,8000,9000,10000,20000
  };
  for (size_t i=0; i<sizeof(fLines)/sizeof(fLines[0]); i++) {
    float f=(float)fLines[i];
    if (f<minF || f>maxF) continue;
    float x=rect.Min.x+cw*(logf(f/minF)/logf(maxF/minF));
    bool major=(fLines[i]==100 || fLines[i]==1000 || fLines[i]==10000);
    dl->AddLine(ImVec2(x,rect.Min.y),ImVec2(x,rect.Max.y),major?majorCol:gridCol,1.0f);
    if (major) {
      char buf[16];
      if (f>=1000.0f) snprintf(buf,16,"%.0fk",f/1000.0f); else snprintf(buf,16,"%.0f",f);
      dl->AddText(ImVec2(x+2.0f*dpiScale,rect.Min.y+1.0f),textCol,buf);
    }
  }
  for (int db=-12; db<=12; db+=6) {
    float yy=rect.Min.y+chh*(0.5f-(float)db/(2.0f*maxDB));
    dl->AddLine(ImVec2(rect.Min.x,yy),ImVec2(rect.Max.x,yy),(db==0)?majorCol:gridCol,(db==0)?1.5f:1.0f);
    if (db!=0) {
      char buf[8]; snprintf(buf,8,"%+d",db);
      dl->AddText(ImVec2(rect.Min.x+2.0f*dpiScale,yy+1.0f),textCol,buf);
    }
  }

  int nb=y.eqCount;
  if (nb>8) nb=8;

  // the summed curve
  const int N=192;
  ImVec2 prev(0.0f,0.0f);
  float zeroY=rect.Min.y+chh*0.5f;
  for (int i=0; i<=N; i++) {
    float x=(float)i/(float)N*cw;
    float f=minF*powf(maxF/minF,x/cw);
    float db=0.0f;
    for (int b=0; b<nb; b++) {
      if (!y.eqEnable || !y.eqBand[b].on) continue;
      db+=yam10EQBandAt(y.eqBand[b].type,yam10_freqTable[y.eqBand[b].freq],
                        yam10_gainTable[y.eqBand[b].gain],yam10_qTable[y.eqBand[b].q],f,sr);
    }
    float yy=rect.Min.y+chh*(0.5f-db/(2.0f*maxDB));
    if (yy<rect.Min.y) yy=rect.Min.y;
    if (yy>rect.Max.y) yy=rect.Max.y;
    ImVec2 cur(rect.Min.x+x,yy);
    if (i>0) {
      dl->AddQuadFilled(prev,cur,ImVec2(cur.x,zeroY),ImVec2(prev.x,zeroY),fillCol);
      dl->AddLine(prev,cur,curveCol,1.5f);
    }
    prev=cur;
  }

  ImVec2 mp=ImGui::GetIO().MousePos;
  /* through ImGui rather than by testing the rectangle, so the band menu
     drawn over the graph takes its own clicks instead of the canvas
     stealing them */
  bool inside=ImGui::IsItemHovered();
  bool held=ImGui::IsMouseDown(ImGuiMouseButton_Left);
  if (!held && yam10EQDrag>=0) {
    yam10EQDrag=-1;
    if (ImGui::GetActiveID()==id) ImGui::ClearActiveID();
  }

  bool overNode=false;
  for (int b=0; b<nb; b++) {
    DivInstrumentYAM10::EQBand& bd=y.eqBand[b];
    float f=yam10_freqTable[bd.freq];
    if (f<minF) f=minF;
    if (f>maxF) f=maxF;
    float nx=rect.Min.x+cw*(logf(f/minF)/logf(maxF/minF));
    float gdb=yam10_gainTable[bd.gain];
    /* the shapes with no gain of their own sit on the zero line */
    if (bd.type>=3) gdb=0.0f;
    float ny=rect.Min.y+chh*(0.5f-gdb/(2.0f*maxDB));
    if (ny<rect.Min.y) ny=rect.Min.y;
    if (ny>rect.Max.y) ny=rect.Max.y;

    float dx=mp.x-nx, dy=mp.y-ny;
    bool hovered=inside && (dx*dx+dy*dy)<(nodeR+5.0f*dpiScale)*(nodeR+5.0f*dpiScale);
    if (hovered) overNode=true;

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && yam10EQDrag<0) {
      yam10EQDrag=b;
      yam10EQBand=b;
      ImGui::SetActiveID(id,window);
      ImGui::SetFocusID(id,window);
    }
    if (yam10EQDrag==b && held) {
      float rx=mp.x-rect.Min.x;
      if (rx<0.0f) rx=0.0f;
      if (rx>cw) rx=cw;
      unsigned char fb=yam10NearestIn(yam10_freqTable,minF*powf(maxF/minF,rx/cw));
      if (fb<_YAM10_EQF_MIN) fb=_YAM10_EQF_MIN;
      if (fb>_YAM10_EQF_MAX) fb=_YAM10_EQF_MAX;
      if (fb!=bd.freq) { bd.freq=fb; PARAMETER }
      if (bd.type<3) {
        float ndb=(0.5f-(mp.y-rect.Min.y)/chh)*(2.0f*maxDB);
        if (ndb<-12.0f) ndb=-12.0f;
        if (ndb>12.0f) ndb=12.0f;
        unsigned char gb=yam10NearestIn(yam10_gainTable,ndb);
        if (gb!=bd.gain) { bd.gain=gb; PARAMETER }
      }
    }
    if ((hovered || yam10EQDrag==b) && ImGui::GetIO().MouseWheel!=0.0f) {
      float q=yam10_qTable[bd.q]*powf(1.2f,ImGui::GetIO().MouseWheel);
      unsigned char qb=yam10NearestIn(yam10_qTable,q);
      if (qb<_YAM10_EQQ_MIN) qb=_YAM10_EQQ_MIN;
      if (qb>_YAM10_EQQ_MAX) qb=_YAM10_EQQ_MAX;
      if (qb!=bd.q) { bd.q=qb; PARAMETER }
    }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      yam10EQBand=b;
      yam10EQPopupBand=b;
      ImGui::OpenPopup("##YAM10EQBand");
    }

    ImU32 c=(y.eqEnable&&bd.on)?curveCol:ImGui::GetColorU32(gridV);
    if (hovered||yam10EQDrag==b||yam10EQBand==b) {
      dl->AddCircleFilled(ImVec2(nx,ny),nodeR+3.0f*dpiScale,ImGui::GetColorU32(fillV));
    }
    dl->AddCircleFilled(ImVec2(nx,ny),nodeR,c);
    dl->AddCircle(ImVec2(nx,ny),nodeR,ImGui::GetColorU32(majorV),0,1.5f);
    char lbl[8]; snprintf(lbl,8,"%d",b+1);
    ImVec2 ts=ImGui::CalcTextSize(lbl);
    dl->AddText(ImVec2(nx-ts.x*0.5f,ny-ts.y*0.5f),ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ALG_BG]),lbl);
  }

  // clicking empty space drops a new band there
  if (inside && !overNode && yam10EQDrag<0 && !ImGui::IsPopupOpen("##YAM10EQBand") &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (y.eqCount<32) {
      int b=y.eqCount;
      DivInstrumentYAM10::EQBand& bd=y.eqBand[b];
      bd=DivInstrumentYAM10::EQBand();
      float rx=mp.x-rect.Min.x;
      if (rx<0.0f) rx=0.0f;
      if (rx>cw) rx=cw;
      /* a band dropped at the ends of the range wants to be a shelf. the
         bottom quarter takes a low shelf, the top quarter a high shelf, and
         anything between them stays a peak. */
      float where=rx/((cw>0.0f)?cw:1.0f);
      if (where<0.25f) bd.type=1;
      else if (where>0.75f) bd.type=2;
      else bd.type=0;
      unsigned char nf=yam10NearestIn(yam10_freqTable,minF*powf(maxF/minF,rx/cw));
      if (nf<_YAM10_EQF_MIN) nf=_YAM10_EQF_MIN;
      if (nf>_YAM10_EQF_MAX) nf=_YAM10_EQF_MAX;
      bd.freq=nf;
      float ndb=(0.5f-(mp.y-rect.Min.y)/chh)*(2.0f*maxDB);
      if (ndb<-12.0f) ndb=-12.0f;
      if (ndb>12.0f) ndb=12.0f;
      bd.gain=yam10NearestIn(yam10_gainTable,ndb);
      y.eqCount++;
      yam10EQBand=b;
      yam10EQDrag=b;
      ImGui::SetActiveID(id,window);
      ImGui::SetFocusID(id,window);
      PARAMETER
    }
  }

  dl->PopClipRect();

  if (ImGui::BeginPopup("##YAM10EQBand")) {
    int b=yam10EQPopupBand;
    if (b>=0 && b<(int)y.eqCount) {
      DivInstrumentYAM10::EQBand& bd=y.eqBand[b];
      ImGui::Text("%s %d",_("Band"),b+1);
      ImGui::Separator();
      P(ImGui::Checkbox(_("On##YAM10EQBON"),&bd.on));
      ImGui::Separator();
      for (int t=0; t<6; t++) {
        if (ImGui::Selectable(_(yam10EQTypeNames[t]),bd.type==t)) { bd.type=(unsigned char)t; PARAMETER }
      }
      ImGui::Separator();
      if (ImGui::Selectable(_("Remove"))) {
        for (int i=b; i+1<(int)y.eqCount; i++) y.eqBand[i]=y.eqBand[i+1];
        y.eqCount--;
        yam10EQBand=((int)y.eqCount>0)?((b<(int)y.eqCount)?b:(int)y.eqCount-1):-1;
        yam10EQPopupBand=-1;
        PARAMETER
      }
    }
    ImGui::EndPopup();
  }
}

void FurnaceGUI::drawInsYAM10DSP(DivInstrument* ins) {
  if (!ImGui::BeginTabItem("DSP")) return;
  DivInstrumentYAM10& y=ins->yam10;
  char tempID[1024];

  // three filters in series, then distortion, chorus and echo
  if (ImGui::BeginTable("yam10filters",3,ImGuiTableFlags_Borders|ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextRow();
    for (int i=0; i<3; i++) {
      ImGui::TableNextColumn();
      snprintf(tempID,1024,_("Filter %d"),i+1);
      CENTER_TEXT(tempID);
      ImGui::TextUnformatted(tempID);
    }

    ImGui::TableNextRow();
    for (int i=0; i<3; i++) {
      DivInstrumentYAM10::Filter& f=y.filter[i];
      ImGui::TableNextColumn();
      ImGui::PushID(fmt::sprintf("yam10filt%d",i).c_str());

      bool en=f.enable;
      if (ImGui::Checkbox(_("Enable"),&en)) { PARAMETER
        f.enable=en;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("the three filters run in series"));
      }

      ImGui::BeginDisabled(!f.enable);

      int mode=f.mode&3;
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      if (ImGui::Combo("##YAM10FMODE",&mode,yam10FilterModes,4)) { PARAMETER
        f.mode=mode;
      }

      snprintf(tempID,1024,"%s: %%d (%.0f Hz)",_("Cutoff"),yam10CutoffHz(f.cutoff));
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      P(CWSliderScalar("##YAM10FCUT",ImGuiDataType_U8,&f.cutoff,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,tempID)); rightClickable

      snprintf(tempID,1024,"%s: %%d (Q %.2f)",_("Resonance"),yam10ResQ(f.res));
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      P(CWSliderScalar("##YAM10FRES",ImGuiDataType_U8,&f.res,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,tempID)); rightClickable

      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("yam10dsp2",2,ImGuiTableFlags_Borders|ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    CENTER_TEXT(_("Distortion"));
    ImGui::TextUnformatted(_("Distortion"));
    ImGui::TableNextColumn();
    CENTER_TEXT(_("Chorus"));
    ImGui::TextUnformatted(_("Chorus"));

    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    {
      bool en=y.distEnable;
      if (ImGui::Checkbox(_("Enable##YAM10DISTEN"),&en)) { PARAMETER
        y.distEnable=en;
      }
      ImGui::BeginDisabled(!y.distEnable);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Gain"));
      P(CWSliderScalar("##YAM10DGAIN",ImGuiDataType_U8,&y.distGain,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("drive before the clip"));
      }

      snprintf(tempID,1024,"%s: %%d (%.0f Hz)",_("Cutoff"),yam10CutoffHz(y.distCutoff));
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      P(CWSliderScalar("##YAM10DCUT",ImGuiDataType_U8,&y.distCutoff,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,tempID)); rightClickable
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("cut lows before the clip"));
      }

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Level"));
      P(CWSliderScalar("##YAM10DLVL",ImGuiDataType_U8,&y.distLevel,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("level after the clip"));
      }

      ImGui::EndDisabled();
    }

    ImGui::TableNextColumn();
    {
      bool en=y.chorusEnable;
      if (ImGui::Checkbox(_("Enable##YAM10CHOREN"),&en)) { PARAMETER
        y.chorusEnable=en;
      }
      ImGui::BeginDisabled(!y.chorusEnable);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Mix"));
      P(CWSliderScalar("##YAM10CMIX",ImGuiDataType_U8,&y.chorusMix,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Rate"));
      P(CWSliderScalar("##YAM10CRATE",ImGuiDataType_U8,&y.chorusRate,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Depth"));
      P(CWSliderScalar("##YAM10CDEPTH",ImGuiDataType_U8,&y.chorusDepth,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Feedback"));
      P(CWSliderScalar("##YAM10CFB",ImGuiDataType_U8,&y.chorusFeedback,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      snprintf(tempID,1024,"%s: %%d",_("Width"));
      P(CWSliderScalar("##YAM10CWIDTH",ImGuiDataType_U8,&y.chorusWidth,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,tempID)); rightClickable
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("offset between the two sides\n0 is mono"));
      }

      ImGui::EndDisabled();
    }

    ImGui::EndTable();
  }

  if (ImGui::BeginTable("yam10rev",2,ImGuiTableFlags_Borders|ImGuiTableFlags_SizingStretchSame)) {
    yam10_build_dsp();
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    CENTER_TEXT(_("Reverb"));
    ImGui::TextUnformatted(_("Reverb"));
    P(ImGui::Checkbox(_("Enable##YAM10RVEN"),&y.reverbEnable));
    ImGui::BeginDisabled(!y.reverbEnable);
    snprintf(tempID,1024,"%s: %%d",_("Mix"));
    P(CWSliderScalar("##YAM10RVMIX",ImGuiDataType_U8,&y.reverbMix,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("wet level"));
    }
    snprintf(tempID,1024,"%s: %%d",_("Send"));
    P(CWSliderScalar("##YAM10RVSEND",ImGuiDataType_U8,&y.reverbSend,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    snprintf(tempID,1024,"%s: %%d",_("Decay"));
    P(CWSliderScalar("##YAM10RVDECAY",ImGuiDataType_U8,&y.reverbDecay,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("tail length"));
    }
    ImGui::EndDisabled();

    ImGui::TableNextColumn();
    CENTER_TEXT(_("Room"));
    ImGui::TextUnformatted(_("Room"));
    ImGui::BeginDisabled(!y.reverbEnable);
    snprintf(tempID,1024,"%s: %%d",_("Early"));
    P(CWSliderScalar("##YAM10RVEARLY",ImGuiDataType_U8,&y.reverbEarly,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("level of the first reflections"));
    }
    snprintf(tempID,1024,"%s: %%d",_("Diffusion"));
    P(CWSliderScalar("##YAM10RVDIFF",ImGuiDataType_U8,&y.reverbDiffusion,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("how much the tail is smeared\nlow leaves echoes, high is a wash"));
    }
    snprintf(tempID,1024,"%s: %%d",_("Size"));
    P(CWSliderScalar("##YAM10RVSIZE",ImGuiDataType_U8,&y.reverbSize,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("spacing of the early reflections"));
    }
    snprintf(tempID,1024,"%s: %%d",_("Damping"));
    P(CWSliderScalar("##YAM10RVDAMP",ImGuiDataType_U8,&y.reverbDamp,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("treble lost on every pass"));
    }
    ImGui::EndDisabled();
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("yam10comp",1,ImGuiTableFlags_Borders)) {
    yam10_build_dsp();
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    CENTER_TEXT(_("Compressor"));
    ImGui::TextUnformatted(_("Compressor"));

    P(ImGui::Checkbox(_("Enable##YAM10COMPEN"),&y.compEnable));

    ImGui::BeginDisabled(!y.compEnable);

    snprintf(tempID,1024,"%s: %+.1f dB",_("Threshold"),-60.0f+(float)y.compThreshold/127.0f*60.0f);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    P(CWSliderScalar("##YAM10CTHR",ImGuiDataType_U8,&y.compThreshold,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("above comes down, below goes up"));
    }

    /* timing and ratios, three across so the pairs line up */
    if (ImGui::BeginTable("yam10comptime",3,ImGuiTableFlags_SizingStretchSame)) {
      struct Knob { const char* name; const char* id; unsigned char* val; int kind; const char* tip; };
      char b0[64],b1[64],b2[64],b3[64],b4[64],b5[64];
      snprintf(b0,64,"%.1f ms",0.1f*powf(1000.0f,(float)y.compAttack/127.0f));
      snprintf(b1,64,"%.2f:1",1.0f+(float)y.compRatio/127.0f*19.0f);
      snprintf(b2,64,"%.2f:1",1.0f+(float)y.compUpRatio/127.0f*19.0f);
      snprintf(b3,64,"%.0f ms",1.0f*powf(1000.0f,(float)y.compDecay/127.0f));
      snprintf(b4,64,"%.0f Hz",yam10_freqTable[y.compLoMid]);
      snprintf(b5,64,"%.0f Hz",yam10_freqTable[y.compMidHi]);
      const char* names[6]={_("Attack"),_("Down"),_("Up"),_("Decay"),_("lo/mid"),_("mid/hi")};
      const char* ids[6]={"##YAM10CATK","##YAM10CDN","##YAM10CUP","##YAM10CDEC","##YAM10CX1","##YAM10CX2"};
      unsigned char* vals[6]={&y.compAttack,&y.compRatio,&y.compUpRatio,&y.compDecay,&y.compLoMid,&y.compMidHi};
      const char* reads[6]={b0,b1,b2,b3,b4,b5};
      const char* tips[6]={
        _("how fast it reacts to a rise"),
        _("ratio above the threshold"),
        _("ratio below the threshold\n1.00:1 is off"),
        _("how fast it lets go"),
        _("low to mid split"),
        _("mid to high split")
      };
      for (int i=0; i<6; i++) {
        if ((i%3)==0) ImGui::TableNextRow();
        ImGui::TableNextColumn();
        CENTER_TEXT(names[i]);
        ImGui::TextUnformatted(names[i]);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        const unsigned char* lo=&_YAM10_U8_ZERO;
        const unsigned char* hi=&_YAM10_U8_127;
        if (i==4) { lo=&_YAM10_XLO_MIN; hi=&_YAM10_XLO_MAX; }
        if (i==5) { lo=&_YAM10_XHI_MIN; hi=&_YAM10_XHI_MAX; }
        P(CWSliderScalar(ids[i],ImGuiDataType_U8,vals[i],lo,hi,reads[i])); rightClickable
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",tips[i]);
      }
      ImGui::EndTable();
    }

    ImGui::Separator();
    CENTER_TEXT(_("Band Levels"));
    ImGui::TextUnformatted(_("Band Levels"));

    /* the three band levels as faders, the way a rack unit lays them out */
    if (ImGui::BeginTable("yam10compband",3,ImGuiTableFlags_SizingStretchSame)) {
      const char* gname[3]={_("Low"),_("Mid"),_("High")};
      unsigned char* gp[3]={&y.compLowGain,&y.compMidGain,&y.compHighGain};
      const char* gid[3]={"##YAM10CGL","##YAM10CGM","##YAM10CGH"};
      float faderH=90.0f*dpiScale;
      ImGui::TableNextRow();
      for (int i=0; i<3; i++) {
        ImGui::TableNextColumn();
        CENTER_TEXT(gname[i]);
        ImGui::TextUnformatted(gname[i]);
        float avail=ImGui::GetContentRegionAvail().x;
        ImGui::Dummy(ImVec2((avail-24.0f*dpiScale)*0.5f,0.0f));
        ImGui::SameLine(0.0f,0.0f);
        P(CWVSliderScalar(gid[i],ImVec2(24.0f*dpiScale,faderH),ImGuiDataType_U8,gp[i],&_YAM10_GAIN_MIN,&_YAM10_GAIN_MAX,"")); rightClickable
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s",_("level back into this band\n128 is flat"));
        }
        snprintf(tempID,1024,"%+.1f dB",yam10_gainTable[*gp[i]]);
        CENTER_TEXT(tempID);
        ImGui::TextUnformatted(tempID);
      }
      ImGui::EndTable();
    }

    snprintf(tempID,1024,"%s: %+.1f dB",_("Output"),((float)y.compMakeup-64.0f)/63.0f*12.0f);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    P(CWSliderScalar("##YAM10CMK",ImGuiDataType_U8,&y.compMakeup,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN,tempID)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("level after the bands recombine"));
    }

    ImGui::EndDisabled();
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("yam10eq",1,ImGuiTableFlags_Borders)) {
    yam10_build_dsp();
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    P(ImGui::Checkbox(_("EQ##YAM10EQEN"),&y.eqEnable));
    ImGui::SameLine();
    P(ImGui::Checkbox(_("Invert Left##YAM10PIL"),&y.phaseInvL));
    ImGui::SameLine();
    P(ImGui::Checkbox(_("Invert Right##YAM10PIR"),&y.phaseInvR));
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("flips one side\ncancels on a mono mix"));
    }
    ImGui::SameLine();
    ImGui::Text("%d/%d",y.eqCount,32);
    ImGui::SameLine();
    ImGui::BeginDisabled(y.eqCount==0);
    if (ImGui::Button(_("Clear##YAM10EQCLR"))) { y.eqCount=0; yam10EQBand=-1; PARAMETER }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!y.eqEnable);
    yam10EQPreviewRate=(e->getAudioDescGot().rate>0)?e->getAudioDescGot().rate:44100;
    drawYAM10EQ(y,ImVec2(ImGui::GetContentRegionAvail().x,160.0f*dpiScale));
    if (ImGui::IsItemHovered() && yam10EQBand<0) {
      ImGui::SetTooltip("%s",_("click to place a band\ndrag to move, wheel for Q\nright click for the shape"));
    }

    if (y.eqCount==0) {
      ImGui::TextDisabled("%s",_("no bands. click the graph to place one"));
    } else {
      if (yam10EQBand<0 || yam10EQBand>=y.eqCount) yam10EQBand=0;
      int b=yam10EQBand;
      DivInstrumentYAM10::EQBand& bd=y.eqBand[b];

      for (int i=0; i<y.eqCount; i++) {
        if (i && (i%16)) ImGui::SameLine();
        String lbl=fmt::sprintf("%d##YAM10EQSEL%d",i+1,i);
        if (ImGui::RadioButton(lbl.c_str(),yam10EQBand==i)) yam10EQBand=i;
      }
      ImGui::SameLine();
      P(ImGui::Checkbox(_("On##YAM10EQON"),&bd.on));
      ImGui::SameLine();
      if (ImGui::Button(_("Remove##YAM10EQRM"))) {
        for (int i=b; i+1<y.eqCount; i++) y.eqBand[i]=y.eqBand[i+1];
        y.eqCount--;
        if (yam10EQBand>=y.eqCount) yam10EQBand=y.eqCount-1;
        PARAMETER
      }

      ImGui::SetNextItemWidth(160.0f*dpiScale);
      if (ImGui::BeginCombo(_("Type##YAM10EQTY"),_(yam10EQTypeNames[bd.type%6]))) {
        for (int t=0; t<6; t++) {
          if (ImGui::Selectable(_(yam10EQTypeNames[t]),bd.type==t)) { bd.type=(unsigned char)t; PARAMETER }
        }
        ImGui::EndCombo();
      }

      snprintf(tempID,1024,"%s: %.0f Hz",_("Freq"),yam10_freqTable[bd.freq]);
      P(CWSliderScalar("##YAM10EQF",ImGuiDataType_U8,&bd.freq,&_YAM10_EQF_MIN,&_YAM10_EQF_MAX,tempID)); rightClickable
      ImGui::BeginDisabled(bd.type>=3);
      snprintf(tempID,1024,"%s: %+.2f dB",_("Gain"),yam10_gainTable[bd.gain]);
      P(CWSliderScalar("##YAM10EQG",ImGuiDataType_U8,&bd.gain,&_YAM10_GAIN_MIN,&_YAM10_GAIN_MAX,tempID)); rightClickable
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",_("128 is flat\npass and notch have no gain"));
      }
      ImGui::EndDisabled();
      snprintf(tempID,1024,"%s: %.2f",_("Q"),yam10_qTable[bd.q]);
      P(CWSliderScalar("##YAM10EQQ",ImGuiDataType_U8,&bd.q,&_YAM10_EQQ_MIN,&_YAM10_EQQ_MAX,tempID)); rightClickable
    }

    ImGui::EndDisabled();
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("yam10echo",1,ImGuiTableFlags_Borders)) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    CENTER_TEXT(_("Echo"));
    ImGui::TextUnformatted(_("Echo"));

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(_("Mix")).x);
    P(CWSliderScalar(_("Mix##YAM10ECHOMIX"),ImGuiDataType_U8,&y.echoMix,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("wet level\n0 is off"));
    }

    int ed=y.echoDelay;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(_("Delay (ms)")).x);
    if (ImGui::InputInt(_("Delay (ms)##YAM10ECHODELAY"),&ed)) {
      if (ed<1) ed=1;
      if (ed>1000) ed=1000;
      y.echoDelay=ed;
      PARAMETER
    } rightClickable

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(_("Feedback")).x);
    P(CWSliderScalar(_("Feedback##YAM10ECHOFB"),ImGuiDataType_U8,&y.echoFeedback,&_ZERO,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s",_("how much echo feeds back"));
    }

    ImGui::EndTable();
  }

  ImGui::EndTabItem();
}

void FurnaceGUI::drawInsSID3(DivInstrument* ins) {
  char buffer[100];
  char buffer2[100];

  if (ImGui::BeginTabItem("SID3")) {
    if (ImGui::BeginTable("sid3Waves",2,0)) {
      ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed,0.0f);
      ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,0.0f);

      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      ImGui::AlignTextToFramePadding();
      ImGui::Text(_("Waveform"));
      ImGui::SameLine();
      pushToggleColors(ins->sid3.triOn);
      if (ImGui::Button(_("tri"))) { PARAMETER
        ins->sid3.triOn=!ins->sid3.triOn;
      }
      popToggleColors();
      ImGui::SameLine();
      pushToggleColors(ins->sid3.sawOn);
      if (ImGui::Button(_("saw"))) { PARAMETER
        ins->sid3.sawOn=!ins->sid3.sawOn;
      }
      popToggleColors();
      ImGui::SameLine();
      pushToggleColors(ins->sid3.pulseOn);
      if (ImGui::Button(_("pulse"))) { PARAMETER
        ins->sid3.pulseOn=!ins->sid3.pulseOn;
      }
      popToggleColors();
      ImGui::SameLine();
      pushToggleColors(ins->sid3.noiseOn);
      if (ImGui::Button(_("noise"))) { PARAMETER
        ins->sid3.noiseOn=!ins->sid3.noiseOn;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_("Like in SID2,specific noise LFSR feedback bits config can produce tonal waves.\n"
        "Refer to the manual for LFSR bits macro configurations for which frequency calculation is altered\n"
        "in a way that makes tonal noise stay in tune."));
      }
      popToggleColors();
      ImGui::SameLine();

      P(ImGui::Checkbox(_("1-bit noise"),&ins->sid3.oneBitNoise));
      ImGui::SameLine();

      pushToggleColors(ins->sid3.specialWaveOn);
      if (ImGui::Button(_("special"))) { PARAMETER
        ins->sid3.specialWaveOn=!ins->sid3.specialWaveOn;
      }
      popToggleColors();

      P(CWSliderScalar(_("Special wave"),ImGuiDataType_U8,&ins->sid3.special_wave,&_ZERO,&_SID3_SPECIAL_WAVES,_(sid3SpecialWaveforms[ins->sid3.special_wave%SID3_NUM_SPECIAL_WAVES]))); rightClickable

      if (ImGui::Checkbox(_("Wavetable channel"),&ins->sid3.doWavetable)) {
        PARAMETER;
        ins->temp.vZoom[DIV_MACRO_WAVE]=-1;
        for (int i=0; i<256; i++) {
          ins->std.waveMacro.val[i]=0;
        }
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_("Forces waveform macro to control wavetable index."));
      }

      bool invLeft=ins->sid3.phaseInv&SID3_INV_SIGNAL_LEFT;
      if (ImGui::Checkbox(_("Inv. left"),&invLeft)) { PARAMETER
        ins->sid3.phaseInv^=SID3_INV_SIGNAL_LEFT;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_("Invert left channel signal"));
      }
      ImGui::SameLine();
      bool invRight=ins->sid3.phaseInv&SID3_INV_SIGNAL_RIGHT;
      if (ImGui::Checkbox(_("Inv. right"),&invRight)) { PARAMETER
        ins->sid3.phaseInv^=SID3_INV_SIGNAL_RIGHT;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_("Invert right channel signal"));
      }

      ImGui::TableNextColumn();

      CENTER_TEXT(_("Special wave preview"));
      ImGui::TextUnformatted(_("Special wave preview"));
      drawWaveformSID3(ins->sid3.special_wave,ImVec2(120.0f*dpiScale,70.0f*dpiScale));

      ImGui::EndTable();
    }

    ImVec2 sliderSize=ImVec2(30.0f*dpiScale,256.0*dpiScale);

    if (ImGui::BeginTable("SID3EnvParams",6,ImGuiTableFlags_NoHostExtendX)) {
      ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
      ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
      ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
      ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
      ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
      ImGui::TableSetupColumn("c5",ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      CENTER_TEXT(_("A"));
      ImGui::TextUnformatted(_("A"));
      ImGui::TableNextColumn();
      CENTER_TEXT(_("D"));
      ImGui::TextUnformatted(_("D"));
      ImGui::TableNextColumn();
      CENTER_TEXT(_("S"));
      ImGui::TextUnformatted(_("S"));
      ImGui::TableNextColumn();
      CENTER_TEXT(_("SR"));
      ImGui::TextUnformatted(_("SR"));
      ImGui::TableNextColumn();
      CENTER_TEXT(_("R"));
      ImGui::TextUnformatted(_("R"));
      ImGui::TableNextColumn();
      CENTER_TEXT(_("Envelope"));
      ImGui::TextUnformatted(_("Envelope"));

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      P(CWVSliderScalar("##Attack",sliderSize,ImGuiDataType_U8,&ins->sid3.a,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
      ImGui::TableNextColumn();
      P(CWVSliderScalar("##Decay",sliderSize,ImGuiDataType_U8,&ins->sid3.d,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
      ImGui::TableNextColumn();
      P(CWVSliderScalar("##Sustain",sliderSize,ImGuiDataType_U8,&ins->sid3.s,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
      ImGui::TableNextColumn();
      P(CWVSliderScalar("##SustainRate",sliderSize,ImGuiDataType_U8,&ins->sid3.sr,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
      ImGui::TableNextColumn();
      P(CWVSliderScalar("##Release",sliderSize,ImGuiDataType_U8,&ins->sid3.r,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
      ImGui::TableNextColumn();
      // the (ins->sid3.r==15?(ins->sid3.r-1):ins->sid3.r) is used so release part never becomes horizontal (which isn't the case with SID3 envelope)
      drawSID3Env(0,(ins->sid3.a==0?(255):(256-ins->sid3.a)),(ins->sid3.d==0?(255):(256-ins->sid3.d)),ins->sid3.sr,255-(ins->sid3.r==255?(ins->sid3.r-1):ins->sid3.r),255-ins->sid3.s,0,0,0,255,256,255,ImVec2(ImGui::GetContentRegionAvail().x,sliderSize.y),ins->type);

      ImGui::EndTable();
    }
    
    if (!ins->sid3.doWavetable) {
      strncpy(buffer,macroSID3WaveMixMode(0,(float)ins->sid3.mixMode,NULL).c_str(),40);
      P(CWSliderScalar(_("Wave Mix Mode"),ImGuiDataType_U8,&ins->sid3.mixMode,&_ZERO,&_FOUR,buffer));
      P(CWSliderScalar(_("Duty"),ImGuiDataType_U16,&ins->sid3.duty,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
      P(CWSliderScalar(_("Feedback"),ImGuiDataType_U8,&ins->sid3.feedback,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE));
      bool resetDuty=ins->sid3.resetDuty;
      if (ImGui::Checkbox(_("Reset duty on new note"),&resetDuty)) { PARAMETER
        ins->sid3.resetDuty=resetDuty;
      }
      if (ImGui::Checkbox(_("Absolute Duty Macro"),&ins->sid3.dutyIsAbs)) {
        ins->temp.vZoom[DIV_MACRO_DUTY]=-1;
        PARAMETER;
      }
    }

    bool ringMod=ins->sid3.ringMod;
    if (ImGui::Checkbox(_("Ring Modulation"),&ringMod)) { PARAMETER
      ins->sid3.ringMod=ringMod;
    }

    ImGui::SameLine();

    strncpy(buffer,macroSID3SourceChan(0,(float)ins->sid3.ring_mod_source,NULL).c_str(),40);
    P(CWSliderScalar(_("Source channel##rmsrc"),ImGuiDataType_U8,&ins->sid3.ring_mod_source,&_ZERO,&_SID3_NUM_CHANNELS,buffer));

    bool oscSync=ins->sid3.oscSync;
    if (ImGui::Checkbox(_("Oscillator Sync"),&oscSync)) { PARAMETER
      ins->sid3.oscSync=oscSync;
    }

    ImGui::SameLine();

    strncpy(buffer,macroSID3SourceChan(0,(float)ins->sid3.sync_source,NULL).c_str(),40);
    P(CWSliderScalar(_("Source channel##hssrc"),ImGuiDataType_U8,&ins->sid3.sync_source,&_ZERO,&_SID3_NUM_CHANNELS_MINUS_ONE,buffer));

    bool phaseMod=ins->sid3.phase_mod;
    if (ImGui::Checkbox(_("Phase modulation"),&phaseMod)) { PARAMETER
      ins->sid3.phase_mod=phaseMod;
    }

    ImGui::SameLine();

    strncpy(buffer,macroSID3SourceChan(0,(float)ins->sid3.phase_mod_source,NULL).c_str(),40);
    P(CWSliderScalar(_("Source channel##pmsrc"),ImGuiDataType_U8,&ins->sid3.phase_mod_source,&_ZERO,&_SID3_NUM_CHANNELS_MINUS_ONE,buffer));

    ImGui::Separator();

    if (!ins->sid3.doWavetable) {
      bool sepNoisePitch=ins->sid3.separateNoisePitch;
      if (ImGui::Checkbox(_("Separate noise pitch"),&sepNoisePitch)) { PARAMETER
        ins->sid3.separateNoisePitch=sepNoisePitch;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_("Make noise pitch independent from other waves' pitch.\nNoise pitch will be controllable via macros."));
      }
    }

    for (int i=0; i<SID3_NUM_FILTERS; i++) {
      DivInstrumentSID3::Filter* filt=&ins->sid3.filt[i];

      if (filt->enabled) {
        ImGui::Separator();
      }

      bool enable=filt->enabled;
      snprintf(buffer,100,_("Enable filter %d"),i+1);
      if (ImGui::Checkbox(buffer,&enable)) { PARAMETER
        filt->enabled=enable;
      }

      if (filt->enabled) {
        bool init=filt->init;
        snprintf(buffer,100,_("Initialize filter %d"),i+1);
        if (ImGui::Checkbox(buffer,&init)) { PARAMETER
          filt->init=init;
        }
        ImGui::SameLine();
        snprintf(buffer,100,_("Connect to channel input##contoinput%d"),i+1);
        bool toInput=filt->mode&SID3_FILTER_CHANNEL_INPUT;
        if (ImGui::Checkbox(buffer,&toInput)) { PARAMETER
          filt->mode^=SID3_FILTER_CHANNEL_INPUT;
        }

        snprintf(buffer,100,_("Cutoff##fcut%d"),i+1);
        P(CWSliderScalar(buffer,ImGuiDataType_U16,&filt->cutoff,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
        snprintf(buffer,100,_("Resonance##fres%d"),i+1);
        P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->resonance,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
        snprintf(buffer,100,_("Output volume##foutvol%d"),i+1);
        P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->output_volume,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
        snprintf(buffer,100,_("Distortion level##fdist%d"),i+1);
        P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->distortion_level,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable

        ImGui::AlignTextToFramePadding();
        ImGui::Text(_("Filter Mode"));
        ImGui::SameLine();

        bool lp=filt->mode&SID3_FILTER_LP;
        pushToggleColors(lp);
        snprintf(buffer,100,_("low##flow%d"),i+1);
        if (ImGui::Button(buffer)) { PARAMETER
          filt->mode^=SID3_FILTER_LP;
        }
        popToggleColors();
        ImGui::SameLine();

        bool bp=filt->mode&SID3_FILTER_BP;
        pushToggleColors(bp);
        snprintf(buffer,100,_("band##fband%d"),i+1);
        if (ImGui::Button(buffer)) { PARAMETER
          filt->mode^=SID3_FILTER_BP;
        }
        popToggleColors();
        ImGui::SameLine();

        bool hp=filt->mode&SID3_FILTER_HP;
        pushToggleColors(hp);
        snprintf(buffer,100,_("high##fhigh%d"),i+1);
        if (ImGui::Button(buffer)) { PARAMETER
          filt->mode^=SID3_FILTER_HP;
        }
        popToggleColors();


        ImGui::SameLine();
        snprintf(buffer,100,_("Connect to channel output##contooutput%d"),i+1);
        bool toOutput=filt->mode&SID3_FILTER_OUTPUT;
        if (ImGui::Checkbox(buffer,&toOutput)) { PARAMETER
          filt->mode^=SID3_FILTER_OUTPUT;
        }

        snprintf(buffer,100,_("Absolute cutoff macro##abscutoff%d"),i+1);
        bool absCutoff=filt->absoluteCutoff;
        if (ImGui::Checkbox(buffer,&absCutoff)) { PARAMETER
          filt->absoluteCutoff=!filt->absoluteCutoff;
          ins->temp.vZoom[DIV_MACRO_OP_D2R+(i<<5)]=-1;
        }

        snprintf(buffer,100,_("Change cutoff with pitch##bindcutoff%d"),i+1);
        P(ImGui::Checkbox(buffer,&filt->bindCutoffToNote));
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(_("Filter cutoff will change with frequency/pitch.\nSee settings below."));
        }

        if (filt->bindCutoffToNote) {
          snprintf(buffer,100,_("Decrease cutoff when pitch increases##decreasecutoff%d"),i+1);
          P(ImGui::Checkbox(buffer,&filt->bindCutoffToNoteDir));
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("If this is enabled,filter cutoff will decrease if you increase the pitch.\n"
            "If this is disabled,filter cutoff will increase if you increase the pitch."));
          }

          snprintf(buffer2,100,_("%s"),noteNameNormal(filt->bindCutoffToNoteCenter));
          snprintf(buffer,100,_("Cutoff change center note##bindcutcenternote%d"),i+1);
          P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->bindCutoffToNoteCenter,&_ZERO,&_ONE_HUNDRED_SEVENTY_NINE,buffer2)); rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("The center note for cutoff changes. At this note no cutoff change happens.\nAs pitch goes lower or higher,cutoff changes apply."));
          }

          snprintf(buffer,100,_("Cutoff change strength##bindcutstrength%d"),i+1);
          P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->bindCutoffToNoteStrength,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("How much cutoff changes for given pitch change."));
          }
          snprintf(buffer,100,_("Scale cutoff only once on new note##bindcutnn%d"),i+1);
          P(ImGui::Checkbox(buffer,&filt->bindCutoffOnNote));
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("Filter cutoff will be changed only once on new note.\nIf this option is disabled,cutoff scaling will be applied\nevery time a pitch change happens."));
          }
        }

        snprintf(buffer,100,_("Change resonance with pitch##bindres%d"),i+1);
        P(ImGui::Checkbox(buffer,&filt->bindResonanceToNote));
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(_("Filter resonance will change with frequency/pitch.\nSee settings below."));
        }

        if (filt->bindResonanceToNote) {
          snprintf(buffer,100,_("Decrease resonance when pitch increases##decreaseres%d"),i+1);
          P(ImGui::Checkbox(buffer,&filt->bindResonanceToNoteDir));
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("If this is enabled,filter resonance will decrease if you increase the pitch.\n"
            "If this is disabled,filter resonance will increase if you increase the pitch."));
          }

          snprintf(buffer2,100,_("%s"),noteNameNormal(filt->bindResonanceToNoteCenter));
          snprintf(buffer,100,_("Resonance change center note##bindrescenternote%d"),i+1);
          P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->bindResonanceToNoteCenter,&_ZERO,&_ONE_HUNDRED_SEVENTY_NINE,buffer2)); rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("The center note for resonance changes. At this note no resonance change happens.\nAs pitch goes lower or higher,resonance changes apply."));
          }

          snprintf(buffer,100,_("Resonance change strength##bindresstrength%d"),i+1);
          P(CWSliderScalar(buffer,ImGuiDataType_U8,&filt->bindResonanceToNoteStrength,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("How much resonance changes for given pitch change."));
          }
          snprintf(buffer,100,_("Scale resonance only once on new note##bindresnn%d"),i+1);
          P(ImGui::Checkbox(buffer,&filt->bindResonanceOnNote));
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("Filter resonance will be changed only once on new note.\nIf this option is disabled,resonance scaling will be applied\nevery time a pitch change happens."));
          }
        }
      }
    }

    ImGui::Separator();

    if (ImGui::BeginTable("SID3filtmatrix",1)) {
      if (waveGenVisible) ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,250.0f*dpiScale);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      CENTER_TEXT(_("Filters connection matrix"));
      ImGui::Text(_("Filters connection matrix"));

      if (ImGui::BeginTable("SID3checkboxesmatrix",3+SID3_NUM_FILTERS)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text(">>");
        ImGui::TableNextColumn();
        ImGui::Text(_("In"));

        for (int i=0; i<SID3_NUM_FILTERS; i++) {
          ImGui::TableNextColumn();
          ImGui::Text("%d",i+1);
        }

        ImGui::TableNextColumn();
        ImGui::Text(_("Out"));

        ImGui::TableNextRow();

        for (int i=0; i<SID3_NUM_FILTERS; i++) {
          DivInstrumentSID3::Filter* filt=&ins->sid3.filt[i];

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%d",i+1);

          ImGui::TableNextColumn();

          snprintf(buffer,40,"##filtmatrixin%d",i+1);
          bool toInput=filt->mode&SID3_FILTER_CHANNEL_INPUT;
          if (ImGui::Checkbox(buffer,&toInput)) { PARAMETER
            filt->mode^=SID3_FILTER_CHANNEL_INPUT;
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("Feed signal from channel to filter %d input"),i+1);
          }

          for (int j=0; j<SID3_NUM_FILTERS; j++) {
            ImGui::TableNextColumn();
            snprintf(buffer,40,"##filtmatrix%d%d",i+1,j+1);

            bool enable=filt->filter_matrix&(1<<j);
            if (ImGui::Checkbox(buffer,&enable)) { PARAMETER
              filt->filter_matrix^=(1<<j);
            }
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip(_("Feed signal from filter %d output to filter %d input"),j+1,i+1);
            }
          }

          ImGui::TableNextColumn();

          snprintf(buffer,40,"##filtmatrixout%d",i+1);
          bool toOutput=filt->mode&SID3_FILTER_OUTPUT;
          if (ImGui::Checkbox(buffer,&toOutput)) { PARAMETER
            filt->mode^=SID3_FILTER_OUTPUT;
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("Feed signal from filter %d output to channel output"),i+1);
          }
        }

        ImGui::EndTable();
      }

      ImGui::EndTable();
    }

    ImGui::EndTabItem();
  }

  if (!ins->amiga.useSample) {
    insTabWavetable(ins);
  }
  insTabSample(ins);

  std::vector<FurnaceGUIMacroDesc> macroList;

  for (int i=0; i<SID3_NUM_FILTERS; i++) {
    snprintf(buffer,40,_("Filter %d macros"),i+1);

    if (ImGui::BeginTabItem(buffer)) {
      macroList.push_back(FurnaceGUIMacroDesc(_("Cutoff"),&ins->std.opMacros[i].d2rMacro,ins->sid3.filt[i].absoluteCutoff?0:-65535,65535,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Resonance"),&ins->std.opMacros[i].damMacro,0,255,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter Toggle"),&ins->std.opMacros[i].drMacro,0,1,32,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Filter Mode"),&ins->std.opMacros[i].ksrMacro,0,3,48,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,filtModeBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Distortion Level"),&ins->std.opMacros[i].dt2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Output Volume"),&ins->std.opMacros[i].dtMacro,0,255,160,uiColors[GUI_COLOR_MACRO_FILTER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Channel Input Connection"),&ins->std.opMacros[i].dvbMacro,0,1,32,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Channel Output Connection"),&ins->std.opMacros[i].egtMacro,0,1,32,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true));
      macroList.push_back(FurnaceGUIMacroDesc(_("Connection Matrix Row"),&ins->std.opMacros[i].kslMacro,0,SID3_NUM_FILTERS,16*SID3_NUM_FILTERS,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,sid3FilterMatrixBits));

      drawMacros(macroList,macroEditStateOP[i],ins);

      ImGui::EndTabItem();
    }
  }
}

void FurnaceGUI::drawInsEdit() {
  if (nextWindow==GUI_WINDOW_INS_EDIT) {
    insEditOpen=true;
    ImGui::SetNextWindowFocus();
    nextWindow=GUI_WINDOW_NOTHING;
  }
  if (!insEditOpen) return;
  if (mobileUI) {
    patWindowPos=(portrait?ImVec2(0.0f,(mobileMenuPos*-0.65*canvasH)):ImVec2((0.16*canvasH)+0.5*canvasW*mobileMenuPos,0.0f));
    patWindowSize=(portrait?ImVec2(canvasW,canvasH-(0.16*canvasW)-(pianoOpen?(0.4*canvasW):0.0f)):ImVec2(canvasW-(0.16*canvasH),canvasH-(pianoOpen?(0.3*canvasH):0.0f)));
    ImGui::SetNextWindowPos(patWindowPos);
    ImGui::SetNextWindowSize(patWindowSize);
  } else {
    ImGui::SetNextWindowSizeConstraints(ImVec2(440.0f*dpiScale,400.0f*dpiScale),ImVec2(canvasW,canvasH));
  }
  if (ImGui::Begin("Instrument Editor",&insEditOpen,globalWinFlags|(settings.allowEditDocking?0:ImGuiWindowFlags_NoDocking),_("Instrument Editor"))) {
    DivInstrument* ins=NULL;
    if (curIns==-2) {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY()+(ImGui::GetContentRegionAvail().y-ImGui::GetFrameHeightWithSpacing()+ImGui::GetStyle().ItemSpacing.y)*0.5f);
      CENTER_TEXT(_("waiting..."));
      ImGui::Text(_("waiting..."));
    } else if (curIns<0 || curIns>=(int)e->song.ins.size()) {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY()+(ImGui::GetContentRegionAvail().y-ImGui::GetFrameHeightWithSpacing()*(e->song.ins.empty()?2.0f:3.0f)+ImGui::GetStyle().ItemSpacing.y)*0.5f);
      CENTER_TEXT(_("no instrument selected"));
      ImGui::Text(_("no instrument selected"));
      if (ImGui::BeginTable("noAssetCenter",3)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.5f);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,0.5f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();

        if (e->song.ins.size()>0) {
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (ImGui::BeginCombo("##InsSelect",_("select one..."))) {
            String name;
            for (size_t i=0; i<e->song.ins.size(); i++) {
              name=fmt::sprintf("%.2X: %s##_INSS%d",i,e->song.ins[i]->name,i);
              if (ImGui::Selectable(name.c_str(),curIns==(int)i)) {
                setCurIns(i);
                wavePreviewInit=true;
                updateFMPreview=true;
                ins=e->song.ins[curIns];
              }
            }
            ImGui::EndCombo();
          }
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted(_("or"));
          ImGui::SameLine();
        }
        if (ImGui::Button(_("Open"))) {
          doAction(GUI_ACTION_INS_LIST_OPEN);
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(_("or"));
        ImGui::SameLine();
        if (ImGui::Button(_("Create New"))) {
          doAction(GUI_ACTION_INS_LIST_ADD);
        }

        ImGui::TableNextColumn();
        ImGui::EndTable();
      }
    } else {
      ins=e->song.ins[curIns];
      if (updateFMPreview) {
        renderFMPreview(ins);
        updateFMPreview=false;
      }
      if (settings.insEditColorize) {
        if (ins->type>=DIV_INS_MAX) {
          pushAccentColors(uiColors[GUI_COLOR_INSTR_UNKNOWN],uiColors[GUI_COLOR_INSTR_UNKNOWN],uiColors[GUI_COLOR_INSTR_UNKNOWN],ImVec4(0.0f,0.0f,0.0f,0.0f));
        } else {
          pushAccentColors(uiColors[GUI_COLOR_INSTR_STD+ins->type],uiColors[GUI_COLOR_INSTR_STD+ins->type],uiColors[GUI_COLOR_INSTR_STD+ins->type],ImVec4(0.0f,0.0f,0.0f,0.0f));
        }
      }
      if (ImGui::BeginTable("InsProp",3)) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        String insIndex=fmt::sprintf("%.2X",curIns);
        ImGui::SetNextItemWidth(72.0f*dpiScale);
        if (ImGui::BeginCombo("##InsSelect",insIndex.c_str())) {
          String name;
          for (size_t i=0; i<e->song.ins.size(); i++) {
            name=fmt::sprintf("%.2X: %s##_INSS%d",i,e->song.ins[i]->name,i);
            if (ImGui::Selectable(name.c_str(),curIns==(int)i)) {
              setCurIns(i);
              ins=e->song.ins[curIns];
              wavePreviewInit=true;
              updateFMPreview=true;
            }
          }
          ImGui::EndCombo();
        }

        ImGui::TableNextColumn();
        ImGui::Text(_("Name"));

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushID(2+curIns);
        if (ImGui::InputText("##Name",&ins->name)) {
          MARK_MODIFIED;
        }
        ImGui::PopID();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN "##IELoad")) {
          doAction(GUI_ACTION_INS_LIST_OPEN_REPLACE);
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(_("Open"));
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FLOPPY_O "##IESave")) {
          doAction(GUI_ACTION_INS_LIST_SAVE);
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(_("Save"));
        }
        if (ImGui::BeginPopupContextItem("InsSaveFormats",ImGuiMouseButton_Right)) {
          if (ImGui::MenuItem(_("save as .dmp..."))) {
            doAction(GUI_ACTION_INS_LIST_SAVE_DMP);
          }
          ImGui::EndPopup();
        }

        ImGui::TableNextColumn();
        ImGui::Text(_("Type"));

        ImGui::TableNextColumn();
        int insType=ins->type;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        bool warnType=true;
        for (DivInstrumentType i: e->getPossibleInsTypes()) {
          if (i==insType) {
            warnType=false;
          }
        }

        pushWarningColor(warnType,warnType && failedNoteOn);
        if (ImGui::BeginCombo("##Type",(insType>=DIV_INS_MAX)?_("Unknown"):_(insTypes[insType][0]))) {
          std::vector<DivInstrumentType> insTypeList;
          if (settings.displayAllInsTypes) {
            for (int i=0; insTypes[i][0]; i++) {
              insTypeList.push_back((DivInstrumentType)i);
            }
          } else {
            insTypeList=e->getPossibleInsTypes();
          }
          for (DivInstrumentType i: insTypeList) {
            if (ImGui::Selectable(insTypes[i][0],insType==i)) {
              ins->type=i;
              MARK_MODIFIED;

              // reset macro zoom
              memset(ins->temp.vZoom,-1,sizeof(ins->temp.vZoom));
            }
          }
          ImGui::EndCombo();
        } else if (warnType) {
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("none of the currently present chips are able to play this instrument type!"));
          }
        }
        popWarningColor();

        ImGui::EndTable();
      }
      

      if (ImGui::BeginTabBar("insEditTab")) {
        std::vector<FurnaceGUIMacroDesc> macroList;
        if (ins->type==DIV_INS_YAM10) {
          drawInsYAM10(ins);
          drawInsYAM10DSP(ins);
        }

        if (ins->type==DIV_INS_KLATTSCH) if (ImGui::BeginTabItem("klattsch")) {
          ImGui::TextWrapped(_("These defaults shape the phoneme-bank voice. Pattern effects override them; reset values return to this profile."));
          ImGui::Separator();

          P(CWSliderScalar(_("Transition time (ticks)"),ImGuiDataType_U8,&ins->klattsch.transition,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable

          String valueText=(ins->klattsch.voicing==0xff)?_("bank value"):fmt::sprintf("%.3f",(float)ins->klattsch.voicing/255.0f);
          P(CWSliderScalar(_("Voicing"),ImGuiDataType_U8,&ins->klattsch.voicing,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          valueText=(ins->klattsch.aspiration==0xff)?_("bank value"):fmt::sprintf("%.3f",(float)ins->klattsch.aspiration/255.0f);
          P(CWSliderScalar(_("Aspiration"),ImGuiDataType_U8,&ins->klattsch.aspiration,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          if (ins->klattsch.tilt==0xff) {
            valueText=_("bank value");
          } else {
            int tilt=(ins->klattsch.tilt<0x80)?ins->klattsch.tilt:(ins->klattsch.tilt-0x100);
            valueText=fmt::sprintf("%d",tilt);
          }
          P(CWSliderScalar(_("Spectral tilt"),ImGuiDataType_U8,&ins->klattsch.tilt,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          valueText=(ins->klattsch.effort==0xff)?_("bank value"):fmt::sprintf("%.3f",(float)ins->klattsch.effort/255.0f);
          P(CWSliderScalar(_("Glottal effort"),ImGuiDataType_U8,&ins->klattsch.effort,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          valueText=(ins->klattsch.gain==0)?_("bank value"):fmt::sprintf("%.3f",(float)ins->klattsch.gain/16.0f);
          P(CWSliderScalar(_("Gain"),ImGuiDataType_U8,&ins->klattsch.gain,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          valueText=(ins->klattsch.bandwidth==0)?_("neutral"):fmt::sprintf("%.3f",(float)ins->klattsch.bandwidth/64.0f);
          P(CWSliderScalar(_("Formant bandwidth scale"),ImGuiDataType_U8,&ins->klattsch.bandwidth,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          valueText=(ins->klattsch.formantShift==0)?_("neutral"):fmt::sprintf("%.3f",(float)ins->klattsch.formantShift/64.0f);
          P(CWSliderScalar(_("Formant shift"),ImGuiDataType_U8,&ins->klattsch.formantShift,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE,valueText.c_str())); rightClickable

          ImGui::SeparatorText(_("Modulation defaults"));
          int vibRate=(ins->klattsch.vibrato>>4)&15;
          int vibDepth=ins->klattsch.vibrato&15;
          if (CWSliderInt(_("Vibrato rate (Hz)"),&vibRate,0,15)) {
            ins->klattsch.vibrato=(vibRate<<4)|vibDepth;
            PARAMETER
          }
          if (CWSliderInt(_("Vibrato depth (4 Hz steps)"),&vibDepth,0,15)) {
            ins->klattsch.vibrato=(vibRate<<4)|vibDepth;
            PARAMETER
          }

          int tremRate=(ins->klattsch.tremolo>>4)&15;
          int tremDepth=ins->klattsch.tremolo&15;
          if (CWSliderInt(_("Tremolo rate (Hz)"),&tremRate,0,15)) {
            ins->klattsch.tremolo=(tremRate<<4)|tremDepth;
            PARAMETER
          }
          if (CWSliderInt(_("Tremolo depth"),&tremDepth,0,15)) {
            ins->klattsch.tremolo=(tremRate<<4)|tremDepth;
            PARAMETER
          }

          ImGui::EndTabItem();
        }

        if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPLL || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_OPM || ins->type==DIV_INS_ESFM || ins->type==DIV_INS_YAM10) {
          char label[32];
          int opCount=4;
          if (ins->type==DIV_INS_YAM10) opCount=6;
          if (ins->type==DIV_INS_OPLL) opCount=2;
          if (ins->type==DIV_INS_OPL) opCount=(ins->fm.ops==4)?4:2;

          if (ins->type!=DIV_INS_YAM10) insTabFM(ins);
          if (ins->type==DIV_INS_YAM10) {
            // the FM page carries what is chip wide FM, the way every other
            // FM chip's does. everything else YAM10 shapes is per operator
            // and lives on the OP pages.
            if (ImGui::BeginTabItem(_("FM Macros"))) {
              macroList.push_back(FurnaceGUIMacroDesc(_("OpMask"),&ins->std.algMacro,0,6,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,yam10OperatorBits));
              macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FB),&ins->std.fbMacro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
              drawMacros(macroList,macroEditStateFM,ins);
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(_("DSP Macros"))) {
  macroList.push_back(FurnaceGUIMacroDesc(_("Filter 1 Cutoff"),&ins->std.ex1Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Filter 1 Resonance"),&ins->std.ex2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Filter 2 Cutoff"),&ins->std.ex3Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Filter 2 Resonance"),&ins->std.ex4Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Filter 3 Cutoff"),&ins->std.ex5Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Filter 3 Resonance"),&ins->std.ex6Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Distortion Gain"),&ins->std.ex7Macro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Distortion Level"),&ins->std.ex8Macro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Chorus Mix"),&ins->std.ex9Macro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Chorus Rate"),&ins->std.waveMacro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Chorus Depth"),&ins->std.dutyMacro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Echo Mix"),&ins->std.ex10Macro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
  macroList.push_back(FurnaceGUIMacroDesc(_("Echo Feedback"),&ins->std.amsMacro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              drawMacros(macroList,macroEditStateDSP,ins);
              ImGui::EndTabItem();
            }
          }
          if (ins->type!=DIV_INS_ESFM && ins->type!=DIV_INS_YAM10) {
            if (ImGui::BeginTabItem(_("FM Macros"))) {
              if (ins->type==DIV_INS_OPLL) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SUS),&ins->std.algMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FB),&ins->std.fbMacro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DC),&ins->std.fmsMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DM),&ins->std.amsMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              } else {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FB),&ins->std.fbMacro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                if (ins->type!=DIV_INS_OPL && ins->type!=DIV_INS_OPL_DRUMS) {
                  macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_ALG),&ins->std.algMacro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                  if (ins->type==DIV_INS_OPZ) {
                    macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FMS),&ins->std.fmsMacro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                    macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AMS),&ins->std.amsMacro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER]));
                    macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FMS2),&ins->std.ex9Macro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                    macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AMS2),&ins->std.ex10Macro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER]));
                  } else {
                    macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FMS),&ins->std.fmsMacro,0,7,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                    macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AMS),&ins->std.amsMacro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER]));
                  }
                } else {
                  macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_ALG),&ins->std.algMacro,0,4,96,uiColors[GUI_COLOR_MACRO_OTHER]));
                }
              }

              if (ins->type==DIV_INS_FM) {
                macroList.push_back(FurnaceGUIMacroDesc(_("LFO Speed"),&ins->std.ex3Macro,0,8,96,uiColors[GUI_COLOR_MACRO_OTHER]));
              }
              if (ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM) {
                macroList.push_back(FurnaceGUIMacroDesc(_("AM Depth"),&ins->std.ex1Macro,0,127,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("PM Depth"),&ins->std.ex2Macro,0,127,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("LFO Speed"),&ins->std.ex3Macro,0,255,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("LFO Shape"),&ins->std.waveMacro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroLFOWaves));
              }
              if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPM || ins->type==DIV_INS_OPZ) {
                macroList.push_back(FurnaceGUIMacroDesc(_("OpMask"),&ins->std.ex4Macro,0,4,128,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,fmOperatorBits));
              }
              if (ins->type==DIV_INS_OPZ) {
                macroList.push_back(FurnaceGUIMacroDesc(_("AM Depth 2"),&ins->std.ex5Macro,0,127,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("PM Depth 2"),&ins->std.ex6Macro,0,127,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("LFO2 Speed"),&ins->std.ex7Macro,0,255,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("LFO2 Shape"),&ins->std.ex8Macro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroLFOWaves));
              }
              drawMacros(macroList,macroEditStateFM,ins);
              ImGui::EndTabItem();
            }
          }
          for (int i=0; i<opCount; i++) {
            if (ins->type==DIV_INS_OPL_DRUMS) {
              if (i>0) break;
              snprintf(label,31,_("Operator Macros"));
            } else {
              snprintf(label,31,_("OP%d Macros"),i+1);
            }
            if (ImGui::BeginTabItem(label)) {
              ImGui::PushID(i);
              int ordi=(opCount==4 && ins->type!=DIV_INS_ESFM)?orderedOps[i]:i;
              int maxTl=127;
              if (ins->type==DIV_INS_OPLL) {
                if (i==1) {
                  maxTl=15;
                } else {
                  maxTl=63;
                }
              }
              if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS || ins->type==DIV_INS_ESFM) {
                maxTl=63;
              }
              int maxArDr=(ins->type==DIV_INS_FM || ins->type==DIV_INS_OPZ || ins->type==DIV_INS_OPM)?31:15;
              if (ins->type==DIV_INS_YAM10) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_TL),&ins->std.opMacros[ordi].tlMacro,0,127,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AR),&ins->std.opMacros[ordi].arMacro,0,31,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DR),&ins->std.opMacros[ordi].drMacro,0,31,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_D2R),&ins->std.opMacros[ordi].d2rMacro,0,31,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SL),&ins->std.opMacros[ordi].slMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RR),&ins->std.opMacros[ordi].rrMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_MULT),&ins->std.opMacros[ordi].multMacro,0,16,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DT),&ins->std.opMacros[ordi].dtMacro,-36,36,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RS),&ins->std.opMacros[ordi].rsMacro,0,3,32,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_WS),&ins->std.opMacros[ordi].wsMacro,0,YAM10_WAVES-1,160,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSR),&ins->std.opMacros[ordi].ksrMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                // the rest of the operator's FM shape, which had no macro
                // before. the fields follow ESFM's choices where they are free.
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FB),&ins->std.opMacros[ordi].dt2Macro,0,7,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Out Level"),&ins->std.opMacros[ordi].egtMacro,0,127,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Mod Input"),&ins->std.opMacros[ordi].kslMacro,0,6,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,yam10OperatorBits));
                macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.opMacros[ordi].amMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Fine Detune"),&ins->std.opMacros[ordi].ssgMacro,-64,63,128,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Delay"),&ins->std.opMacros[ordi].damMacro,0,7,64,uiColors[GUI_COLOR_MACRO_OTHER]));
              } else if (ins->type==DIV_INS_OPL || ins->type==DIV_INS_OPL_DRUMS) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_TL),&ins->std.opMacros[ordi].tlMacro,0,maxTl,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AR),&ins->std.opMacros[ordi].arMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DR),&ins->std.opMacros[ordi].drMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SL),&ins->std.opMacros[ordi].slMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RR),&ins->std.opMacros[ordi].rrMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSL),&ins->std.opMacros[ordi].kslMacro,0,3,32,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_MULT),&ins->std.opMacros[ordi].multMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_WS),&ins->std.opMacros[ordi].wsMacro,0,7,64,uiColors[GUI_COLOR_MACRO_OTHER]));

                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AM),&ins->std.opMacros[ordi].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_VIB),&ins->std.opMacros[ordi].vibMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSR),&ins->std.opMacros[ordi].ksrMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SUS),&ins->std.opMacros[ordi].susMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              } else if (ins->type==DIV_INS_OPLL) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_TL),&ins->std.opMacros[ordi].tlMacro,0,maxTl,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AR),&ins->std.opMacros[ordi].arMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DR),&ins->std.opMacros[ordi].drMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SL),&ins->std.opMacros[ordi].slMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RR),&ins->std.opMacros[ordi].rrMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSL),&ins->std.opMacros[ordi].kslMacro,0,3,32,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_MULT),&ins->std.opMacros[ordi].multMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));

                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AM),&ins->std.opMacros[ordi].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_VIB),&ins->std.opMacros[ordi].vibMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSR),&ins->std.opMacros[ordi].ksrMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_EGS),&ins->std.opMacros[ordi].egtMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              } else if (ins->type==DIV_INS_ESFM) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_TL),&ins->std.opMacros[ordi].tlMacro,0,maxTl,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                if (ins->esfm.op[ordi].fixed) {
                  macroList.push_back(FurnaceGUIMacroDesc(_("Block"),&ins->std.opMacros[ordi].ssgMacro,0,7,64,uiColors[GUI_COLOR_MACRO_PITCH],true));
                  macroList.push_back(FurnaceGUIMacroDesc(_("FreqNum"),&ins->std.opMacros[ordi].dtMacro,0,1023,160,uiColors[GUI_COLOR_MACRO_PITCH]));
                } else {
                  macroList.push_back(FurnaceGUIMacroDesc(_("Op. Arpeggio"),&ins->std.opMacros[ordi].ssgMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.opMacros[ordi].ssgMacro.val,true));
                  macroList.push_back(FurnaceGUIMacroDesc(_("Op. Pitch"),&ins->std.opMacros[ordi].dtMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode,NULL,false,NULL,false,NULL,false,true));
                }
                macroList.push_back(FurnaceGUIMacroDesc(ESFM_NAME(ESFM_DELAY),&ins->std.opMacros[ordi].dt2Macro,0,7,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AR),&ins->std.opMacros[ordi].arMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DR),&ins->std.opMacros[ordi].drMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SL),&ins->std.opMacros[ordi].slMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RR),&ins->std.opMacros[ordi].rrMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSL),&ins->std.opMacros[ordi].kslMacro,0,3,32,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_MULT),&ins->std.opMacros[ordi].multMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_WS),&ins->std.opMacros[ordi].wsMacro,0,7,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(ESFM_NAME(ESFM_OUTLVL),&ins->std.opMacros[ordi].egtMacro,0,7,64,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(ESFM_NAME(ESFM_MODIN),&ins->std.opMacros[ordi].d2rMacro,0,7,64,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AM),&ins->std.opMacros[ordi].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_VIB),&ins->std.opMacros[ordi].vibMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DAM),&ins->std.opMacros[ordi].damMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DVB),&ins->std.opMacros[ordi].dvbMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_KSR),&ins->std.opMacros[ordi].ksrMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SUS),&ins->std.opMacros[ordi].susMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(_("Op. Panning"),&ins->std.opMacros[ordi].rsMacro,0,2,40,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              } else if (ins->type==DIV_INS_FM || ins->type==DIV_INS_OPM) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_TL),&ins->std.opMacros[ordi].tlMacro,0,maxTl,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AR),&ins->std.opMacros[ordi].arMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DR),&ins->std.opMacros[ordi].drMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_D2R),&ins->std.opMacros[ordi].d2rMacro,0,31,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RR),&ins->std.opMacros[ordi].rrMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SL),&ins->std.opMacros[ordi].slMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RS),&ins->std.opMacros[ordi].rsMacro,0,3,32,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_MULT),&ins->std.opMacros[ordi].multMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DT),&ins->std.opMacros[ordi].dtMacro,0,7,64,uiColors[GUI_COLOR_MACRO_PITCH]));
                if (ins->type==DIV_INS_OPM) {
                  macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DT2),&ins->std.opMacros[ordi].dt2Macro,0,3,32,uiColors[GUI_COLOR_MACRO_PITCH]));
                }
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AM),&ins->std.opMacros[ordi].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));

                if (ins->type==DIV_INS_FM) {
                  macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SSG),&ins->std.opMacros[ordi].ssgMacro,0,4,64,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,true,ssgEnvBits));
                }
              } else if (ins->type==DIV_INS_OPZ) {
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_TL),&ins->std.opMacros[ordi].tlMacro,0,maxTl,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
                if (ins->fm.op[ordi].egt) {
                  if (!ins->fm.op[ordi].sus) {
                    macroList.push_back(FurnaceGUIMacroDesc(_("Block"),&ins->std.opMacros[ordi].ssgMacro,0,7,64,uiColors[GUI_COLOR_MACRO_PITCH],true));
                    macroList.push_back(FurnaceGUIMacroDesc(_("FreqNum"),&ins->std.opMacros[ordi].susMacro,0,255,160,uiColors[GUI_COLOR_MACRO_PITCH]));
                  } else {
                    macroList.push_back(FurnaceGUIMacroDesc(_("Op. Arpeggio"),&ins->std.opMacros[ordi].ssgMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.opMacros[ordi].ssgMacro.val,true));
                    macroList.push_back(FurnaceGUIMacroDesc(_("Op. Pitch"),&ins->std.opMacros[ordi].susMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode,NULL,false,NULL,false,NULL,false,true));
                  }
                }
                
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AR),&ins->std.opMacros[ordi].arMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DR),&ins->std.opMacros[ordi].drMacro,0,maxArDr,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_D2R),&ins->std.opMacros[ordi].d2rMacro,0,31,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RR),&ins->std.opMacros[ordi].rrMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_SL),&ins->std.opMacros[ordi].slMacro,0,15,64,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_RS),&ins->std.opMacros[ordi].rsMacro,0,3,32,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_MULT),&ins->std.opMacros[ordi].multMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DT),&ins->std.opMacros[ordi].dtMacro,0,7,64,uiColors[GUI_COLOR_MACRO_PITCH]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_DT2),&ins->std.opMacros[ordi].dt2Macro,0,3,32,uiColors[GUI_COLOR_MACRO_PITCH]));
                macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_AM),&ins->std.opMacros[ordi].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              }
              drawMacros(macroList,macroEditStateOP[ordi],ins);
              ImGui::PopID();
              ImGui::EndTabItem();
            }
          }
        }
        if (ins->type==DIV_INS_GB) if (ImGui::BeginTabItem("Game Boy")) {
          P(ImGui::Checkbox(_("Use software envelope"),&ins->gb.softEnv));
          P(ImGui::Checkbox(_("Initialize envelope on every note"),&ins->gb.alwaysInit));
          P(ImGui::Checkbox(_("Double wave length (GBA only)"),&ins->gb.doubleWave));

          ImGui::BeginDisabled(ins->gb.softEnv);
          if (ImGui::BeginTable("GBParams",2)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.6f);
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.4f);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::BeginTable("GBParamsI",2)) {
              ImGui::TableSetupColumn("ci0",ImGuiTableColumnFlags_WidthFixed);
              ImGui::TableSetupColumn("ci1",ImGuiTableColumnFlags_WidthStretch);

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::Text(_("Volume"));
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##GBVolume",ImGuiDataType_U8,&ins->gb.envVol,&_ZERO,&_FIFTEEN)); rightClickable

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::Text(_("Length"));
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##GBEnvLen",ImGuiDataType_U8,&ins->gb.envLen,&_ZERO,&_SEVEN)); rightClickable

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::Text(_("Sound Length"));
              ImGui::TableNextColumn();
              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
              P(CWSliderScalar("##GBSoundLen",ImGuiDataType_U8,&ins->gb.soundLen,&_ZERO,&_SIXTY_FOUR,ins->gb.soundLen>63?_("Infinity"):"%d")); rightClickable

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              ImGui::Text(_("Direction"));
              ImGui::TableNextColumn();
              bool goesUp=ins->gb.envDir;
              if (ImGui::RadioButton(_("Up"),goesUp)) { PARAMETER
                goesUp=true;
                ins->gb.envDir=goesUp;
              }
              ImGui::SameLine();
              if (ImGui::RadioButton(_("Down"),!goesUp)) { PARAMETER
                goesUp=false;
                ins->gb.envDir=goesUp;
              }

              ImGui::EndTable();
            }

            ImGui::TableNextColumn();
            drawGBEnv(ins->gb.envVol,ins->gb.envLen,ins->gb.soundLen,ins->gb.envDir,ImVec2(ImGui::GetContentRegionAvail().x,100.0f*dpiScale));

            ImGui::EndTable();
          }

          if (ImGui::BeginChild("HWSeq",ImGui::GetContentRegionAvail(),ImGuiChildFlags_Borders,ImGuiWindowFlags_MenuBar)) {
            ImGui::BeginMenuBar();
            ImGui::Text(_("Hardware Sequence"));
            ImGui::EndMenuBar();

            if (ins->gb.hwSeqLen>0) if (ImGui::BeginTable("HWSeqList",3)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed);
              int curFrame=0;
              ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
              ImGui::TableNextColumn();
              ImGui::Text(_("Tick"));
              ImGui::TableNextColumn();
              ImGui::Text(_("Command"));
              ImGui::TableNextColumn();
              ImGui::Text(_("Move/Remove"));
              for (int i=0; i<ins->gb.hwSeqLen; i++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d (#%d)",curFrame,i);
                ImGui::TableNextColumn();
                ImGui::PushID(i);
                if (ins->gb.hwSeq[i].cmd>=DivInstrumentGB::DIV_GB_HWCMD_MAX) {
                  ins->gb.hwSeq[i].cmd=0;
                }
                int cmd=ins->gb.hwSeq[i].cmd;
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::Combo("##HWSeqCmd",&cmd,LocalizedComboGetter,gbHWSeqCmdTypes,DivInstrumentGB::DIV_GB_HWCMD_MAX)) {
                  if (ins->gb.hwSeq[i].cmd!=cmd) {
                    ins->gb.hwSeq[i].cmd=cmd;
                    ins->gb.hwSeq[i].data=0;
                  }
                }
                bool somethingChanged=false;
                switch (ins->gb.hwSeq[i].cmd) {
                  case DivInstrumentGB::DIV_GB_HWCMD_ENVELOPE: {
                    int hwsVol=(ins->gb.hwSeq[i].data&0xf0)>>4;
                    bool hwsDir=ins->gb.hwSeq[i].data&8;
                    int hwsLen=ins->gb.hwSeq[i].data&7;
                    int hwsSoundLen=ins->gb.hwSeq[i].data>>8;

                    if (CWSliderInt(_("Volume"),&hwsVol,0,15)) {
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Env Length"),&hwsLen,0,7)) {
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Sound Length"),&hwsSoundLen,0,64,hwsSoundLen>63?_("Infinity"):"%d")) {
                      somethingChanged=true;
                    }
                    if (ImGui::RadioButton(_("Up"),hwsDir)) { PARAMETER
                      hwsDir=true;
                      somethingChanged=true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton(_("Down"),!hwsDir)) { PARAMETER
                      hwsDir=false;
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->gb.hwSeq[i].data=(hwsLen&7)|(hwsDir?8:0)|(hwsVol<<4)|(hwsSoundLen<<8);
                      PARAMETER;
                    }
                    break;
                  }
                  case DivInstrumentGB::DIV_GB_HWCMD_SWEEP: {
                    int hwsShift=ins->gb.hwSeq[i].data&7;
                    int hwsSpeed=(ins->gb.hwSeq[i].data&0x70)>>4;
                    bool hwsDir=ins->gb.hwSeq[i].data&8;

                    if (CWSliderInt(_("Shift"),&hwsShift,0,7)) {
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Speed"),&hwsSpeed,0,7)) {
                      somethingChanged=true;
                    }

                    if (ImGui::RadioButton(_("Up"),!hwsDir)) { PARAMETER
                      hwsDir=false;
                      somethingChanged=true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton(_("Down"),hwsDir)) { PARAMETER
                      hwsDir=true;
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->gb.hwSeq[i].data=(hwsShift&7)|(hwsDir?8:0)|(hwsSpeed<<4);
                      PARAMETER;
                    }
                    break;
                  }
                  case DivInstrumentGB::DIV_GB_HWCMD_WAIT: {
                    int len=ins->gb.hwSeq[i].data+1;
                    curFrame+=ins->gb.hwSeq[i].data+1;

                    if (ImGui::InputInt(_("Ticks"),&len,1,4)) {
                      if (len<1) len=1;
                      if (len>255) len=256;
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->gb.hwSeq[i].data=len-1;
                      PARAMETER;
                    }
                    break;
                  }
                  case DivInstrumentGB::DIV_GB_HWCMD_WAIT_REL:
                    curFrame++;
                    break;
                  case DivInstrumentGB::DIV_GB_HWCMD_LOOP:
                  case DivInstrumentGB::DIV_GB_HWCMD_LOOP_REL: {
                    int pos=ins->gb.hwSeq[i].data;

                    if (ImGui::InputInt(_("Position"),&pos,1,1)) {
                      if (pos<0) pos=0;
                      if (pos>(ins->gb.hwSeqLen-1)) pos=(ins->gb.hwSeqLen-1);
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->gb.hwSeq[i].data=pos;
                      PARAMETER;
                    }
                    break;
                  }
                  default:
                    break;
                }
                ImGui::PopID();
                ImGui::TableNextColumn();
                ImGui::PushID(i+512);
                if (ImGui::Button(ICON_FA_CHEVRON_UP "##HWCmdUp")) {
                  if (i>0) {
                    e->lockEngine([ins,i]() {
                      ins->gb.hwSeq[i-1].cmd^=ins->gb.hwSeq[i].cmd;
                      ins->gb.hwSeq[i].cmd^=ins->gb.hwSeq[i-1].cmd;
                      ins->gb.hwSeq[i-1].cmd^=ins->gb.hwSeq[i].cmd;

                      ins->gb.hwSeq[i-1].data^=ins->gb.hwSeq[i].data;
                      ins->gb.hwSeq[i].data^=ins->gb.hwSeq[i-1].data;
                      ins->gb.hwSeq[i-1].data^=ins->gb.hwSeq[i].data;
                    });
                  }
                  MARK_MODIFIED;
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_CHEVRON_DOWN "##HWCmdDown")) {
                  if (i<ins->gb.hwSeqLen-1) {
                    e->lockEngine([ins,i]() {
                      ins->gb.hwSeq[i+1].cmd^=ins->gb.hwSeq[i].cmd;
                      ins->gb.hwSeq[i].cmd^=ins->gb.hwSeq[i+1].cmd;
                      ins->gb.hwSeq[i+1].cmd^=ins->gb.hwSeq[i].cmd;

                      ins->gb.hwSeq[i+1].data^=ins->gb.hwSeq[i].data;
                      ins->gb.hwSeq[i].data^=ins->gb.hwSeq[i+1].data;
                      ins->gb.hwSeq[i+1].data^=ins->gb.hwSeq[i].data;
                    });
                  }
                  MARK_MODIFIED;
                }
                ImGui::SameLine();
                pushDestColor();
                if (ImGui::Button(ICON_FA_TIMES "##HWCmdDel")) {
                  for (int j=i; j<ins->gb.hwSeqLen-1; j++) {
                    ins->gb.hwSeq[j].cmd=ins->gb.hwSeq[j+1].cmd;
                    ins->gb.hwSeq[j].data=ins->gb.hwSeq[j+1].data;
                  }
                  ins->gb.hwSeqLen--;
                }
                popDestColor();
                ImGui::PopID();
              }
              ImGui::EndTable();
            }

            if (ImGui::Button(ICON_FA_PLUS "##HWCmdAdd")) {
              if (ins->gb.hwSeqLen<255) {
                ins->gb.hwSeq[ins->gb.hwSeqLen].cmd=0;
                ins->gb.hwSeq[ins->gb.hwSeqLen].data=0;
                ins->gb.hwSeqLen++;
              }
            }
          }
          ImGui::EndChild();
          ImGui::EndDisabled();
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_C64 || ins->type==DIV_INS_SID2) if (ImGui::BeginTabItem((ins->type==DIV_INS_SID2)?"SID2":"C64")) {
          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Waveform"));
          ImGui::SameLine();
          pushToggleColors(ins->c64.triOn);
          if (ImGui::Button(_("tri"))) { PARAMETER
            ins->c64.triOn=!ins->c64.triOn;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(ins->c64.sawOn);
          if (ImGui::Button(_("saw"))) { PARAMETER
            ins->c64.sawOn=!ins->c64.sawOn;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(ins->c64.pulseOn);
          if (ImGui::Button(_("pulse"))) { PARAMETER
            ins->c64.pulseOn=!ins->c64.pulseOn;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(ins->c64.noiseOn);
          if (ImGui::Button(_("noise"))) { PARAMETER
            ins->c64.noiseOn=!ins->c64.noiseOn;
          }
          popToggleColors();

          ImVec2 sliderSize=ImVec2(20.0f*dpiScale,128.0*dpiScale);

          if (ImGui::BeginTable("C64EnvParams",5,ImGuiTableFlags_NoHostExtendX)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
            ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
            ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
            ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            CENTER_TEXT("A");
            ImGui::TextUnformatted("A");
            ImGui::TableNextColumn();
            CENTER_TEXT("D");
            ImGui::TextUnformatted("D");
            ImGui::TableNextColumn();
            CENTER_TEXT("S");
            ImGui::TextUnformatted("S");
            ImGui::TableNextColumn();
            CENTER_TEXT("R");
            ImGui::TextUnformatted("R");
            ImGui::TableNextColumn();
            CENTER_TEXT(_("Envelope"));
            ImGui::TextUnformatted(_("Envelope"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            P(CWVSliderScalar("##Attack",sliderSize,ImGuiDataType_U8,&ins->c64.a,&_ZERO,&_FIFTEEN)); rightClickable
            ImGui::TableNextColumn();
            P(CWVSliderScalar("##Decay",sliderSize,ImGuiDataType_U8,&ins->c64.d,&_ZERO,&_FIFTEEN)); rightClickable
            ImGui::TableNextColumn();
            P(CWVSliderScalar("##Sustain",sliderSize,ImGuiDataType_U8,&ins->c64.s,&_ZERO,&_FIFTEEN)); rightClickable
            ImGui::TableNextColumn();
            P(CWVSliderScalar("##Release",sliderSize,ImGuiDataType_U8,&ins->c64.r,&_ZERO,&_FIFTEEN)); rightClickable
            ImGui::TableNextColumn();
            drawFMEnv((ins->type==DIV_INS_SID2)?(15-ins->sid2.volume):0,16-ins->c64.a,16-ins->c64.d,15-ins->c64.r,15-ins->c64.r,15-ins->c64.s,0,0,0,15,16,15,ImVec2(ImGui::GetContentRegionAvail().x,sliderSize.y),ins->type);

            ImGui::EndTable();
          }

          P(CWSliderScalar(_("Duty"),ImGuiDataType_U16,&ins->c64.duty,&_ZERO,&_FOUR_THOUSAND_NINETY_FIVE)); rightClickable

          bool resetDuty=ins->c64.resetDuty;
          if (ImGui::Checkbox(_("Reset duty on new note"),&resetDuty)) { PARAMETER
            ins->c64.resetDuty=resetDuty;
          }

          bool ringMod=ins->c64.ringMod;
          if (ImGui::Checkbox(_("Ring Modulation"),&ringMod)) { PARAMETER
            ins->c64.ringMod=ringMod;
          }
          bool oscSync=ins->c64.oscSync;
          if (ImGui::Checkbox(_("Oscillator Sync"),&oscSync)) { PARAMETER
            ins->c64.oscSync=oscSync;
          }

          P(ImGui::Checkbox(_("Enable filter"),&ins->c64.toFilter));
          P(ImGui::Checkbox(_("Initialize filter"),&ins->c64.initFilter));
          
          if (ins->type==DIV_INS_SID2) {
            P(CWSliderScalar(_("Cutoff"),ImGuiDataType_U16,&ins->c64.cut,&_ZERO,&_FOUR_THOUSAND_NINETY_FIVE)); rightClickable
            P(CWSliderScalar(_("Resonance"),ImGuiDataType_U8,&ins->c64.res,&_ZERO,&_TWO_HUNDRED_FIFTY_FIVE)); rightClickable
          } else {
            P(CWSliderScalar(_("Cutoff"),ImGuiDataType_U16,&ins->c64.cut,&_ZERO,&_TWO_THOUSAND_FORTY_SEVEN)); rightClickable
            P(CWSliderScalar(_("Resonance"),ImGuiDataType_U8,&ins->c64.res,&_ZERO,&_FIFTEEN)); rightClickable
          }

          ImGui::AlignTextToFramePadding();
          ImGui::Text(_("Filter Mode"));
          ImGui::SameLine();
          pushToggleColors(ins->c64.lp);
          if (ImGui::Button(_("low"))) { PARAMETER
            ins->c64.lp=!ins->c64.lp;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(ins->c64.bp);
          if (ImGui::Button(_("band"))) { PARAMETER
            ins->c64.bp=!ins->c64.bp;
          }
          popToggleColors();
          ImGui::SameLine();
          pushToggleColors(ins->c64.hp);
          if (ImGui::Button(_("high"))) { PARAMETER
            ins->c64.hp=!ins->c64.hp;
          }
          popToggleColors();
          if (ins->type!=DIV_INS_SID2) {
            ImGui::SameLine();
            pushToggleColors(ins->c64.ch3off);
            if (ImGui::Button(_("ch3off"))) { PARAMETER
              ins->c64.ch3off=!ins->c64.ch3off;
            }
            popToggleColors();
          }

          if (ins->type==DIV_INS_SID2) {
            P(CWSliderScalar(_("Noise Mode"),ImGuiDataType_U8,&ins->sid2.noiseMode,&_ZERO,&_THREE));
            P(CWSliderScalar(_("Wave Mix Mode"),ImGuiDataType_U8,&ins->sid2.mixMode,&_ZERO,&_THREE,sid2WaveMixModes[ins->sid2.mixMode&3]));
          }

          if (ImGui::Checkbox(_("Absolute Cutoff Macro"),&ins->c64.filterIsAbs)) {
            ins->temp.vZoom[DIV_MACRO_ALG]=-1;
            PARAMETER;
          }
          if (ImGui::Checkbox(_("Absolute Duty Macro"),&ins->c64.dutyIsAbs)) {
            ins->temp.vZoom[DIV_MACRO_DUTY]=-1;
            PARAMETER;
          }

          if (ins->type!=DIV_INS_SID2) {
            P(ImGui::Checkbox(_("Don't test before new note"),&ins->c64.noTest));
          }
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_SU) if (ImGui::BeginTabItem("Sound Unit")) {
          P(ImGui::Checkbox(_("Switch roles of frequency and phase reset timer"),&ins->su.switchRoles));
          if (ImGui::BeginChild("HWSeqSU",ImGui::GetContentRegionAvail(),ImGuiChildFlags_Borders,ImGuiWindowFlags_MenuBar)) {
            ImGui::BeginMenuBar();
            ImGui::Text(_("Hardware Sequence"));
            ImGui::EndMenuBar();

            if (ins->su.hwSeqLen>0) if (ImGui::BeginTable("HWSeqListSU",3)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed);
              int curFrame=0;
              ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
              ImGui::TableNextColumn();
              ImGui::Text(_("Tick"));
              ImGui::TableNextColumn();
              ImGui::Text(_("Command"));
              ImGui::TableNextColumn();
              ImGui::Text(_("Move/Remove"));
              for (int i=0; i<ins->su.hwSeqLen; i++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d (#%d)",curFrame,i);
                ImGui::TableNextColumn();
                ImGui::PushID(i);
                if (ins->su.hwSeq[i].cmd>=DivInstrumentSoundUnit::DIV_SU_HWCMD_MAX) {
                  ins->su.hwSeq[i].cmd=0;
                }
                int cmd=ins->su.hwSeq[i].cmd;
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::Combo("##HWSeqCmd",&cmd,LocalizedComboGetter,suHWSeqCmdTypes,DivInstrumentSoundUnit::DIV_SU_HWCMD_MAX)) {
                  if (ins->su.hwSeq[i].cmd!=cmd) {
                    ins->su.hwSeq[i].cmd=cmd;
                    ins->su.hwSeq[i].val=0;
                    ins->su.hwSeq[i].bound=0;
                    ins->su.hwSeq[i].speed=0;
                  }
                }
                bool somethingChanged=false;
                switch (ins->su.hwSeq[i].cmd) {
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_VOL: {
                    int swPeriod=ins->su.hwSeq[i].speed;
                    int swBound=ins->su.hwSeq[i].bound;
                    int swVal=ins->su.hwSeq[i].val&31;
                    bool swDir=ins->su.hwSeq[i].val&32;
                    bool swLoop=ins->su.hwSeq[i].val&64;
                    bool swInvert=ins->su.hwSeq[i].val&128;

                    if (ImGui::InputInt(_("Period"),&swPeriod,1,16)) {
                      if (swPeriod<0) swPeriod=0;
                      if (swPeriod>65535) swPeriod=65535;
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Amount"),&swVal,0,31)) {
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Bound"),&swBound,0,255)) {
                      somethingChanged=true;
                    }
                    if (ImGui::RadioButton(_("Up"),swDir)) { PARAMETER
                      swDir=true;
                      somethingChanged=true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton(_("Down"),!swDir)) { PARAMETER
                      swDir=false;
                      somethingChanged=true;
                    }
                    if (ImGui::Checkbox(_("Loop"),&swLoop)) { PARAMETER
                      somethingChanged=true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox(_("Flip"),&swInvert)) { PARAMETER
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->su.hwSeq[i].speed=swPeriod;
                      ins->su.hwSeq[i].bound=swBound;
                      ins->su.hwSeq[i].val=(swVal&31)|(swDir?32:0)|(swLoop?64:0)|(swInvert?128:0);
                      PARAMETER;
                    }
                    break;
                  }
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_PITCH:
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_CUT: {
                    int swPeriod=ins->su.hwSeq[i].speed;
                    int swBound=ins->su.hwSeq[i].bound;
                    int swVal=ins->su.hwSeq[i].val&127;
                    bool swDir=ins->su.hwSeq[i].val&128;

                    if (ImGui::InputInt(_("Period"),&swPeriod,1,16)) {
                      if (swPeriod<0) swPeriod=0;
                      if (swPeriod>65535) swPeriod=65535;
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Amount"),&swVal,0,31)) {
                      somethingChanged=true;
                    }
                    if (CWSliderInt(_("Bound"),&swBound,0,255)) {
                      somethingChanged=true;
                    }
                    if (ImGui::RadioButton(_("Up"),swDir)) { PARAMETER
                      swDir=true;
                      somethingChanged=true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton(_("Down"),!swDir)) { PARAMETER
                      swDir=false;
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->su.hwSeq[i].speed=swPeriod;
                      ins->su.hwSeq[i].bound=swBound;
                      ins->su.hwSeq[i].val=(swVal&127)|(swDir?128:0);
                      PARAMETER;
                    }
                    break;
                  }
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_WAIT: {
                    int len=ins->su.hwSeq[i].val+1;
                    curFrame+=ins->su.hwSeq[i].val+1;

                    if (ImGui::InputInt(_("Ticks"),&len)) {
                      if (len<1) len=1;
                      if (len>255) len=256;
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->su.hwSeq[i].val=len-1;
                      PARAMETER;
                    }
                    break;
                  }
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_WAIT_REL:
                    curFrame++;
                    break;
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_LOOP:
                  case DivInstrumentSoundUnit::DIV_SU_HWCMD_LOOP_REL: {
                    int pos=ins->su.hwSeq[i].val;

                    if (ImGui::InputInt(_("Position"),&pos,1,4)) {
                      if (pos<0) pos=0;
                      if (pos>(ins->su.hwSeqLen-1)) pos=(ins->su.hwSeqLen-1);
                      somethingChanged=true;
                    }

                    if (somethingChanged) {
                      ins->su.hwSeq[i].val=pos;
                      PARAMETER;
                    }
                    break;
                  }
                  default:
                    break;
                }
                ImGui::PopID();
                ImGui::TableNextColumn();
                ImGui::PushID(i+512);
                if (ImGui::Button(ICON_FA_CHEVRON_UP "##HWCmdUp")) {
                  if (i>0) {
                    e->lockEngine([ins,i]() {
                      ins->su.hwSeq[i-1].cmd^=ins->su.hwSeq[i].cmd;
                      ins->su.hwSeq[i].cmd^=ins->su.hwSeq[i-1].cmd;
                      ins->su.hwSeq[i-1].cmd^=ins->su.hwSeq[i].cmd;

                      ins->su.hwSeq[i-1].speed^=ins->su.hwSeq[i].speed;
                      ins->su.hwSeq[i].speed^=ins->su.hwSeq[i-1].speed;
                      ins->su.hwSeq[i-1].speed^=ins->su.hwSeq[i].speed;

                      ins->su.hwSeq[i-1].val^=ins->su.hwSeq[i].val;
                      ins->su.hwSeq[i].val^=ins->su.hwSeq[i-1].val;
                      ins->su.hwSeq[i-1].val^=ins->su.hwSeq[i].val;

                      ins->su.hwSeq[i-1].bound^=ins->su.hwSeq[i].bound;
                      ins->su.hwSeq[i].bound^=ins->su.hwSeq[i-1].bound;
                      ins->su.hwSeq[i-1].bound^=ins->su.hwSeq[i].bound;
                    });
                  }
                  MARK_MODIFIED;
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_CHEVRON_DOWN "##HWCmdDown")) {
                  if (i<ins->su.hwSeqLen-1) {
                    e->lockEngine([ins,i]() {
                      ins->su.hwSeq[i+1].cmd^=ins->su.hwSeq[i].cmd;
                      ins->su.hwSeq[i].cmd^=ins->su.hwSeq[i+1].cmd;
                      ins->su.hwSeq[i+1].cmd^=ins->su.hwSeq[i].cmd;

                      ins->su.hwSeq[i+1].speed^=ins->su.hwSeq[i].speed;
                      ins->su.hwSeq[i].speed^=ins->su.hwSeq[i+1].speed;
                      ins->su.hwSeq[i+1].speed^=ins->su.hwSeq[i].speed;

                      ins->su.hwSeq[i+1].val^=ins->su.hwSeq[i].val;
                      ins->su.hwSeq[i].val^=ins->su.hwSeq[i+1].val;
                      ins->su.hwSeq[i+1].val^=ins->su.hwSeq[i].val;

                      ins->su.hwSeq[i+1].bound^=ins->su.hwSeq[i].bound;
                      ins->su.hwSeq[i].bound^=ins->su.hwSeq[i+1].bound;
                      ins->su.hwSeq[i+1].bound^=ins->su.hwSeq[i].bound;
                    });
                  }
                  MARK_MODIFIED;
                }
                ImGui::SameLine();
                pushDestColor();
                if (ImGui::Button(ICON_FA_TIMES "##HWCmdDel")) {
                  for (int j=i; j<ins->su.hwSeqLen-1; j++) {
                    ins->su.hwSeq[j].cmd=ins->su.hwSeq[j+1].cmd;
                    ins->su.hwSeq[j].speed=ins->su.hwSeq[j+1].speed;
                    ins->su.hwSeq[j].val=ins->su.hwSeq[j+1].val;
                    ins->su.hwSeq[j].bound=ins->su.hwSeq[j+1].bound;
                  }
                  ins->su.hwSeqLen--;
                }
                popDestColor();
                ImGui::PopID();
              }
              ImGui::EndTable();
            }

            if (ImGui::Button(ICON_FA_PLUS "##HWCmdAdd")) {
              if (ins->su.hwSeqLen<255) {
                ins->su.hwSeq[ins->su.hwSeqLen].cmd=0;
                ins->su.hwSeq[ins->su.hwSeqLen].speed=0;
                ins->su.hwSeq[ins->su.hwSeqLen].val=0;
                ins->su.hwSeq[ins->su.hwSeqLen].bound=0;
                ins->su.hwSeqLen++;
              }
            }
          }
          ImGui::EndChild();
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_SID3) {
          drawInsSID3(ins);
        }
        if (ins->type==DIV_INS_MSM6258 ||
            ins->type==DIV_INS_MSM6295 ||
            ins->type==DIV_INS_ADPCMA ||
            ins->type==DIV_INS_ADPCMB ||
            ins->type==DIV_INS_SEGAPCM ||
            ins->type==DIV_INS_QSOUND ||
            ins->type==DIV_INS_YMZ280B ||
            ins->type==DIV_INS_RF5C68 ||
            ins->type==DIV_INS_AMIGA ||
            ins->type==DIV_INS_MULTIPCM ||
            ins->type==DIV_INS_SU ||
            ins->type==DIV_INS_SNES ||
            ins->type==DIV_INS_ES5506 ||
            ins->type==DIV_INS_K007232 ||
            ins->type==DIV_INS_GA20 ||
            ins->type==DIV_INS_K053260 ||
            ins->type==DIV_INS_C140 ||
            ins->type==DIV_INS_C219 ||
            ins->type==DIV_INS_NDS ||
            ins->type==DIV_INS_GBA_DMA ||
            ins->type==DIV_INS_GBA_MINMOD ||
            ins->type==DIV_INS_SUPERVISION) {
          insTabSample(ins);
        }
        if (ins->type==DIV_INS_N163) if (ImGui::BeginTabItem("Namco 163")) {
          bool preLoad=ins->n163.waveMode&0x1;
          if (ImGui::Checkbox(_("Load waveform"),&preLoad)) { PARAMETER
            ins->n163.waveMode=(ins->n163.waveMode&~0x1)|(preLoad?0x1:0);
          }

          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("when enabled, a waveform will be loaded into RAM.\nwhen disabled, only the offset and length change."));
          }

          if (preLoad) {
            if (ImGui::InputInt(_("Waveform##WAVE"),&ins->n163.wave,1,10)) { PARAMETER
              if (ins->n163.wave<0) ins->n163.wave=0;
              if (ins->n163.wave>=e->song.waveLen) ins->n163.wave=e->song.waveLen-1;
            }
          }

          ImGui::Separator();

          P(ImGui::Checkbox(_("Per-channel wave position/length"),&ins->n163.perChanPos));

          if (ins->n163.perChanPos) {
            if (ImGui::BeginTable("N1PerChPos",3)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.5f);
              ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,0.5f);

              ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
              ImGui::TableNextColumn();
              ImGui::Text(_("Ch"));
              ImGui::TableNextColumn();
              ImGui::Text(_("Position"));
              ImGui::TableNextColumn();
              ImGui::Text(_("Length"));

              for (int i=0; i<8; i++) {
                ImGui::PushID(64+i);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Dummy(ImVec2(dpiScale,ImGui::GetFrameHeightWithSpacing()));
                ImGui::SameLine();
                ImGui::Text("%d",i+1);

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputInt("##pcOff",&ins->n163.wavePosCh[i],1,16)) { PARAMETER
                  if (ins->n163.wavePosCh[i]<0) ins->n163.wavePosCh[i]=0;
                  if (ins->n163.wavePosCh[i]>255) ins->n163.wavePosCh[i]=255;
                }

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputInt("##pcLen",&ins->n163.waveLenCh[i],4,16)) { PARAMETER
                  if (ins->n163.waveLenCh[i]<0) ins->n163.waveLenCh[i]=0;
                  if (ins->n163.waveLenCh[i]>252) ins->n163.waveLenCh[i]=252;
                  ins->n163.waveLenCh[i]&=0xfc;
                }
                ImGui::PopID();
              }

              ImGui::EndTable();
            }
          } else {
            if (ImGui::InputInt("Position##WAVEPOS",&ins->n163.wavePos,1,16)) { PARAMETER
              if (ins->n163.wavePos<0) ins->n163.wavePos=0;
              if (ins->n163.wavePos>255) ins->n163.wavePos=255;
            }
            if (ImGui::InputInt("Length##WAVELEN",&ins->n163.waveLen,4,16)) { PARAMETER
              if (ins->n163.waveLen<0) ins->n163.waveLen=0;
              if (ins->n163.waveLen>252) ins->n163.waveLen=252;
              ins->n163.waveLen&=0xfc;
            }
          }
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_FDS) if (ImGui::BeginTabItem("FDS")) {
          float modTable[32];
          int modTableInt[256];
          ImGui::Checkbox(_("Compatibility mode"),&ins->fds.initModTableWithFirstWave);
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_("only use for compatibility with .dmf modules!\n- initializes modulation table with first wavetable\n- does not alter modulation parameters on instrument change"));
          }
          if (ImGui::InputInt(_("Modulation depth"),&ins->fds.modDepth,1,4)) {
            if (ins->fds.modDepth<0) ins->fds.modDepth=0;
            if (ins->fds.modDepth>63) ins->fds.modDepth=63;
          }
          if (ImGui::InputInt(_("Modulation speed"),&ins->fds.modSpeed,1,4)) {
            if (ins->fds.modSpeed<0) ins->fds.modSpeed=0;
            if (ins->fds.modSpeed>4095) ins->fds.modSpeed=4095;
          }
          ImGui::Text(_("Modulation table"));
          for (int i=0; i<32; i++) {
            modTable[i]=ins->fds.modTable[i];
            modTableInt[i]=ins->fds.modTable[i];
          }
          ImVec2 modTableSize=ImVec2(ImGui::GetContentRegionAvail().x,96.0f*dpiScale);
          PlotCustom("##ModTable",modTable,32,0,NULL,-4,3,modTableSize,sizeof(float),ImVec4(1.0f,1.0f,1.0f,1.0f),0,NULL,NULL,true);
          if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            macroDragStart=ImGui::GetItemRectMin();
            macroDragAreaSize=modTableSize;
            macroDragMin=-4;
            macroDragMax=3;
            macroDragBitMode=false;
            macroDragInitialValueSet=false;
            macroDragInitialValue=false;
            macroDragLen=32;
            macroDragActive=true;
            macroDragCTarget=(unsigned char*)ins->fds.modTable;
            macroDragChar=true;
            macroDragLineMode=false;
            macroDragLineInitial=ImVec2(0,0);
            processDrags(ImGui::GetMousePos().x,ImGui::GetMousePos().y);
            ImGui::InhibitInertialScroll();
          }

          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // wavetable text input size found here
          if (ImGui::InputText("##MMLModTable",&mmlStringModTable)) {
            int discardIt=0;
            memset(modTableInt,0,256*sizeof(int));
            decodeMMLStrW(mmlStringModTable,modTableInt,discardIt,-4,3,false);
            for (int i=0; i<32; i++) {
              if (i>=discardIt) {
                modTableInt[i]=0;
              } else {
                if (modTableInt[i]<-4) modTableInt[i]=-4;
                if (modTableInt[i]>3) modTableInt[i]=3;
              }
              ins->fds.modTable[i]=modTableInt[i];
            }
            MARK_MODIFIED;
          }
          if (!ImGui::IsItemActive()) {
            encodeMMLStr(mmlStringModTable,modTableInt,32,-1,-1,false);
          }
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_VBOY) if (ImGui::BeginTabItem("Virtual Boy")) {
          float modTable[32];
          int modTableInt[256];
          P(ImGui::Checkbox(_("Set modulation table (channel 5 only)"),&ins->fds.initModTableWithFirstWave));

          ImGui::BeginDisabled(!ins->fds.initModTableWithFirstWave);
          for (int i=0; i<32; i++) {
            modTable[i]=ins->fds.modTable[i];
            modTableInt[i]=modTableHex?((unsigned char)ins->fds.modTable[i]):ins->fds.modTable[i];
          }
          ImVec2 modTableSize=ImVec2(ImGui::GetContentRegionAvail().x,256.0f*dpiScale);
          PlotCustom("##ModTable",modTable,32,0,NULL,-128,127,modTableSize,sizeof(float),ImVec4(1.0f,1.0f,1.0f,1.0f),0,NULL,NULL,true);
          if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            macroDragStart=ImGui::GetItemRectMin();
            macroDragAreaSize=modTableSize;
            macroDragMin=-128;
            macroDragMax=127;
            macroDragBitMode=false;
            macroDragInitialValueSet=false;
            macroDragInitialValue=false;
            macroDragLen=32;
            macroDragActive=true;
            macroDragCTarget=(unsigned char*)ins->fds.modTable;
            macroDragChar=true;
            macroDragLineMode=false;
            macroDragLineInitial=ImVec2(0,0);
            processDrags(ImGui::GetMousePos().x,ImGui::GetMousePos().y);
            ImGui::InhibitInertialScroll();
          }
          
          if (ImGui::Button(modTableHex?"Hex##MTHex":"Dec##MTHex")) {
            modTableHex=!modTableHex;
          }
          ImGui::SameLine();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // wavetable text input size found here
          if (ImGui::InputText("##MMLModTable",&mmlStringModTable)) {
            int discardIt=0;
            memset(modTableInt,0,256*sizeof(int));
            decodeMMLStrW(mmlStringModTable,modTableInt,discardIt,modTableHex?0:-128,modTableHex?255:127,modTableHex);
            for (int i=0; i<32; i++) {
              if (i>=discardIt) {
                modTableInt[i]=0;
              } else {
                if (modTableInt[i]>=128) modTableInt[i]-=256;
              }
              ins->fds.modTable[i]=modTableInt[i];
            }
            MARK_MODIFIED;
          }
          if (!ImGui::IsItemActive()) {
            encodeMMLStr(mmlStringModTable,modTableInt,32,-1,-1,modTableHex);
          }
          ImGui::SameLine();

          ImGui::EndDisabled();
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_ES5506) if (ImGui::BeginTabItem("ES5506")) {
          if (ImGui::BeginTable("ESParams",2,ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0);
            ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.0);
            // filter
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Filter Mode"),ImGuiDataType_U8,&ins->es5506.filter.mode,&_ZERO,&_THREE,es5506FilterModes[ins->es5506.filter.mode&3]));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Filter K1"),ImGuiDataType_U16,&ins->es5506.filter.k1,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Filter K2"),ImGuiDataType_U16,&ins->es5506.filter.k2,&_ZERO,&_SIXTY_FIVE_THOUSAND_FIVE_HUNDRED_THIRTY_FIVE)); rightClickable
            // envelope
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Envelope length"),ImGuiDataType_U16,&ins->es5506.envelope.ecount,&_ZERO,&_FIVE_HUNDRED_ELEVEN)); rightClickable
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Left Volume Ramp"),ImGuiDataType_S8,&ins->es5506.envelope.lVRamp,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Right Volume Ramp"),ImGuiDataType_S8,&ins->es5506.envelope.rVRamp,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Filter K1 Ramp"),ImGuiDataType_S8,&ins->es5506.envelope.k1Ramp,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
            ImGui::TableNextColumn();
            P(CWSliderScalar(_("Filter K2 Ramp"),ImGuiDataType_S8,&ins->es5506.envelope.k2Ramp,&_MINUS_ONE_HUNDRED_TWENTY_EIGHT,&_ONE_HUNDRED_TWENTY_SEVEN)); rightClickable
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Checkbox(_("K1 Ramp Slowdown"),&ins->es5506.envelope.k1Slow);
            ImGui::TableNextColumn();
            ImGui::Checkbox(_("K2 Ramp Slowdown"),&ins->es5506.envelope.k2Slow);
            ImGui::EndTable();
          }
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_MULTIPCM) {
          if (ImGui::BeginTabItem("MultiPCM")) {
            ImVec2 sliderSize=ImVec2(20.0f*dpiScale,128.0*dpiScale);
            if (ImGui::BeginTable("MultiPCMADSRParams",7,ImGuiTableFlags_NoHostExtendX)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c5",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c6",ImGuiTableColumnFlags_WidthStretch);

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              CENTER_TEXT("AR");
              ImGui::TextUnformatted("AR");
              if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Attack Rate"));
              }
              ImGui::TableNextColumn();
              CENTER_TEXT("D1R");
              ImGui::TextUnformatted("D1R");
              if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Decay 1 Rate"));
              }
              ImGui::TableNextColumn();
              CENTER_TEXT("DL");
              ImGui::TextUnformatted("DL");
              if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Decay Level"));
              }
              ImGui::TableNextColumn();
              CENTER_TEXT("D2R");
              ImGui::TextUnformatted("D2R");
              if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Decay 2 Rate"));
              }
              ImGui::TableNextColumn();
              CENTER_TEXT("RR");
              ImGui::TextUnformatted("RR");
              if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Release Rate"));
              }
              ImGui::TableNextColumn();
              CENTER_TEXT("RC");
              ImGui::TextUnformatted("RC");
              if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(_("Rate Correction"));
              }
              ImGui::TableNextColumn();
              CENTER_TEXT(_("Envelope"));
              ImGui::TextUnformatted(_("Envelope"));

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Attack Rate",sliderSize,ImGuiDataType_U8,&ins->multipcm.ar,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Decay 1 Rate",sliderSize,ImGuiDataType_U8,&ins->multipcm.d1r,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Decay Level",sliderSize,ImGuiDataType_U8,&ins->multipcm.dl,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Decay 2 Rate",sliderSize,ImGuiDataType_U8,&ins->multipcm.d2r,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Release Rate",sliderSize,ImGuiDataType_U8,&ins->multipcm.rr,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Rate Correction",sliderSize,ImGuiDataType_U8,&ins->multipcm.rc,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              drawFMEnv(0,ins->multipcm.ar,ins->multipcm.d1r,ins->multipcm.d2r,ins->multipcm.rr,ins->multipcm.dl,0,0,0,127,15,15,ImVec2(ImGui::GetContentRegionAvail().x,sliderSize.y),ins->type);
              ImGui::EndTable();
            }
            if (ImGui::BeginTable("MultiPCMLFOParams",3,ImGuiTableFlags_SizingStretchSame)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch,0.0);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch,0.0);
              ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthStretch,0.0);
              ImGui::TableNextColumn();
              P(CWSliderScalar(_("LFO Rate"),ImGuiDataType_U8,&ins->multipcm.lfo,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWSliderScalar(_("PM Depth"),ImGuiDataType_U8,&ins->multipcm.vib,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWSliderScalar(_("AM Depth"),ImGuiDataType_U8,&ins->multipcm.am,&_ZERO,&_SEVEN)); rightClickable
              ImGui::EndTable();
            }
            P(ImGui::Checkbox(_("Damp"),&ins->multipcm.damp));
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip(_("Only for OPL4 PCM."));
            }
            P(ImGui::Checkbox(_("Pseudo Reverb"),&ins->multipcm.pseudoReverb));
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip(_("Only for OPL4 PCM."));
            }
            P(ImGui::Checkbox(_("LFO Reset"),&ins->multipcm.lfoReset));
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip(_("Only for OPL4 PCM."));
            }
            P(ImGui::Checkbox(_("Disable volume change ramp"),&ins->multipcm.levelDirect));
            ImGui::EndTabItem();
          }
        }
        if (ins->type==DIV_INS_SNES) if (ImGui::BeginTabItem("SNES")) {
          P(ImGui::Checkbox(_("Use envelope"),&ins->snes.useEnv));
          ImVec2 sliderSize=ImVec2(20.0f*dpiScale,128.0*dpiScale);
          if (ins->snes.useEnv) {
            if (ImGui::BeginTable("SNESEnvParams",ins->snes.sus?6:5,ImGuiTableFlags_NoHostExtendX)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c2",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              if (ins->snes.sus) {
                ImGui::TableSetupColumn("c2x",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              }
              ImGui::TableSetupColumn("c3",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);
              ImGui::TableSetupColumn("c4",ImGuiTableColumnFlags_WidthStretch);

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              CENTER_TEXT("A");
              ImGui::TextUnformatted("A");
              ImGui::TableNextColumn();
              CENTER_TEXT("D");
              ImGui::TextUnformatted("D");
              ImGui::TableNextColumn();
              CENTER_TEXT("S");
              ImGui::TextUnformatted("S");
              if (ins->snes.sus) {
                ImGui::TableNextColumn();
                CENTER_TEXT("D2");
                ImGui::TextUnformatted("D2");
              }
              ImGui::TableNextColumn();
              CENTER_TEXT("R");
              ImGui::TextUnformatted("R");
              ImGui::TableNextColumn();
              CENTER_TEXT(_("Envelope"));
              ImGui::TextUnformatted(_("Envelope"));

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Attack",sliderSize,ImGuiDataType_U8,&ins->snes.a,&_ZERO,&_FIFTEEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Decay",sliderSize,ImGuiDataType_U8,&ins->snes.d,&_ZERO,&_SEVEN)); rightClickable
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Sustain",sliderSize,ImGuiDataType_U8,&ins->snes.s,&_ZERO,&_SEVEN)); rightClickable
              if (ins->snes.sus) {
                ImGui::TableNextColumn();
                P(CWVSliderScalar("##Decay2",sliderSize,ImGuiDataType_U8,&ins->snes.d2,&_ZERO,&_THIRTY_ONE)); rightClickable
              }
              ImGui::TableNextColumn();
              P(CWVSliderScalar("##Release",sliderSize,ImGuiDataType_U8,&ins->snes.r,&_ZERO,&_THIRTY_ONE)); rightClickable
              ImGui::TableNextColumn();
              drawFMEnv(0,ins->snes.a+1,1+ins->snes.d*2,ins->snes.sus?ins->snes.d2:ins->snes.r,ins->snes.sus?ins->snes.r:31,(14-ins->snes.s*2),(ins->snes.r==0 || (ins->snes.sus && ins->snes.d2==0)),0,0,7,16,31,ImVec2(ImGui::GetContentRegionAvail().x,sliderSize.y),ins->type);

              ImGui::EndTable();
            }
            ImGui::Text(_("Sustain/release mode:"));
            if (ImGui::RadioButton(_("Direct (cut on release)"),ins->snes.sus==0)) {
              ins->snes.sus=0;
            }
            if (ImGui::RadioButton(_("Effective (linear decrease)"),ins->snes.sus==1)) {
              ins->snes.sus=1;
            }
            if (ImGui::RadioButton(_("Effective (exponential decrease)"),ins->snes.sus==2)) {
              ins->snes.sus=2;
            }
            if (ImGui::RadioButton(_("Delayed (write R on release)"),ins->snes.sus==3)) {
              ins->snes.sus=3;
            }
          } else {
            if (ImGui::BeginTable("SNESGainParams",2,ImGuiTableFlags_NoHostExtendX)) {
              ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed,sliderSize.x);

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              CENTER_TEXT(_("Gain Mode"));
              ImGui::TextUnformatted(_("Gain Mode"));
              ImGui::TableNextColumn();
              CENTER_TEXT(_("Gain"));
              ImGui::TextUnformatted(_("Gain"));

              ImGui::TableNextRow();
              ImGui::TableNextColumn();
              if (ImGui::RadioButton(_("Direct"),ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_DIRECT)) {
                ins->snes.gainMode=DivInstrumentSNES::GAIN_MODE_DIRECT;
                PARAMETER;
              }
              if (ImGui::RadioButton(_("Decrease (linear)"),ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_DEC_LINEAR)) {
                ins->snes.gainMode=DivInstrumentSNES::GAIN_MODE_DEC_LINEAR;
                PARAMETER;
              }
              if (ImGui::RadioButton(_("Decrease (logarithmic)"),ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_DEC_LOG)) {
                ins->snes.gainMode=DivInstrumentSNES::GAIN_MODE_DEC_LOG;
                PARAMETER;
              }
              if (ImGui::RadioButton(_("Increase (linear)"),ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_INC_LINEAR)) {
                ins->snes.gainMode=DivInstrumentSNES::GAIN_MODE_INC_LINEAR;
                PARAMETER;
              }
              if (ImGui::RadioButton(_("Increase (bent line)"),ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_INC_INVLOG)) {
                ins->snes.gainMode=DivInstrumentSNES::GAIN_MODE_INC_INVLOG;
                PARAMETER;
              }

              ImGui::TableNextColumn();
              unsigned char gainMax=(ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_DIRECT)?127:31;
              if (ins->snes.gain>gainMax) ins->snes.gain=gainMax;
              P(CWVSliderScalar("##Gain",sliderSize,ImGuiDataType_U8,&ins->snes.gain,&_ZERO,&gainMax)); rightClickable

              ImGui::EndTable();
            }
            if (ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_DEC_LINEAR || ins->snes.gainMode==DivInstrumentSNES::GAIN_MODE_DEC_LOG) {
              ImGui::TextWrapped(_("using decrease modes will not produce any sound at all, unless you know what you are doing.\nit is recommended to use the Gain macro for decrease instead."));
            }
          }
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_GB ||
            (ins->type==DIV_INS_AMIGA && ins->amiga.useWave) ||
            (ins->type==DIV_INS_GBA_DMA && ins->amiga.useWave) ||
            (ins->type==DIV_INS_X1_010 && !ins->amiga.useSample) ||
            ins->type==DIV_INS_N163 ||
            ins->type==DIV_INS_FDS ||
            (ins->type==DIV_INS_SWAN && !ins->amiga.useSample) ||
            (ins->type==DIV_INS_PCE && !ins->amiga.useSample) ||
            (ins->type==DIV_INS_VBOY) ||
            ins->type==DIV_INS_SCC ||
            ins->type==DIV_INS_SNES ||
            ins->type==DIV_INS_NAMCO ||
            ins->type==DIV_INS_SM8521 ||
            (ins->type==DIV_INS_GBA_MINMOD && ins->amiga.useWave)) 
        {
          insTabWavetable(ins);
        }
        if (ins->type<DIV_INS_MAX) if (ImGui::BeginTabItem(_("Macros"))) {
          // NEW CODE
          // this is only the first part of an insEdit refactor.
          // don't complain yet!
          int waveCount=MAX(1,e->song.waveLen-1);

          switch (ins->type) {
            case DIV_INS_STD:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Mode"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_KLATTSCH:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Voicing"),&ins->std.ex1Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Aspiration"),&ins->std.ex2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Spectral Tilt"),&ins->std.ex3Macro,-128,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Glottal Effort"),&ins->std.ex4Macro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Formant Shift"),&ins->std.ex5Macro,1,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              break;
            case DIV_INS_FM:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_GB:
              if (ins->gb.softEnv) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Noise"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_C64:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.dutyMacro,ins->c64.dutyIsAbs?0:-4095,4095,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,4,64,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,c64ShapeBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Cutoff"),&ins->std.algMacro,ins->c64.filterIsAbs?0:-2047,2047,160,uiColors[GUI_COLOR_MACRO_FILTER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Resonance"),&ins->std.ex2Macro,0,15,64,uiColors[GUI_COLOR_MACRO_FILTER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter Mode"),&ins->std.ex1Macro,0,4,64,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,filtModeBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter Toggle"),&ins->std.ex3Macro,0,1,32,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Special"),&ins->std.ex4Macro,0,4,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,c64TestGateBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Attack"),&ins->std.ex5Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Decay"),&ins->std.ex6Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Sustain"),&ins->std.ex7Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Release"),&ins->std.ex8Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              break;
            case DIV_INS_AMIGA:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,64,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              if (ins->std.panLMacro.mode) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-16,16,63,uiColors[GUI_COLOR_MACRO_OTHER],false,macroQSoundMode));
                macroList.push_back(FurnaceGUIMacroDesc(_("Surround"),&ins->std.panRMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              } else {
                macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,127,158,uiColors[GUI_COLOR_MACRO_OTHER],false,macroQSoundMode));
                macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,127,158,uiColors[GUI_COLOR_MACRO_OTHER]));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_PCE:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,31,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise"),&ins->std.dutyMacro,0,1,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_AY:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise Freq"),&ins->std.dutyMacro,0,31,160,uiColors[GUI_COLOR_MACRO_NOISE]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,3,48,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,ayShapeBits));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Envelope"),&ins->std.ex2Macro,0,4,64,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,true,ayEnvBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("AutoEnv Num"),&ins->std.ex3Macro,0,15,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("AutoEnv Den"),&ins->std.algMacro,0,15,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Force Period"),&ins->std.ex4Macro,0,4095,160,uiColors[GUI_COLOR_MACRO_PITCH]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Env Period"),&ins->std.ex5Macro,0,65535,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              break;
            case DIV_INS_AY8930:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,31,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise Freq"),&ins->std.dutyMacro,0,255,160,uiColors[GUI_COLOR_MACRO_NOISE]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,3,64,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,ayShapeBits));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.ex1Macro,0,8,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Envelope"),&ins->std.ex2Macro,0,4,64,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,true,ayEnvBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("AutoEnv Num"),&ins->std.ex3Macro,0,15,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("AutoEnv Den"),&ins->std.algMacro,0,15,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Force Period"),&ins->std.ex4Macro,0,65535,160,uiColors[GUI_COLOR_MACRO_PITCH]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Env Period"),&ins->std.ex5Macro,0,65535,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise AND Mask"),&ins->std.fbMacro,0,8,96,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise OR Mask"),&ins->std.fmsMacro,0,8,96,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
              break;
            case DIV_INS_TIA:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,15,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_SAA1099:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Noise"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,2,64,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,ayShapeBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Envelope"),&ins->std.ex1Macro,0,8,160,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,true,saaEnvBits));
              break;
            case DIV_INS_VIC:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("On/Off"),&ins->std.dutyMacro,0,1,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,15,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_PET:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,1,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,8,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_VRC6:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.dutyMacro,0,7,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              } else {
                macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              if (ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              }
              break;
            case DIV_INS_OPLL:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Patch"),&ins->std.waveMacro,0,15,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_OPL:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,63,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,4,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_FDS:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,32,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Mod Depth"),&ins->std.ex1Macro,0,63,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Mod Speed"),&ins->std.ex2Macro,0,4095,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Mod Position"),&ins->std.ex3Macro,0,127,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              break;
            case DIV_INS_VBOY:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Length"),&ins->std.dutyMacro,0,7,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_N163:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Wave Pos"),&ins->std.dutyMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Wave Length"),&ins->std.ex1Macro,0,252,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              break;
            case DIV_INS_SCC:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_OPZ:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Freq"),&ins->std.dutyMacro,0,32,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_POKEY:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("AUDCTL"),&ins->std.dutyMacro,0,8,160,uiColors[GUI_COLOR_MACRO_GLOBAL],false,NULL,NULL,true,pokeyCtlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,7,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_BEEPER: {
              bool zxPresent=false;

              for (int i=0; i<e->song.systemLen; i++) {
                if (e->song.system[i]==DIV_SYSTEM_SFX_BEEPER) {
                  zxPresent=true;
                  break;
                }
              }

              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,1,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (zxPresent) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Pulse Width"),&ins->std.dutyMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            }
            case DIV_INS_SWAN:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise"),&ins->std.dutyMacro,0,8,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_MIKEY:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Int"),&ins->std.dutyMacro,0,10,160,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true,mikeyFeedbackBits));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Load LFSR"),&ins->std.ex1Macro,0,12,160,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
              break;
            case DIV_INS_VERA:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,63,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.dutyMacro,0,63,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,3,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,macroVERAWaves,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_X1_010:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              if (ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              } else {
                macroList.push_back(FurnaceGUIMacroDesc(_("Envelope Mode"),&ins->std.ex1Macro,0,7,160,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,true,x1_010EnvBits));
                macroList.push_back(FurnaceGUIMacroDesc(_("Envelope"),&ins->std.ex2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_ENVELOPE],false,NULL,NULL,false,ayEnvBits));
                macroList.push_back(FurnaceGUIMacroDesc(_("AutoEnv Num"),&ins->std.ex3Macro,0,15,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
                macroList.push_back(FurnaceGUIMacroDesc(_("AutoEnv Den"),&ins->std.algMacro,0,15,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              }
              break;
            case DIV_INS_VRC6_SAW:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,63,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_ES5506: {
              float usesAmigaVol=true;
              for (int i=0; i<e->song.systemLen; i++) {
                if (e->song.system[i]==DIV_SYSTEM_ES5506) {
                  if (!e->song.systemFlags[i].getBool("amigaVol",false)) {
                    usesAmigaVol=false;
                    break;
                  }
                }
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,usesAmigaVol?64:4095,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter Mode"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,&macroHoverES5506FilterMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,4095,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,4095,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter K1"),&ins->std.ex1Macro,((ins->std.ex1Macro.mode==1)?(-65535):0),65535,160,uiColors[GUI_COLOR_MACRO_FILTER],false,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter K2"),&ins->std.ex2Macro,((ins->std.ex2Macro.mode==1)?(-65535):0),65535,160,uiColors[GUI_COLOR_MACRO_FILTER],false,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Outputs"),&ins->std.fbMacro,0,5,64,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Control"),&ins->std.algMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,es5506ControlModes));
              break;
            }
            case DIV_INS_MULTIPCM:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-7,7,45,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("LFO Speed"),&ins->std.ex1Macro,0,7,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("LFO Vib Depth"),&ins->std.fmsMacro,0,7,160,uiColors[GUI_COLOR_MACRO_PITCH]));
              macroList.push_back(FurnaceGUIMacroDesc(_("LFO AM Depth"),&ins->std.amsMacro,0,7,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              break;
            case DIV_INS_SNES:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Freq"),&ins->std.dutyMacro,0,31,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,127,158,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,127,158,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Special"),&ins->std.ex1Macro,0,5,96,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,snesModeBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Gain"),&ins->std.ex2Macro,0,255,256,uiColors[GUI_COLOR_MACRO_VOLUME],false,NULL,macroHoverGain,false));
              break;
            case DIV_INS_SU:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Noise"),&ins->std.dutyMacro,0,127,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,7,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,macroSoundUnitWaves,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-127,127,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Cutoff"),&ins->std.ex1Macro,0,16383,160,uiColors[GUI_COLOR_MACRO_FILTER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Resonance"),&ins->std.ex2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_FILTER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Control"),&ins->std.ex3Macro,0,4,64,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,suControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset Timer"),&ins->std.ex4Macro,0,65535,160,uiColors[GUI_COLOR_MACRO_PITCH])); // again reuse code from resonance macro but use ex4 instead
              break;
            case DIV_INS_NAMCO:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise"),&ins->std.dutyMacro,0,1,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (rear left)"),&ins->std.ex1Macro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (rear right)"),&ins->std.ex2Macro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_OPL_DRUMS:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,63,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,4,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_OPM:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Freq"),&ins->std.dutyMacro,0,32,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_NES:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Noise"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_MSM6258:
              macroList.push_back(FurnaceGUIMacroDesc(_("Freq Divider"),&ins->std.dutyMacro,0,2,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Clock Divider"),&ins->std.ex1Macro,0,1,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              break;
            case DIV_INS_MSM6295:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,8,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Freq Divider"),&ins->std.dutyMacro,0,1,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_ADPCMA:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,31,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Global Volume"),&ins->std.dutyMacro,0,63,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_ADPCMB:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_SEGAPCM:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,127,158,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,127,158,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_QSOUND:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,16383,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Echo Level"),&ins->std.dutyMacro,0,32767,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-16,16,63,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Surround"),&ins->std.panRMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Echo Feedback"),&ins->std.ex1Macro,0,16383,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Echo Length"),&ins->std.ex2Macro,0,2725,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              break;
            case DIV_INS_YMZ280B:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-7,7,45,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_RF5C68:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_MSM5232:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Group Ctrl"),&ins->std.dutyMacro,0,5,160,uiColors[GUI_COLOR_MACRO_GLOBAL],false,NULL,NULL,true,msm5232ControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Group Attack"),&ins->std.ex1Macro,0,5,96,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Group Decay"),&ins->std.ex2Macro,0,11,160,uiColors[GUI_COLOR_MACRO_GLOBAL]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise"),&ins->std.ex3Macro,0,1,32,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
              break;
            case DIV_INS_T6W28:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Type"),&ins->std.dutyMacro,0,1,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_K007232:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_GA20:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_POKEMINI:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,2,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pulse Width"),&ins->std.dutyMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_SUPERVISION:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Noise"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise/PCM Pan"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_SM8521:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,31,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_PV1000:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,1,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_K053260:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-3,3,37,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_TED:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,8,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Square/Noise"),&ins->std.dutyMacro,0,2,80,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true,tedControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_C140:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_C219:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Control"),&ins->std.dutyMacro,0,3,120,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,c219ControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_YAM10:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_ESFM:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,63,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("OP4 Noise Mode"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_POWERNOISE:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Control"),&ins->std.ex1Macro,0,2,32,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true,powerNoiseControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Tap A Location"),&ins->std.ex4Macro,0,15,96,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Tap B Location"),&ins->std.ex5Macro,0,15,96,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Load LFSR"),&ins->std.ex8Macro,0,16,256,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
              break;
            case DIV_INS_POWERNOISE_SLOPE:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,15,46,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Control"),&ins->std.ex1Macro,0,6,96,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,powerNoiseSlopeControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Portion A Length"),&ins->std.ex2Macro,0,255,128,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Portion B Length"),&ins->std.ex3Macro,0,255,128,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Portion A Offset"),&ins->std.ex6Macro,0,15,96,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Portion B Offset"),&ins->std.ex7Macro,0,15,96,uiColors[GUI_COLOR_MACRO_OTHER]));
              break;
            case DIV_INS_DAVE:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,63,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Freq"),&ins->std.dutyMacro,0,3,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,4,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,63,94,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,63,94,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Control"),&ins->std.ex1Macro,0,4,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,daveControlBits));
              break;
            case DIV_INS_NDS:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              if (!ins->amiga.useSample) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.dutyMacro,0,7,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,-64,63,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_GBA_DMA:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,2,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,panBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              break;
            case DIV_INS_GBA_MINMOD:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Special"),&ins->std.ex1Macro,0,2,96,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,minModModeBits));
              break;
            case DIV_INS_BIFURCATOR:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Parameter"),&ins->std.dutyMacro,0,65535,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Load Value"),&ins->std.ex1Macro,0,65535,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              break;
            case DIV_INS_SID2:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,15,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.dutyMacro,ins->c64.dutyIsAbs?0:-4095,4095,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,4,64,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,c64ShapeBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Cutoff"),&ins->std.algMacro,ins->c64.filterIsAbs?0:-4095,4095,160,uiColors[GUI_COLOR_MACRO_FILTER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Resonance"),&ins->std.ex2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_FILTER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter Mode"),&ins->std.ex1Macro,0,3,64,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true,filtModeBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Filter Toggle"),&ins->std.ex3Macro,0,1,32,uiColors[GUI_COLOR_MACRO_FILTER],false,NULL,NULL,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Special"),&ins->std.ex4Macro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,sid2ControlBits));
              macroList.push_back(FurnaceGUIMacroDesc(_("Attack"),&ins->std.ex5Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Decay"),&ins->std.ex6Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Sustain"),&ins->std.ex7Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Release"),&ins->std.ex8Macro,0,15,128,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Noise Mode"),&ins->std.fmsMacro,0,3,64,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Wave Mix"),&ins->std.amsMacro,0,3,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroSID2WaveMixMode));
              break;
            case DIV_INS_UPD1771C:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,31,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,7,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Wave Pos"),&ins->std.ex1Macro,0,31,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Duty/Mode"),&ins->std.dutyMacro,0,1,160,uiColors[GUI_COLOR_MACRO_NOISE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
              break;
            case DIV_INS_SID3:
              macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,255,160,uiColors[GUI_COLOR_MACRO_VOLUME]));

              macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
              macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));

              if (ins->sid3.doWavetable) {
                int waveCount=MAX(1,e->song.waveLen-1);
                macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,waveCount,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,false,NULL));
              } else {
                macroList.push_back(FurnaceGUIMacroDesc(_("Duty"),&ins->std.dutyMacro,ins->sid3.dutyIsAbs?0:-65535,65535,160,uiColors[GUI_COLOR_MACRO_OTHER]));
                macroList.push_back(FurnaceGUIMacroDesc(_("Waveform"),&ins->std.waveMacro,0,5,16 * 5,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,NULL,true,sid3ShapeBits));
                macroList.push_back(FurnaceGUIMacroDesc(_("Special Wave"),&ins->std.algMacro,0,SID3_NUM_SPECIAL_WAVES - 1,160,uiColors[GUI_COLOR_MACRO_WAVE],false,NULL,macroSID3SpecialWaves));
              }

              if (ins->sid3.separateNoisePitch && !ins->sid3.doWavetable) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise Arpeggio"),&ins->std.opMacros[3].amMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.opMacros[3].amMacro.val,true));
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise Pitch"),&ins->std.opMacros[0].arMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode,NULL,false,NULL,false,NULL,false,true));
              }

              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (left)"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL));
              macroList.push_back(FurnaceGUIMacroDesc(_("Panning (right)"),&ins->std.panRMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));

              macroList.push_back(FurnaceGUIMacroDesc(_("Channel inversion"),&ins->std.opMacros[2].arMacro,0,2,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,minModModeBits));

              macroList.push_back(FurnaceGUIMacroDesc(_("Key On/Off"),&ins->std.opMacros[0].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));

              macroList.push_back(FurnaceGUIMacroDesc(_("Special"),&ins->std.ex1Macro,0,3,48,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,sid3ControlBits));

              macroList.push_back(FurnaceGUIMacroDesc(_("Ring Mod Source"),&ins->std.fmsMacro,0,SID3_NUM_CHANNELS,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroSID3SourceChan));
              macroList.push_back(FurnaceGUIMacroDesc(_("Hard Sync Source"),&ins->std.amsMacro,0,SID3_NUM_CHANNELS - 1,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroSID3SourceChan));
              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Mod Source"),&ins->std.fbMacro,0,SID3_NUM_CHANNELS - 1,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroSID3SourceChan));
              
              if (!ins->sid3.doWavetable) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Feedback"),&ins->std.opMacros[3].arMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
              }

              macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));

              if (!ins->sid3.doWavetable) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise Phase Reset"),&ins->std.opMacros[1].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));
              }
              macroList.push_back(FurnaceGUIMacroDesc(_("Envelope Reset"),&ins->std.opMacros[2].amMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));

              macroList.push_back(FurnaceGUIMacroDesc(_("Attack"),&ins->std.ex2Macro,0,255,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Decay"),&ins->std.ex3Macro,0,255,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Sustain"),&ins->std.ex4Macro,0,255,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Sustain Rate"),&ins->std.ex5Macro,0,255,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Release"),&ins->std.ex6Macro,0,255,160,uiColors[GUI_COLOR_MACRO_ENVELOPE]));

              if (!ins->sid3.doWavetable) {
                macroList.push_back(FurnaceGUIMacroDesc(_("Noise LFSR bits"),&ins->std.ex7Macro,0,30,16 * 30,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,macroSID3NoiseLFSR,true));
                macroList.push_back(FurnaceGUIMacroDesc(_("1-Bit Noise"),&ins->std.opMacros[1].arMacro,0,1,32,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
                macroList.push_back(FurnaceGUIMacroDesc(_("Wave Mix"),&ins->std.ex8Macro,0,4,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroSID3WaveMixMode));
              } else {
                macroList.push_back(FurnaceGUIMacroDesc(_("Sample Mode"),&ins->std.opMacros[1].arMacro,0,1,32,uiColors[GUI_COLOR_MACRO_NOISE],false,NULL,NULL,true));
              }
              break;
            case DIV_INS_MAX:
            case DIV_INS_NULL:
              break;
          }

          drawMacros(macroList,macroEditStateMacros,ins);
          ImGui::EndTabItem();
        }
        if (ins->type==DIV_INS_AY) {
          if (!ins->amiga.useSample)
          {
            if (ImGui::BeginTabItem(_("Timer Macros")))
            {
              ImGui::Text(_("warning: timer effects require direct stream mode to be enabled during VGM export!"));
              macroList.push_back(FurnaceGUIMacroDesc(_("Timer FX"),&ins->std.ex6Macro,0,2,64,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,macroTFXModes));
              macroList.push_back(FurnaceGUIMacroDesc(_("Timer Offset"),&ins->std.ex7Macro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode,NULL,false,NULL,false,NULL,false,true));
              macroList.push_back(FurnaceGUIMacroDesc(_("Timer Num"),&ins->std.ex8Macro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("Timer Den"),&ins->std.fmsMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
              macroList.push_back(FurnaceGUIMacroDesc(_("PWM Boundary"),&ins->std.amsMacro,0,15,64,uiColors[GUI_COLOR_MACRO_OTHER]));
              drawMacros(macroList,macroEditStateMacros,ins);
              ImGui::EndTabItem();
            }
          }
        }
        if (ins->type==DIV_INS_POWERNOISE || ins->type==DIV_INS_POWERNOISE_SLOPE) {
          if (ImGui::BeginTabItem("PowerNoise")) {
            int pnOctave=ins->powernoise.octave;
            if (ImGui::InputInt(_("Octave offset"),&pnOctave,1,4)) { PARAMETER
              if (pnOctave<0) pnOctave=0;
              if (pnOctave>15) pnOctave=15;
              ins->powernoise.octave=pnOctave;
            }
            ImGui::Text(_("go to Macros for other parameters."));
            ImGui::EndTabItem();
          }
        }
        if (ins->type==DIV_INS_NES ||
            ins->type==DIV_INS_AY ||
            ins->type==DIV_INS_AY8930 ||
            ins->type==DIV_INS_MIKEY ||
            ins->type==DIV_INS_PCE ||
            ins->type==DIV_INS_X1_010 ||
            ins->type==DIV_INS_SWAN ||
            ins->type==DIV_INS_VRC6) {
          insTabSample(ins);
        }
        if (ins->type>=DIV_INS_MAX) {
          if (ImGui::BeginTabItem(_("Error"))) {
            ImGui::Text(_("invalid instrument type! change it first."));
            ImGui::EndTabItem();
          }
        }
        ImGui::EndTabBar();
      }
      if (settings.insEditColorize) {
        popAccentColors();
      }
    }
    if (displayMacroMenu) {
      displayMacroMenu=false;
      if (lastMacroDesc.macro!=NULL) {
        ImGui::OpenPopup("macroMenu");
      }
    }
    if (ImGui::BeginPopup("macroMenu",ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings)) {
      if (ImGui::MenuItem(_("copy"))) {
        String mmlStr;
        encodeMMLStr(mmlStr,lastMacroDesc.macro->val,lastMacroDesc.macro->len,lastMacroDesc.macro->loop,lastMacroDesc.macro->rel);
        SDL_SetClipboardText(mmlStr.c_str());
      }
      if (ImGui::MenuItem(_("paste"))) {
        String mmlStr;
        char* clipText=SDL_GetClipboardText();
        if (clipText!=NULL) {
          if (clipText[0]) {
            mmlStr=clipText;
          }
          SDL_free(clipText);
        }
        if (!mmlStr.empty()) {
          decodeMMLStr(mmlStr,lastMacroDesc.macro->val,lastMacroDesc.macro->len,lastMacroDesc.macro->loop,lastMacroDesc.min,(lastMacroDesc.isBitfield)?((1<<(lastMacroDesc.isBitfield?lastMacroDesc.max:0))-1):lastMacroDesc.max,lastMacroDesc.macro->rel);
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem(_("clear"))) {
        lastMacroDesc.macro->len=0;
        lastMacroDesc.macro->loop=255;
        lastMacroDesc.macro->rel=255;
        for (int i=0; i<256; i++) {
          lastMacroDesc.macro->val[i]=0;
        }
      }
      if (ImGui::MenuItem(_("clear contents"))) {
        for (int i=0; i<256; i++) {
          lastMacroDesc.macro->val[i]=0;
        }
      }
      ImGui::Separator();
      if (ImGui::BeginMenu(_("offset..."))) {
        ImGui::InputInt(_("X"),&macroOffX,1,10);
        ImGui::InputInt(_("Y"),&macroOffY,1,10);
        if (ImGui::Button(_("offset"))) {
          int oldData[256];
          memset(oldData,0,256*sizeof(int));
          memcpy(oldData,lastMacroDesc.macro->val,lastMacroDesc.macro->len*sizeof(int));

          for (int i=0; i<lastMacroDesc.macro->len; i++) {
            int val=0;
            bool bit30=false;
            if ((i-macroOffX)>=0 && (i-macroOffX)<lastMacroDesc.macro->len) {
              bit30=enBit30(oldData[i-macroOffX]);
              val=deBit30(oldData[i-macroOffX])+macroOffY;
              if (val<lastMacroDesc.min) val=lastMacroDesc.min;
              if (val>lastMacroDesc.max) val=lastMacroDesc.max;
            }
            lastMacroDesc.macro->val[i]=val^(bit30?0x40000000:0);
          }

          if (lastMacroDesc.macro->loop<lastMacroDesc.macro->len) {
            lastMacroDesc.macro->loop+=macroOffX;
          } else {
            lastMacroDesc.macro->loop=255;
          }
          if ((lastMacroDesc.macro->rel+macroOffX)>=0 && (lastMacroDesc.macro->rel+macroOffX)<lastMacroDesc.macro->len) {
            lastMacroDesc.macro->rel+=macroOffX;
          } else {
            lastMacroDesc.macro->rel=255;
          }

          ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu(_("scale..."))) {
        if (ImGui::InputFloat(_("X"),&macroScaleX,1.0f,10.0f,"%.2f%%")) {
          if (macroScaleX<0.1) macroScaleX=0.1;
          if (macroScaleX>12800.0) macroScaleX=12800.0;
        }
        ImGui::InputFloat(_("Y"),&macroScaleY,1.0f,10.0f,"%.2f%%");
        if (ImGui::Button(_("scale"))) {
          int oldData[256];
          memset(oldData,0,256*sizeof(int));
          memcpy(oldData,lastMacroDesc.macro->val,lastMacroDesc.macro->len*sizeof(int));

          unsigned char oldLen=lastMacroDesc.macro->len;
          lastMacroDesc.macro->len=MIN(255,((double)lastMacroDesc.macro->len*(macroScaleX/100.0)));

          for (int i=0; i<lastMacroDesc.macro->len; i++) {
            int val=0;
            bool bit30=false;
            double posX=round((double)i*(100.0/macroScaleX)-0.01);
            if (posX>=0 && posX<oldLen) {
              val=round((double)deBit30(oldData[(int)posX])*(macroScaleY/100.0));
              bit30=enBit30(oldData[(int)posX]);
              if (val<lastMacroDesc.min) val=lastMacroDesc.min;
              if (val>lastMacroDesc.max) val=lastMacroDesc.max;
            }
            lastMacroDesc.macro->val[i]=val^(bit30?0x40000000:0);
          }

          ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu(_("randomize..."))) {
        if (macroRandMin<lastMacroDesc.min) macroRandMin=lastMacroDesc.min;
        if (macroRandMin>lastMacroDesc.max) macroRandMin=lastMacroDesc.max;
        if (macroRandMax<lastMacroDesc.min) macroRandMax=lastMacroDesc.min;
        if (macroRandMax>lastMacroDesc.max) macroRandMax=lastMacroDesc.max;
        ImGui::InputInt(_("Min"),&macroRandMin,1,10);
        ImGui::InputInt(_("Max"),&macroRandMax,1,10);
        if (ImGui::Button(_("randomize"))) {
          for (int i=0; i<lastMacroDesc.macro->len; i++) {
            int val=0;
            if (macroRandMax<=macroRandMin) {
              val=macroRandMin;
            } else {
              val=macroRandMin+(rand()%(macroRandMax-macroRandMin+1));
            }
            lastMacroDesc.macro->val[i]=val;
          }

          ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
      }
      
      ImGui::EndPopup();
    }
  }

  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) curWindow=GUI_WINDOW_INS_EDIT;
  ImGui::End();
}

void FurnaceGUI::checkRecordInstrumentUndoStep() {
  if (insEditOpen && curIns>=0 && curIns<(int)e->song.ins.size()) {
    DivInstrument* ins=e->song.ins[curIns];

    // invalidate cachedCurIns/any possible changes if the cachedCurIns was referencing a different
    // instrument altgoether
    bool insChanged=ins!=cachedCurInsPtr;
    if (insChanged) {
      insEditMayBeDirty=false;
      cachedCurInsPtr=ins;
      cachedCurIns=*ins;
    }

    cachedCurInsPtr=ins;

    // check against the last cached to see if diff -- note that modifications to instruments
    // happen outside drawInsEdit (e.g. cursor inputs are processed and can directly modify
    // macro data).  but don't check until we think the user input is complete.
    bool delayDiff=ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::GetIO().WantCaptureKeyboard;
    if (!delayDiff && insEditMayBeDirty) {
      bool hasChange=ins->recordUndoStepIfChanged(e->processTime, &cachedCurIns);
      if (hasChange) {
        cachedCurIns=*ins;
      }
      insEditMayBeDirty=false;
    }
  } else {
    cachedCurInsPtr=NULL;
    insEditMayBeDirty=false;
  }
}

void FurnaceGUI::doUndoInstrument() {
  if (!insEditOpen) return;
  if (curIns<0 || curIns>=(int)e->song.ins.size()) return;
  DivInstrument* ins=e->song.ins[curIns];
  // is locking the engine necessary? copied from doUndoSample
  e->lockEngine([this,ins]() {
    ins->undo();
    cachedCurInsPtr=ins;
    cachedCurIns=*ins;
  });
}

void FurnaceGUI::doRedoInstrument() {
  if (!insEditOpen) return;
  if (curIns<0 || curIns>=(int)e->song.ins.size()) return;
  DivInstrument* ins=e->song.ins[curIns];
  // is locking the engine necessary? copied from doRedoSample
  e->lockEngine([this,ins]() {
    ins->redo();
    cachedCurInsPtr=ins;
    cachedCurIns=*ins;
  });
}
