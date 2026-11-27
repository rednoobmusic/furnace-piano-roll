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

#include "insEditCommon.h"
#include "../../engine/platform/sound/yam10.h"
#include "../intConst.h"
#include "IconsFontAwesome4.h"

#define CENTER_TEXT_20(text) \
  ImGui::SetCursorPosX(ImGui::GetCursorPosX()+0.5*(20.0f*dpiScale-ImGui::CalcTextSize(text).x));

#define TOOLTIP_TEXT(text) \
  if (ImGui::IsItemHovered()) { \
    ImGui::SetTooltip("%s", text); \
  }

const char* yam10OperatorBits[7]={
  "op1", "op2", "op3", "op4", "op5", "op6", NULL
};

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

void FurnaceGUI::insEditYAM10(DivInstrument* ins) {
  std::vector<FurnaceGUIMacroDesc> macroList;
  char label[32];

  drawInsYAM10(ins);
  drawInsYAM10DSP(ins);

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

  for (int i=0; i<6; i++) {
    snprintf(label,31,_("OP%d Macros"),i+1);
    if (ImGui::BeginTabItem(label)) {
      ImGui::PushID(i);
      int ordi=i;
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
      macroList.push_back(FurnaceGUIMacroDesc(FM_NAME(FM_FB),&ins->std.opMacros[ordi].dt2Macro,0,7,64,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Out Level"),&ins->std.opMacros[ordi].egtMacro,0,127,128,uiColors[GUI_COLOR_MACRO_VOLUME]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Mod Input"),&ins->std.opMacros[ordi].kslMacro,0,6,160,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true,yam10OperatorBits));
      macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.opMacros[ordi].amMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Fine Detune"),&ins->std.opMacros[ordi].ssgMacro,-64,63,128,uiColors[GUI_COLOR_MACRO_OTHER]));
      macroList.push_back(FurnaceGUIMacroDesc(_("Delay"),&ins->std.opMacros[ordi].damMacro,0,7,64,uiColors[GUI_COLOR_MACRO_OTHER]));

      drawMacros(macroList,macroEditStateOP[ordi],ins);
      ImGui::PopID();
      ImGui::EndTabItem();
    }
  }

  if (ImGui::BeginTabItem(_("Macros"))) {
    macroList.push_back(FurnaceGUIMacroDesc(_("Volume"),&ins->std.volMacro,0,127,160,uiColors[GUI_COLOR_MACRO_VOLUME]));
    macroList.push_back(FurnaceGUIMacroDesc(_("Arpeggio"),&ins->std.arpMacro,-120,120,160,uiColors[GUI_COLOR_MACRO_PITCH],true,NULL,macroHoverNote,false,NULL,true,ins->std.arpMacro.val));
    macroList.push_back(FurnaceGUIMacroDesc(_("Pitch"),&ins->std.pitchMacro,-2048,2047,160,uiColors[GUI_COLOR_MACRO_PITCH],true,macroRelativeMode));
    macroList.push_back(FurnaceGUIMacroDesc(_("Panning"),&ins->std.panLMacro,0,255,160,uiColors[GUI_COLOR_MACRO_OTHER]));
    macroList.push_back(FurnaceGUIMacroDesc(_("Phase Reset"),&ins->std.phaseResetMacro,0,1,32,uiColors[GUI_COLOR_MACRO_OTHER],false,NULL,NULL,true));

    drawMacros(macroList,macroEditStateMacros,ins);
    ImGui::EndTabItem();
  }
}
