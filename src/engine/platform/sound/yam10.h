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

/**
 * YAM10 - a fictional 10-channel, 6-operator FM chip, by rednoobmusic.
 *
 * Synthesis is done the way the real Yamaha parts do it rather than in
 * floating point: a quarter-wave log-sine ROM, a 256-entry exponential
 * ROM, 10-bit attenuation and fixed-point phase. That is what gives FM
 * chips their particular grain, and it means an operator here behaves
 * like one on hardware.
 *
 * Borrowed rather than reinvented:
 *   - logsin/exp ROMs and the waveform derivation from Nuked-OPL3
 *     (extern/opl/opl3.c, already vendored here)
 *   - OPN envelope model: rate = 2*reg + key scale value, ADSR plus a
 *     second decay rate (D2R) that keeps running past the sustain point
 *   - OPL/OPZ waveform set
 *   - YM2609 style free routing: each operator has a modulation input
 *     mask, which also gives feedback on any operator for free
 *
 * What no real part does: six operators with free routing between them,
 * a delay before any operator attacks, per-operator fine panning, custom
 * wavetables of any length, fixed operator pitch all the way to 0 Hz, and
 * a DSP chain on every channel rather than one for the whole chip.
 */

#ifndef _YAM10_CHIP_H
#define _YAM10_CHIP_H

#include <math.h>
#include <string.h>
#include <stdint.h>

/* MSVC only hands out M_PI and friends when _USE_MATH_DEFINES was set before
   math.h, and this header is pulled into translation units that do not set
   it, so carry our own rather than depend on include order. */
#define YAM10_PI 3.14159265358979323846
#define YAM10_SQRT2 1.41421356237309504880

#define YAM10_CHANS 10
#define YAM10_OPS 6
/* how many waveforms the chip carries. the wavetable is not one of them, it
   sits one past the end so a waveform can be added without moving it. */
#define YAM10_WAVES 79
#define YAM10_WF_NOISE  21
#define YAM10_WF_NOISE1 22
#define YAM10_WF_SH     23
/* the duty is a per operator setting, so this one is worked out as it plays
   rather than being a fixed row */
#define YAM10_WF_PULSE  37
/* not a row in the bank: the oscillator reads a wavetable instead */
#define YAM10_WAVE_WT 0x7f
#define YAM10_MAX_ECHO 48000
#define YAM10_FILTERS 3
#define YAM10_EQ_BANDS 32
#define YAM10_MAX_CHORUS 4096
/* a small Schroeder tank per channel: two combs and an allpass */
#define YAM10_REV_C1 1447
#define YAM10_REV_C2 1723
#define YAM10_REV_AP 233
#define YAM10_REV_AP2 149
#define YAM10_REV_AP3 97
#define YAM10_REV_EARLY 2048
#define YAM10_EARLY_TAPS 8

/* ---- ROMs from Nuked-OPL3 (see extern/opl/opl3.c) ---- */
static const uint16_t yam10_logsinrom[256] = {
    0x859, 0x6c3, 0x607, 0x58b, 0x52e, 0x4e4, 0x4a6, 0x471,
    0x443, 0x41a, 0x3f5, 0x3d3, 0x3b5, 0x398, 0x37e, 0x365,
    0x34e, 0x339, 0x324, 0x311, 0x2ff, 0x2ed, 0x2dc, 0x2cd,
    0x2bd, 0x2af, 0x2a0, 0x293, 0x286, 0x279, 0x26d, 0x261,
    0x256, 0x24b, 0x240, 0x236, 0x22c, 0x222, 0x218, 0x20f,
    0x206, 0x1fd, 0x1f5, 0x1ec, 0x1e4, 0x1dc, 0x1d4, 0x1cd,
    0x1c5, 0x1be, 0x1b7, 0x1b0, 0x1a9, 0x1a2, 0x19b, 0x195,
    0x18f, 0x188, 0x182, 0x17c, 0x177, 0x171, 0x16b, 0x166,
    0x160, 0x15b, 0x155, 0x150, 0x14b, 0x146, 0x141, 0x13c,
    0x137, 0x133, 0x12e, 0x129, 0x125, 0x121, 0x11c, 0x118,
    0x114, 0x10f, 0x10b, 0x107, 0x103, 0x0ff, 0x0fb, 0x0f8,
    0x0f4, 0x0f0, 0x0ec, 0x0e9, 0x0e5, 0x0e2, 0x0de, 0x0db,
    0x0d7, 0x0d4, 0x0d1, 0x0cd, 0x0ca, 0x0c7, 0x0c4, 0x0c1,
    0x0be, 0x0bb, 0x0b8, 0x0b5, 0x0b2, 0x0af, 0x0ac, 0x0a9,
    0x0a7, 0x0a4, 0x0a1, 0x09f, 0x09c, 0x099, 0x097, 0x094,
    0x092, 0x08f, 0x08d, 0x08a, 0x088, 0x086, 0x083, 0x081,
    0x07f, 0x07d, 0x07a, 0x078, 0x076, 0x074, 0x072, 0x070,
    0x06e, 0x06c, 0x06a, 0x068, 0x066, 0x064, 0x062, 0x060,
    0x05e, 0x05c, 0x05b, 0x059, 0x057, 0x055, 0x053, 0x052,
    0x050, 0x04e, 0x04d, 0x04b, 0x04a, 0x048, 0x046, 0x045,
    0x043, 0x042, 0x040, 0x03f, 0x03e, 0x03c, 0x03b, 0x039,
    0x038, 0x037, 0x035, 0x034, 0x033, 0x031, 0x030, 0x02f,
    0x02e, 0x02d, 0x02b, 0x02a, 0x029, 0x028, 0x027, 0x026,
    0x025, 0x024, 0x023, 0x022, 0x021, 0x020, 0x01f, 0x01e,
    0x01d, 0x01c, 0x01b, 0x01a, 0x019, 0x018, 0x017, 0x017,
    0x016, 0x015, 0x014, 0x014, 0x013, 0x012, 0x011, 0x011,
    0x010, 0x00f, 0x00f, 0x00e, 0x00d, 0x00d, 0x00c, 0x00c,
    0x00b, 0x00a, 0x00a, 0x009, 0x009, 0x008, 0x008, 0x007,
    0x007, 0x007, 0x006, 0x006, 0x005, 0x005, 0x005, 0x004,
    0x004, 0x004, 0x003, 0x003, 0x003, 0x002, 0x002, 0x002,
    0x002, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};

static const uint16_t yam10_exprom[256] = {
    0xff4, 0xfea, 0xfde, 0xfd4, 0xfc8, 0xfbe, 0xfb4, 0xfa8,
    0xf9e, 0xf92, 0xf88, 0xf7e, 0xf72, 0xf68, 0xf5c, 0xf52,
    0xf48, 0xf3e, 0xf32, 0xf28, 0xf1e, 0xf14, 0xf08, 0xefe,
    0xef4, 0xeea, 0xee0, 0xed4, 0xeca, 0xec0, 0xeb6, 0xeac,
    0xea2, 0xe98, 0xe8e, 0xe84, 0xe7a, 0xe70, 0xe66, 0xe5c,
    0xe52, 0xe48, 0xe3e, 0xe34, 0xe2a, 0xe20, 0xe16, 0xe0c,
    0xe04, 0xdfa, 0xdf0, 0xde6, 0xddc, 0xdd2, 0xdca, 0xdc0,
    0xdb6, 0xdac, 0xda4, 0xd9a, 0xd90, 0xd88, 0xd7e, 0xd74,
    0xd6a, 0xd62, 0xd58, 0xd50, 0xd46, 0xd3c, 0xd34, 0xd2a,
    0xd22, 0xd18, 0xd10, 0xd06, 0xcfe, 0xcf4, 0xcec, 0xce2,
    0xcda, 0xcd0, 0xcc8, 0xcbe, 0xcb6, 0xcae, 0xca4, 0xc9c,
    0xc92, 0xc8a, 0xc82, 0xc78, 0xc70, 0xc68, 0xc60, 0xc56,
    0xc4e, 0xc46, 0xc3c, 0xc34, 0xc2c, 0xc24, 0xc1c, 0xc12,
    0xc0a, 0xc02, 0xbfa, 0xbf2, 0xbea, 0xbe0, 0xbd8, 0xbd0,
    0xbc8, 0xbc0, 0xbb8, 0xbb0, 0xba8, 0xba0, 0xb98, 0xb90,
    0xb88, 0xb80, 0xb78, 0xb70, 0xb68, 0xb60, 0xb58, 0xb50,
    0xb48, 0xb40, 0xb38, 0xb32, 0xb2a, 0xb22, 0xb1a, 0xb12,
    0xb0a, 0xb02, 0xafc, 0xaf4, 0xaec, 0xae4, 0xade, 0xad6,
    0xace, 0xac6, 0xac0, 0xab8, 0xab0, 0xaa8, 0xaa2, 0xa9a,
    0xa92, 0xa8c, 0xa84, 0xa7c, 0xa76, 0xa6e, 0xa68, 0xa60,
    0xa58, 0xa52, 0xa4a, 0xa44, 0xa3c, 0xa36, 0xa2e, 0xa28,
    0xa20, 0xa18, 0xa12, 0xa0c, 0xa04, 0x9fe, 0x9f6, 0x9f0,
    0x9e8, 0x9e2, 0x9da, 0x9d4, 0x9ce, 0x9c6, 0x9c0, 0x9b8,
    0x9b2, 0x9ac, 0x9a4, 0x99e, 0x998, 0x990, 0x98a, 0x984,
    0x97c, 0x976, 0x970, 0x96a, 0x962, 0x95c, 0x956, 0x950,
    0x948, 0x942, 0x93c, 0x936, 0x930, 0x928, 0x922, 0x91c,
    0x916, 0x910, 0x90a, 0x904, 0x8fc, 0x8f6, 0x8f0, 0x8ea,
    0x8e4, 0x8de, 0x8d8, 0x8d2, 0x8cc, 0x8c6, 0x8c0, 0x8ba,
    0x8b4, 0x8ae, 0x8a8, 0x8a2, 0x89c, 0x896, 0x890, 0x88a,
    0x884, 0x87e, 0x878, 0x872, 0x86c, 0x866, 0x860, 0x85a,
    0x854, 0x850, 0x84a, 0x844, 0x83e, 0x838, 0x832, 0x82c,
    0x828, 0x822, 0x81c, 0x816, 0x810, 0x80c, 0x806, 0x800,
};


/* waveform bank, built once: [wave][1024] of log-domain magnitude.
 * bit 15 of an entry means "silent", matching the OPL convention. */
static uint16_t yam10_wf[YAM10_WAVES+1][1024];

static uint8_t yam10_wf_built=0;

static inline uint16_t yam10_logsin(uint32_t p) {
  /* full-cycle log sine from the quarter-wave ROM; bit15 = negative */
  uint16_t v=(p&0x100)?yam10_logsinrom[(p&0xff)^0xff]:yam10_logsinrom[p&0xff];
  return v|((p&0x200)?0x8000:0);
}

/* triangle in the log domain, bit 15 negative, matching yam10_logsin */
static inline unsigned short yam10_tri(unsigned int p) {
  int t=(int)(p&0x3ff);
  int h=t&0x1ff;
  int lin=511-((h<256)?((255-h)*2):((h-256)*2));
  if (lin<1) lin=1;
  unsigned short att=(unsigned short)(-(int)(log2((double)lin/511.0)*256.0));
  return att|((t<512)?0:0x8000);
}

/* the half of the ROM the even-period waves read. opl3 mirrors the quarter
   and then doubles; doubling first and mirroring after lands one ROM entry
   away, so this keeps the hardware's order. */
static inline uint16_t yam10_logsin_even(uint32_t p) {
  return (p&0x80)?yam10_logsinrom[((p^0xff)<<1)&0xff]:yam10_logsinrom[(p<<1)&0xff];
}

/* a waveform is either read from the bank above or generated as it plays.
   keeping that in a table rather than in the number itself means a new
   waveform of either sort can take the next free number, and a song saved
   before it was added still means what it meant. */
enum YAM10WaveKind {
  YAM10_WK_TABLE=0,   /* read from yam10_wf */
  YAM10_WK_NOISE,     /* the long register, full depth */
  YAM10_WK_NOISE1,    /* the long register, one bit deep */
  YAM10_WK_SH,        /* one step a cycle, so a much coarser noise */
  YAM10_WK_POLY,      /* a short register, so the pattern repeats and pitches */
  YAM10_WK_WAVETABLE,
  YAM10_WK_PULSE      /* square at whatever duty the operator asks for */
};

/* width is how many bits the register holds and taps is the xor feedback
   mask. a four bit register comes back around every fifteen steps and a five
   bit one every thirty one, which is heard as a rough pitch rather than as
   noise. */
struct YAM10WaveDef {
  uint8_t kind;
  uint8_t polyWidth;
  uint16_t polyPeriod;
  uint8_t polyShift;   /* pattern spans 1<<polyShift cycles of the note */
  uint8_t polyIndex;   /* which pattern in yam10_poly_bits */
};

/* filled by yam10_build_wf. almost every waveform reads the bank, so the
   exceptions are listed rather than the rule. */
static YAM10WaveDef yam10_wave_def[YAM10_WAVES];

/* how a pattern is generated. most are a short shift register, shifting the
   same way as the long one above. the 2600 is not a shift register at all:
   it runs a four bit pulse counter and a five bit noise counter that feed
   each other, and AUDC picks which feeds which. that is where its particular
   set of tones comes from. */
enum YAM10PolyRule {
  YAM10_PR_LFSR=0,
  YAM10_PR_TIA        /* taps holds the AUDC value */
};

struct YAM10PolyDef {
  uint8_t ws;
  uint8_t width;
  uint16_t taps;
  uint8_t shiftLeft; /* POKEY runs its registers the other way from the rest */
  uint16_t period;   /* steps before the pattern comes back around */
  uint8_t rule;
  uint8_t invertOut; /* the NES gates its channel off when the bit is set */
};

/* the first four are plain registers picked for their length. everything
   after them is the real thing, taken from the hardware each is named after:
   the 2600 from src/engine/platform/sound/tia/AudioChannel.cpp, POKEY from
   src/engine/platform/sound/pokey/mzpokeysnd.c, and the NES short mode.
   POKEY shifts left where the others shift right, which is why the direction
   is a field rather than an assumption. */
static const YAM10PolyDef yam10_polys[]={
  {67,3,0x005,0,7,YAM10_PR_LFSR,0},
  {68,4,0x009,0,15,YAM10_PR_LFSR,0},
  {69,6,0x021,0,63,YAM10_PR_LFSR,0},
  {70,7,0x041,0,127,YAM10_PR_LFSR,0},
  {71,4,0x00c,1,15,YAM10_PR_LFSR,0},   /* POKEY's four bit poly */
  {72,5,0x005,0,31,YAM10_PR_LFSR,0},   /* the 2600's noise counter on its own */
  {73,9,0x108,1,511,YAM10_PR_LFSR,0},  /* POKEY's nine bit poly */
  {74,15,0x041,0,93,YAM10_PR_LFSR,1},  /* the NES in its short mode */
  /* the 2600's own tones, by AUDC. only the ones that are really the 2600's
     are here: its four divider settings come out as plain pulses, which is
     what waveform 37 already is, and its four bit counter turns out to run
     the same bits as POKEY's. */
  {75,0,15,0,93,YAM10_PR_TIA,0},       /* five bit poly into div 6 */
  {76,0,2,0,465,YAM10_PR_TIA,0},       /* div 15 into the four bit poly */
  {77,0,3,0,465,YAM10_PR_TIA,0},       /* five bit poly into the four bit poly */
  {78,0,8,0,511,YAM10_PR_TIA,0}        /* the nine bit white noise */
};
#define YAM10_POLY_COUNT ((int)(sizeof(yam10_polys)/sizeof(yam10_polys[0])))
#define YAM10_POLY_MAX 511

/* each pattern is built once and then read. keeping the generator out of the
   sample loop means a rule that is not a plain shift register can be exact
   rather than approximated, and costs nothing per sample. */
static uint8_t yam10_poly_bits[YAM10_POLY_COUNT][YAM10_POLY_MAX];

/* the falling saw, the jump at the half cycle. bit 15 is the sign, the same
   as yam10_logsin. waves 17 and 18 were doing this inline. */
static inline uint16_t yam10_saw(uint32_t p) {
  uint32_t q=(p+512)&0x3ff;
  uint16_t lin=(q<512)?(uint16_t)(511-q):(uint16_t)(q-512);
  uint16_t att=(lin==0)?0x1000:(uint16_t)(-(int)(log2((double)lin/511.0)*256.0));
  return att|((q<512)?0:0x8000);
}

/* the saw folded up rather than mirrored, so a unipolar ramp runs twice a
   cycle. taking the plain absolute value of a saw only gives a triangle,
   which the chip already has at wave 7. */
static inline uint16_t yam10_saw_abs(uint32_t p) {
  uint16_t lin=(uint16_t)(511-(p&0x1ff));
  return (lin==0)?0x1000:(uint16_t)(-(int)(log2((double)lin/511.0)*256.0));
}

/* raising the linear amplitude to the nth power is multiplying the log
   domain value by n. the sign is kept, so an odd power keeps its negative
   lobe, and anything that overflows past silence stays silent. n=2 is what
   wave 12 always did. */
static inline uint16_t yam10_pow(uint16_t e, unsigned int n) {
  uint16_t mag=e&0x7fff, neg=e&0x8000;
  uint32_t v=(uint32_t)mag*n;
  return (uint16_t)(((mag>=0x1000)||(v>0xfff))?0x1000:v)|neg;
}

static void yam10_build_wf(void) {
  uint32_t p;
  if (yam10_wf_built) return;
  for (p=0; p<1024; p++) {
    uint16_t s=yam10_logsin(p);
    uint16_t mag=s&0x7fff;
    uint16_t neg=s&0x8000;

    /* 0-6: the sines */
    yam10_wf[0][p]=s;                                   /* sine */
    yam10_wf[1][p]=(p&0x200)?0x1000:s;                  /* half sine */
    yam10_wf[2][p]=mag;                                 /* absolute sine */
    yam10_wf[3][p]=(p&0x100)?0x1000:mag;                /* pulse sine */
    yam10_wf[4][p]=(p&0x200)?0x1000:((((p&0x300)==0x100)?0x8000:0)|yam10_logsin_even(p));
    yam10_wf[5][p]=(p&0x200)?0x1000:yam10_logsin_even(p);
    yam10_wf[6][p]=(p&0x300)?0x1000:yam10_logsin(p*4);  /* quarter squished */

    /* 7-11: the triangles */
    {
      unsigned short tri=yam10_tri(p);
      yam10_wf[7][p]=tri;                               /* triangle */
      yam10_wf[8][p]=tri&0x7fff;                        /* absolute */
      yam10_wf[9][p]=(p&0x200)?0x1000:tri;              /* half */
      yam10_wf[10][p]=(p&0x200)?0x1000:yam10_tri(p*2);  /* squished */
      yam10_wf[11][p]=(p&0x200)?0x1000:(yam10_tri(p*2)&0x7fff);
    }

    /* 12: doubling the log domain value squares the amplitude. the sign is
       kept, so this is sin times the absolute value of sin, not sin squared
       on its own, which would never go negative. slot 13 is the true one.
       13-16 come from this in the second pass. */
    yam10_wf[12][p]=yam10_pow(s,2);

    /* 17-18: the saws */
    yam10_wf[17][p]=yam10_saw(p);
    yam10_wf[18][p]=(p&0x200)?0x1000:yam10_wf[17][p];

    /* 19-20: the squares */
    yam10_wf[19][p]=(p&0x200)?0x8000:0;
    /* an exponential decay either side of zero, so a saw rather than
       anything square */
    yam10_wf[20][p]=neg?(((((p&0x1ff)^0x1ff))<<3)|0x8000):((p&0x1ff)<<3);

    /* 21-23 are generated as they play, so they have no bank entry */
    yam10_wf[21][p]=0;
    yam10_wf[22][p]=0;
    yam10_wf[23][p]=0;

    /* 24-25: the two the triangle family was missing */
    yam10_wf[24][p]=(p&0x100)?0x1000:(yam10_tri(p)&0x7fff);   /* pulse */
    yam10_wf[25][p]=(p&0x300)?0x1000:yam10_tri(p*4);          /* quarter squished */

    /* 26: pulse squared sine. the quarter squished one reads wave 12 at four
       times the phase, so it waits for the second pass. */
    yam10_wf[26][p]=(p&0x100)?0x1000:(yam10_pow(s,2)&0x7fff);

    /* 28-32: the rest of the saw family */
    yam10_wf[28][p]=yam10_saw_abs(p);                         /* absolute */
    yam10_wf[29][p]=(p&0x100)?0x1000:yam10_saw_abs(p);        /* pulse */
    yam10_wf[30][p]=(p&0x200)?0x1000:yam10_saw(p*2);          /* squished */
    yam10_wf[31][p]=(p&0x200)?0x1000:yam10_saw_abs(p*2);      /* squished abs */
    yam10_wf[32][p]=(p&0x300)?0x1000:yam10_saw(p*4);          /* quarter squished */

    /* 33-36: the square gated the same ways as every other family. an
       absolute square is only DC and a squished absolute square repeats 33,
       so neither gets a number. */
    yam10_wf[33][p]=(p&0x200)?0x1000:0;                       /* half */
    yam10_wf[34][p]=(p&0x100)?0x1000:0;                       /* pulse */
    yam10_wf[35][p]=(p&0x200)?0x1000:((p&0x100)?0x8000:0);    /* squished */
    yam10_wf[36][p]=(p&0x300)?0x1000:((p&0x80)?0x8000:0);     /* quarter squished */

    /* 37-39: the PSG duties. 50% is wave 19. */
    yam10_wf[37][p]=(p<128)?0:0x8000;                         /* 12.5% */
    yam10_wf[38][p]=(p<256)?0:0x8000;                         /* 25% */
    yam10_wf[39][p]=(p<768)?0:0x8000;                         /* 75% */

    /* the periodic registers are generated as they play */
    {
      int w;
      for (w=0; w<YAM10_POLY_COUNT; w++) yam10_wf[yam10_polys[w].ws][p]=0;
    }
  }
  for (p=0; p<1024; p++) {
    uint16_t sq=yam10_wf[12][p];
    yam10_wf[13][p]=sq&0x7fff;                          /* absolute */
    yam10_wf[14][p]=(p&0x200)?0x1000:sq;                /* half */
    yam10_wf[15][p]=(p&0x200)?0x1000:yam10_wf[12][(p*2)&0x3ff];
    yam10_wf[16][p]=(p&0x200)?0x1000:(yam10_wf[12][(p*2)&0x1ff]&0x7fff);

    /* 27: the last gap in the squared sine family */
    yam10_wf[27][p]=(p&0x300)?0x1000:yam10_wf[12][(p*4)&0x3ff];

    /* 40-45: the logarithmic saw, gated like every other family */
    yam10_wf[40][p]=(p&0x200)?0x1000:yam10_wf[20][p];
    yam10_wf[41][p]=yam10_wf[20][p]&0x7fff;
    yam10_wf[42][p]=(p&0x100)?0x1000:(yam10_wf[20][p]&0x7fff);
    yam10_wf[43][p]=(p&0x200)?0x1000:yam10_wf[20][(p*2)&0x3ff];
    yam10_wf[44][p]=(p&0x200)?0x1000:(yam10_wf[20][(p*2)&0x3ff]&0x7fff);
    yam10_wf[45][p]=(p&0x300)?0x1000:yam10_wf[20][(p*4)&0x3ff];

    /* 46-66: the cubed families. tripling the log domain value cubes the
       amplitude, so each one is the matching plain wave cubed and the gating
       comes along with it. */
    {
      static const unsigned char yam10_cubeOf[21]={
        0,1,2,3,4,5,6,        /* 46-52, the sines */
        7,9,8,24,10,11,25,    /* 53-59, the triangles */
        17,18,28,29,30,31,32  /* 60-66, the saws */
      };
      int w;
      for (w=0; w<21; w++) {
        yam10_wf[46+w][p]=yam10_pow(yam10_wf[yam10_cubeOf[w]][p],3);
      }
    }
  }

  /* which waveforms are generated rather than read from the bank */
  {
    int w;
    for (w=0; w<YAM10_WAVES; w++) {
      yam10_wave_def[w].kind=YAM10_WK_TABLE;
      yam10_wave_def[w].polyWidth=0;
      yam10_wave_def[w].polyPeriod=0;
      yam10_wave_def[w].polyShift=0;
      yam10_wave_def[w].polyIndex=0;
    }
    yam10_wave_def[YAM10_WF_NOISE].kind=YAM10_WK_NOISE;
    yam10_wave_def[YAM10_WF_NOISE1].kind=YAM10_WK_NOISE1;
    yam10_wave_def[YAM10_WF_SH].kind=YAM10_WK_SH;
    yam10_wave_def[YAM10_WF_PULSE].kind=YAM10_WK_PULSE;
    for (w=0; w<YAM10_POLY_COUNT; w++) {
      YAM10WaveDef& d=yam10_wave_def[yam10_polys[w].ws];
      d.kind=YAM10_WK_POLY;
      d.polyWidth=yam10_polys[w].width;
      d.polyPeriod=yam10_polys[w].period;
      d.polyIndex=(uint8_t)w;
      if (yam10_polys[w].rule==YAM10_PR_TIA) {
        /* AudioChannel::phase0 and phase1 with the divider always clocking,
           so what comes out is the raw tone rather than a pitched one. some
           AUDC settings run in before they settle, so let the counters reach
           their loop first and only then take a period's worth. there are
           only 512 states to pass through, so the warm up is generous. */
        uint8_t audc=(uint8_t)yam10_polys[w].taps;
        uint8_t noise=0, pulse=0, nbit4=0;
        uint8_t hold=0, nfb=0;
        int i;
        const int warmUp=8192;
        for (i=0; i<warmUp+(int)yam10_polys[w].period; i++) {
          uint8_t m=audc&3, pf, pfb;
          nbit4=noise&1;
          if (m<2) hold=0;
          else if (m==2) hold=(uint8_t)((noise&0x1e)!=0x02);
          else hold=(uint8_t)(!nbit4);
          if (m==0) {
            nfb=(uint8_t)((((pulse^noise)&1)!=0) || !(noise || (pulse!=0x0a)) || !(audc&0x0c));
          } else {
            nfb=(uint8_t)(((((noise&4)?1:0)^(noise&1))!=0) || noise==0);
          }
          pf=(uint8_t)(audc>>2);
          if (pf==0)      pfb=(uint8_t)(((((pulse&2)?1:0)^(pulse&1))!=0) && (pulse!=0x0a) && (audc&3));
          else if (pf==1) pfb=(uint8_t)(!(pulse&8));
          else if (pf==2) pfb=(uint8_t)(!nbit4);
          else            pfb=(uint8_t)(!((pulse&2) || !(pulse&0x0e)));
          noise>>=1;
          if (nfb) noise|=0x10;
          if (!hold) {
            pulse=(uint8_t)((~(pulse>>1))&7);
            if (pfb) pulse|=8;
          }
          if (i>=warmUp) yam10_poly_bits[w][i-warmUp]=(uint8_t)(pulse&1);
        }
      } else {
        uint32_t mask=(1u<<yam10_polys[w].width)-1u;
        uint32_t reg=1;
        int i;
        for (i=0; i<(int)yam10_polys[w].period; i++) {
          yam10_poly_bits[w][i]=(uint8_t)((reg&1u)^yam10_polys[w].invertOut);
          uint32_t fb=reg&yam10_polys[w].taps;
          fb^=fb>>8; fb^=fb>>4; fb^=fb>>2; fb^=fb>>1;
          if (yam10_polys[w].shiftLeft) {
            reg=((reg<<1)|(fb&1u))&mask;
          } else {
            reg=((reg>>1)|((fb&1u)<<(yam10_polys[w].width-1)))&mask;
          }
          if (reg==0) reg=1;
        }
      }
      /* a long pattern cannot fit into one cycle of the note: 511 steps at
         440 Hz would need content past 100 kHz. spread it over a power of two
         number of cycles instead, so it lands a whole number of octaves down
         rather than at some ratio that reads as out of tune. */
      {
        uint8_t sh=0;
        while ((yam10_polys[w].period>>sh)>32) sh++;
        d.polyShift=sh;
      }
    }
  }
  yam10_wf_built=1;
}

/* linear magnitude from a log-domain level, exactly as OPL does it */
static inline int16_t yam10_exp(uint32_t level, uint16_t neg) {
  if (level>0x1fff) level=0x1fff;
  int16_t v=(int16_t)(yam10_exprom[level&0xff]>>(level>>8));
  return neg?-v:v;
}

/* ---- DSP ----
 * the filter maths and the frequency/Q tables are the YM2609 effect chain's,
 * so a cutoff number means the same thing on both chips.
 */
static float yam10_freqTable[256];
static float yam10_qTable[256];
static float yam10_gainTable[256];
static uint8_t yam10_dsp_built=0;

static void yam10_build_dsp(void) {
  if (yam10_dsp_built) return;
  for (int i=0; i<256; i++) {
    if (i<96)       yam10_freqTable[i]=(float)(i+1);
    else if (i<160) yam10_freqTable[i]=(float)((i-96)*10+100);
    else if (i<224) yam10_freqTable[i]=(float)((i-160)*100+800);
    else            yam10_freqTable[i]=(float)((i-224)*1000+7500);

    if (i<96)       yam10_qTable[i]=(float)(1.0/96.0*(i+1));
    else if (i<192) yam10_qTable[i]=(float)(10.0/96.0*(i+1-96)+1.0);
    else            yam10_qTable[i]=(float)(10.0/64.0*(i+1-192)+11.0);

    /* -20 dB to +19.84 dB, 128 is flat */
    if (i<128) yam10_gainTable[i]=(float)(-20.0/128.0*(128-i));
    else       yam10_gainTable[i]=(float)( 20.0/128.0*(i-128));
  }
  yam10_dsp_built=1;
}

/* a value on its way to zero must not be left subnormal: the arithmetic
   costs many times more and every feedback path here converges on zero */
static inline float yam10_fz(float v) { return (v>-1.0e-20f && v<1.0e-20f)?0.0f:v; }

/* direct form I biquad, same shape as the YM2609 one */
struct YAM10Biquad {
  float a0,a1,a2,b0,b1,b2;
  float in1,in2,out1,out2;
  YAM10Biquad(): a0(1.0f),a1(0.0f),a2(0.0f),b0(1.0f),b1(0.0f),b2(0.0f),
                 in1(0.0f),in2(0.0f),out1(0.0f),out2(0.0f) {}
  void clear() { in1=in2=out1=out2=0.0f; }
  inline float process(float x) {
    float y=b0/a0*x + b1/a0*in1 + b2/a0*in2 - a1/a0*out1 - a2/a0*out2;
    y=yam10_fz(y);
    in2=in1; in1=yam10_fz(x);
    out2=out1; out1=y;
    return y;
  }
  /* the three EQ shapes, coefficient for coefficient as the YM2609 has them.
     peaking takes a bandwidth rather than a Q, which is why it looks
     different from the other two. */
  void lowShelf(float freq, float q, float gain, float sr) {
    if (freq<1.0f) freq=1.0f;
    if (freq>sr*0.45f) freq=sr*0.45f;
    if (q<0.01f) q=0.01f;
    float omega=2.0f*3.14159265f*freq/sr;
    float A=(float)pow(10.0f,gain/40.0f);
    float beta=(float)(sqrt(A)/q);
    float co=(float)cos(omega), si=(float)sin(omega);
    a0=(A+1.0f)+(A-1.0f)*co+beta*si;
    a1=-2.0f*((A-1.0f)+(A+1.0f)*co);
    a2=(A+1.0f)+(A-1.0f)*co-beta*si;
    b0=A*((A+1.0f)-(A-1.0f)*co+beta*si);
    b1=2.0f*A*((A-1.0f)-(A+1.0f)*co);
    b2=A*((A+1.0f)-(A-1.0f)*co-beta*si);
  }
  void highShelf(float freq, float q, float gain, float sr) {
    if (freq<1.0f) freq=1.0f;
    if (freq>sr*0.45f) freq=sr*0.45f;
    if (q<0.01f) q=0.01f;
    float omega=2.0f*3.14159265f*freq/sr;
    float A=(float)pow(10.0f,gain/40.0f);
    float beta=(float)(sqrt(A)/q);
    float co=(float)cos(omega), si=(float)sin(omega);
    a0=(A+1.0f)-(A-1.0f)*co+beta*si;
    a1=2.0f*((A-1.0f)-(A+1.0f)*co);
    a2=(A+1.0f)-(A-1.0f)*co-beta*si;
    b0=A*((A+1.0f)+(A-1.0f)*co+beta*si);
    b1=-2.0f*A*((A-1.0f)+(A+1.0f)*co);
    b2=A*((A+1.0f)+(A-1.0f)*co-beta*si);
  }
  void peaking(float freq, float bw, float gain, float sr) {
    if (freq<1.0f) freq=1.0f;
    if (freq>sr*0.45f) freq=sr*0.45f;
    if (bw<0.01f) bw=0.01f;
    float omega=2.0f*3.14159265f*freq/sr;
    float si=(float)sin(omega);
    if (si<1.0e-6f) si=1.0e-6f;
    float alpha=(float)(si*sinh(log(2.0)/2.0*bw*omega/si));
    float A=(float)pow(10.0f,gain/40.0f);
    a0=1.0f+alpha/A;
    a1=(float)(-2.0f*cos(omega));
    a2=1.0f-alpha/A;
    b0=1.0f+alpha*A;
    b1=a1;
    b2=1.0f-alpha*A;
  }

  /* pick a shape by number, so a band can be anything */
  void setEQ(int type, float freq, float q, float gain, float sr) {
    switch (type) {
      case 1: lowShelf(freq,q,gain,sr); break;
      case 2: highShelf(freq,q,gain,sr); break;
      case 3: set(0,freq,q,sr); break;
      case 4: set(1,freq,q,sr); break;
      case 5: set(3,freq,q,sr); break;
      default: peaking(freq,q,gain,sr); break;
    }
  }

  void set(int mode, float freq, float q, float sr) {
    if (freq<1.0f) freq=1.0f;
    if (freq>sr*0.45f) freq=sr*0.45f;
    if (q<0.01f) q=0.01f;
    float omega=2.0f*3.14159265f*freq/sr;
    float alpha=(float)(sin(omega)/(2.0f*q));
    float co=(float)cos(omega);
    switch (mode) {
      case 1: /* high pass */
        a0=1.0f+alpha; a1=-2.0f*co; a2=1.0f-alpha;
        b0=(1.0f+co)/2.0f; b1=-(1.0f+co); b2=(1.0f+co)/2.0f;
        break;
      case 2: /* band pass */
        a0=1.0f+alpha; a1=-2.0f*co; a2=1.0f-alpha;
        b0=alpha; b1=0.0f; b2=-alpha;
        break;
      case 3: /* notch */
        a0=1.0f+alpha; a1=-2.0f*co; a2=1.0f-alpha;
        b0=1.0f; b1=-2.0f*co; b2=1.0f;
        break;
      default: /* low pass */
        a0=1.0f+alpha; a1=-2.0f*co; a2=1.0f-alpha;
        b0=(1.0f-co)/2.0f; b1=1.0f-co; b2=(1.0f-co)/2.0f;
        break;
    }
  }
};

struct YAM10FilterParam {
  bool enable;
  unsigned char mode;    /* 0 low, 1 high, 2 band, 3 notch */
  unsigned char cutoff;  /* index into the frequency table */
  unsigned char res;     /* index into the Q table */
  YAM10FilterParam(): enable(false), mode(0), cutoff(220), res(20) {}
};

struct YAM10OpParam {
  bool enable, fixedMode, ksr, customWave;
  unsigned char ws;        /* 0 to YAM10_WAVES-1; the wavetable is customWave */
  unsigned char tl;        /* 0-127, 0.75 dB/step */
  unsigned char ar, dr, d2r;
  unsigned char sl, rr;
  unsigned char rs;        /* rate scaling 0-3 */
  unsigned char mult;      /* 0 is half, then 1 to 16 */
  unsigned char delay;     /* 0 is none, else 256<<delay samples of it */
  signed char dtFine;      /* cents */
  signed char dtSemi;      /* semitones */
  unsigned char fb;        /* self feedback 0-7 */
  unsigned char outLvl;    /* carrier output level */
  unsigned char pan;       /* 0-255, 128 = centre */
  unsigned char modIn;     /* bitmask of operators feeding this one */
  unsigned char duty;      /* pulse width, 32 is the 12.5% this wave used to be */
  double fixedFreq;        /* Hz, usable down to 0 */
  unsigned int phaseResetPeriod; /* engine ticks, 0 = off */
  const int* waveData;
  int waveLen, waveMax;
  YAM10OpParam():
    enable(false), fixedMode(false), ksr(false), customWave(false),
    ws(0), tl(127), ar(31), dr(0), d2r(0), sl(0), rr(7), rs(0), mult(1), delay(0),
    dtFine(0), dtSemi(0), fb(0), outLvl(0), pan(128), modIn(0), duty(32),
    fixedFreq(0.0), phaseResetPeriod(0), waveData(NULL), waveLen(0), waveMax(255) {}
};

struct YAM10EQBand {
  bool on;
  unsigned char type;      /* 0 peak, 1 low shelf, 2 high shelf,
                              3 low pass, 4 high pass, 5 notch */
  unsigned char freq, gain, q;
  YAM10EQBand(): on(true), type(0), freq(160), gain(128), q(67) {}
};

struct YAM10ChanParam {
  YAM10OpParam op[YAM10_OPS];
  YAM10FilterParam filter[YAM10_FILTERS];
  bool distEnable;
  unsigned char distGain, distCutoff, distLevel;
  bool chorusEnable;
  unsigned char chorusMix, chorusRate, chorusDepth, chorusFeedback, chorusWidth;
  bool reverbEnable;
  unsigned char reverbMix, reverbSend, reverbDecay;
  unsigned char reverbEarly, reverbDiffusion, reverbSize, reverbDamp;
  bool compEnable;
  /* three bands, each squeezed on its own. ratios run both ways: anything
     over the threshold comes down by the down ratio, anything under comes
     up by the up ratio. */
  unsigned char compThreshold, compRatio, compUpRatio, compMakeup;
  unsigned char compAttack, compDecay;
  unsigned char compLoMid, compMidHi;                 /* crossover points */
  unsigned char compLowGain, compMidGain, compHighGain;
  /* the YM2609 EQ: a low shelf, a peak and a high shelf */
  bool eqEnable;
  unsigned char eqCount;               /* how many bands have been placed */
  YAM10EQBand eqBand[YAM10_EQ_BANDS];
  bool phaseInvL, phaseInvR;
  unsigned char echoMix;
  unsigned short echoDelay;   /* ms */
  unsigned char echoFeedback;
  YAM10ChanParam():
    distEnable(false), distGain(48), distCutoff(80), distLevel(16),
    chorusEnable(false), chorusMix(64), chorusRate(16), chorusDepth(32),
    chorusFeedback(32), chorusWidth(0),
    reverbEnable(false), reverbMix(64), reverbSend(96), reverbDecay(80),
    reverbEarly(48), reverbDiffusion(80), reverbSize(64), reverbDamp(48),
    compEnable(false),
    compThreshold(96), compRatio(32), compUpRatio(0), compMakeup(64),
    compAttack(40), compDecay(64),
    compLoMid(116), compMidHi(172),
    compLowGain(128), compMidGain(128), compHighGain(128),
    eqEnable(false), eqCount(0),
    phaseInvL(false), phaseInvR(false),
    echoMix(0), echoDelay(250), echoFeedback(64) {}
};

class YAM10Chip {
  struct OpState {
    uint32_t phase;      /* 20-bit fractional per cycle, OPN style */
    uint32_t delayCount; /* counts up to the operator delay after a key on */
    int16_t out, prev;   /* last two outputs, for feedback averaging */
    uint16_t att;        /* attenuation 0 (loud) .. 0x3ff (silent) */
    uint8_t egState;     /* 0 atk 1 dec 2 sus 3 rel 4 off */
    uint32_t prCount;
    uint32_t noise;
    uint32_t noiseStep;
    uint32_t shCycle;
    int16_t sh;
    OpState(): phase(0), out(0), prev(0), att(0x3ff), egState(4),
               prCount(0), noise(0xACE1u), noiseStep(0), shCycle(0xffffffffu), sh(0) {}
  };
  struct ChanState {
    OpState op[YAM10_OPS];
    double freq;
    bool keyOn;
    float echoBuf[2][YAM10_MAX_ECHO];
    int echoPos;
    /* DSP */
    YAM10Biquad fL[YAM10_FILTERS], fR[YAM10_FILTERS];
    uint8_t fMode[YAM10_FILTERS], fCut[YAM10_FILTERS], fRes[YAM10_FILTERS];
    bool fValid[YAM10_FILTERS];
    YAM10Biquad distL, distR;
    uint8_t distCut;
    bool distValid;
    float chorusBuf[2][YAM10_MAX_CHORUS];
    int chorusPos;
    double chorusPhase;
    float revC1[2][YAM10_REV_C1], revC2[2][YAM10_REV_C2], revAP[2][YAM10_REV_AP];
    int revP1, revP2, revPA;
    /* one set of crossover filters a side: two poles each so the bands
       recombine flat, then an envelope and a gain per band */
    YAM10Biquad xLowA[2], xLowB[2], xMidHA[2], xMidHB[2], xMidLA[2], xMidLB[2], xHighA[2], xHighB[2];
    unsigned char xCache[2];
    bool xValid;
    float compEnv[3][2], compGain[3][2];
    int quiet;
    float cAtk, cDec, cThrDb, cDnR, cUpR, cMakeup, cBandLin[3];
    unsigned char cCache[9];
    bool cValid;
    /* reverb */
    float earlyBuf[2][YAM10_REV_EARLY];
    int earlyPos;
    float revDampS[2][2];
    float revAP2[2][YAM10_REV_AP2], revAP3[2][YAM10_REV_AP3];
    int revPA2, revPA3;
    YAM10Biquad eqB[YAM10_EQ_BANDS][2];
    /* the bytes the EQ coefficients were last built from */
    unsigned char eqCache[1+YAM10_EQ_BANDS*4];
    bool eqValid;
    ChanState() { reset(); }
    /* in place: this holds most of a megabyte of delay lines, so building a
       temporary one to assign from would overflow a modest stack */
    void reset() {
      freq=0; keyOn=false; echoPos=0; distCut=0; distValid=false;
      chorusPos=0; chorusPhase=0.0; revP1=0; revP2=0; revPA=0;
      memset(echoBuf,0,sizeof(echoBuf));
      memset(chorusBuf,0,sizeof(chorusBuf));
      memset(revC1,0,sizeof(revC1));
      memset(revC2,0,sizeof(revC2));
      memset(revAP,0,sizeof(revAP));
      for (int b=0; b<3; b++) for (int sd=0; sd<2; sd++) {
        compEnv[b][sd]=0.0f;
        compGain[b][sd]=1.0f;
      }
      xCache[0]=xCache[1]=0;
      xValid=false;
      quiet=0;
      memset(cCache,0,sizeof(cCache));
      cValid=false;
      cAtk=cDec=0.0f; cThrDb=0.0f; cDnR=cUpR=1.0f; cMakeup=1.0f;
      cBandLin[0]=cBandLin[1]=cBandLin[2]=1.0f;
      memset(earlyBuf,0,sizeof(earlyBuf));
      earlyPos=0;
      revDampS[0][0]=revDampS[0][1]=revDampS[1][0]=revDampS[1][1]=0.0f;
      memset(revAP2,0,sizeof(revAP2));
      memset(revAP3,0,sizeof(revAP3));
      revPA2=0; revPA3=0;
      memset(eqCache,0,sizeof(eqCache));
      eqValid=false;
      for (int i=0; i<YAM10_FILTERS; i++) { fMode[i]=0; fCut[i]=0; fRes[i]=0; fValid[i]=false; }
    }
  };

  double rate;
  ChanState chan[YAM10_CHANS];
  uint32_t egTimer;
  uint8_t egSubCount;

  /* OPN style: rate = 2*reg + key scale value, clamped to 63 */
  static inline int combinedRate(int reg, int kcode, int rs, bool ksr) {
    if (reg<=0) return 0;
    int ksv=kcode>>(3-rs);
    if (!ksr) ksv>>=2;
    int r=2*reg+ksv;
    return (r>63)?63:r;
  }
  /* attenuation step for this rate on this envelope tick */
  inline uint16_t egStep(int rate_, uint32_t counter) const {
    if (rate_<=0) return 0;
    int shift=11-(rate_>>2);
    if (shift<0) shift=0;
    if (counter&((1u<<shift)-1)) return 0;
    static const uint8_t inc[4][8]={
      {0,1,0,1,0,1,0,1},{0,1,0,1,1,1,0,1},
      {0,1,1,1,0,1,1,1},{0,1,1,1,1,1,1,1}
    };
    uint8_t base=inc[rate_&3][(counter>>shift)&7];
    if (rate_<48) return base;
    /* above rate 48 the increment itself scales up */
    int extra=(rate_>>2)-12;
    if (extra>3) extra=3;
    return (uint16_t)((base?base:1)<<extra);
  }
  static inline int kcodeOf(double f) {
    if (f<13.75) return 0;
    int k=(int)(log2(f/13.75)*4.0);
    return k<0?0:((k>31)?31:k);
  }

public:
  YAM10ChanParam par[YAM10_CHANS];

  void init(double outRate) {
    yam10_build_wf();
    yam10_build_dsp();
    rate=outRate;
    egTimer=0; egSubCount=0;
    for (int c=0; c<YAM10_CHANS; c++) chan[c].reset();
  }
  void setFreq(int c, double hz) { chan[c].freq=hz; }
  void keyOn(int c) {
    chan[c].keyOn=true;
    chan[c].quiet=0;
    for (int i=0; i<YAM10_OPS; i++) {
      chan[c].op[i].egState=0;
      chan[c].op[i].phase=0;
      chan[c].op[i].prCount=0;
      chan[c].op[i].delayCount=0;
      chan[c].op[i].shCycle=0xffffffffu;
    }
  }
  void keyOff(int c) {
    chan[c].keyOn=false;
    for (int i=0; i<YAM10_OPS; i++) if (chan[c].op[i].egState<3) chan[c].op[i].egState=3;
  }
  /* a released channel whose output has sat at nothing long enough for the
     delay lines to have drained does not need rendering at all */
  bool chanIdle(int c) const { return chan[c].quiet>8192; }
  bool isSilent(int c) const {
    for (int i=0; i<YAM10_OPS; i++)
      if (chan[c].op[i].egState<4 && chan[c].op[i].att<0x3f0) return false;
    return true;
  }

  /* the early reflection taps, in samples at 44100. size stretches them. */
  static inline float earlyTap(ChanState& ch, int side, int d) {
    int i=ch.earlyPos-d;
    while (i<0) i+=YAM10_REV_EARLY;
    return ch.earlyBuf[side][i%YAM10_REV_EARLY];
  }

  /* read the chorus delay line back with linear interpolation */
  static inline float chorusTap(ChanState& ch, int side, double d) {
    double rp=(double)ch.chorusPos-d;
    while (rp<0.0) rp+=YAM10_MAX_CHORUS;
    int i0=(int)rp;
    int i1=(i0+1)%YAM10_MAX_CHORUS;
    float fr=(float)(rp-(double)i0);
    return ch.chorusBuf[side][i0]*(1.0f-fr)+ch.chorusBuf[side][i1]*fr;
  }

  void tickEG() { egTimer++; }

  /* called once per engine tick so the phase reset timer follows the tempo */
  void tickPhaseReset() {
    for (int c=0; c<YAM10_CHANS; c++) {
      for (int i=0; i<YAM10_OPS; i++) {
        YAM10OpParam& p=par[c].op[i];
        if (p.phaseResetPeriod==0) continue;
        OpState& o=chan[c].op[i];
        if (++o.prCount>=p.phaseResetPeriod) {
          o.prCount=0;
          o.phase=0;
          /* start the operator over rather than only its phase, so the
             timer retriggers the envelope the way a key press does */
          o.egState=0;
          o.att=0x3ff;
          o.shCycle=0xffffffffu;
        }
      }
    }
  }

  void renderChan(int c, float& outL, float& outR) {
    ChanState& ch=chan[c];
    YAM10ChanParam& cp=par[c];
    int16_t now[YAM10_OPS];
    double mixL=0.0, mixR=0.0;
    int kcode=kcodeOf(ch.freq);

    for (int i=0; i<YAM10_OPS; i++) {
      YAM10OpParam& p=cp.op[i];
      OpState& o=ch.op[i];
      if (!p.enable || o.egState==4) { now[i]=0; o.prev=o.out; o.out=0; continue; }

      /* ---- envelope (OPN model, attenuation domain) ---- */
      /* a delay holds the operator at silence before its attack begins, so
         one operator can come in after another rather than with it */
      if (p.delay>0 && o.delayCount<(256u<<((p.delay>7)?7:p.delay))) {
        o.delayCount++;
        o.att=0x3ff;
      } else
      switch (o.egState) {
        case 0: { /* attack: exponential approach to 0 */
          int r=combinedRate(p.ar,kcode,p.rs,p.ksr);
          if (r>=62) { o.att=0; o.egState=1; break; }
          uint16_t st=egStep(r,egTimer);
          if (st) {
            /* the complement is negative, so this falls by a fraction of
             * whatever attenuation is left, like ymfm */
            int d=((~(int)o.att)*(int)st)>>4;
            int na=(int)o.att+d;
            if (na<=0) { o.att=0; o.egState=1; } else o.att=(uint16_t)na;
          }
          break;
        }
        case 1: { /* decay to sustain level */
          /* 32 a step, so each one is 3 dB, and the top setting means off.
             at 68 a step the middle settings landed 50 dB down and anything
             above about 8 was inaudible, which is not a sustain. */
          uint16_t slAtt=(p.sl>=15)?1023:(uint16_t)(p.sl*32);
          o.att+=egStep(combinedRate(p.dr,kcode,p.rs,p.ksr),egTimer);
          if (o.att>=slAtt) { o.att=slAtt; o.egState=2; }
          break;
        }
        case 2: /* second decay keeps going, like OPN D2R */
          o.att+=egStep(combinedRate(p.d2r,kcode,p.rs,p.ksr),egTimer);
          break;
        case 3:
          o.att+=egStep(combinedRate(p.rr*2+1,kcode,p.rs,p.ksr),egTimer);
          break;
      }
      if (o.att>=0x3ff) { o.att=0x3ff; if (o.egState==3) { o.egState=4; } }

      /* ---- phase ---- */
      double f;
      if (p.fixedMode) {
        f=p.fixedFreq;                      /* 0 Hz simply halts the phase */
      } else {
        f=ch.freq*pow(2.0,(double)p.dtSemi/12.0+(double)p.dtFine/1200.0);
      }
      f*=(p.mult==0)?0.5:(double)p.mult;
      uint32_t step=(uint32_t)((f/rate)*1048576.0);   /* 20-bit cycle */

      /* ---- modulation: earlier ops from this sample, others from last ---- */
      int mod=0;
      for (int j=0; j<YAM10_OPS; j++) {
        if (!(p.modIn&(1<<j))) continue;
        mod+=(j<i)?now[j]:ch.op[j].out;
      }
      /* two steps gentler than OPL's, so even feedback 7 stays periodic
       * instead of collapsing into noise */
      if (p.fb>0) mod+=((int)o.out+(int)o.prev)>>(11-p.fb);

      uint32_t idx=((o.phase>>10)+(uint32_t)(mod>>1))&0x3ff;
      uint8_t wf=p.customWave?YAM10_WAVE_WT:((p.ws>=YAM10_WAVES)?0:p.ws);
      const YAM10WaveDef& wd=yam10_wave_def[(wf==YAM10_WAVE_WT)?0:wf];
      uint8_t kind=(wf==YAM10_WAVE_WT)?YAM10_WK_WAVETABLE:wd.kind;
      uint16_t lg, neg;

      if (kind==YAM10_WK_NOISE || kind==YAM10_WK_NOISE1) {  /* noise, pitched */
        /* the shift register is clocked off the phase, so the note and any
         * pitch or arpeggio macro move the noise with it */
        uint32_t ns=o.phase>>15;
        uint32_t adv=ns-o.noiseStep;
        if (adv) {
          if (adv>32) adv=32;       /* do not spin when the phase wraps */
          for (uint32_t k=0; k<adv; k++) {
            o.noise=(o.noise>>1)^(-(int32_t)(o.noise&1)&0xB400u);
          }
          o.noiseStep=ns;
        }
        if (kind==YAM10_WK_NOISE1) {                  /* 1-bit noise */
          neg=(o.noise&1)?0:0x8000;
          lg=0;
        } else {
          int16_t n=(int16_t)((o.noise&0x1fff)-0x1000);
          lg=(uint16_t)(n<0?-n:n); neg=(n<0)?0x8000:0;
          lg=(lg<1)?0x1000:(uint16_t)(-(int)(log2((double)lg/4096.0)*256.0));
        }
      } else if (kind==YAM10_WK_POLY) {
        /* one whole pattern every 1<<polyShift cycles of the note, so it lands
           a clean number of octaves down rather than at a ratio that reads as
           out of tune. a long pattern cannot fit in one cycle: 511 steps at
           440 Hz would carry content past 100 kHz. */
        uint32_t ns=(uint32_t)(((uint64_t)o.phase*(uint64_t)wd.polyPeriod)>>(20+wd.polyShift));
        neg=yam10_poly_bits[wd.polyIndex][ns%wd.polyPeriod]?0:0x8000;
        lg=0;
      } else if (kind==YAM10_WK_PULSE) {
        /* duty counts in 256ths of a cycle, so 32 lands on the 12.5% this
           wave was fixed at before and 64, 128 and 192 give the other
           duties a PSG offers */
        neg=(idx<((uint32_t)p.duty<<2))?0:0x8000;
        lg=0;
      } else if (kind==YAM10_WK_SH) {                 /* sample & hold */
        uint32_t cyc=o.phase>>20;                     /* which cycle we are in */
        if (cyc!=o.shCycle) {
          o.shCycle=cyc;
          o.noise=(o.noise>>1)^(-(int32_t)(o.noise&1)&0xB400u);
          o.sh=(int16_t)((o.noise&0x1fff)-0x1000);
        }
        int16_t n=o.sh;
        lg=(uint16_t)(n<0?-n:n); neg=(n<0)?0x8000:0;
        lg=(lg<1)?0x1000:(uint16_t)(-(int)(log2((double)lg/4096.0)*256.0));
      } else if (kind==YAM10_WK_WAVETABLE && p.waveData!=NULL && p.waveLen>=2) {
        /* custom wavetable of any length, converted into the log domain */
        int wi=(int)(((uint64_t)idx*(uint32_t)p.waveLen)>>10);
        if (wi>=p.waveLen) wi=p.waveLen-1;
        double half=(double)p.waveMax*0.5;
        double v=((double)p.waveData[wi]-half)/(half>0?half:1.0);
        neg=(v<0)?0x8000:0;
        double a=fabs(v);
        lg=(a<(1.0/4096.0))?0x1000:(uint16_t)(-(int)(log2(a)*256.0));
      } else if (kind==YAM10_WK_TABLE) {
        uint16_t e=yam10_wf[wf][idx];
        lg=e&0x7fff; neg=e&0x8000;
      } else {
        lg=0x1000; neg=0;                             /* nothing bound: silent */
      }

      /* attenuation: TL joins the envelope in the 10-bit domain, then the
       * sum is scaled up, the way OPN does it. 0.75 dB a TL step. */
      uint32_t level=lg+(((uint32_t)o.att+((uint32_t)p.tl<<3))<<2);
      int16_t out=(lg>=0x1000)?0:yam10_exp(level,neg);

      o.phase+=step;
      o.prev=o.out;
      o.out=out;
      now[i]=out;

      if (p.outLvl>0) {
        /* 2*sqrt(2) makes centre pan put the operator's full output on each
         * side while staying constant power, so one voice reads clearly on
         * the oscilloscope and four carriers fill the scale. */
        double amp=(double)out*((double)p.outLvl/127.0)*(2.0*YAM10_SQRT2);
        double pp=(double)p.pan/255.0;
        mixL+=amp*cos(pp*YAM10_PI*0.5);
        mixR+=amp*sin(pp*YAM10_PI*0.5);
      }
    }

    /* ---- per channel DSP ---- */
    {
      float dL=(float)mixL, dR=(float)mixR;

      /* multi filter, three biquads in series */
      for (int f=0; f<YAM10_FILTERS; f++) {
        YAM10FilterParam& fp=cp.filter[f];
        if (!fp.enable) { ch.fValid[f]=false; continue; }
        if (!ch.fValid[f] || ch.fMode[f]!=fp.mode || ch.fCut[f]!=fp.cutoff || ch.fRes[f]!=fp.res) {
          ch.fL[f].set(fp.mode,yam10_freqTable[fp.cutoff],yam10_qTable[fp.res],(float)rate);
          ch.fR[f].set(fp.mode,yam10_freqTable[fp.cutoff],yam10_qTable[fp.res],(float)rate);
          ch.fMode[f]=fp.mode; ch.fCut[f]=fp.cutoff; ch.fRes[f]=fp.res; ch.fValid[f]=true;
        }
        dL=ch.fL[f].process(dL);
        dR=ch.fR[f].process(dR);
      }

      /* distortion: cut the lows, amplify hard, clip, then bring the level back */
      if (cp.distEnable) {
        if (!ch.distValid || ch.distCut!=cp.distCutoff) {
          ch.distL.set(1,yam10_freqTable[cp.distCutoff],0.707f,(float)rate);
          ch.distR.set(1,yam10_freqTable[cp.distCutoff],0.707f,(float)rate);
          ch.distCut=cp.distCutoff; ch.distValid=true;
        }
        float g=1.0f+(float)cp.distGain*(299.0f/127.0f);
        float v=(float)cp.distLevel/127.0f;
        float tl=ch.distL.process(dL/32768.0f)*g;
        float tr=ch.distR.process(dR/32768.0f)*g;
        if (tl>1.0f) tl=1.0f; if (tl<-1.0f) tl=-1.0f;
        if (tr>1.0f) tr=1.0f; if (tr<-1.0f) tr=-1.0f;
        dL=tl*v*32768.0f;
        dR=tr*v*32768.0f;
      }

      /* chorus: one modulated delay a side, the width offsets their LFOs */
      if (cp.chorusEnable && cp.chorusMix>0) {
        double lfoHz=0.05+(double)cp.chorusRate*(7.95/127.0);
        ch.chorusPhase+=lfoHz/rate;
        if (ch.chorusPhase>=1.0) ch.chorusPhase-=1.0;
        double wOff=(double)cp.chorusWidth/254.0;
        double aL=ch.chorusPhase*2.0*YAM10_PI;
        double aR=(ch.chorusPhase+wOff)*2.0*YAM10_PI;
        double base=rate*0.006;                        /* 6 ms centre */
        double depth=base*0.9*((double)cp.chorusDepth/127.0);
        double dl=base+depth*sin(aL);
        double dr=base+depth*sin(aR);
        int maxD=YAM10_MAX_CHORUS-2;
        if (dl<1.0) dl=1.0; if (dl>maxD) dl=maxD;
        if (dr<1.0) dr=1.0; if (dr>maxD) dr=maxD;
        float sL=chorusTap(ch,0,dl), sR=chorusTap(ch,1,dr);
        float fbk=(float)cp.chorusFeedback/127.0f*0.7f;
        ch.chorusBuf[0][ch.chorusPos]=yam10_fz(dL+sL*fbk);
        ch.chorusBuf[1][ch.chorusPos]=yam10_fz(dR+sR*fbk);
        if (++ch.chorusPos>=YAM10_MAX_CHORUS) ch.chorusPos=0;
        float m=(float)cp.chorusMix/127.0f;
        dL+=sL*m;
        dR+=sR*m;
      }

      /* reverb: a burst of early reflections, then a damped tail smeared
         through three allpasses */
      if (cp.reverbEnable && cp.reverbMix>0) {
        static const int earlyOff[YAM10_EARLY_TAPS]={
          113,251,397,557,719,907,1087,1279
        };
        float g=(float)cp.reverbDecay/127.0f*0.85f;
        float send=(float)cp.reverbSend/127.0f;
        float m=(float)cp.reverbMix/127.0f;
        float early=(float)cp.reverbEarly/127.0f;
        float diff=(float)cp.reverbDiffusion/127.0f*0.7f;
        float damp=(float)cp.reverbDamp/127.0f*0.85f;
        /* size stretches the tap spacing, and the rate keeps the room the
           same width whatever the chip is clocked at */
        float sizeScale=(0.25f+(float)cp.reverbSize/127.0f*1.75f)*(float)(rate/44100.0);
        float wet[2];
        for (int sd=0; sd<2; sd++) {
          float in=(sd?dR:dL)*send;
          ch.earlyBuf[sd][ch.earlyPos]=yam10_fz(in);
          float er=0.0f;
          if (early>0.0f) {
            for (int t=0; t<YAM10_EARLY_TAPS; t++) {
              /* the far side is offset so the two ears do not agree */
              int d=(int)((float)earlyOff[t]*sizeScale)+(sd?37:0);
              if (d<1) d=1;
              if (d>=YAM10_REV_EARLY) d=YAM10_REV_EARLY-1;
              float a=1.0f-(float)t/(float)YAM10_EARLY_TAPS*0.7f;
              er+=earlyTap(ch,sd,d)*a*((t&1)?-1.0f:1.0f);
            }
            er*=early*0.35f;
          }
          /* two combs, each losing treble a little more every pass */
          float c1=ch.revC1[sd][ch.revP1];
          float c2=ch.revC2[sd][ch.revP2];
          ch.revDampS[sd][0]=yam10_fz(ch.revDampS[sd][0]+(c1-ch.revDampS[sd][0])*(1.0f-damp));
          ch.revDampS[sd][1]=yam10_fz(ch.revDampS[sd][1]+(c2-ch.revDampS[sd][1])*(1.0f-damp));
          ch.revC1[sd][ch.revP1]=yam10_fz(in+ch.revDampS[sd][0]*g);
          ch.revC2[sd][ch.revP2]=yam10_fz(in+ch.revDampS[sd][1]*g);
          float sum=(c1+c2)*0.5f;
          /* three allpasses in series: diffusion sets how hard they smear */
          float ap=ch.revAP[sd][ch.revPA];
          float o1=ap-sum;
          ch.revAP[sd][ch.revPA]=yam10_fz(sum+ap*diff);
          float ap2=ch.revAP2[sd][ch.revPA2];
          float o2=ap2-o1;
          ch.revAP2[sd][ch.revPA2]=yam10_fz(o1+ap2*diff);
          float ap3=ch.revAP3[sd][ch.revPA3];
          float o3=ap3-o2;
          ch.revAP3[sd][ch.revPA3]=yam10_fz(o2+ap3*diff);
          wet[sd]=o3+er;
        }
        if (++ch.earlyPos>=YAM10_REV_EARLY) ch.earlyPos=0;
        if (++ch.revP1>=YAM10_REV_C1) ch.revP1=0;
        if (++ch.revP2>=YAM10_REV_C2) ch.revP2=0;
        if (++ch.revPA>=YAM10_REV_AP) ch.revPA=0;
        if (++ch.revPA2>=YAM10_REV_AP2) ch.revPA2=0;
        if (++ch.revPA3>=YAM10_REV_AP3) ch.revPA3=0;
        dL+=wet[0]*m;
        dR+=wet[1]*m;
      }

      /* compressor: split into three, squeeze each one on its own, then
         put them back together */
      if (cp.compEnable) {
        if (!ch.xValid || ch.xCache[0]!=cp.compLoMid || ch.xCache[1]!=cp.compMidHi) {
          float f1=yam10_freqTable[cp.compLoMid];
          float f2=yam10_freqTable[cp.compMidHi];
          if (f2<f1*1.2f) f2=f1*1.2f;      /* never let the bands cross over */
          for (int sd=0; sd<2; sd++) {
            ch.xLowA[sd].set(0,f1,0.7071f,(float)rate);
            ch.xLowB[sd].set(0,f1,0.7071f,(float)rate);
            ch.xMidHA[sd].set(1,f1,0.7071f,(float)rate);
            ch.xMidHB[sd].set(1,f1,0.7071f,(float)rate);
            ch.xMidLA[sd].set(0,f2,0.7071f,(float)rate);
            ch.xMidLB[sd].set(0,f2,0.7071f,(float)rate);
            ch.xHighA[sd].set(1,f2,0.7071f,(float)rate);
            ch.xHighB[sd].set(1,f2,0.7071f,(float)rate);
          }
          ch.xCache[0]=cp.compLoMid;
          ch.xCache[1]=cp.compMidHi;
          ch.xValid=true;
        }
        const unsigned char cWant[9]={
          cp.compAttack,cp.compDecay,cp.compThreshold,cp.compRatio,cp.compUpRatio,
          cp.compMakeup,cp.compLowGain,cp.compMidGain,cp.compHighGain
        };
        if (!ch.cValid || memcmp(ch.cCache,cWant,9)!=0) {
          float atkMs=0.1f*(float)pow(1000.0,(double)cp.compAttack/127.0);
          float decMs=1.0f*(float)pow(1000.0,(double)cp.compDecay/127.0);
          ch.cAtk=1.0f-(float)exp(-1.0/(atkMs*0.001*rate));
          ch.cDec=1.0f-(float)exp(-1.0/(decMs*0.001*rate));
          if (ch.cAtk>1.0f) ch.cAtk=1.0f;
          if (ch.cDec>1.0f) ch.cDec=1.0f;
          ch.cThrDb=-60.0f+(float)cp.compThreshold/127.0f*60.0f;
          ch.cDnR=1.0f+(float)cp.compRatio/127.0f*19.0f;
          /* the upward lift stops at 4:1 */
          ch.cUpR=1.0f+(float)cp.compUpRatio/127.0f*3.0f;
          ch.cMakeup=(float)pow(10.0,((double)cp.compMakeup-64.0)/63.0*12.0/20.0);
          ch.cBandLin[0]=(float)pow(10.0,(double)yam10_gainTable[cp.compLowGain]/20.0);
          ch.cBandLin[1]=(float)pow(10.0,(double)yam10_gainTable[cp.compMidGain]/20.0);
          ch.cBandLin[2]=(float)pow(10.0,(double)yam10_gainTable[cp.compHighGain]/20.0);
          memcpy(ch.cCache,cWant,9);
          ch.cValid=true;
        }
        const float atkC=ch.cAtk, decC=ch.cDec;
        const float thrDb=ch.cThrDb, dnR=ch.cDnR, upR=ch.cUpR;
        const float makeup=ch.cMakeup;
        /* the level is followed as power rather than peak, and the corner is
           rounded off over 12 dB, so it holds a line without sounding worked */
        const float knee=12.0f;
        for (int sd=0; sd<2; sd++) {
          float v=(sd?dR:dL)/32768.0f;
          float low=ch.xLowB[sd].process(ch.xLowA[sd].process(v));
          float mid=ch.xMidLB[sd].process(ch.xMidLA[sd].process(
                    ch.xMidHB[sd].process(ch.xMidHA[sd].process(v))));
          float high=ch.xHighB[sd].process(ch.xHighA[sd].process(v));
          float band[3]={low,mid,high};
          float sum=0.0f;
          for (int b=0; b<3; b++) {
            float a=(band[b]<0.0f)?-band[b]:band[b];
            float det=a*a;
            float e=ch.compEnv[b][sd];
            e+=(det-e)*((det>e)?atkC:decC);
            ch.compEnv[b][sd]=yam10_fz(e);
            if (e<1.0e-9f && upR<=1.0f) {
              ch.compGain[b][sd]+=(1.0f-ch.compGain[b][sd])*decC;
              sum+=band[b]*ch.compGain[b][sd]*ch.cBandLin[b];
              continue;
            }
            float lvl=(float)sqrt((double)e);
            float db=20.0f*(float)log10((double)lvl+1.0e-9);
            float outDb=db;
            if (db>thrDb-knee*0.5f && db<thrDb+knee*0.5f) {
              /* the rounded corner, one quadratic across the knee */
              float x=db-(thrDb-knee*0.5f);
              outDb=db-(1.0f-1.0f/dnR)*x*x/(2.0f*knee);
            } else if (db>thrDb) {
              outDb=thrDb+(db-thrDb)/dnR;
            } else if (upR>1.0f) {
              /* a band that is nearly empty is left alone. without this the
                 leakage either side of a crossover gets lifted and summed
                 back in, which is far louder than the signal that caused it */
              float under=thrDb-db;
              if (under>15.0f) under=0.0f;
              
              outDb=db+under*(1.0f-1.0f/upR);
            }
            float want=(float)pow(10.0,(double)(outDb-db)/20.0);
            if (want>2.0f) want=2.0f;      /* never more than 6 dB up */
            ch.compGain[b][sd]+=(want-ch.compGain[b][sd])*decC;
            sum+=band[b]*ch.compGain[b][sd]*ch.cBandLin[b];
          }
          if (sd) dR=sum*makeup*32768.0f; else dL=sum*makeup*32768.0f;
        }
      }

      /* EQ: low shelf, peak, high shelf, the YM2609 chain per channel */
      if (cp.eqEnable) {
        int nb=cp.eqCount;
        if (nb>YAM10_EQ_BANDS) nb=YAM10_EQ_BANDS;
        unsigned char want[1+YAM10_EQ_BANDS*4];
        want[0]=(unsigned char)nb;
        for (int b=0; b<YAM10_EQ_BANDS; b++) {
          want[1+b*4]=cp.eqBand[b].type;
          want[2+b*4]=cp.eqBand[b].freq;
          want[3+b*4]=cp.eqBand[b].gain;
          want[4+b*4]=cp.eqBand[b].q;
        }
        if (!ch.eqValid || memcmp(ch.eqCache,want,sizeof(want))!=0) {
          for (int b=0; b<nb; b++) {
            for (int sd=0; sd<2; sd++) {
              ch.eqB[b][sd].setEQ(cp.eqBand[b].type,yam10_freqTable[cp.eqBand[b].freq],
                                  yam10_qTable[cp.eqBand[b].q],
                                  yam10_gainTable[cp.eqBand[b].gain],(float)rate);
            }
          }
          memcpy(ch.eqCache,want,sizeof(want));
          ch.eqValid=true;
        }
        float e[2]={dL/32768.0f,dR/32768.0f};
        for (int sd=0; sd<2; sd++) {
          for (int b=0; b<nb; b++) {
            if (cp.eqBand[b].on) e[sd]=ch.eqB[b][sd].process(e[sd]);
          }
        }
        dL=e[0]*32768.0f;
        dR=e[1]*32768.0f;
      }

      if (cp.phaseInvL) dL=-dL;
      if (cp.phaseInvR) dR=-dR;

      mixL=dL; mixR=dR;
    }

    /* ---- echo ---- */
    if (cp.echoMix>0) {
      int d=(int)((double)cp.echoDelay*rate/1000.0);
      if (d<1) d=1;
      if (d>=YAM10_MAX_ECHO) d=YAM10_MAX_ECHO-1;
      int rd=ch.echoPos-d; if (rd<0) rd+=YAM10_MAX_ECHO;
      double eL=ch.echoBuf[0][rd], eR=ch.echoBuf[1][rd];
      double fbk=(double)cp.echoFeedback/127.0*0.9;
      ch.echoBuf[0][ch.echoPos]=yam10_fz((float)(mixL+eL*fbk));
      ch.echoBuf[1][ch.echoPos]=yam10_fz((float)(mixR+eR*fbk));
      ch.echoPos=(ch.echoPos+1)%YAM10_MAX_ECHO;
      double m=(double)cp.echoMix/127.0;
      mixL+=eL*m; mixR+=eR*m;
    }

    outL=(float)mixL;
    outR=(float)mixR;

    if (!ch.keyOn && isSilent(c) && fabs(outL)+fabs(outR)<0.5f) ch.quiet++; else ch.quiet=0;
  }
};

#endif
