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

#include "yam10.h"
#include "../engine.h"
#include "../../ta-log.h"
#include <math.h>
#include <string.h>

// the volume column used to scale the output level straight, which put most
// of the audible change in the bottom of the fader. a mild curve spreads it
// out: half volume now lands at -4.5 dB rather than -6, and the last few
// steps no longer fall off a cliff.
static unsigned char yam10VolCurve[128];
static bool yam10VolCurveBuilt=false;

static void yam10BuildVolCurve() {
  if (yam10VolCurveBuilt) return;
  for (int i=0; i<128; i++) {
    yam10VolCurve[i]=(unsigned char)(127.0*pow((double)i/127.0,0.75)+0.5);
  }
  yam10VolCurveBuilt=true;
}

const char** DivPlatformYAM10::getRegisterSheet() {
  return NULL;
}

void DivPlatformYAM10::acquire(short** buf, size_t len) {
  // the scope buffer has to be opened before any sample goes in, otherwise the
  // slots the rate conversion skips keep whatever the last wrap left there and
  // a clean tone draws with spikes through it
  for (int i=0; i<YAM10_CHANS; i++) {
    if (oscBuf[i]!=NULL) oscBuf[i]->begin(len);
  }
  for (size_t h=0; h<len; h++) {
    chip.tickEG();
    // accumulate in float and round once at the end. truncating every
    // channel to int first cost a bit per channel and showed up as a
    // staircase on the per channel scope, which auto scales quiet tails.
    float mixL=0.0f, mixR=0.0f;
    for (int i=0; i<YAM10_CHANS; i++) {
      float l=0.0f, r=0.0f;
      if (!isMuted[i] && !chip.chanIdle(i)) chip.renderChan(i,l,r);
      mixL+=l;
      mixR+=r;
      if (oscBuf[i]!=NULL) {
        oscBuf[i]->putSample(h,CLAMP((int)floorf((l+r)*0.5f+0.5f),-32768,32767));
      }
    }
    buf[0][h]=CLAMP((int)floorf(mixL+0.5f),-32768,32767);
    buf[1][h]=CLAMP((int)floorf(mixR+0.5f),-32768,32767);
  }
  for (int i=0; i<YAM10_CHANS; i++) {
    if (oscBuf[i]!=NULL) oscBuf[i]->end(len);
  }
}

// push one channel's instrument settings into the chip
// everything an instrument sets on a channel, in one place so the editor
// preview builds its channel exactly the way playback does
void yam10ApplyInsToParam(YAM10ChanParam& dst, const DivInstrumentYAM10& srcIn, unsigned char vol, bool muted, DivEngine* song, std::vector<int>* waveStore) {
  yam10BuildVolCurve();
  const DivInstrumentYAM10& src=srcIn;
  dst.echoMix=src.echoMix;
  dst.echoDelay=src.echoDelay;
  dst.echoFeedback=src.echoFeedback;
  for (int i=0; i<YAM10_FILTERS; i++) {
    dst.filter[i].enable=src.filter[i].enable;
    dst.filter[i].mode=src.filter[i].mode;
    dst.filter[i].cutoff=src.filter[i].cutoff;
    dst.filter[i].res=src.filter[i].res;
  }
  dst.distEnable=src.distEnable;
  dst.distGain=src.distGain;
  dst.distCutoff=src.distCutoff;
  dst.distLevel=src.distLevel;
  dst.chorusEnable=src.chorusEnable;
  dst.chorusMix=src.chorusMix;
  dst.chorusRate=src.chorusRate;
  dst.chorusDepth=src.chorusDepth;
  dst.chorusFeedback=src.chorusFeedback;
  dst.chorusWidth=src.chorusWidth;
  dst.reverbMix=src.reverbMix;
  dst.reverbSend=src.reverbSend;
  dst.reverbDecay=src.reverbDecay;
  dst.compEnable=src.compEnable;
  dst.compThreshold=src.compThreshold;
  dst.compRatio=src.compRatio;
  dst.compMakeup=src.compMakeup;
  dst.compAttack=src.compAttack;
  dst.compDecay=src.compDecay;
  dst.compUpRatio=src.compUpRatio;
  dst.compLoMid=src.compLoMid;
  dst.compMidHi=src.compMidHi;
  dst.compLowGain=src.compLowGain;
  dst.compMidGain=src.compMidGain;
  dst.compHighGain=src.compHighGain;
  dst.reverbEnable=src.reverbEnable;
  dst.reverbEarly=src.reverbEarly;
  dst.reverbDiffusion=src.reverbDiffusion;
  dst.reverbSize=src.reverbSize;
  dst.reverbDamp=src.reverbDamp;
  dst.eqEnable=src.eqEnable;
  dst.eqCount=src.eqCount;
  for (int i=0; i<YAM10_EQ_BANDS; i++) {
    dst.eqBand[i].on=src.eqBand[i].on;
    dst.eqBand[i].type=src.eqBand[i].type;
    dst.eqBand[i].freq=src.eqBand[i].freq;
    dst.eqBand[i].gain=src.eqBand[i].gain;
    dst.eqBand[i].q=src.eqBand[i].q;
  }
  dst.phaseInvL=src.phaseInvL;
  dst.phaseInvR=src.phaseInvR;
  for (int i=0; i<YAM10_OPS; i++) {
    const DivInstrumentYAM10::Operator& o=src.op[i];
    YAM10OpParam& d=dst.op[i];
    d.enable=o.enable;
    d.fixedMode=o.fixedMode;
    d.ksr=o.ksr;
    d.customWave=o.customWave;
    d.ws=o.ws;
    d.tl=o.tl;
    d.ar=o.ar; d.dr=o.dr; d.d2r=o.d2r; d.sl=o.sl; d.rr=o.rr;
    d.rs=o.rs; d.mult=o.mult; d.delay=o.delay;
    d.dtFine=o.dtFine; d.dtSemi=o.dtSemi;
    d.fb=o.fb;
    d.outLvl=muted?0:(unsigned char)((int)o.outLvl*yam10VolCurve[vol&127]/127);
    d.pan=o.pan;
    d.modIn=o.modIn;
    d.duty=o.duty;
    // block and F-num, like OPZ. Hz = fnum * 2^block / 8
    d.fixedFreq=(double)(o.fixedFreq&1023)*(double)(1<<((o.fixedFreq>>10)&7))/8.0;
    d.phaseResetPeriod=o.phaseReset;
    // custom wavetable: copied where the caller asked, then pointed at
    d.waveData=NULL; d.waveLen=0; d.waveMax=255;
    if (o.customWave && song!=NULL && waveStore!=NULL) {
      DivWavetable* wt=song->getWave(o.customWaveIndex);
      if (wt!=NULL && wt->len>1) {
        waveStore[i].assign(wt->data,wt->data+wt->len);
        d.waveData=waveStore[i].data();
        d.waveLen=(int)waveStore[i].size();
        d.waveMax=wt->max>0?wt->max:255;
      }
    }
  }
}
void DivPlatformYAM10::applyInstrument(int ch) {
  yam10ApplyInsToParam(chip.par[ch],chan[ch].state,chan[ch].outVol,isMuted[ch],parent,waveCopy[ch]);
}

void DivPlatformYAM10::tick(bool sysTick) {
  if (sysTick) chip.tickPhaseReset();
  for (int i=0; i<YAM10_CHANS; i++) {
    chan[i].std.next();

    if (chan[i].std.vol.had) {
      chan[i].outVol=VOL_SCALE_LOG(chan[i].vol,MIN(127,chan[i].std.vol.val),127);
      for (int j=0; j<YAM10_OPS; j++) {
        if (chan[i].state.op[j].outLvl>0) {
          chip.par[i].op[j].outLvl=(isMuted[i])?0:
            (unsigned char)((int)chan[i].state.op[j].outLvl*yam10VolCurve[chan[i].outVol&127]/127);
        }
      }
    }
    if (NEW_ARP_STRAT) {
      chan[i].handleArp();
    } else if (chan[i].std.arp.had) {
      if (!chan[i].inPorta) {
        chan[i].baseFreq=chan[i].calcBaseFreq(parent->calcArp(chan[i].note,chan[i].std.arp.val));
      }
      chan[i].freqChanged=true;
    }
    if (chan[i].std.pitch.had) {
      if (chan[i].std.pitch.mode) {
        chan[i].pitch2+=chan[i].std.pitch.val;
        CLAMP_VAR(chan[i].pitch2,-32768,32767);
      } else {
        chan[i].pitch2=chan[i].std.pitch.val;
      }
      chan[i].freqChanged=true;
    }

    if (chan[i].std.panL.had) {
      for (int j=0; j<YAM10_OPS; j++) {
        chan[i].state.op[j].pan=chan[i].std.panL.val&255;
        chip.par[i].op[j].pan=chan[i].state.op[j].pan;
      }
    }

    if (chan[i].std.phaseReset.had && chan[i].std.phaseReset.val==1) {
      chip.keyOn(i);
    }

    // DSP macros
    {
      DivInstrumentYAM10& st=chan[i].state;
      YAM10ChanParam& cp=chip.par[i];
      if (chan[i].std.ex1.had)  { st.filter[0].cutoff=chan[i].std.ex1.val&255;  cp.filter[0].cutoff=st.filter[0].cutoff; }
      if (chan[i].std.ex2.had)  { st.filter[0].res=chan[i].std.ex2.val&255;     cp.filter[0].res=st.filter[0].res; }
      if (chan[i].std.ex3.had)  { st.filter[1].cutoff=chan[i].std.ex3.val&255;  cp.filter[1].cutoff=st.filter[1].cutoff; }
      if (chan[i].std.ex4.had)  { st.filter[1].res=chan[i].std.ex4.val&255;     cp.filter[1].res=st.filter[1].res; }
      if (chan[i].std.ex5.had)  { st.filter[2].cutoff=chan[i].std.ex5.val&255;  cp.filter[2].cutoff=st.filter[2].cutoff; }
      if (chan[i].std.ex6.had)  { st.filter[2].res=chan[i].std.ex6.val&255;     cp.filter[2].res=st.filter[2].res; }
      if (chan[i].std.ex7.had)  { st.distGain=chan[i].std.ex7.val&127;          cp.distGain=st.distGain; }
      if (chan[i].std.ex8.had)  { st.distLevel=chan[i].std.ex8.val&127;         cp.distLevel=st.distLevel; }
      if (chan[i].std.ex9.had)  { st.chorusMix=chan[i].std.ex9.val&127;         cp.chorusMix=st.chorusMix; }
      if (chan[i].std.wave.had) { st.chorusRate=chan[i].std.wave.val&127;       cp.chorusRate=st.chorusRate; }
      if (chan[i].std.duty.had) { st.chorusDepth=chan[i].std.duty.val&127;      cp.chorusDepth=st.chorusDepth; }
      if (chan[i].std.ex10.had) { st.echoMix=chan[i].std.ex10.val&127;          cp.echoMix=st.echoMix; }
      if (chan[i].std.ams.had)  { st.echoFeedback=chan[i].std.ams.val&127;      cp.echoFeedback=st.echoFeedback; }
      // chip wide FM: which operators sound, and one feedback for all of them
      if (chan[i].std.alg.had) {
        for (int j=0; j<YAM10_OPS; j++) {
          st.op[j].enable=(chan[i].std.alg.val>>j)&1;
          cp.op[j].enable=st.op[j].enable;
        }
      }
      if (chan[i].std.fb.had) {
        for (int j=0; j<YAM10_OPS; j++) {
          st.op[j].fb=chan[i].std.fb.val&7;
          cp.op[j].fb=st.op[j].fb;
        }
      }
    }

    // per operator macros
    for (int j=0; j<YAM10_OPS; j++) {
      DivMacroInt::IntOp& m=chan[i].std.op[j];
      DivInstrumentYAM10::Operator& o=chan[i].state.op[j];
      YAM10OpParam& d=chip.par[i].op[j];
      if (m.tl.had)   { o.tl=m.tl.val&127;    d.tl=o.tl; }
      if (m.ar.had)   { o.ar=m.ar.val&31;     d.ar=o.ar; }
      if (m.dr.had)   { o.dr=m.dr.val&31;     d.dr=o.dr; }
      if (m.d2r.had)  { o.d2r=m.d2r.val&31;   d.d2r=o.d2r; }
      if (m.sl.had)   { o.sl=m.sl.val&15;     d.sl=o.sl; }
      if (m.rr.had)   { o.rr=m.rr.val&15;     d.rr=o.rr; }
      if (m.mult.had) { o.mult=(m.mult.val>16)?16:m.mult.val; d.mult=o.mult; }
      if (m.rs.had)   { o.rs=m.rs.val&3;      d.rs=o.rs; }
      if (m.ksr.had)  { o.ksr=m.ksr.val;      d.ksr=o.ksr; }
      if (m.dam.had)  { o.delay=m.dam.val&7;  d.delay=o.delay; }
      if (m.ws.had)   { o.ws=(m.ws.val<0||m.ws.val>=YAM10_WAVES)?0:(unsigned char)m.ws.val; d.ws=o.ws; }
      if (m.dt.had)   { o.dtSemi=m.dt.val;    d.dtSemi=o.dtSemi; }
      if (m.dt2.had)  { o.fb=m.dt2.val&7;     d.fb=o.fb; }
      if (m.egt.had)  { o.outLvl=m.egt.val&127; d.outLvl=o.outLvl; }
      if (m.ksl.had)  { o.modIn=m.ksl.val&63; d.modIn=o.modIn; }
      if (m.am.had)   { o.pan=m.am.val&255;   d.pan=o.pan; }
      if (m.ssg.had)  { o.dtFine=m.ssg.val;   d.dtFine=o.dtFine; }
    }

    if (chan[i].freqChanged) {
      chan[i].freq=chan[i].calcFreq();
      // the pitch table is set up to yield Hz*256
      double hz=(double)chan[i].freq/256.0;
      if (hz<0.0) hz=0.0;
      chip.setFreq(i,hz);
      chan[i].freqChanged=false;
    }

    if (chan[i].keyOn) {
      applyInstrument(i);
      chip.keyOn(i);
      chan[i].keyOn=false;
      chan[i].active=true;
    }
    if (chan[i].keyOff) {
      chip.keyOff(i);
      chan[i].keyOff=false;
      chan[i].active=false;
    }
  }
}

int DivPlatformYAM10::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[c.chan].ins,DIV_INS_YAM10);
      chan[c.chan].state=ins->yam10;
      // the band an earlier AFxx picked must not steer the new note
      chan[c.chan].eqSel=0;
      if (c.value!=DIV_NOTE_NULL) {
        chan[c.chan].baseFreq=chan[c.chan].calcBaseFreq(c.value);
        chan[c.chan].note=c.value;
        chan[c.chan].freqChanged=true;
      }
      chan[c.chan].macroInit(ins);
      if (!chan[c.chan].std.vol.will) {
        chan[c.chan].outVol=chan[c.chan].vol;
      }
      chan[c.chan].keyOn=true;
      chan[c.chan].insChanged=false;
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[c.chan].keyOff=true;
      chan[c.chan].macroInit(NULL);
      break;
    case DIV_CMD_NOTE_OFF_ENV:
    case DIV_CMD_ENV_RELEASE:
      chan[c.chan].std.release();
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[c.chan].ins!=c.value || c.value2==1) {
        chan[c.chan].ins=c.value;
        chan[c.chan].insChanged=true;
      }
      break;
    case DIV_CMD_VOLUME:
      if (chan[c.chan].vol!=c.value) {
        chan[c.chan].vol=c.value;
        if (!chan[c.chan].std.vol.has) {
          chan[c.chan].outVol=c.value;
          for (int j=0; j<YAM10_OPS; j++) {
            if (chan[c.chan].state.op[j].outLvl>0) {
              chip.par[c.chan].op[j].outLvl=(isMuted[c.chan])?0:
                (unsigned char)((int)chan[c.chan].state.op[j].outLvl*c.value/127);
            }
          }
        }
      }
      break;
    case DIV_CMD_GET_VOLUME:
      return chan[c.chan].vol;
    case DIV_CMD_PITCH:
      chan[c.chan].pitch=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=chan[c.chan].calcBaseFreq(c.value2);
      // in linear mode baseFreq counts 1/128 semitones, so one effect unit is
      // already one step. the multiplier belongs to the raw frequency units
      // the non-linear table hands back, and applying it to both made every
      // slide run sixteen times too fast.
      int step=c.value*(parent->song.compatFlags.linearPitch?1:16);
      bool return2=false;
      if (destFreq>chan[c.chan].baseFreq) {
        chan[c.chan].baseFreq+=step;
        if (chan[c.chan].baseFreq>=destFreq) { chan[c.chan].baseFreq=destFreq; return2=true; }
      } else {
        chan[c.chan].baseFreq-=step;
        if (chan[c.chan].baseFreq<=destFreq) { chan[c.chan].baseFreq=destFreq; return2=true; }
      }
      chan[c.chan].freqChanged=true;
      if (return2) { chan[c.chan].inPorta=false; return 2; }
      break;
    }
    case DIV_CMD_LEGATO:
      chan[c.chan].baseFreq=chan[c.chan].calcBaseFreq(c.value+((HACKY_LEGATO_MESS)?(chan[c.chan].std.arp.val):(0)));
      chan[c.chan].note=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_PRE_PORTA:
      if (chan[c.chan].active && c.value2) {
        if (parent->song.compatFlags.resetMacroOnPorta) chan[c.chan].macroInit(parent->getIns(chan[c.chan].ins,DIV_INS_YAM10));
      }
      chan[c.chan].inPorta=c.value;
      break;
    // --- YAM10 specific: operator level / panning / feedback live edits ---
    // -1 as the operator means every operator
#define YAM10_OP_LOOP(field,expr) { \
      for (int o=0; o<YAM10_OPS; o++) { \
        if (c.value>=0 && o!=c.value) continue; \
        chan[c.chan].state.op[o].field=(expr); \
        chip.par[c.chan].op[o].field=chan[c.chan].state.op[o].field; \
      } \
    }
    case DIV_CMD_FM_TL:
      YAM10_OP_LOOP(tl,c.value2&127)
      break;
    case DIV_CMD_FM_AR:
      YAM10_OP_LOOP(ar,c.value2&31)
      break;
    case DIV_CMD_FM_FB:
      YAM10_OP_LOOP(fb,c.value2&7)
      break;
    case DIV_CMD_FM_DR:
      YAM10_OP_LOOP(dr,c.value2&31)
      break;
    case DIV_CMD_FM_D2R:
      YAM10_OP_LOOP(d2r,c.value2&31)
      break;
    case DIV_CMD_FM_SL:
      YAM10_OP_LOOP(sl,c.value2&15)
      break;
    case DIV_CMD_FM_RR:
      YAM10_OP_LOOP(rr,c.value2&15)
      break;
    case DIV_CMD_FM_MULT:
      YAM10_OP_LOOP(mult,(c.value2>16)?16:c.value2)
      break;
    case DIV_CMD_FM_RS:
      YAM10_OP_LOOP(rs,c.value2&3)
      break;
    case DIV_CMD_FM_KSR:
      YAM10_OP_LOOP(ksr,c.value2&1)
      break;
    case DIV_CMD_FM_WS:
      YAM10_OP_LOOP(ws,(c.value2>=YAM10_WAVES)?0:c.value2)
      break;
    case DIV_CMD_FM_DT:
      YAM10_OP_LOOP(dtSemi,(signed char)(c.value2-64))
      break;
    case DIV_CMD_FM_FINE:
      YAM10_OP_LOOP(dtFine,(signed char)(c.value2-64))
      break;
    case DIV_CMD_FM_OPMASK:
      for (int o=0; o<YAM10_OPS; o++) {
        chan[c.chan].state.op[o].enable=(c.value>>o)&1;
        chip.par[c.chan].op[o].enable=chan[c.chan].state.op[o].enable;
      }
      break;
    case DIV_CMD_YAM10_FILTER_CUTOFF:
      if (c.value>=0 && c.value<YAM10_FILTERS) {
        chan[c.chan].state.filter[c.value].cutoff=c.value2&255;
        chip.par[c.chan].filter[c.value].cutoff=c.value2&255;
      }
      break;
    case DIV_CMD_YAM10_FILTER_RES:
      if (c.value>=0 && c.value<YAM10_FILTERS) {
        chan[c.chan].state.filter[c.value].res=c.value2&255;
        chip.par[c.chan].filter[c.value].res=c.value2&255;
      }
      break;
    case DIV_CMD_YAM10_FILTER_MODE:
      if (c.value>=0 && c.value<YAM10_FILTERS) {
        chan[c.chan].state.filter[c.value].mode=c.value2&3;
        chip.par[c.chan].filter[c.value].mode=c.value2&3;
      }
      break;
    case DIV_CMD_YAM10_FILTER_ENABLE:
      if (c.value>=0 && c.value<YAM10_FILTERS) {
        chan[c.chan].state.filter[c.value].enable=c.value2&1;
        chip.par[c.chan].filter[c.value].enable=c.value2&1;
      }
      break;
    case DIV_CMD_YAM10_DIST_GAIN:
      chan[c.chan].state.distEnable=true;
      chan[c.chan].state.distGain=c.value&127;
      chip.par[c.chan].distGain=c.value&127;
      chip.par[c.chan].distEnable=true;
      break;
    case DIV_CMD_YAM10_DIST_LEVEL:
      chan[c.chan].state.distLevel=c.value&127;
      chip.par[c.chan].distLevel=c.value&127;
      break;
    case DIV_CMD_YAM10_CHORUS_MIX:
      chan[c.chan].state.chorusEnable=true;
      chan[c.chan].state.chorusMix=c.value&127;
      chip.par[c.chan].chorusMix=c.value&127;
      chip.par[c.chan].chorusEnable=(c.value&127)>0;
      break;
    case DIV_CMD_YAM10_CHORUS_RATE:
      chan[c.chan].state.chorusRate=c.value&127;
      chip.par[c.chan].chorusRate=c.value&127;
      break;
    case DIV_CMD_YAM10_CHORUS_DEPTH:
      chan[c.chan].state.chorusDepth=c.value&127;
      chip.par[c.chan].chorusDepth=c.value&127;
      break;
    case DIV_CMD_YAM10_ECHO_MIX:
      chan[c.chan].state.echoMix=c.value&127;
      chip.par[c.chan].echoMix=c.value&127;
      break;
    case DIV_CMD_YAM10_ECHO_FB:
      chan[c.chan].state.echoFeedback=c.value&127;
      chip.par[c.chan].echoFeedback=c.value&127;
      break;
    case DIV_CMD_YAM10_REVERB_MIX:
      chan[c.chan].state.reverbMix=c.value&127;
      chip.par[c.chan].reverbMix=chan[c.chan].state.reverbMix;
      break;
    case DIV_CMD_YAM10_REVERB_SEND:
      chan[c.chan].state.reverbSend=c.value&127;
      chip.par[c.chan].reverbSend=chan[c.chan].state.reverbSend;
      break;
    case DIV_CMD_YAM10_REVERB_DECAY:
      chan[c.chan].state.reverbDecay=c.value&127;
      chip.par[c.chan].reverbDecay=chan[c.chan].state.reverbDecay;
      break;
    case DIV_CMD_YAM10_REVERB_EARLY:
      chan[c.chan].state.reverbEarly=c.value&127;
      chip.par[c.chan].reverbEarly=chan[c.chan].state.reverbEarly;
      break;
    case DIV_CMD_YAM10_REVERB_DIFF:
      chan[c.chan].state.reverbDiffusion=c.value&127;
      chip.par[c.chan].reverbDiffusion=chan[c.chan].state.reverbDiffusion;
      break;
    case DIV_CMD_YAM10_REVERB_SIZE:
      chan[c.chan].state.reverbSize=c.value&127;
      chip.par[c.chan].reverbSize=chan[c.chan].state.reverbSize;
      break;
    case DIV_CMD_YAM10_REVERB_DAMP:
      chan[c.chan].state.reverbDamp=c.value&127;
      chip.par[c.chan].reverbDamp=chan[c.chan].state.reverbDamp;
      break;
    case DIV_CMD_YAM10_REVERB_EN:
      chan[c.chan].state.reverbEnable=c.value&1;
      chip.par[c.chan].reverbEnable=chan[c.chan].state.reverbEnable;
      break;
    case DIV_CMD_YAM10_COMP_THR:
      chan[c.chan].state.compThreshold=c.value&127;
      chip.par[c.chan].compThreshold=chan[c.chan].state.compThreshold;
      break;
    case DIV_CMD_YAM10_COMP_DOWN:
      chan[c.chan].state.compRatio=c.value&127;
      chip.par[c.chan].compRatio=chan[c.chan].state.compRatio;
      break;
    case DIV_CMD_YAM10_COMP_UP:
      chan[c.chan].state.compUpRatio=c.value&127;
      chip.par[c.chan].compUpRatio=chan[c.chan].state.compUpRatio;
      break;
    case DIV_CMD_YAM10_COMP_ATK:
      chan[c.chan].state.compAttack=c.value&127;
      chip.par[c.chan].compAttack=chan[c.chan].state.compAttack;
      break;
    case DIV_CMD_YAM10_COMP_DEC:
      chan[c.chan].state.compDecay=c.value&127;
      chip.par[c.chan].compDecay=chan[c.chan].state.compDecay;
      break;
    case DIV_CMD_YAM10_COMP_XLO:
      chan[c.chan].state.compLoMid=c.value&255;
      chip.par[c.chan].compLoMid=chan[c.chan].state.compLoMid;
      break;
    case DIV_CMD_YAM10_COMP_XHI:
      chan[c.chan].state.compMidHi=c.value&255;
      chip.par[c.chan].compMidHi=chan[c.chan].state.compMidHi;
      break;
    case DIV_CMD_YAM10_COMP_GLO:
      chan[c.chan].state.compLowGain=c.value&255;
      chip.par[c.chan].compLowGain=chan[c.chan].state.compLowGain;
      break;
    case DIV_CMD_YAM10_COMP_GMID:
      chan[c.chan].state.compMidGain=c.value&255;
      chip.par[c.chan].compMidGain=chan[c.chan].state.compMidGain;
      break;
    case DIV_CMD_YAM10_COMP_GHI:
      chan[c.chan].state.compHighGain=c.value&255;
      chip.par[c.chan].compHighGain=chan[c.chan].state.compHighGain;
      break;
    case DIV_CMD_YAM10_COMP_OUT:
      chan[c.chan].state.compMakeup=c.value&127;
      chip.par[c.chan].compMakeup=chan[c.chan].state.compMakeup;
      break;
    case DIV_CMD_YAM10_COMP_EN:
      chan[c.chan].state.compEnable=c.value&1;
      chip.par[c.chan].compEnable=chan[c.chan].state.compEnable;
      break;
    case DIV_CMD_YAM10_EQ_EN:
      chan[c.chan].state.eqEnable=c.value&1;
      chip.par[c.chan].eqEnable=chan[c.chan].state.eqEnable;
      break;
    case DIV_CMD_YAM10_PHASE_INV:
      chan[c.chan].state.phaseInvL=c.value&1;
      chan[c.chan].state.phaseInvR=(c.value>>1)&1;
      chip.par[c.chan].phaseInvL=chan[c.chan].state.phaseInvL;
      chip.par[c.chan].phaseInvR=chan[c.chan].state.phaseInvR;
      break;
    case DIV_CMD_YAM10_EQ_SEL:
      chan[c.chan].eqSel=c.value&7;
      break;
    case DIV_CMD_YAM10_EQ_FREQ:
      chan[c.chan].state.eqBand[chan[c.chan].eqSel].freq=c.value&255;
      chip.par[c.chan].eqBand[chan[c.chan].eqSel].freq=c.value&255;
      break;
    case DIV_CMD_YAM10_EQ_GAIN:
      chan[c.chan].state.eqBand[chan[c.chan].eqSel].gain=c.value&255;
      chip.par[c.chan].eqBand[chan[c.chan].eqSel].gain=c.value&255;
      break;
    case DIV_CMD_YAM10_EQ_Q:
      chan[c.chan].state.eqBand[chan[c.chan].eqSel].q=c.value&255;
      chip.par[c.chan].eqBand[chan[c.chan].eqSel].q=c.value&255;
      break;
    case DIV_CMD_YAM10_EQ_TYPE:
      chan[c.chan].state.eqBand[chan[c.chan].eqSel].type=(c.value>5)?5:c.value;
      chip.par[c.chan].eqBand[chan[c.chan].eqSel].type=chan[c.chan].state.eqBand[chan[c.chan].eqSel].type;
      break;
    case DIV_CMD_YAM10_DIST_CUTOFF:
      chan[c.chan].state.distCutoff=c.value&255;
      chip.par[c.chan].distCutoff=chan[c.chan].state.distCutoff;
      break;
    case DIV_CMD_YAM10_DIST_EN:
      chan[c.chan].state.distEnable=c.value&1;
      chip.par[c.chan].distEnable=chan[c.chan].state.distEnable;
      break;
    case DIV_CMD_YAM10_CHORUS_EN:
      chan[c.chan].state.chorusEnable=c.value&1;
      chip.par[c.chan].chorusEnable=chan[c.chan].state.chorusEnable;
      break;
    case DIV_CMD_YAM10_CHORUS_FB:
      chan[c.chan].state.chorusFeedback=c.value&127;
      chip.par[c.chan].chorusFeedback=chan[c.chan].state.chorusFeedback;
      break;
    case DIV_CMD_YAM10_CHORUS_WIDTH:
      chan[c.chan].state.chorusWidth=c.value&255;
      chip.par[c.chan].chorusWidth=chan[c.chan].state.chorusWidth;
      break;
    case DIV_CMD_YAM10_ECHO_DELAY:
      // four milliseconds a step, so one byte covers the whole range
      chan[c.chan].state.echoDelay=(unsigned short)((c.value&255)*4+1);
      chip.par[c.chan].echoDelay=chan[c.chan].state.echoDelay;
      break;
    case DIV_CMD_YAM10_OP_OUTLVL:
      YAM10_OP_LOOP(outLvl,c.value2&127)
      break;
    case DIV_CMD_YAM10_OP_PAN:
      YAM10_OP_LOOP(pan,c.value2&255)
      break;
    case DIV_CMD_YAM10_OP_MODIN:
      YAM10_OP_LOOP(modIn,c.value2&63)
      break;
    case DIV_CMD_YAM10_OP_PHRESET:
      // the chip calls this one something else, so it cannot use the macro
      for (int o=0; o<YAM10_OPS; o++) {
        if (c.value>=0 && o!=c.value) continue;
        chan[c.chan].state.op[o].phaseReset=(unsigned short)(c.value2&255);
        chip.par[c.chan].op[o].phaseResetPeriod=chan[c.chan].state.op[o].phaseReset;
      }
      break;
    case DIV_CMD_YAM10_OP_DELAY:
      YAM10_OP_LOOP(delay,c.value2&7)
      break;
    case DIV_CMD_YAM10_WS_HI:
      YAM10_OP_LOOP(ws,(unsigned char)(16+(c.value2&7)))
      break;
    case DIV_CMD_YAM10_WS_SEL:
      // 5Dxx picks who 5Exx talks to. one digit cannot carry a waveform
      // number any more, so the operator and the waveform take an effect each
      chan[c.chan].wsSel=(c.value>YAM10_OPS)?0:(unsigned char)c.value;
      break;
    case DIV_CMD_YAM10_WS_FULL: {
      int sel=(int)chan[c.chan].wsSel-1;
      unsigned char v=(c.value<0||c.value>=YAM10_WAVES)?0:(unsigned char)c.value;
      for (int o=0; o<YAM10_OPS; o++) {
        if (sel>=0 && o!=sel) continue;
        chan[c.chan].state.op[o].ws=v;
        chip.par[c.chan].op[o].ws=v;
      }
      break;
    }
    case DIV_CMD_YAM10_EQ_ON:
      chan[c.chan].state.eqBand[chan[c.chan].eqSel].on=c.value&1;
      chip.par[c.chan].eqBand[chan[c.chan].eqSel].on=chan[c.chan].state.eqBand[chan[c.chan].eqSel].on;
      // a band the pattern reaches for has to exist for the chip to run it
      if (chan[c.chan].eqSel>=chan[c.chan].state.eqCount) {
        chan[c.chan].state.eqCount=chan[c.chan].eqSel+1;
        chip.par[c.chan].eqCount=chan[c.chan].state.eqCount;
      }
      break;
#undef YAM10_OP_LOOP
    case DIV_CMD_PANNING: {
      // the two sides arrive as levels from 0 to 255, not as a position, so
      // the position is the difference between them. both at zero means the
      // channel is off either side, which this chip cannot do, so it centres.
      int pl=c.value&0xff;
      int pr=c.value2&0xff;
      int pp=128;
      if (pl!=0 || pr!=0) pp=128+((pr-pl)>>1);
      pp=CLAMP(pp,0,255);
      // written to the channel as well, or the next note puts it back
      for (int j=0; j<YAM10_OPS; j++) {
        chan[c.chan].state.op[j].pan=pp;
        chip.par[c.chan].op[j].pan=pp;
      }
      break;
    }
    case DIV_CMD_GET_VOLMAX:
      return 127;
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

void DivPlatformYAM10::muteChannel(int ch, bool mute) {
  isMuted[ch]=mute;
  yam10BuildVolCurve();
  for (int j=0; j<YAM10_OPS; j++) {
    // come back at the volume the channel is actually playing at, not the
    // instrument's own level
    chip.par[ch].op[j].outLvl=mute?0:
      (unsigned char)((int)chan[ch].state.op[j].outLvl*yam10VolCurve[chan[ch].outVol&127]/127);
  }
}

void DivPlatformYAM10::forceIns() {
  for (int i=0; i<YAM10_CHANS; i++) {
    applyInstrument(i);
    chan[i].freqChanged=true;
  }
}

SharedChannel* DivPlatformYAM10::getChanState(int ch) { return &chan[ch]; }
DivDispatchOscBuffer* DivPlatformYAM10::getOscBuffer(int ch) { return oscBuf[ch]; }
unsigned char* DivPlatformYAM10::getRegisterPool() { return regPool; }
int DivPlatformYAM10::getRegisterPoolSize() { return 256; }
int DivPlatformYAM10::getOutputCount() { return 2; }

void DivPlatformYAM10::reset() {
  memset(regPool,0,256);
  chip.init(rate);
  for (int i=0; i<YAM10_CHANS; i++) {
    chan[i]=DivPlatformYAM10::Channel(parent->song.compatFlags.linearPitch);
    chan[i].std.setEngine(parent);
    chan[i].pitchTable=&pitchTable;
    chip.par[i]=YAM10ChanParam();
  }
}

void DivPlatformYAM10::notifyInsDeletion(void* ins) {
  // without this the macro interpreter keeps reading the freed instrument
  for (int i=0; i<YAM10_CHANS; i++) {
    chan[i].std.notifyInsDeletion((DivInstrument*)ins);
  }
}

void DivPlatformYAM10::notifyInsChange(int ins) {
  for (int i=0; i<YAM10_CHANS; i++) {
    if (chan[i].ins==ins) chan[i].insChanged=true;
  }
}

void DivPlatformYAM10::notifyWaveChange(int wave) {
  // custom wavetables are read by pointer, so just re-point them
  for (int i=0; i<YAM10_CHANS; i++) applyInstrument(i);
}

void DivPlatformYAM10::setFlags(const DivConfig& flags) {
  chipClock=1000000;
  CHECK_CUSTOM_CLOCK;
  rate=chipClock/16;
  // the table's frequency output already carries a built in 32x, so it is
  // 32*Hz*(divider/clock). divider=clock*8 lands on Hz*256, which is what the
  // freqChanged block below divides by. clock*256 made every note five
  // octaves sharp.
  notifyPitchTable();
  for (int i=0; i<YAM10_CHANS; i++) {
    if (oscBuf[i]!=NULL) oscBuf[i]->setRate(rate);
  }
  chip.init(rate);
}

// the engine calls this when the song's tuning or pitch linearity changes
void DivPlatformYAM10::notifyPitchTable(int sample) {
  pitchTable.init(parent->song.tuning,chipClock,(double)chipClock*8.0,0xffffff,false,parent->song.compatFlags.linearPitch);
}

void DivPlatformYAM10::poke(unsigned int addr, unsigned short val) {}
void DivPlatformYAM10::poke(std::vector<DivRegWrite>& wlist) {}

int DivPlatformYAM10::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent=p;
  dumpWrites=false;
  skipRegisterWrites=false;
  yam10BuildVolCurve();
  for (int i=0; i<YAM10_CHANS; i++) {
    isMuted[i]=false;
    oscBuf[i]=new DivDispatchOscBuffer;
  }
  setFlags(flags);
  reset();
  return YAM10_CHANS;
}

void DivPlatformYAM10::quit() {
  for (int i=0; i<YAM10_CHANS; i++) {
    if (oscBuf[i]!=NULL) { delete oscBuf[i]; oscBuf[i]=NULL; }
  }
}

DivPlatformYAM10::~DivPlatformYAM10() {}
