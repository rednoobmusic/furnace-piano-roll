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

#ifndef _I8244_H
#define _I8244_H

#include "../dispatch.h"
#include "../waveSynth.h"

class DivPlatformI8244: public DivDispatch {
  struct Channel: public SharedChannel {
    int wave;
    // 0: square, 1: noise, 2: wavetable
    int mode;
    DivWaveSynth ws;
    Channel(bool linear=true):
      SharedChannel(15,linear),
      wave(-1),
      mode(0) {}
  };
  Channel chan[1];
  DivDispatchOscBuffer* oscBuf;
  DivPitchTable pitchTable;
  bool isMuted;

  // sound unit state
  // 24-bit recirculating shift register (A7/A8/A9). bit 0 is the output.
  unsigned int sreg;
  // control register (AA): bit 7: enable, bit 6: recirculate,
  // bit 5: rate select (4 lines/shift instead of 16), bit 4: noise,
  // bits 0-3: volume
  unsigned char sndCtrl;
  // scanlines until next shift (the shift clock is derived from hsync)
  int shiftCnt;
  // noise generator
  unsigned short lfsr;
  unsigned char lfsrBit;

  // current pitch quantization state
  int rateSel; // 1: high rate (clock/1820), 0: low rate (clock/7280)
  int kCycles; // waveform cycles per shift register loop (1-12)
  unsigned int lastPat;
  bool patWritten;

  // composer overrides (set by effects/macros, persist until changed)
  int forceRate;     // -1: auto, 0: force low, 1: force high
  int forceK;        // 0: auto, 1-12: force this many cycles
  int pulseWidth;    // square-mode on-width out of 24 (12 = 50%)
  bool recirculate;  // true: looping tone, false: one-shot decay
  int noiseOverride; // -1: follow mode, 0: force off, 1: force on

  unsigned char regPool[4];
  friend void putDispatchChip(void*,int);
  friend void putDispatchChan(void*,int,int);
  public:
    void acquire(short** buf, size_t len);
    int dispatch(DivCommand c);
    SharedChannel* getChanState(int chan);
    DivMacroInt* getChanMacroInt(int ch);
    DivDispatchOscBuffer* getOscBuffer(int chan);
    unsigned char* getRegisterPool();
    int getRegisterPoolSize();
    void reset();
    void forceIns();
    void tick(bool sysTick=true);
    void muteChannel(int ch, bool mute);
    bool keyOffAffectsArp(int ch);
    void notifyWaveChange(int wave);
    void notifyInsDeletion(void* ins);
    void notifyPitchTable(int sample=-1);
    int getOutputCount();
    void setFlags(const DivConfig& flags);
    void poke(unsigned int addr, unsigned short val);
    void poke(std::vector<DivRegWrite>& wlist);
    const char** getRegisterSheet();
    int init(DivEngine* parent, int channels, int sugRate, const DivConfig& flags);
    void quit();
    ~DivPlatformI8244();
  private:
    void rWrite(unsigned int addr, unsigned char val);
    void writeCtrl();
    void writeSR(unsigned int pat);
    unsigned int buildPattern(int k);
    void findRateAndK(int period);
};

#endif
