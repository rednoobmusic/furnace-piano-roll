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

#include "i8244.h"
#include "../engine.h"
#include <math.h>

// the sound unit of the Intel 8244/8245 (Magnavox Odyssey², Philips Videopac).
// a 24-bit recirculating shift register whose output bit drives a 4-bit volume DAC.
// the shift clock is derived from the horizontal line counter: one shift every
// 4 scanlines (high rate, ~3933Hz on NTSC) or 16 scanlines (low rate, ~983Hz).
// a polynomial counter may be XORed into the output for noise.
// pitch therefore comes from the bit pattern: k evenly-spaced waveform cycles
// in the 24-bit loop yield a tone of (shift rate)*k/24.

#define LINE_CLOCKS 455
#define SHIFT_LINES_HI 4
#define SHIFT_LINES_LO 16

const char* regCheatSheetI8244[]={
  "SR0", "A7",
  "SR1", "A8",
  "SR2", "A9",
  "Control", "AA",
  NULL
};

const char** DivPlatformI8244::getRegisterSheet() {
  return regCheatSheetI8244;
}

void DivPlatformI8244::rWrite(unsigned int addr, unsigned char val) {
  addr&=3;
  regPool[addr]=val;
  switch (addr) {
    case 0:
      sreg=(sreg&0x00ffff)|((unsigned int)val<<16);
      break;
    case 1:
      sreg=(sreg&0xff00ff)|((unsigned int)val<<8);
      break;
    case 2:
      sreg=(sreg&0xffff00)|val;
      break;
    case 3:
      sndCtrl=val;
      break;
  }
  if (dumpWrites && !skipRegisterWrites) addWrite(0xa7+addr,val);
}

void DivPlatformI8244::acquire(short** buf, size_t len) {
  oscBuf->begin(len);
  // one sample per scanline
  for (size_t h=0; h<len; h++) {
    int out=0;
    if (sndCtrl&0x80) {
      int bit=sreg&1;
      if (sndCtrl&0x10) bit^=lfsrBit;
      if (bit) out=(sndCtrl&15)*2184;
    }
    if (--shiftCnt<=0) {
      shiftCnt=(sndCtrl&0x20)?SHIFT_LINES_HI:SHIFT_LINES_LO;
      if (sndCtrl&0x40) {
        sreg=((sreg>>1)|((sreg&1)<<23))&0xffffff;
      } else {
        sreg>>=1;
      }
      regPool[0]=(sreg>>16)&0xff;
      regPool[1]=(sreg>>8)&0xff;
      regPool[2]=sreg&0xff;
      unsigned short fb=((lfsr>>0)^(lfsr>>1))&1;
      lfsr=(lfsr>>1)|(fb<<14);
      lfsrBit=lfsr&1;
    }
    if (isMuted) {
      buf[0][h]=0;
      oscBuf->putSample(h,0);
    } else {
      buf[0][h]=out;
      oscBuf->putSample(h,out);
    }
  }
  oscBuf->end(len);
}

void DivPlatformI8244::writeCtrl() {
  unsigned char ctrl=(chan[0].outVol&15);
  bool noise=(noiseOverride>=0)?(noiseOverride!=0):(chan[0].mode==1);
  if (noise) ctrl|=0x10;
  if (rateSel) ctrl|=0x20;
  if (recirculate) ctrl|=0x40;
  if (chan[0].active) ctrl|=0x80;
  rWrite(3,ctrl);
}

void DivPlatformI8244::writeSR(unsigned int pat) {
  pat&=0xffffff;
  rWrite(0,(pat>>16)&0xff);
  rWrite(1,(pat>>8)&0xff);
  rWrite(2,pat&0xff);
  lastPat=pat;
  patWritten=true;
}

unsigned int DivPlatformI8244::buildPattern(int k) {
  unsigned int pat=0;
  switch (chan[0].mode) {
    case 1: // noise - clear the register so only the polynomial counter is heard
      break;
    case 2: // wavetable - fit k cycles of the 24-step wave in the register
      for (int i=0; i<24; i++) {
        if (chan[0].ws.output[(i*k)%24]>=8) pat|=1U<<i;
      }
      break;
    default: // square - k cycles at the configured pulse width (0-24, 12=50%)
      for (int i=0; i<24; i++) {
        if (((i*k)%24)<pulseWidth) pat|=1U<<i;
      }
      break;
  }
  return pat;
}

void DivPlatformI8244::findRateAndK(int period) {
  if (period<1) period=1;
  int bestRate=1;
  int bestK=12;
  double bestErr=1e99;
  // prefer the high rate on ties
  for (int r=1; r>=0; r--) {
    if (forceRate>=0 && r!=forceRate) continue;
    int s=LINE_CLOCKS*(r?SHIFT_LINES_HI:SHIFT_LINES_LO);
    int k;
    if (forceK>=1) {
      k=forceK;
    } else {
      k=(int)(((24.0*s)/(double)period)+0.5);
    }
    if (k<1) k=1;
    if (k>12) k=12;
    double err=fabs(log(((double)period*k)/(24.0*s)));
    if (err<bestErr) {
      bestErr=err;
      bestRate=r;
      bestK=k;
    }
  }
  rateSel=bestRate;
  kCycles=bestK;
}

void DivPlatformI8244::tick(bool sysTick) {
  bool dirty=false;
  chan[0].std.next();
  if (chan[0].std.vol.had) {
    chan[0].outVol=((chan[0].vol&15)*MIN(15,chan[0].std.vol.val))/15;
    writeCtrl();
  }
  if (NEW_ARP_STRAT) {
    chan[0].handleArp();
  } else if (chan[0].std.arp.had) {
    if (!chan[0].inPorta) {
      chan[0].baseFreq=chan[0].calcBaseFreq(parent->calcArp(chan[0].note,chan[0].std.arp.val));
    }
    chan[0].freqChanged=true;
  }
  if (chan[0].std.duty.had) {
    int mode=CLAMP(chan[0].std.duty.val,0,2);
    if (chan[0].mode!=mode) {
      chan[0].mode=mode;
      writeCtrl();
      dirty=true;
    }
  }
  if (chan[0].std.wave.had) {
    if (chan[0].wave!=chan[0].std.wave.val || chan[0].ws.activeChanged()) {
      chan[0].wave=chan[0].std.wave.val;
      chan[0].ws.changeWave1(chan[0].wave);
    }
  }
  if (chan[0].std.ex1.had) {
    int pw=CLAMP(chan[0].std.ex1.val,0,24);
    if (pulseWidth!=pw) {
      pulseWidth=pw;
      patWritten=false;
      dirty=true;
    }
  }
  if (chan[0].std.pitch.had) {
    if (chan[0].std.pitch.mode) {
      chan[0].pitch2+=chan[0].std.pitch.val;
      CLAMP_VAR(chan[0].pitch2,-32768,32767);
    } else {
      chan[0].pitch2=chan[0].std.pitch.val;
    }
    chan[0].freqChanged=true;
  }
  if (chan[0].active && chan[0].mode==2) {
    if (chan[0].ws.tick()) {
      dirty=true;
    }
  }
  if (chan[0].std.phaseReset.had && chan[0].std.phaseReset.val==1) {
    lfsr=0x7fff;
    lfsrBit=1;
    shiftCnt=(sndCtrl&0x20)?SHIFT_LINES_HI:SHIFT_LINES_LO;
    patWritten=false;
    dirty=true;
  }
  if (chan[0].freqChanged || chan[0].keyOn || chan[0].keyOff) {
    chan[0].freq=chan[0].calcFreq();
    if (chan[0].freq<16) chan[0].freq=16;
    if (chan[0].freq>0x80000) chan[0].freq=0x80000;
    int oldRate=rateSel;
    int oldK=kCycles;
    findRateAndK(chan[0].freq);
    if (oldRate!=rateSel) writeCtrl();
    if (oldK!=kCycles || oldRate!=rateSel) dirty=true;
    if (chan[0].keyOn) {
      if (!chan[0].std.vol.will) {
        chan[0].outVol=chan[0].vol;
      }
      patWritten=false;
      dirty=true;
      chan[0].keyOn=false;
      writeCtrl();
    }
    if (chan[0].keyOff) {
      chan[0].keyOff=false;
      writeCtrl();
    }
    chan[0].freqChanged=false;
  }
  if (dirty || !patWritten) {
    unsigned int pat=buildPattern(kCycles);
    // rewriting the register resets its phase, so only do it when it changed
    if (!patWritten || pat!=lastPat) {
      writeSR(pat);
    }
  }
}

int DivPlatformI8244::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[0].ins,DIV_INS_I8244);
      if (c.value!=DIV_NOTE_NULL) {
        chan[0].baseFreq=chan[0].calcBaseFreq(c.value);
        chan[0].freqChanged=true;
        chan[0].note=c.value;
      }
      chan[0].active=true;
      chan[0].keyOn=true;
      chan[0].macroInit(ins);
      if (!parent->song.compatFlags.brokenOutVol && !chan[0].std.vol.will) {
        chan[0].outVol=chan[0].vol;
      }
      if (chan[0].wave<0) {
        chan[0].wave=0;
        chan[0].ws.changeWave1(chan[0].wave);
      }
      chan[0].ws.init(ins,24,15,chan[0].insChanged);
      chan[0].insChanged=false;
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[0].active=false;
      chan[0].keyOff=true;
      chan[0].macroInit(NULL);
      break;
    case DIV_CMD_NOTE_OFF_ENV:
    case DIV_CMD_ENV_RELEASE:
      chan[0].std.release();
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[0].ins!=c.value || c.value2==1) {
        chan[0].ins=c.value;
        chan[0].insChanged=true;
      }
      break;
    case DIV_CMD_VOLUME:
      if (chan[0].vol!=c.value) {
        chan[0].vol=c.value;
        if (!chan[0].std.vol.had) {
          chan[0].outVol=chan[0].vol;
          writeCtrl();
        }
      }
      break;
    case DIV_CMD_GET_VOLUME:
      return chan[0].vol;
      break;
    case DIV_CMD_PITCH:
      chan[0].pitch=c.value;
      chan[0].freqChanged=true;
      break;
    case DIV_CMD_WAVE:
      chan[0].wave=c.value;
      chan[0].ws.changeWave1(chan[0].wave);
      if (chan[0].mode==2) {
        patWritten=false;
        chan[0].freqChanged=true;
      }
      break;
    case DIV_CMD_STD_NOISE_MODE:
      chan[0].mode=CLAMP(c.value,0,2);
      writeCtrl();
      patWritten=false;
      chan[0].freqChanged=true;
      break;
    case DIV_CMD_I8244_RATE:
      if (c.value<=0) forceRate=-1;
      else if (c.value==1) forceRate=0;
      else forceRate=1;
      chan[0].freqChanged=true;
      break;
    case DIV_CMD_I8244_PULSE_WIDTH:
      pulseWidth=CLAMP(c.value,0,24);
      patWritten=false;
      break;
    case DIV_CMD_I8244_RECIRCULATE:
      recirculate=(c.value==0);
      writeCtrl();
      break;
    case DIV_CMD_I8244_CYCLES:
      forceK=CLAMP(c.value,0,12);
      chan[0].freqChanged=true;
      break;
    case DIV_CMD_I8244_NOISE:
      noiseOverride=(c.value!=0)?1:0;
      writeCtrl();
      patWritten=false;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=chan[0].calcBaseFreq(c.value2);
      bool return2=false;
      if (destFreq>chan[0].baseFreq) {
        chan[0].baseFreq+=c.value;
        if (chan[0].baseFreq>=destFreq) {
          chan[0].baseFreq=destFreq;
          return2=true;
        }
      } else {
        chan[0].baseFreq-=c.value;
        if (chan[0].baseFreq<=destFreq) {
          chan[0].baseFreq=destFreq;
          return2=true;
        }
      }
      chan[0].freqChanged=true;
      if (return2) {
        chan[0].inPorta=false;
        return 2;
      }
      break;
    }
    case DIV_CMD_LEGATO:
      chan[0].baseFreq=chan[0].calcBaseFreq(c.value+((HACKY_LEGATO_MESS)?(chan[0].std.arp.val):(0)));
      chan[0].freqChanged=true;
      chan[0].note=c.value;
      break;
    case DIV_CMD_PRE_PORTA:
      if (chan[0].active && c.value2) {
        if (parent->song.compatFlags.resetMacroOnPorta) chan[0].macroInit(parent->getIns(chan[0].ins,DIV_INS_I8244));
      }
      if (!chan[0].inPorta && c.value && !parent->song.compatFlags.brokenPortaArp && chan[0].std.arp.will && !NEW_ARP_STRAT) chan[0].baseFreq=chan[0].calcBaseFreq(chan[0].note);
      chan[0].inPorta=c.value;
      break;
    case DIV_CMD_GET_VOLMAX:
      return 15;
      break;
    case DIV_CMD_MACRO_OFF:
      chan[c.chan].std.mask(c.value,true);
      break;
    case DIV_CMD_MACRO_ON:
      chan[c.chan].std.mask(c.value,false);
      break;
    case DIV_CMD_MACRO_RESTART:
      chan[c.chan].std.restart(c.value);
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatformI8244::muteChannel(int ch, bool mute) {
  isMuted=mute;
}

void DivPlatformI8244::forceIns() {
  chan[0].insChanged=true;
  chan[0].freqChanged=true;
  patWritten=false;
  writeCtrl();
}

SharedChannel* DivPlatformI8244::getChanState(int ch) {
  return &chan[0];
}

DivMacroInt* DivPlatformI8244::getChanMacroInt(int ch) {
  return &chan[0].std;
}

DivDispatchOscBuffer* DivPlatformI8244::getOscBuffer(int ch) {
  return oscBuf;
}

unsigned char* DivPlatformI8244::getRegisterPool() {
  return regPool;
}

int DivPlatformI8244::getRegisterPoolSize() {
  return 4;
}

bool DivPlatformI8244::keyOffAffectsArp(int ch) {
  return true;
}

void DivPlatformI8244::notifyWaveChange(int wave) {
  if (chan[0].wave==wave) {
    chan[0].ws.changeWave1(wave);
    if (chan[0].mode==2) {
      patWritten=false;
      chan[0].freqChanged=true;
    }
  }
}

void DivPlatformI8244::notifyInsDeletion(void* ins) {
  chan[0].std.notifyInsDeletion((DivInstrument*)ins);
}

void DivPlatformI8244::notifyPitchTable(int sample) {
  pitchTable.init(parent->song.tuning,chipClock,1,0x80000,true,parent->song.compatFlags.linearPitch);
}

void DivPlatformI8244::reset() {
  memset(regPool,0,4);
  chan[0]=Channel(parent->song.compatFlags.linearPitch);
  chan[0].pitchTable=&pitchTable;
  chan[0].std.setEngine(parent);
  chan[0].ws.setEngine(parent,0);
  chan[0].ws.init(NULL,24,15,false);
  sreg=0;
  sndCtrl=0;
  shiftCnt=SHIFT_LINES_LO;
  lfsr=0x7fff;
  lfsrBit=1;
  rateSel=1;
  kCycles=1;
  lastPat=0;
  patWritten=false;
  forceRate=-1;
  forceK=0;
  pulseWidth=12;
  recirculate=true;
  noiseOverride=-1;
  if (dumpWrites) addWrite(0xffffffff,0);
  writeCtrl();
}

int DivPlatformI8244::getOutputCount() {
  return 1;
}

void DivPlatformI8244::setFlags(const DivConfig& flags) {
  if (flags.getInt("clockSel",0)==1) {
    // 8245 (PAL)
    chipClock=COLOR_PAL*2.0;
  } else {
    // 8244 (NTSC)
    chipClock=COLOR_NTSC*2.0;
  }
  CHECK_CUSTOM_CLOCK;
  rate=chipClock/LINE_CLOCKS;
  oscBuf->setRate(rate);
  notifyPitchTable();
}

void DivPlatformI8244::poke(unsigned int addr, unsigned short val) {
  rWrite(addr,val);
}

void DivPlatformI8244::poke(std::vector<DivRegWrite>& wlist) {
  for (DivRegWrite& i: wlist) rWrite(i.addr,i.val);
}

int DivPlatformI8244::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent=p;
  dumpWrites=false;
  skipRegisterWrites=false;
  isMuted=false;
  oscBuf=new DivDispatchOscBuffer;
  setFlags(flags);
  reset();
  return 1;
}

void DivPlatformI8244::quit() {
  delete oscBuf;
}

DivPlatformI8244::~DivPlatformI8244() {
}
