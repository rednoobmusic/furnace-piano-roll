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

#include "gui.h"
#include "guiConst.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <fmt/printf.h>
#include <string.h>
#include <math.h>
#include <vector>

static const bool PR_BLACK_KEY[12]={false,true,false,true,false,false,true,false,true,false,true,false};

static float prSyncScrollX=0.0f;
static bool  prFollow=true;
static bool  prShowPitchSlide=true;
static bool  prShowVolBars=true;
static int   prEffectLane=0;
static bool  prPainting=false;
static bool  prErasing=false;
static bool  prResizing=false;
static int   prResizeRow=-1;
static bool  prSelecting=false;
static int   prSelR0=-1,prSelR1=-1,prSelN0=-1,prSelN1=-1;
static bool  prFxUndoOpen=false;
static bool  prNoteUndoOpen=false;
static float prPanDX=0.0f, prPanDY=0.0f;
static int   prPianoHeld=-1;
static int   prFxLastDragRow=-1;
static bool  prFxSlopeActive=false;
static int   prFxSlopeR0=-1, prFxSlopeR1=-1;
static int   prFxSlopeV0=0,  prFxSlopeV1=0;
static float prFxSlopeTension=0.0f;
static int   prLastNote=96;
static int   prPaintNote=-1;
static int   prQuantize=1;
static int   prScaleRoot=0;
static int   prScaleType=0;
static int   prDragSelStartR=-1;
static int   prDragSelStartN=-1;
static int   prChanEnd=-1;
static bool  prPolyEnabled=false;
static int   prPaintHeld=-1;
static int   prPaintChan=-1;
static int   prPaintLen=1;
static int   prPaintStartRow=-1;
static int   prMode=0;
static bool  prDragMaybe=false;
static bool  prDragging=false;
static int   prDragStartR=-1, prDragStartN=-1;
static int   prDragClickN=-1;
static int   prDragDeltaR=0,  prDragDeltaN=0;
static ImVec2 prDragMouseStart={0,0};
struct PrDragNote { int row, endRow, chan; short note, ins, vol; };
static std::vector<PrDragNote> prDragBuf;
static bool  prDragHasCopy=false;
static int prPreviewTimer=0;
static int prPreviewChan=-1;
static int prFollowPrevPlayOrd=-1;
static int prSnapTargetOrd=-1;
static float prPrevZoom=1.0f;
static bool  prKbdShowConfig=false;
static bool  prColorByIns=false;
static bool  prNoteTooltip=false;
static int   prLoopR0=-1, prLoopR1=-1;
static bool  prLoopDragging=false;
static int   prLoopDragStart=-1;
static bool  prScrollInit=false;
static int   prLastScrollChan=-1;
static float prFollowOffset=0.35f;
static bool  prFollowOffsetDrag=false;
static bool  prWasPlaying=false;
static float prFollowScrollTarget=-1.0f;
static bool  prFxViewAll=false;
static bool  prFxRows=false;
static int   prFxPreviewLast=-1;
static int   prCtxRow=-1, prCtxNote=-1;
static bool  prFxPickerOpen=false;
static int   prFxPickerRow=-1;
static int   prFxPickerEffIdx=0;
static char  prFxPickerSearch[128]="";
struct PrFxEntry { unsigned char code; char label[64]; };
static std::vector<PrFxEntry> prFxPickerList;

struct PrClipEntry { int rowOff; short note; short ins; short vol; };
static std::vector<PrClipEntry> prClipboard;
static int prClipRows=0;

struct PrPolyGroup { int from, to; };
static std::vector<PrPolyGroup> prPolyGroups;
static int prNewGrpFrom=0;
static int prNewGrpTo=0;

static void prPolySerialize(DivSong* s) {
  String& n=s->notes;
  size_t p=n.find("\n<!-- jfpoly:");
  if (p!=String::npos) n=n.substr(0,p);
  if (prPolyGroups.empty()) return;
  n+="\n<!-- jfpoly:";
  for (int i=0;i<(int)prPolyGroups.size();i++) {
    if (i) n+=",";
    n+=fmt::sprintf("%d-%d",prPolyGroups[i].from,prPolyGroups[i].to);
  }
  n+=" -->";
}

static String prPolyMarkerCache;

static String prPolyExtractMarker(const String& notes) {
  size_t p=notes.find("\n<!-- jfpoly:");
  if (p==String::npos) return "";
  size_t e2=notes.find("-->",p);
  if (e2==String::npos) return "";
  return notes.substr(p,e2+3-p);
}

static void prPolyDeserialize(DivSong* s) {
  prPolyGroups.clear();
  const String& n=s->notes;
  size_t p=n.find("\n<!-- jfpoly:");
  if (p==String::npos) return;
  size_t e2=n.find("-->",p);
  if (e2==String::npos) return;
  String data=n.substr(p+13,e2-(p+13));
  data.erase(data.find_last_not_of(" \t")+1);
  const char* c=data.c_str();
  while (*c) {
    int a=-1,b=-1;
    if (sscanf(c,"%d-%d",&a,&b)==2&&a>=0&&b>=a) {
      prPolyGroups.push_back({a,b});
    }
    while (*c&&*c!=',') c++;
    if (*c==',') c++;
  }
}

static void prPolyCommit(DivSong* s) {
  prPolySerialize(s);
  prPolyMarkerCache=prPolyExtractMarker(s->notes);
}

static ImU32 prColorMulAlpha(ImVec4 c,float a) {
  return IM_COL32((int)(c.x*255),(int)(c.y*255),(int)(c.z*255),(int)(c.w*a*255));
}

static ImU32 prColorBrighter(ImVec4 c,float f) {
  return IM_COL32((int)(ImMin(c.x*f,1.0f)*255),(int)(ImMin(c.y*f,1.0f)*255),(int)(ImMin(c.z*f,1.0f)*255),(int)(c.w*255));
}

static int prInferDuration(const DivPattern* pat,int row,int patLen) {
  for (int r=row+1;r<patLen;r++) {
    if (pat->newData[r][DIV_PAT_NOTE]!=-1) return r-row;
  }
  return patLen-row;
}

static ImU32 prChanColor(int chan, int alpha) {
  float h=(float)((chan*137)%360)/360.0f;
  float r2,g2,b2;
  ImGui::ColorConvertHSVtoRGB(h,0.62f,0.84f,r2,g2,b2);
  return IM_COL32((int)(r2*255),(int)(g2*255),(int)(b2*255),alpha);
}

static ImVec4 prInsColor4(int ins) {
  float h=(float)(((ins+1)*97)%360)/360.0f;
  float r2,g2,b2;
  ImGui::ColorConvertHSVtoRGB(h,0.55f,0.88f,r2,g2,b2);
  return ImVec4(r2,g2,b2,1.0f);
}

static bool prIsSpecial(short v) {
  return v==DIV_NOTE_OFF||v==DIV_NOTE_REL||v==DIV_MACRO_REL;
}

static const int PR_SCALE_NOTES[10]={7,7,7,7,7,5,5,7,7,6};
static const int PR_SCALE_IV[10][7]={
  {0,2,4,5,7,9,11},     
  {0,2,3,5,7,8,10},     
  {0,2,3,5,7,9,10},     
  {0,1,3,5,7,8,10},     
  {0,2,4,6,7,9,11},     
  {0,2,4,7,9,0,0},      
  {0,3,5,7,10,0,0},     
  {0,2,3,5,7,8,11},     
  {0,2,4,5,7,8,11},     
  {0,2,4,6,8,10,0},     
};

static bool prScaleHasNote(int pitchClass) {
  if (prScaleType==0) return true;
  int rel=((pitchClass-prScaleRoot)+12)%12;
  int n=PR_SCALE_NOTES[prScaleType-1];
  for (int i=0;i<n;i++) if (PR_SCALE_IV[prScaleType-1][i]==rel) return true;
  return false;
}

static int prSnapScale(int note) {
  if (prScaleType==0) return note;
  const int* iv=PR_SCALE_IV[prScaleType-1];
  int n=PR_SCALE_NOTES[prScaleType-1];
  int best=note, bestDist=127;
  for (int delta=-11;delta<=11;delta++) {
    int cand=note+delta;
    if (cand<0||cand>=180) continue;
    int rel=((cand%12)-prScaleRoot+12)%12;
    for (int i=0;i<n;i++) {
      if (iv[i]==rel) {
        int d=abs(delta);
        if (d<bestDist) { bestDist=d; best=cand; }
        break;
      }
    }
  }
  return best;
}

static float prFxCurve(float t, float tension) {
  if (fabsf(tension)<0.01f) return t;
  float base=powf(10.0f,fabsf(tension));
  if (tension>0) return (powf(base,t)-1.0f)/(base-1.0f);
  return 1.0f-(powf(base,1.0f-t)-1.0f)/(base-1.0f);
}

void FurnaceGUI::drawPianoRoll() {
  if (!pianoRollOpen) return;

  ImGui::SetNextWindowSizeConstraints(ImVec2(420*dpiScale,300*dpiScale),ImVec2(FLT_MAX,FLT_MAX));
  if (!ImGui::Begin("Piano Roll##pianoRoll",&pianoRollOpen,
      ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::End(); return;
  }

  if (settings.prFontScale!=1.0f) ImGui::SetWindowFontScale(settings.prFontScale);
  if (!e->curSubSong) { ImGui::Text("No song loaded."); ImGui::End(); return; }
  int totalChans=e->getTotalChannelCount();
  if (totalChans<=0) { ImGui::Text("No channels."); ImGui::End(); return; }
  if (prChan<0||prChan>=totalChans) prChan=0;
  if (prChanEnd<prChan||prChanEnd>=totalChans) prChanEnd=prChan;
  if (prPolyEnabled&&prChanEnd<=prChan) prPolyEnabled=false;
  if (prChan!=prLastScrollChan) { prLastScrollChan=prChan; prScrollInit=false; }

  {
    String curMarker=prPolyExtractMarker(e->song.notes);
    if (curMarker!=prPolyMarkerCache) {
      prPolyMarkerCache=curMarker;
      prPolyDeserialize(&e->song);
      prPolyEnabled=false;
    }
  }

  {
    String chBtnLbl=fmt::sprintf("Ch %d: %s###prChBtn",prChan+1,e->getChannelName(prChan));
    if (ImGui::Button(chBtnLbl.c_str())) ImGui::OpenPopup("##prChPop");
    if (ImGui::IsItemHovered()) {
      int dw=-(int)ImGui::GetIO().MouseWheel;
      if (dw) { prChan=ImClamp(prChan+dw,0,totalChans-1); prChanEnd=prChan; prPolyEnabled=false; prSelRow0=prSelRow1=-1; }
      ImGui::SetTooltip("Active channel\nClick to pick  |  Scroll to change");
    }
    ImGui::SameLine();
  }

  {
    bool polyHasGrps=!prPolyGroups.empty();
    if (prPolyEnabled) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.12f,0.38f,0.68f,0.9f));
    else if (polyHasGrps) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.18f,0.25f,0.38f,0.8f));
    String polyLbl=prPolyEnabled
      ?fmt::sprintf("Poly: Ch%d-%d###prPolyBtn",prChan+1,prChanEnd+1)
      :"Poly###prPolyBtn";
    if (ImGui::Button(polyLbl.c_str())) ImGui::OpenPopup("##prPolyPop");
    if (prPolyEnabled||polyHasGrps) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
      if (prPolyEnabled) ImGui::SetTooltip("Polyphony ON: Ch%d-Ch%d\nClick to manage groups",prChan+1,prChanEnd+1);
      else if (polyHasGrps) ImGui::SetTooltip("%d poly group(s) defined\nClick to manage",(int)prPolyGroups.size());
      else ImGui::SetTooltip("Polyphonic multi-channel editing\nClick to set up groups");
    }
    ImGui::SameLine();
  }

  if (ImGui::BeginPopup("##prChPop")) {
    static const char* chTypeName[6]={"FM","Pulse","Noise","Wave","PCM","OP"};

    auto isBound=[&](int ch)->int {
      for (int gi=0;gi<(int)prPolyGroups.size();gi++)
        if (ch>=prPolyGroups[gi].from&&ch<=prPolyGroups[gi].to) return gi;
      return -1;
    };

    float btnSz=ImMax(26.0f*(float)dpiScale,ImGui::GetFrameHeight()*1.2f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(3.0f*(float)dpiScale,3.0f*(float)dpiScale));
    int prevChip=-1;
    int inRow=0;
    const int chPerRow=12;

    for (int ch=0;ch<totalChans;ch++) {
      int boundGi=isBound(ch);
      int chip=e->song.dispatchOfChan[ch];
      if (chip!=prevChip) {
        if (prevChip>=0) ImGui::NewLine();
        inRow=0;
        DivSystem sys=e->song.sysOfChan[ch];
        ImGui::TextColored(ImVec4(0.55f,0.78f,1.0f,0.9f),"%s",e->getSystemName(sys));
        prevChip=chip;
      } else if (inRow>0&&inRow%chPerRow==0) {
        ImGui::NewLine();
      } else {
        ImGui::SameLine(0,3.0f*(float)dpiScale);
      }

      int ct=ImClamp(e->getChannelType(ch),0,5);
      ImVec4 col=uiColors[GUI_COLOR_CHANNEL_FM+ct];
      bool isSel=(ch==prChan&&!prPolyEnabled);
      bool inGrp=(boundGi>=0);

      float br=isSel?col.x*0.65f:(inGrp?col.x*0.2f:col.x*0.22f);
      float bg=isSel?col.y*0.65f:(inGrp?col.y*0.2f:col.y*0.22f);
      float bb=isSel?col.z*0.65f:(inGrp?col.z*0.2f:col.z*0.22f);
      float ba=inGrp?0.4f:0.85f;
      ImGui::PushStyleColor(ImGuiCol_Button,    ImVec4(br,bg,bb,ba));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(ImMin(br+0.2f,1.0f),ImMin(bg+0.2f,1.0f),ImMin(bb+0.2f,1.0f),1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImMin(br+0.35f,1.0f),ImMin(bg+0.35f,1.0f),ImMin(bb+0.35f,1.0f),1.0f));

      const char* sn=e->getChannelShortName(ch);
      bool clicked=ImGui::Button(fmt::sprintf("%s##cb%d",sn?sn:"?",ch).c_str(),ImVec2(btnSz,btnSz));
      ImGui::PopStyleColor(3);

      if (isSel) {
        ImVec2 rm=ImGui::GetItemRectMin(), rx=ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(rm,rx,IM_COL32(100,185,255,255),2.0f*(float)dpiScale,0,2.0f);
      }
      if (ImGui::IsItemHovered()) {
        if (inGrp) ImGui::SetTooltip("Ch %d: %s  [%s]\nIn poly group %d",ch+1,e->getChannelName(ch),chTypeName[ct],boundGi+1);
        else       ImGui::SetTooltip("Ch %d: %s  [%s]",ch+1,e->getChannelName(ch),chTypeName[ct]);
      }
      if (clicked) {
        prChan=ch; prChanEnd=ch; prPolyEnabled=false;
        prSelRow0=prSelRow1=-1;
        ImGui::CloseCurrentPopup();
      }
      inRow++;
    }
    ImGui::PopStyleVar();
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::TextDisabled("  ");
    for (int i=0;i<6;i++) {
      ImGui::SameLine(0,5.0f*(float)dpiScale);
      ImGui::TextColored(uiColors[GUI_COLOR_CHANNEL_FM+i],"\xe2\x97\x8f %s",chTypeName[i]);
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("##prPolyPop")) {
    static const char* chTypeName2[6]={"FM","Pulse","Noise","Wave","PCM","OP"};

    auto isBound2=[&](int ch)->int {
      for (int gi=0;gi<(int)prPolyGroups.size();gi++)
        if (ch>=prPolyGroups[gi].from&&ch<=prPolyGroups[gi].to) return gi;
      return -1;
    };

    float btnSz=ImMax(26.0f*(float)dpiScale,ImGui::GetFrameHeight()*1.2f);
    float popW=ImMax(320.0f*(float)dpiScale,ImGui::GetContentRegionAvail().x);
    ImGui::Dummy(ImVec2(popW,0));

    ImGui::TextColored(ImVec4(0.55f,0.78f,1.0f,1.0f),"Polyphonic Groups");
    ImGui::SameLine();
    ImGui::TextDisabled(" — notes spread across voices in a group");
    ImGui::Separator();
    ImGui::Spacing();

    if (prPolyGroups.empty()) {
      ImGui::TextDisabled("No groups. Add one below.");
      ImGui::Spacing();
    } else {
      int toRemove=-1;
      for (int gi=0;gi<(int)prPolyGroups.size();gi++) {
        PrPolyGroup& g=prPolyGroups[gi];
        bool isActive=(prChan==g.from&&prChanEnd==g.to&&prPolyEnabled);
        ImGui::PushID(gi);

        if (isActive) {
          ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.1f,0.42f,0.75f,0.9f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.15f,0.52f,0.88f,1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.2f,0.6f,1.0f,1.0f));
        } else {
          ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.15f,0.2f,0.28f,0.8f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.22f,0.32f,0.45f,1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.28f,0.42f,0.6f,1.0f));
        }
        bool activate=ImGui::Button(isActive?"\xe2\x96\xba ON##act":"\xe2\x96\xba##act",ImVec2(isActive?50.0f*(float)dpiScale:32.0f*(float)dpiScale,btnSz));
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(isActive?"Poly active on this group\nClick to deactivate":"Click to activate this group for polyphony");
        if (activate) {
          if (isActive) { prPolyEnabled=false; }
          else { prChan=g.from; prChanEnd=g.to; prPolyEnabled=true; prSelRow0=prSelRow1=-1; }
        }

        ImGui::SameLine(0,4.0f*(float)dpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(2.0f*(float)dpiScale,2.0f*(float)dpiScale));
        for (int ch=g.from;ch<=g.to;ch++) {
          int ct=ImClamp(e->getChannelType(ch),0,5);
          ImVec4 col=uiColors[GUI_COLOR_CHANNEL_FM+ct];
          ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(col.x*0.4f,col.y*0.4f,col.z*0.4f,0.9f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(col.x*0.6f,col.y*0.6f,col.z*0.6f,1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(col.x*0.75f,col.y*0.75f,col.z*0.75f,1.0f));
          const char* sn=e->getChannelShortName(ch);
          ImGui::Button(fmt::sprintf("%s##pgch%d",sn?sn:"?",ch).c_str(),ImVec2(btnSz,btnSz));
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ch %d: %s  [%s]",ch+1,e->getChannelName(ch),chTypeName2[ct]);
          ImGui::PopStyleColor(3);
          ImGui::SameLine(0,2.0f*(float)dpiScale);
        }
        ImGui::PopStyleVar();
        ImGui::TextDisabled("(%d voices)",g.to-g.from+1);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.4f,0.1f,0.1f,0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.7f,0.15f,0.15f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.85f,0.25f,0.25f,1.0f));
        if (ImGui::SmallButton("Remove##rm")) toRemove=gi;
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove this poly group");
        ImGui::PopID();
        ImGui::Spacing();
      }
      if (toRemove>=0) {
        if (prChan==prPolyGroups[toRemove].from&&prChanEnd==prPolyGroups[toRemove].to)
          { prChanEnd=prChan; prPolyEnabled=false; }
        prPolyGroups.erase(prPolyGroups.begin()+toRemove);
        prPolyCommit(&e->song);
      }
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.55f,0.78f,1.0f,0.85f),"New Group");
    ImGui::Spacing();

    if (prNewGrpFrom<0||prNewGrpFrom>=totalChans) prNewGrpFrom=0;
    if (prNewGrpTo<prNewGrpFrom||prNewGrpTo>=totalChans) prNewGrpTo=prNewGrpFrom;

    ImGui::Text("From:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f*(float)dpiScale);
    if (ImGui::BeginCombo("##ngFrom",fmt::sprintf("Ch %d: %s",prNewGrpFrom+1,e->getChannelName(prNewGrpFrom)).c_str())) {
      for (int ch=0;ch<totalChans;ch++) {
        if (ImGui::Selectable(fmt::sprintf("Ch %d: %s",ch+1,e->getChannelName(ch)).c_str(),ch==prNewGrpFrom)) {
          prNewGrpFrom=ch; if (prNewGrpTo<prNewGrpFrom) prNewGrpTo=prNewGrpFrom;
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text("To:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f*(float)dpiScale);
    if (ImGui::BeginCombo("##ngTo",fmt::sprintf("Ch %d: %s",prNewGrpTo+1,e->getChannelName(prNewGrpTo)).c_str())) {
      for (int ch=prNewGrpFrom;ch<totalChans;ch++) {
        if (ImGui::Selectable(fmt::sprintf("Ch %d: %s",ch+1,e->getChannelName(ch)).c_str(),ch==prNewGrpTo))
          prNewGrpTo=ch;
      }
      ImGui::EndCombo();
    }

    bool canAdd=(prNewGrpTo>prNewGrpFrom);
    String addErr;
    if (canAdd) {
      int refChip=e->song.dispatchOfChan[prNewGrpFrom];
      int refType=e->getChannelType(prNewGrpFrom);
      for (int ch=prNewGrpFrom+1;ch<=prNewGrpTo&&canAdd;ch++) {
        if (e->song.dispatchOfChan[ch]!=refChip) { canAdd=false; addErr="Channels span multiple chips"; }
        else if (e->getChannelType(ch)!=refType) { canAdd=false; addErr="Mixed channel types (e.g. FM + noise)"; }
        else if (isBound2(ch)>=0)                { canAdd=false; addErr=fmt::sprintf("Ch %d already in a group",ch+1); }
      }
      if (isBound2(prNewGrpFrom)>=0) { canAdd=false; addErr=fmt::sprintf("Ch %d already in a group",prNewGrpFrom+1); }
    } else if (prNewGrpTo==prNewGrpFrom) {
      addErr="Need at least 2 channels";
    }

    ImGui::Spacing();
    if (!canAdd) {
      ImGui::TextColored(ImVec4(0.9f,0.5f,0.3f,0.9f),"%s",addErr.empty()?"Select a range above":addErr.c_str());
    } else {
      int voices=prNewGrpTo-prNewGrpFrom+1;
      int ct=ImClamp(e->getChannelType(prNewGrpFrom),0,5);
      ImGui::TextColored(ImVec4(0.5f,0.9f,0.5f,0.9f),"%d %s voices: Ch%d - Ch%d",voices,chTypeName2[ct],prNewGrpFrom+1,prNewGrpTo+1);
    }
    ImGui::Spacing();

    if (!canAdd) ImGui::BeginDisabled();
    if (ImGui::Button("Add Group##prAddGrp")) {
      PrPolyGroup ng={prNewGrpFrom,prNewGrpTo};
      prPolyGroups.push_back(ng);
      prChan=ng.from; prChanEnd=ng.to; prPolyEnabled=true;
      prSelRow0=prSelRow1=-1;
      prPolyCommit(&e->song);
    }
    if (ImGui::IsItemHovered()&&canAdd)
      ImGui::SetTooltip("Add Ch%d-Ch%d as a poly group and activate it",prNewGrpFrom+1,prNewGrpTo+1);
    if (!canAdd) ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextDisabled("Groups are saved automatically with the module.");

    ImGui::EndPopup();
  }
  ImGui::SetNextItemWidth(90*dpiScale);
  ImGui::SliderFloat("W##prZ",&prZoom,0.125f,16.0f,"%.2fx");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Horizontal zoom (scroll here or in grid to zoom)\nRight-click to reset");
    float wh=ImGui::GetIO().MouseWheel;
    if (wh!=0.0f&&!ImGui::GetIO().KeyShift&&!ImGui::GetIO().KeyCtrl) {
      prZoom=ImClamp(prZoom*powf(1.15f,wh),0.125f,16.0f);
    }
  }
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) prZoom=1.0f;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(70*dpiScale);
  ImGui::SliderFloat("H##prH",&prNoteH,2.0f,64.0f,"%.0f");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Note row height in pixels (scroll here to adjust)\nRight-click to reset");
    float wh=ImGui::GetIO().MouseWheel;
    if (wh!=0.0f&&!ImGui::GetIO().KeyShift&&!ImGui::GetIO().KeyCtrl) {
      prNoteH=ImClamp(prNoteH+wh,2.0f,64.0f);
    }
  }
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) prNoteH=8.0f;
  ImGui::SameLine();
  ImGui::Checkbox("All##prAC",&prShowAllChans);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show all other channels as faint ghost notes");
  ImGui::SameLine();
  ImGui::Checkbox("Follow##prFollow",&prFollow);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto-scroll to follow the playhead during playback\nDrag the playhead line left/right to reposition it");
  ImGui::SameLine();
  ImGui::Separator(); ImGui::SameLine();
  {
    static const char* modeLabels[]={"D##prMod","S##prMod","P##prMod"};
    static const char* modeTips[]={"Draw (D): click to place/pick notes","Select (S): click/drag to select; drag selection to move","Paint (P): drag to paint continuously"};
    for (int m=0;m<3;m++) {
      if (m>0) ImGui::SameLine(0,2.0f*(float)dpiScale);
      bool active=(prMode==m);
      if (active) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.18f,0.45f,0.82f,1.0f));
      if (ImGui::SmallButton(modeLabels[m])) prMode=m;
      if (active) ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",modeTips[m]);
    }
  }
  ImGui::SameLine(0,6.0f*(float)dpiScale);
  ImGui::TextDisabled("N:%d",prPaintLen);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Current note paint length: %d row(s)\nClick an existing note to pick up its length",prPaintLen);
  ImGui::SameLine(0,6.0f*(float)dpiScale);
  if (ImGui::SmallButton("\xe2\x9a\x99##prKbdCfg")) prKbdShowConfig=!prKbdShowConfig;
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Piano roll key bindings");
  if (prKbdShowConfig) {
    ImGui::SetNextWindowSize(ImVec2(260.0f*(float)dpiScale,0),ImGuiCond_Appearing);
    if (ImGui::Begin("Piano Roll Keys##prKbdWin",&prKbdShowConfig,ImGuiWindowFlags_NoScrollbar)) {
      ImGui::TextDisabled("Mode keys");
      ImGui::BulletText("D — Draw mode  (click note = select/drag)");
      ImGui::BulletText("S — Select mode");
      ImGui::BulletText("P — Paint mode  (drag = paint continuously)");
      ImGui::Separator();
      ImGui::TextDisabled("Navigation");
      ImGui::BulletText("Ctrl+scroll — zoom in/out");
      ImGui::BulletText("Shift+scroll — scroll horizontally");
      ImGui::BulletText("Middle-drag — pan");
      ImGui::Separator();
      ImGui::TextDisabled("Selection");
      ImGui::BulletText("Shift+drag — box select (any mode)");
      ImGui::BulletText("Shift+\xe2\x86\x91\xe2\x86\x93 — transpose \xc2\xb1" "1 semitone");
      ImGui::BulletText("Ctrl+Shift+\xe2\x86\x91\xe2\x86\x93 — transpose \xc2\xb1 octave");
      ImGui::BulletText("Shift+\xe2\x86\x90\xe2\x86\x92 — move selection \xc2\xb1" "1 row");
      ImGui::BulletText("Ctrl+A — select all");
      ImGui::BulletText("Ctrl+C / Ctrl+X / Ctrl+V — copy / cut / paste");
      ImGui::BulletText("Delete — erase selection (clears all row data)");
      ImGui::Separator();
      ImGui::TextDisabled("Editing");
      ImGui::BulletText("Right-click — context menu (any mode)");
      ImGui::BulletText("Right-click drag on empty — erase strip");
      ImGui::BulletText("Drag note right edge — resize (left = shrink)");
    }
    ImGui::End();
  }
  ImGui::SameLine();
  ImGui::Separator();
  ImGui::SameLine();
  {
    static const char* scaleTypeNames2[]={"Scale: Off","Major","Minor","Dorian","Phrygian","Lydian","Maj Penta","Min Penta","Harm Minor","Harm Major","Whole Tone"};
    static const char* rootNames2[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    ImGui::SetNextItemWidth(104*dpiScale);
    if (ImGui::BeginCombo("##scType",scaleTypeNames2[prScaleType])) {
      for (int i=0;i<11;i++) {
        if (ImGui::Selectable(scaleTypeNames2[i],prScaleType==i)) prScaleType=i;
        if (prScaleType==i) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap placed notes to a musical scale\nOff = free placement");
    if (prScaleType>0) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(52*dpiScale);
      if (ImGui::BeginCombo("##scRoot",rootNames2[prScaleRoot])) {
        for (int i=0;i<12;i++) {
          bool sel=(prScaleRoot==i);
          if (ImGui::Selectable(rootNames2[i],sel)) prScaleRoot=i;
          if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Root note of the selected scale");
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("View\xef\x83\x83##prView")) ImGui::OpenPopup("prViewMenu");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display and quantize options");
  if (ImGui::BeginPopup("prViewMenu")) {
    ImGui::Checkbox("Pitch Slides##pvPS",&prShowPitchSlide);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw lines showing pitch slide effects (01/02/03)");
    ImGui::Checkbox("Volume Bars##pvVB",&prShowVolBars);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show volume as a bar at the bottom of each note");
    ImGui::Checkbox("Color by Instrument##pvCI",&prColorByIns);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tint notes by their instrument index");
    ImGui::Checkbox("Note Tooltip##pvNT",&prNoteTooltip);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show note name + instrument on hover");
    ImGui::Separator();
    ImGui::Checkbox("FX Overlay##pvFXO",&prFxViewAll);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show all FX lanes as faint ghost bars behind the active lane");
    ImGui::Checkbox("FX Rows##pvFXR",&prFxRows);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show each FX lane as a separate editable row (replaces bar chart)");
    ImGui::EndPopup();
  }

  float availH=ImGui::GetContentRegionAvail().y;
  float availW=ImGui::GetContentRegionAvail().x;

  const float noteH=ImMax(prNoteH*(float)dpiScale,2.0f);
  const float rowW=ImMax(12.0f*(float)dpiScale*prZoom,1.0f);
  const int   NOTES=180;
  const float pianoW=56.0f*(float)dpiScale;
  const float timelineH=ImMax(prTimelineH*(float)dpiScale,14.0f*(float)dpiScale);
  const float splitterH=6.0f*(float)dpiScale;
  const float effectLaneH=ImMax(prEffectLaneH*(float)dpiScale,40.0f*(float)dpiScale);
  const float fxRowH=ImGui::GetFrameHeightWithSpacing();
  const float noteAreaH=ImMax(availH-timelineH-splitterH*2-effectLaneH-fxRowH*1.2f,60.0f*(float)dpiScale);
  const float noteAreaW=availW;
  const float totalH=NOTES*noteH;

  if (e->isPlaying()&&playOrder>=0&&playOrder<e->curSubSong->ordersLen)
    curOrder=playOrder;

  int ord=curOrder;
  if (ord<0) ord=0;
  if (ord>=e->curSubSong->ordersLen) ord=e->curSubSong->ordersLen-1;
  if (ord<0) { ImGui::Text("No orders."); ImGui::End(); return; }

  if (prPreviewTimer>0) {
    prPreviewTimer--;
    if (prPreviewTimer==0&&prPreviewChan>=0) { e->noteOff(prPreviewChan); prPreviewChan=-1; }
  }

  int patIdx=e->curSubSong->orders.ord[prChan][ord];
  DivPattern* pat=e->curPat[prChan].getPattern(patIdx,true);
  if (!pat) { ImGui::Text("Pattern unavailable."); ImGui::End(); return; }

  int patLen=e->curSubSong->patLen;
  if (patLen<=0) { ImGui::Text("Pattern is empty."); ImGui::End(); return; }
  int effectCols=ImMax((int)e->curPat[prChan].effectCols,1);
  int volMax=e->getMaxVolumeChan(prChan);
  if (volMax<=0) volMax=0xff;

  const float totalW=(float)patLen*rowW;

  int ordersLen=e->curSubSong->ordersLen;
  const float allW=totalW*(float)ordersLen;

  int selR0=ImMin(prSelRow0,prSelRow1), selR1=ImMax(prSelRow0,prSelRow1);
  int selN0=ImMin(prSelN0,prSelN1),    selN1=ImMax(prSelN0,prSelN1);
  bool hasSel=(prSelRow0>=0&&prSelRow1>=0&&prSelN0>=0&&prSelN1>=0);


  ImU32 cKeyW    =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_KEY_WHITE]);
  ImU32 cKeyB    =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_KEY_BLACK]);
  ImU32 cKeyBrd  =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_KEY_BORDER]);
  ImU32 cGrid    =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_GRID]);
  ImU32 cGridHi1 =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_GRID_HI1]);
  ImU32 cGridHi2 =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_GRID_HI2]);
  ImVec4 cNote4  =uiColors[GUI_COLOR_PIANO_ROLL_NOTE];
  ImU32 cSel     =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_SELECTION]);
  ImU32 cHead    =ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PATTERN_PLAY_HEAD]);
  int hiA=ImMax((int)e->curSubSong->hilightA,1);
  int hiB=ImMax((int)e->curSubSong->hilightB,1);

  bool isPlaying=e->isPlaying();
  if (isPlaying&&!prWasPlaying) {

    float absX=pianoW+(float)ord*totalW+oldRow*rowW;
    prSyncScrollX=ImMax(absX-noteAreaW*prFollowOffset,0.0f);
    prFollowScrollTarget=prSyncScrollX;
  }
  prWasPlaying=isPlaying;

  if (isPlaying) {
    if (prFollow) {
      float absX=pianoW+(float)ord*totalW+oldRow*rowW;
      float target=ImMax(absX-noteAreaW*prFollowOffset,0.0f);


      if (prFollowScrollTarget<0.0f||fabsf(target-prFollowScrollTarget)>=rowW*0.5f) {
        prFollowScrollTarget=target;
        prSyncScrollX=target;
      }
      prSnapTargetOrd=-1;
    } else {
      bool orderChanged=(prFollowPrevPlayOrd<0||playOrder!=prFollowPrevPlayOrd);
      bool zoomChanged=(prZoom!=prPrevZoom);
      if (orderChanged||zoomChanged) {
        if (orderChanged&&prZoom<1.0f) prZoom=1.0f;
        prSnapTargetOrd=ord;
      }
      prFollowScrollTarget=-1.0f;
    }
    prFollowPrevPlayOrd=playOrder;
  } else {
    prFollowPrevPlayOrd=-1;
    prSnapTargetOrd=-1;
    prFollowScrollTarget=-1.0f;
  }
  if (prZoom!=prPrevZoom&&prPrevZoom>0.0f&&!e->isPlaying()) {
    float rowW_old=ImMax(12.0f*(float)dpiScale*prPrevZoom,1.0f);
    float rowW_new=ImMax(12.0f*(float)dpiScale*prZoom,1.0f);
    float seekPxOld=pianoW+(float)ord*(float)patLen*rowW_old+(float)cursor.y*rowW_old;
    float seekPxNew=pianoW+(float)ord*(float)patLen*rowW_new+(float)cursor.y*rowW_new;
    prSyncScrollX=ImMax(seekPxNew-(seekPxOld-prSyncScrollX),0.0f);
  }
  prPrevZoom=prZoom;
  if (prSnapTargetOrd>=0) {
    prSyncScrollX=(float)prSnapTargetOrd*totalW;
    prSnapTargetOrd=-1;
  }

  if (!e->isPlaying()||!prFollow) {
    if (prPanDX!=0.0f) {
      prSyncScrollX=ImMax(prSyncScrollX+prPanDX,0.0f);
    }
    float hDelta=ImGui::GetIO().MouseWheelH;
    if (ImGui::GetIO().KeyShift&&!ImGui::GetIO().KeyCtrl) {
      hDelta-=ImGui::GetIO().MouseWheel;
    }
    if (hDelta!=0.0f) {
      prSyncScrollX=ImMax(prSyncScrollX-hDelta*rowW*3.0f,0.0f);
    }
    if (ImGui::GetIO().KeyCtrl&&!ImGui::GetIO().KeyShift) {
      float wh=ImGui::GetIO().MouseWheel;
      if (wh!=0.0f) {
        float oldZoom=prZoom;
        prZoom=ImClamp(prZoom*powf(1.18f,wh),0.125f,16.0f);
        float rowW_old=ImMax(12.0f*(float)dpiScale*oldZoom,1.0f);
        float rowW_new=ImMax(12.0f*(float)dpiScale*prZoom,1.0f);
        float seekPxOld=pianoW+(float)ord*(float)patLen*rowW_old+(float)cursor.y*rowW_old;
        float seekPxNew=pianoW+(float)ord*(float)patLen*rowW_new+(float)cursor.y*rowW_new;
        prSyncScrollX=ImMax(seekPxNew-(seekPxOld-prSyncScrollX),0.0f);
      }
    }
  }
  prPanDX=0.0f;
  prSyncScrollX=ImClamp(prSyncScrollX,0.0f,ImMax(allW-1.0f,0.0f));

  ImGui::SetNextWindowContentSize(ImVec2(pianoW+allW,timelineH));
  if (ImGui::BeginChild("##prTL",ImVec2(noteAreaW,timelineH),false,
      ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)) {
    if (settings.prFontScale!=1.0f) ImGui::SetWindowFontScale(settings.prFontScale);
    ImGui::SetScrollX(prSyncScrollX);
    float tlSX=ImGui::GetScrollX();
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 twp=ImGui::GetWindowPos();
    float ox=twp.x-tlSX;
    float vx0=twp.x, vx1=twp.x+noteAreaW;
    dl->AddRectFilled(twp,ImVec2(twp.x+noteAreaW,twp.y+timelineH),
      prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],1.5f));
    dl->AddRectFilled(twp,ImVec2(twp.x+pianoW,twp.y+timelineH),
      prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],2.0f));
    {
      char maxLbl[12]; snprintf(maxLbl,sizeof(maxLbl),"%d",patLen-1);
      float maxLblW=ImGui::CalcTextSize(maxLbl).x+3;
      int lblStep=1;
      while (lblStep*rowW<maxLblW&&lblStep<patLen) lblStep++;
      if (lblStep>1&&hiA>1) lblStep=((lblStep+hiA-1)/hiA)*hiA;
      for (int vi=0;vi<ordersLen;vi++) {
        float oBase=ox+pianoW+vi*totalW;
        if (oBase+totalW<vx0||oBase>vx1) continue;
        bool isCur=(vi==ord);
        for (int r=0;r<patLen;r++) {
          float rx=oBase+r*rowW;
          if (rx+rowW<vx0||rx>vx1) continue;
          ImU32 gc=(r%hiB==0)?cGridHi2:(r%hiA==0)?cGridHi1:cGrid;
          if (!isCur) { ImU32 a=(gc>>24)&0xFF; gc=(gc&0x00FFFFFF)|((ImU32)(a*0.5f)<<24); }
          dl->AddLine(ImVec2(rx,twp.y),ImVec2(rx,twp.y+timelineH),gc);
          if (isCur&&(r%lblStep==0)&&rx>=twp.x+pianoW-1)
            dl->AddText(ImVec2(rx+2,twp.y+1),IM_COL32(180,180,180,200),fmt::sprintf("%d",r).c_str());
        }
        dl->AddLine(ImVec2(oBase,twp.y),ImVec2(oBase,twp.y+timelineH),IM_COL32(255,255,255,isCur?80:40),2.0f);
        String olbl=fmt::sprintf("ORD %02X",vi);
        ImVec2 osz=ImGui::CalcTextSize(olbl.c_str());
        float olx=oBase+4, oly=twp.y+2;
        dl->AddRectFilled(ImVec2(olx-1,oly-1),ImVec2(olx+osz.x+2,oly+osz.y+1),
          prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],2.0f));
        dl->AddText(ImVec2(olx,oly),isCur?IM_COL32(200,200,200,230):IM_COL32(140,140,140,160),olbl.c_str());
      }
    }
    if (e->isPlaying()) {
      float phx=ox+pianoW+(float)playOrder*totalW+oldRow*rowW;
      dl->AddLine(ImVec2(phx,twp.y),ImVec2(phx,twp.y+timelineH),cHead,2.0f);
    }
    {
      float cxl=ox+pianoW+(float)ord*totalW+cursor.y*rowW;
      dl->AddRectFilled(ImVec2(cxl,twp.y),ImVec2(cxl+2,twp.y+timelineH),IM_COL32(255,255,100,200));
    }
    dl->AddLine(ImVec2(twp.x,twp.y+timelineH-1),ImVec2(twp.x+noteAreaW,twp.y+timelineH-1),cKeyBrd);
    ImGui::SetCursorPos(ImVec2(0,0));
    ImGui::InvisibleButton("##prSeek",ImVec2(pianoW+allW,timelineH));
    if (ImGui::IsItemHovered()||ImGui::IsItemActive()) {
      float lx=ImGui::GetMousePos().x-twp.x+tlSX;
      if (lx>=pianoW) {
        int absRow=(int)((lx-pianoW)/rowW);
        int seekOrd=ImClamp(absRow/patLen,0,ordersLen-1);
        int seekRow=ImClamp(absRow%patLen,0,patLen-1);
        int absRowClamped=ImClamp(absRow,0,(int)(ordersLen*patLen)-1);
        if (ImGui::GetIO().KeyShift) {
          if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            prLoopDragging=true;
            prLoopDragStart=absRowClamped;
            prLoopR0=absRowClamped; prLoopR1=absRowClamped;
          }
          if (prLoopDragging&&ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            prLoopR0=ImMin(prLoopDragStart,absRowClamped);
            prLoopR1=ImMax(prLoopDragStart,absRowClamped);
          }
        } else {
          ImGui::SetTooltip("Ord %02X  Row %d",seekOrd,seekRow);
          if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            prLoopDragging=false;
            cursor.y=seekRow;
            curOrder=seekOrd;
            e->seekTo((unsigned char)seekOrd,seekRow);
          }
        }
      }
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) prLoopDragging=false;
    if (prLoopR0>=0&&prLoopR1>=0&&prLoopR0<=prLoopR1) {
      float lx0=ox+pianoW+prLoopR0*rowW;
      float lx1=ox+pianoW+(prLoopR1+1)*rowW;
      lx0=ImMax(lx0,twp.x+pianoW); lx1=ImMin(lx1,twp.x+noteAreaW);
      if (lx1>lx0) {
        dl->AddRectFilled(ImVec2(lx0,twp.y),ImVec2(lx1,twp.y+timelineH),IM_COL32(255,200,50,55));
        dl->AddRect(ImVec2(lx0,twp.y),ImVec2(lx1,twp.y+timelineH-1),IM_COL32(255,200,50,180),0.0f,0,1.5f);
      }
    }
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)&&ImGui::IsItemHovered()&&!ImGui::GetIO().KeyShift) {
      prLoopR0=-1; prLoopR1=-1;
    }
  }
  ImGui::EndChild();

  {
    ImGui::InvisibleButton("##prTLSplit",ImVec2(noteAreaW,splitterH));
    bool th=ImGui::IsItemHovered(), ta=ImGui::IsItemActive();
    if (th||ta) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ta) {
      prTimelineH+=ImGui::GetIO().MouseDelta.y/(float)dpiScale;
      prTimelineH=ImClamp(prTimelineH,14.0f,60.0f);
    }
    ImDrawList* tsdl=ImGui::GetWindowDrawList();
    ImVec2 tsmin=ImGui::GetItemRectMin(), tsmax=ImGui::GetItemRectMax();
    float tsmid=tsmin.y+(tsmax.y-tsmin.y)*0.5f;
    tsdl->AddRectFilled(tsmin,tsmax,prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],ta?2.0f:1.6f));
    tsdl->AddLine(ImVec2(tsmin.x+4,tsmid),ImVec2(tsmax.x-4,tsmid),cGridHi1,1.0f);
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,0.0f);
  ImGui::SetNextWindowContentSize(ImVec2(pianoW+allW,totalH));
  bool gridOpen=ImGui::BeginChild("##prGrid",ImVec2(noteAreaW,noteAreaH),false,
    ImGuiWindowFlags_HorizontalScrollbar);

  if (gridOpen) {
    if (settings.prFontScale!=1.0f) ImGui::SetWindowFontScale(settings.prFontScale);
    ImGui::SetScrollX(prSyncScrollX);
    if (!prScrollInit) {
      prScrollInit=true;

      float c4Y=71.0f*noteH;
      ImGui::SetScrollY(ImMax(c4Y-noteAreaH*0.5f,0.0f));
    }
    if (prPanDY!=0) {
      ImGui::SetScrollY(ImGui::GetScrollY()+prPanDY);
      prPanDY=0;
    }
    float curScrollX=ImGui::GetScrollX();
    float scrollY=ImGui::GetScrollY();
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 wp=ImGui::GetWindowPos();
    float ox=wp.x-curScrollX;
    float oy=wp.y-scrollY;
    float vx0=wp.x,vx1=wp.x+ImGui::GetWindowWidth();
    float vy0=wp.y,vy1=wp.y+ImGui::GetWindowHeight();

    ImVec2 fsz=ImGui::CalcTextSize("C-5");
    ImVec4 gv=uiColors[GUI_COLOR_PIANO_ROLL_GRID];
    ImU32 cGridFaint=IM_COL32((int)(gv.x*255),(int)(gv.y*255),(int)(gv.z*255),55);
    int qstep=(prQuantize>1)?ImMax(patLen/prQuantize,1):0;

    for (int vi=0;vi<ordersLen;vi++) {
      float oBase=ox+pianoW+vi*totalW;
      if (oBase+totalW<vx0||oBase>vx1) continue;
      bool isCur=(vi==ord);
      float dimFactor=isCur?1.0f:0.7f;
      int alphaScale=isCur?255:140;

      int viPatIdx=e->curSubSong->orders.ord[prChan][vi];
      DivPattern* viPat=e->curPat[prChan].getPattern(viPatIdx,isCur);
      if (!viPat) continue;

      for (int n=0;n<NOTES;n++) {
        int dn=NOTES-1-n;
        float ry0=oy+n*noteH,ry1=ry0+noteH;
        if (ry1<vy0||ry0>vy1) continue;
        bool blk=PR_BLACK_KEY[dn%12];
        ImVec4 bgV=uiColors[GUI_COLOR_PIANO_ROLL_BG];
        float dim=blk?(0.72f*dimFactor):(1.0f*dimFactor);
        dl->AddRectFilled(ImVec2(oBase,ry0),ImVec2(oBase+totalW,ry1),
          IM_COL32((int)(bgV.x*dim*255),(int)(bgV.y*dim*255),(int)(bgV.z*dim*255),255));
        if (prScaleType>0&&prScaleHasNote(dn%12))
          dl->AddRectFilled(ImVec2(oBase,ry0),ImVec2(oBase+totalW,ry1),IM_COL32(100,180,255,28));
        dl->AddLine(ImVec2(oBase,ry1),ImVec2(oBase+totalW,ry1),
          (dn%12==0)?cGridHi1:cGrid);
      }

      for (int r=0;r<=patLen;r++) {
        float cx=oBase+r*rowW;
        if (cx<vx0-rowW||cx>vx1+rowW) continue;
        if (isCur) {
          bool isQLine=(qstep>0&&r%qstep==0);
          if (isQLine)       dl->AddLine(ImVec2(cx,oy),ImVec2(cx,oy+totalH),cGridHi2,1.5f);
          else if (r%hiB==0) dl->AddLine(ImVec2(cx,oy),ImVec2(cx,oy+totalH),cGridHi2);
          else if (r%hiA==0) dl->AddLine(ImVec2(cx,oy),ImVec2(cx,oy+totalH),cGridHi1);
          else               dl->AddLine(ImVec2(cx,oy),ImVec2(cx,oy+totalH),cGridFaint);
        } else {
          ImU32 gc=(r%hiB==0)?cGridHi2:(r%hiA==0)?cGridHi1:cGrid;
          ImU32 a=(gc>>24)&0xFF;
          dl->AddLine(ImVec2(cx,oy),ImVec2(cx,oy+totalH),(gc&0x00FFFFFF)|((ImU32)(a*0.4f)<<24));
        }
      }

      dl->AddLine(ImVec2(oBase,oy),ImVec2(oBase,oy+totalH),IM_COL32(255,255,255,isCur?80:40),2.0f);

      if (prShowAllChans) {
        for (int ch=0;ch<totalChans;ch++) {
          if (ch==prChan) continue;
          if (prPolyEnabled&&ch>=prChan&&ch<=prChanEnd) continue;
          if (e->isChannelMuted(ch)) continue;
          int cpIdx=e->curSubSong->orders.ord[ch][vi];
          DivPattern* cp=e->curPat[ch].getPattern(cpIdx,false);
          if (!cp) continue;
          int gA=isCur?120:60;
          int gFA=isCur?25:12;
          ImU32 gOutline=prChanColor(ch,gA);
          ImU32 gFill=prChanColor(ch,gFA);
          for (int r=0;r<patLen;r++) {
            short nv=cp->newData[r][DIV_PAT_NOTE];
            if (nv<0||nv>=NOTES||prIsSpecial(nv)) continue;
            int dur=prInferDuration(cp,r,patLen);
            float nx0=oBase+r*rowW+1;
            float ny0=oy+(NOTES-1-nv)*noteH+1;
            float nx1=oBase+(r+dur)*rowW-1;
            float ny1=ny0+noteH-2;
            if (nx1<vx0||nx0>vx1||ny1<vy0||ny0>vy1) continue;
            dl->AddRectFilled(ImVec2(nx0,ny0),ImVec2(nx1,ny1),gFill);
            dl->AddRect(ImVec2(nx0,ny0),ImVec2(nx1,ny1),gOutline,0.0f,0,1.2f);
          }
        }
      }

      if (isCur&&prPolyEnabled&&prChanEnd>prChan) {
        for (int ch=prChan+1;ch<=prChanEnd;ch++) {
          if (e->isChannelMuted(ch)) continue;
          int cpIdx=e->curSubSong->orders.ord[ch][vi];
          DivPattern* cp=e->curPat[ch].getPattern(cpIdx,false);
          if (!cp) continue;
          ImU32 cTop=prChanColor(ch,200);
          ImU32 cBot=prChanColor(ch,130);
          ImU32 cBdr=prChanColor(ch,80);
          for (int r=0;r<patLen;r++) {
            short nv=cp->newData[r][DIV_PAT_NOTE];
            if (nv<0||nv>=NOTES||prIsSpecial(nv)) continue;
            int dur=prInferDuration(cp,r,patLen);
            float nx0=oBase+r*rowW+1;
            float ny0=oy+(NOTES-1-nv)*noteH+1;
            float nx1=oBase+(r+dur)*rowW-1;
            float ny1=ny0+noteH-2;
            if (nx1<vx0||nx0>vx1||ny1<vy0||ny0>vy1) continue;
            dl->AddRectFilledMultiColor(ImVec2(nx0,ny0),ImVec2(nx1,ny1),cTop,cTop,cBot,cBot);
            dl->AddRect(ImVec2(nx0,ny0),ImVec2(nx1,ny1),cBdr,1.5f);
          }
        }
      }

      for (int r=0;r<patLen;r++) {
        short nv=viPat->newData[r][DIV_PAT_NOTE];
        if (nv==-1) continue;
        float rx0=oBase+r*rowW;
        float rx1=oBase+(r+1)*rowW;

        if (prIsSpecial(nv)) {
          if (rx0<vx0-rowW||rx0>vx1) continue;
          if (isCur) {
            ImVec4 mc4=(nv==DIV_NOTE_OFF)?uiColors[GUI_COLOR_PIANO_ROLL_NOTE_OFF]:uiColors[GUI_COLOR_PIANO_ROLL_NOTE_REL];
            ImU32 mc=ImGui::ColorConvertFloat4ToU32(mc4);
            ImU32 mcFill=IM_COL32((int)(mc4.x*255),(int)(mc4.y*255),(int)(mc4.z*255),55);
            dl->AddRectFilled(ImVec2(rx0+1,oy),ImVec2(ImMin(rx0+rowW*0.35f,rx1),oy+totalH),mcFill);
            dl->AddLine(ImVec2(rx0,oy),ImVec2(rx0,oy+totalH),mc,2.0f);
            const char* lbl=(nv==DIV_NOTE_OFF)?"OFF":(nv==DIV_NOTE_REL)?"===":"REL";
            if (noteH>fsz.y*0.8f) dl->AddText(ImVec2(rx0+2,oy+2),mc,lbl);
          }
          continue;
        }
        if (nv<0||nv>=NOTES) continue;

        int dur=prInferDuration(viPat,r,patLen);
        float nx0=oBase+r*rowW+1;
        float ny0=oy+(NOTES-1-nv)*noteH+1;
        float nx1=oBase+(r+dur)*rowW-1;
        float ny1=ny0+noteH-2;
        if (nx1<vx0||nx0>vx1||ny1<vy0||ny0>vy1) continue;

        bool inSel=isCur&&hasSel&&r>=selR0&&r<=selR1&&nv>=selN0&&nv<=selN1;
        float bf=(0.65f+0.35f*((float)(nv/12)/14.0f))*(inSel?1.5f:1.0f);
        int noteAlpha=isCur?255:alphaScale;
        ImVec4 noteColor4=cNote4;
        if (prColorByIns) {
          short ins=viPat->newData[r][DIV_PAT_INS];
          if (ins>=0) noteColor4=prInsColor4(ins);
        }
        ImU32 ncTop=isCur?prColorBrighter(noteColor4,bf*1.18f):prColorMulAlpha(noteColor4,bf*1.18f*(float)noteAlpha/255.0f);
        ImU32 ncBot=isCur?prColorBrighter(noteColor4,bf*0.72f):prColorMulAlpha(noteColor4,bf*0.72f*(float)noteAlpha/255.0f);
        dl->AddRectFilledMultiColor(ImVec2(nx0,ny0),ImVec2(nx1,ny1),ncTop,ncTop,ncBot,ncBot);
        dl->AddRect(ImVec2(nx0,ny0),ImVec2(nx1,ny1),IM_COL32(255,255,255,inSel?90:(isCur?38:20)),1.5f);

        if (isCur&&nx1-nx0>8) {
          float hx=nx1-4*(float)dpiScale;
          dl->AddRectFilled(ImVec2(hx,ny0),ImVec2(nx1,ny1),IM_COL32(255,255,255,55),1.0f);
        }

        if (ny1-ny0>fsz.y*0.5f) {
          dl->PushClipRect(ImVec2(nx0,ny0),ImVec2(nx1,ny1),true);
          float lw=nx1-nx0-6;
          if (lw>4) {
            char lbl[20];
            if (isCur) {
              short ins=viPat->newData[r][DIV_PAT_INS];
              if (ins>=0) snprintf(lbl,sizeof(lbl),"%s i%02X",noteNames[nv],(unsigned)ins);
              else        snprintf(lbl,sizeof(lbl),"%s",noteNames[nv]);
            } else {
              snprintf(lbl,sizeof(lbl),"%s",noteNames[nv]);
            }
            ImVec2 tsz=ImGui::CalcTextSize(lbl);
            if (tsz.x>lw) { snprintf(lbl,sizeof(lbl),"%s",noteNames[nv]); tsz=ImGui::CalcTextSize(lbl); }
            if (tsz.x<=lw) {
              float ty=ny0+(ny1-ny0-tsz.y)*0.5f;
              dl->AddText(ImVec2(nx0+3,ty),IM_COL32(255,255,255,isCur?210:100),lbl);
            }
          }
          dl->PopClipRect();
        }

        if (isCur&&prShowVolBars) {
          short vol=viPat->newData[r][DIV_PAT_VOL];
          if (vol>=0&&volMax>0) {
            float vf=ImClamp((float)vol/(float)volMax,0.0f,1.0f);
            float bh=ImMax((ny1-ny0)*0.18f,2.0f*(float)dpiScale);
            float bw=(nx1-nx0)*vf;
            dl->AddRectFilled(ImVec2(nx0,ny1-bh),ImVec2(nx0+bw,ny1),IM_COL32(255,255,255,120));
          }
        }
      }
    }

    if (prShowPitchSlide) {
      int ec2=ImMax((int)e->curPat[prChan].effectCols,1);
      bool linPitch=(e->song.compatFlags.linearPitch>0);
      float slideSpeedMul=linPitch?(float)ImMax((int)e->song.compatFlags.pitchSlideSpeed,1):1.0f;
      bool skipFirstTick=(bool)e->song.compatFlags.noSlidesOnFirstTick;
      const float unitsPerSemitone=linPitch?128.0f:64.0f;
      const DivGroovePattern& groove=e->getSpeeds();
      int gc=0;
      for (int gi=0;gi<16;gi++) { if (groove.val[gi]==0) break; gc++; }
      if (gc==0) gc=1;

      int globalRow=0;

      auto getPatForOrd=[&](int vi) -> DivPattern* {
        int pidx=e->curSubSong->orders.ord[prChan][vi];
        return e->curPat[prChan].getPattern(pidx,vi==ord);
      };

      auto findPortaTarget=[&](int vi,int startRow) -> int {
        for (int si=vi;si<ordersLen;si++) {
          DivPattern* sp=getPatForOrd(si);
          if (!sp) break;
          int r0=(si==vi)?startRow:0;
          for (int rr=r0;rr<patLen;rr++) {
            short nn=sp->newData[rr][DIV_PAT_NOTE];
            if (nn>=0&&nn<NOTES&&!prIsSpecial(nn)) return (int)nn;
            if (nn>=0&&prIsSpecial(nn)) return -1;
          }
        }
        return -1;
      };

      int dispIdx=e->song.dispatchOfChan[prChan];
      DivDispatch* disp=(dispIdx>=0)?e->getDispatch(dispIdx):nullptr;
      double chipClock=disp?(double)disp->chipClock:3579545.0;

      float accumPitch=0.0f;
      float activeSlideSpeed=0.0f;
      bool slideIsShorthand=false;
      int activePortaTarget=-1;
      float activePortaSpeed=0.0f;
      int activeNote=-1;
      bool legatoOn=false;
      int legatoDelayCtr=-1;
      int legatoDelayTarget=0;
      std::vector<ImVec2> allPts;
      std::vector<bool> allBreaks;
      std::vector<ImU32> allCols;

      auto pushPt=[&](float x,float y,bool brk,ImU32 col) {
        allPts.push_back(ImVec2(x,y));
        allBreaks.push_back(brk);
        allCols.push_back(col);
      };

      auto getCol=[&](int note) -> ImU32 {
        float bf=0.65f+0.35f*((float)(note/12)/14.0f);
        return prColorBrighter(cNote4,bf*1.7f);
      };

      auto noteY=[&](int note,float acc) -> float {
        float semiOff;
        if (linPitch) {
          semiOff=acc/unitsPerSemitone;
        } else {
          double baseP=e->calcBaseFreq(chipClock,1.0,note,true);
          double slidP=baseP-acc;
          if (slidP>0.0) semiOff=(float)(log2(baseP/slidP)*12.0);
          else semiOff=(float)(NOTES-1);
        }
        semiOff=ImClamp(semiOff,-(float)(NOTES-1),(float)(NOTES-1));
        float y=oy+(NOTES-1-note)*noteH+noteH*0.5f-semiOff*noteH;
        return ImClamp(y,oy,oy+(float)NOTES*noteH);
      };

      for (int vi=0;vi<ordersLen;vi++) {
        DivPattern* vp=getPatForOrd(vi);
        if (!vp) { globalRow+=patLen; continue; }
        float oBase=ox+pianoW+vi*totalW;

        for (int r=0;r<patLen;r++,globalRow++) {
          short nv=vp->newData[r][DIV_PAT_NOTE];
          bool hasNote=(nv>=0&&nv<NOTES&&!prIsSpecial(nv));
          bool noteOff=(nv>=0&&prIsSpecial(nv));
          int tpr=(int)groove.val[globalRow%gc];
          if (tpr<1) tpr=6;

          if (slideIsShorthand) { activeSlideSpeed=0.0f; slideIsShorthand=false; }

          bool rowHasPorta=false;
          for (int ei=0;ei<ec2;ei++) {
            short fx=vp->newData[r][DIV_PAT_FX(ei)];
            if (fx==0x03) { rowHasPorta=true; break; }
          }

          if (hasNote) {
            if (rowHasPorta&&activeNote>=0) {
              activePortaTarget=(int)nv;
            } else if (legatoOn&&activeNote>=0) {
              float cx=oBase+r*rowW;
              pushPt(cx,noteY(activeNote,accumPitch),false,getCol(activeNote));
              activeNote=(int)nv;
              accumPitch=0.0f;
              pushPt(cx,noteY(activeNote,0.0f),false,getCol(activeNote));
            } else {
              activeNote=(int)nv;
              accumPitch=0.0f;
              activeSlideSpeed=0.0f;
              activePortaTarget=-1;
              activePortaSpeed=0.0f;
              legatoDelayCtr=-1; legatoDelayTarget=0;
              pushPt(oBase+r*rowW,noteY(activeNote,0.0f),true,getCol(activeNote));
            }
          }
          if (noteOff) {
            activeSlideSpeed=0.0f; activePortaTarget=-1; activePortaSpeed=0.0f;
            activeNote=-1; accumPitch=0.0f; legatoOn=false;
            legatoDelayCtr=-1; legatoDelayTarget=0;
            continue;
          }
          if (activeNote<0) continue;

          for (int ei=0;ei<ec2;ei++) {
            short fx=vp->newData[r][DIV_PAT_FX(ei)];
            short fxv=vp->newData[r][DIV_PAT_FXVAL(ei)];
            if (fx<0) continue;
            if (fx==0x01) {
              int spd=(fxv<0?0:(int)(uint8_t)fxv);
              activeSlideSpeed=(spd==0)?0.0f:(float)spd*slideSpeedMul;
              activePortaTarget=-1; slideIsShorthand=false;
            } else if (fx==0x02) {
              int spd=(fxv<0?0:(int)(uint8_t)fxv);
              activeSlideSpeed=(spd==0)?0.0f:-(float)spd*slideSpeedMul;
              activePortaTarget=-1; slideIsShorthand=false;
            } else if (fx==0xe1) {
              int spd=(fxv<0?0:(int)(uint8_t)fxv);
              activeSlideSpeed=(spd==0)?0.0f:(float)spd*slideSpeedMul;
              activePortaTarget=-1; slideIsShorthand=true;
            } else if (fx==0xe2) {
              int spd=(fxv<0?0:(int)(uint8_t)fxv);
              activeSlideSpeed=(spd==0)?0.0f:-(float)spd*slideSpeedMul;
              activePortaTarget=-1; slideIsShorthand=true;
            } else if (fx==0x03) {
              float spd=(float)ImMax((fxv<0?1:(int)(uint8_t)fxv),1)*(linPitch?slideSpeedMul:1.0f);
              activePortaSpeed=spd;
              activeSlideSpeed=0.0f;
              if (activePortaTarget<0)
                activePortaTarget=findPortaTarget(vi,r+1);
            } else if (fx==0xea) {
              legatoOn=(fxv>0);
            } else if (fx==0xe6) {
              if ((fxv&15)!=0) {
                legatoDelayCtr=(((fxv&0xf0)>>4)&7)+1;
                legatoDelayTarget=((fxv&0x80)?(-(fxv&15)):(fxv&15));
              } else { legatoDelayCtr=-1; legatoDelayTarget=0; }
            } else if (fx==0xe8) {
              if ((fxv&15)!=0) { legatoDelayCtr=((fxv&0xf0)>>4)+1; legatoDelayTarget=(fxv&15); }
              else { legatoDelayCtr=-1; legatoDelayTarget=0; }
            } else if (fx==0xe9) {
              if ((fxv&15)!=0) { legatoDelayCtr=((fxv&0xf0)>>4)+1; legatoDelayTarget=-(fxv&15); }
              else { legatoDelayCtr=-1; legatoDelayTarget=0; }
            }
          }

          bool hasSlide=(activeSlideSpeed!=0.0f||activePortaTarget>=0);
          bool hasDelayedLeg=(legatoDelayCtr>0);

          float rowX0=oBase+r*rowW;
          ImU32 col=getCol(activeNote);

          if (!hasSlide&&!hasDelayedLeg) continue;

          if (allPts.empty()||allPts.back().x<rowX0-0.5f)
            pushPt(rowX0,noteY(activeNote,accumPitch),allPts.empty(),col);

          int tStart=skipFirstTick?1:0;

          for (int t=tStart;t<tpr;t++) {
            float frac=(float)(t+1)/(float)tpr;
            float tx=rowX0+frac*rowW;
            col=getCol(activeNote);

            if (hasDelayedLeg&&legatoDelayCtr>0&&t+1==legatoDelayCtr) {
              pushPt(tx,noteY(activeNote,accumPitch),false,col);
              activeNote=ImClamp(activeNote+legatoDelayTarget,0,NOTES-1);
              accumPitch=0.0f;
              legatoDelayCtr=-1; legatoDelayTarget=0;
              hasDelayedLeg=false;
              pushPt(tx,noteY(activeNote,0.0f),false,getCol(activeNote));
              continue;
            }
            if (activePortaTarget>=0) {
              float targetUnits=linPitch
                ?(float)(activePortaTarget-activeNote)*unitsPerSemitone
                :(float)(e->calcBaseFreq(chipClock,1.0,activePortaTarget,true)-e->calcBaseFreq(chipClock,1.0,activeNote,true));
              if (accumPitch<targetUnits) accumPitch=ImMin(accumPitch+activePortaSpeed,targetUnits);
              else                        accumPitch=ImMax(accumPitch-activePortaSpeed,targetUnits);
              if (fabsf(accumPitch-targetUnits)<0.5f) {
                accumPitch=targetUnits;
                pushPt(tx,noteY(activeNote,accumPitch),false,col);
                activeNote=activePortaTarget;
                accumPitch=0.0f;
                activePortaTarget=-1;
                pushPt(tx,noteY(activeNote,0.0f),false,getCol(activeNote));
                continue;
              }
            } else {
              accumPitch+=activeSlideSpeed;
            }
            pushPt(tx,noteY(activeNote,accumPitch),false,col);
          }
        }
      }

      if (allPts.size()>=2) {
        float lineW=ImClamp(1.5f*(float)dpiScale,1.5f,4.0f);
        dl->PushClipRect(ImVec2(vx0,vy0),ImVec2(vx1,vy1),true);
        for (int i=0;i+1<(int)allPts.size();i++) {
          if (!allBreaks[i+1])
            dl->AddLine(allPts[i],allPts[i+1],allCols[i],lineW);
        }
        dl->PopClipRect();
      }
    }

    if (prDragging&&!prDragBuf.empty()) {
      float curBase=ox+pianoW+(float)ord*totalW;
      for (auto& dn:prDragBuf) {
        int nr=ImClamp(dn.row+prDragDeltaR,0,patLen-1);
        int nn=ImClamp((int)dn.note+prDragDeltaN,0,NOTES-1);
        int nEnd=ImClamp(dn.endRow+prDragDeltaR,0,patLen);
        float gx0=curBase+nr*rowW+1;
        float gx1=curBase+ImMin(nEnd,nr+1)*rowW-1;
        if (dn.endRow>dn.row) gx1=curBase+ImClamp(dn.endRow+prDragDeltaR,nr+1,patLen)*rowW-1;
        float gy0=oy+(NOTES-1-nn)*noteH+1;
        float gy1=gy0+noteH-2;
        if (gx1<vx0||gx0>vx1||gy1<vy0||gy0>vy1) continue;
        dl->AddRect(ImVec2(gx0,gy0),ImVec2(gx1,gy1),IM_COL32(255,220,60,200),1.5f);
        dl->AddRectFilled(ImVec2(gx0,gy0),ImVec2(gx1,gy1),IM_COL32(255,220,60,55));
      }
    }

    if (e->isPlaying()) {
      float phx=ox+pianoW+(float)playOrder*totalW+oldRow*rowW;
      ImVec2 mp0=ImGui::GetMousePos();
      bool nearHead=(fabsf(mp0.x-phx)<=6.0f*(float)dpiScale);
      if (nearHead||prFollowOffsetDrag) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      ImU32 headCol=cHead;
      if (nearHead||prFollowOffsetDrag) {
        ImVec4 hc=ImGui::ColorConvertU32ToFloat4(cHead); hc.w=ImMin(hc.w*1.6f,1.0f);
        headCol=ImGui::ColorConvertFloat4ToU32(hc);
      }
      dl->AddLine(ImVec2(phx,oy),ImVec2(phx,oy+totalH),headCol,nearHead||prFollowOffsetDrag?3.0f:2.0f);
      if (nearHead&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)&&!prPainting&&!prResizing&&!prDragging&&!prSelecting) {
        prFollowOffsetDrag=true;
      }
    }
    if (prFollowOffsetDrag) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        float dx=ImGui::GetIO().MouseDelta.x;
        if (noteAreaW>0) prFollowOffset=ImClamp(prFollowOffset-dx/noteAreaW,0.05f,0.95f);
      } else {
        prFollowOffsetDrag=false;
      }
    }
    {
      float cx=ox+pianoW+(float)ord*totalW+cursor.y*rowW;
      dl->AddLine(ImVec2(cx,oy),ImVec2(cx,oy+totalH),IM_COL32(255,255,100,70),1.0f);
    }

    if (hasSel) {
      float curBase=ox+pianoW+(float)ord*totalW;
      float sx0=curBase+selR0*rowW,   sx1=curBase+(selR1+1)*rowW;
      float sy0=oy+(NOTES-1-selN1)*noteH,sy1=oy+(NOTES-selN0)*noteH;
      dl->AddRectFilled(ImVec2(sx0,sy0),ImVec2(sx1,sy1),cSel);
      dl->AddRect(ImVec2(sx0,sy0),ImVec2(sx1,sy1),IM_COL32(255,255,255,180));
    }

    float pkx=wp.x;
    float bkw=pianoW*0.62f;
    float fszH=ImGui::GetFontSize();
    bool showCOnly  =(noteH>=fszH*0.55f);
    bool showAllWhite=(noteH>=fszH*1.0f);
    bool showBlack  =(noteH>=fszH*1.4f);


    dl->PushClipRect(ImVec2(pkx,vy0),ImVec2(pkx+pianoW,vy1),true);
    for (int n=0;n<NOTES;n++) {
      int dn=NOTES-1-n;
      if (PR_BLACK_KEY[dn%12]) continue;
      float ry0=oy+n*noteH,ry1=ry0+noteH;
      if (ry1<vy0||ry0>vy1) continue;
      bool held=(prPianoHeld==dn);
      bool scKey=(prScaleType>0&&prScaleHasNote(dn%12));
      ImU32 keyCol=held?IM_COL32(180,210,255,255):scKey?IM_COL32(210,230,255,255):cKeyW;
      dl->AddRectFilled(ImVec2(pkx,ry0),ImVec2(pkx+pianoW,ry1),keyCol);
      dl->AddLine(ImVec2(pkx,ry1),ImVec2(pkx+pianoW,ry1),cKeyBrd);
      char lb[8]; lb[0]=0;
      bool isC=(dn%12==0);
      if (isC&&showCOnly)          strncpy(lb,noteNames[dn],7);
      else if (!isC&&showAllWhite) strncpy(lb,noteNames[dn],7);
      if (lb[0]) {
        ImVec2 tsz=ImGui::CalcTextSize(lb);
        if (tsz.y<=ry1-ry0) {
          float tx=pkx+bkw-tsz.x-2;
          if (tx<pkx+1) tx=pkx+1;
          float ty=ry0+(ry1-ry0-tsz.y)*0.5f;
          dl->AddText(ImVec2(tx,ty),IM_COL32(40,40,40,230),lb);
        }
      }
    }
    dl->PopClipRect();

    dl->PushClipRect(ImVec2(pkx,vy0),ImVec2(pkx+bkw,vy1),true);
    for (int n=0;n<NOTES;n++) {
      int dn=NOTES-1-n;
      if (!PR_BLACK_KEY[dn%12]) continue;
      float ry0=oy+n*noteH,ry1=ry0+noteH;
      if (ry1<vy0||ry0>vy1) continue;
      bool held=(prPianoHeld==dn);
      bool scKeyB=(prScaleType>0&&prScaleHasNote(dn%12));
      ImU32 keyColB=held?IM_COL32(60,100,180,255):scKeyB?IM_COL32(40,90,160,255):cKeyB;
      dl->AddRectFilled(ImVec2(pkx,ry0),ImVec2(pkx+bkw,ry1),keyColB);
      dl->AddRect(ImVec2(pkx,ry0),ImVec2(pkx+bkw,ry1),cKeyBrd,0.0f,0,0.8f);
      if (showBlack) {
        char lb[8];
        strncpy(lb,noteNames[dn],7);
        ImVec2 tsz=ImGui::CalcTextSize(lb);
        if (tsz.y<=ry1-ry0) {
          float ty=ry0+(ry1-ry0-tsz.y)*0.5f;
          dl->AddText(ImVec2(pkx+2,ty),IM_COL32(200,200,200,210),lb);
        }
      }
    }
    dl->PopClipRect();

    dl->AddLine(ImVec2(pkx+pianoW,oy),ImVec2(pkx+pianoW,oy+totalH),cKeyBrd,1.5f);

    ImGui::SetCursorPos(ImVec2(0,0));
    ImGui::InvisibleButton("##prInteract",ImVec2(pianoW+allW,totalH));
    bool hov=ImGui::IsItemHovered();
    bool midHeld=ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (hov) {
      ImVec2 mp=ImGui::GetMousePos();
      float lx=mp.x-wp.x+curScrollX;
      float ly=mp.y-wp.y+scrollY;
      bool inPiano=(mp.x-wp.x<pianoW);
      int absRow=(!inPiano)?(int)((lx-pianoW)/rowW):-1;
      int clickOrd=(absRow>=0)?ImClamp(absRow/patLen,0,ordersLen-1):ord;
      int mrow=(absRow>=0)?(absRow-clickOrd*patLen):-1;
      int mnote=ImClamp(NOTES-1-(int)(ly/noteH),0,NOTES-1);

      if (clickOrd!=ord&&!prPainting&&!prErasing&&!prResizing&&!prDragging&&!prDragMaybe&&!prSelecting) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)||ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          curOrder=clickOrd;
          ord=clickOrd;
          patIdx=e->curSubSong->orders.ord[prChan][ord];
          pat=e->curPat[prChan].getPattern(patIdx,true);
        }
      }

      if (prQuantize>1&&mrow>=0) {
        int qstep2=ImMax(patLen/prQuantize,1);
        mrow=(mrow/qstep2)*qstep2;
        mrow=ImClamp(mrow,0,patLen-1);
      } else if (mrow>=0) {
        mrow=ImClamp(mrow,0,patLen-1);
      }

      if (midHeld&&!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta=ImGui::GetIO().MouseDelta;
        prPanDX-=delta.x;
        prPanDY-=delta.y;
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      }

      if (inPiano&&mnote>=0&&mnote<NOTES) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          int snappedPiano=prSnapScale(mnote);
          if (prPianoHeld!=snappedPiano) {
            if (prPianoHeld>=0) e->noteOff(prChan);
            e->noteOn(prChan,curIns>=0?curIns:(prevIns>=0?prevIns:0),snappedPiano-60);
            prPianoHeld=snappedPiano;
          }
        }
        bool blkKey=PR_BLACK_KEY[mnote%12];
        float ry0=oy+(NOTES-1-mnote)*noteH;
        float kw=blkKey?(pianoW*0.62f):pianoW;
        dl->AddRectFilled(ImVec2(pkx,ry0),ImVec2(pkx+kw,ry0+noteH),
          IM_COL32(180,210,255,blkKey?120:80));
      }
      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)&&prPianoHeld>=0&&inPiano) {
        e->noteOff(prChan); prPianoHeld=-1;
      }

      if (!inPiano&&mrow>=0&&mrow<patLen) {
        int hNoteRow=-1;
        short hNv=-1;
        short hNv2=pat->newData[mrow][DIV_PAT_NOTE];
        if (hNv2>=0&&hNv2<NOTES&&!prIsSpecial(hNv2)) {
          hNoteRow=mrow; hNv=hNv2;
        } else {
          for (int rr=mrow-1;rr>=0&&rr>=mrow-512;rr--) {
            short sv=pat->newData[rr][DIV_PAT_NOTE];
            if (sv==-1) continue;
            if (sv>=0&&sv<NOTES&&!prIsSpecial(sv)) {
              int dur=prInferDuration(pat,rr,patLen);
              if (mrow<rr+dur&&mnote==sv) { hNoteRow=rr; hNv=sv; }
            }
            break;
          }
        }
        if (hNv>=0&&hNoteRow>=0) {
          int hDur=prInferDuration(pat,hNoteRow,patLen);
          short hIns=pat->newData[hNoteRow][DIV_PAT_INS];
          short hVol=pat->newData[hNoteRow][DIV_PAT_VOL];
          float nx0=ox+pianoW+(float)ord*totalW+hNoteRow*rowW+1;
          float nx1=ox+pianoW+(float)ord*totalW+(hNoteRow+hDur)*rowW-1;
          float ny0=oy+(NOTES-1-hNv)*noteH+1;
          float ny1=ny0+noteH-2;
          if (mp.x>=nx0&&mp.x<=nx1&&mp.y>=ny0&&mp.y<=ny1) {
            if (hIns>=0&&hIns<(int)e->song.ins.size()) {
              if (hVol>=0)
                ImGui::SetTooltip("%s  len:%d  ins:%d %s  vol:%d",noteNames[hNv],hDur,hIns,e->song.ins[hIns]->name.c_str(),(int)hVol);
              else
                ImGui::SetTooltip("%s  len:%d  ins:%d %s",noteNames[hNv],hDur,hIns,e->song.ins[hIns]->name.c_str());
            } else {
              if (hVol>=0)
                ImGui::SetTooltip("%s  len:%d  vol:%d",noteNames[hNv],hDur,(int)hVol);
              else
                ImGui::SetTooltip("%s  len:%d",noteNames[hNv],hDur);
            }
          }
        }
      }
      if (!inPiano&&mrow>=0&&mrow<patLen) {
        bool nearEdge=false;
        if (!prSelecting&&!prPainting&&!prErasing&&!prResizing) {

          bool allowResize=(prMode!=0)||(!(hasSel&&mrow>=selR0&&mrow<=selR1&&mnote>=selN0&&mnote<=selN1));
          if (allowResize) {
            short ev=pat->newData[mrow][DIV_PAT_NOTE];
            if (ev==mnote&&!prIsSpecial(ev)) {
              float nr=ox+pianoW+(mrow+1)*rowW-1;
              if (mp.x>=nr-5*(float)dpiScale) { nearEdge=true; prResizeRow=mrow; }
            }
            if (!nearEdge) {
              for (int rr=mrow-1;rr>=0&&rr>=mrow-128;rr--) {
                short sv=pat->newData[rr][DIV_PAT_NOTE];
                if (sv==-1) continue;
                if (sv==mnote&&!prIsSpecial(sv)) {
                  int dur=prInferDuration(pat,rr,patLen);
                  if (mrow==rr+dur-1) {
                    float nr=ox+pianoW+(rr+dur)*rowW-1;
                    if (mp.x>=nr-5*(float)dpiScale) { nearEdge=true; prResizeRow=rr; }
                  }
                }
                break;
              }
            }
          }
        }
        if (nearEdge||prResizing) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          if (nearEdge) {
            prResizing=true;
            prepareUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=true;
          } else if (ImGui::GetIO().KeyShift) {
            prSelecting=true;
            prDragSelStartR=mrow; prDragSelStartN=prSnapScale(mnote);
            prSelR0=prSelR1=mrow; prSelN0=prSelN1=prDragSelStartN;
            prSelRow0=mrow; prSelRow1=mrow;
          } else if (prMode==1) {
            short existNote=pat->newData[mrow][DIV_PAT_NOTE];
            bool onNote=(existNote>=0&&existNote<NOTES&&!prIsSpecial(existNote));
            bool onSel=hasSel&&mrow>=selR0&&mrow<=selR1&&onNote&&existNote>=selN0&&existNote<=selN1;
            if (onSel) {

              prDragMaybe=true;
              prDragStartR=mrow; prDragStartN=existNote;
              prDragDeltaR=0; prDragDeltaN=0;
              prDragMouseStart=mp;
              prepareUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=true;
            } else if (onNote) {

              int dur=prInferDuration(pat,mrow,patLen);
              prSelRow0=mrow; prSelRow1=ImMin(mrow+dur-1,patLen-1);
              prSelN0=existNote; prSelN1=existNote;
              prLastNote=existNote;
            } else {
              prSelecting=true;
              prDragSelStartR=mrow; prDragSelStartN=prSnapScale(mnote);
              prSelR0=prSelR1=mrow; prSelN0=prSelN1=prDragSelStartN;
              prSelRow0=mrow; prSelRow1=mrow;
            }
          } else {
            short existNote=pat->newData[mrow][DIV_PAT_NOTE];
            DivPattern* existPat=pat;
            if (prPolyEnabled&&prChanEnd>prChan&&!(existNote>=0&&existNote<NOTES&&!prIsSpecial(existNote))) {
              for (int tc=prChan+1;tc<=prChanEnd;tc++) {
                int tpIdx=e->curSubSong->orders.ord[tc][ord];
                DivPattern* tp=e->curPat[tc].getPattern(tpIdx,false);
                if (tp) {
                  short tn=tp->newData[mrow][DIV_PAT_NOTE];
                  if (tn>=0&&tn<NOTES&&!prIsSpecial(tn)) { existNote=tn; existPat=tp; break; }
                }
              }
            }
            bool onNote=(existNote>=0&&existNote<NOTES&&!prIsSpecial(existNote));
            bool clickedOnNotePitch=(onNote&&mnote==existNote);
            if (onNote&&prMode==0&&!clickedOnNotePitch) {

              if (hasSel) { prSelRow0=prSelRow1=-1; prSelN0=prSelN1=-1; }
              prPainting=true; prErasing=false;
              prPaintStartRow=mrow;
              prPaintNote=prSnapScale(mnote);
              int insToUse=(curIns>=0)?curIns:(prevIns>=0?prevIns:-1);
              if (prPreviewTimer>0&&prPreviewChan>=0) e->noteOff(prPreviewChan);
              e->noteOn(prChan,insToUse>=0?insToUse:0,prPaintNote-60);
              prPreviewTimer=15; prPreviewChan=prChan;
              prepareUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=true;
            } else if (onNote) {
              prPaintLen=prInferDuration(existPat,mrow,patLen);
              prLastNote=existNote;
              if (prMode==0) {
                int dur=prInferDuration(existPat,mrow,patLen);
                prSelRow0=mrow; prSelRow1=ImMin(mrow+dur-1,patLen-1);
                prSelN0=existNote; prSelN1=existNote;
                prDragMaybe=true;
                prDragStartR=mrow; prDragStartN=existNote; prDragClickN=prSnapScale(mnote);
                prDragDeltaR=0; prDragDeltaN=0;
                prDragMouseStart=mp;
                int insToUse=(curIns>=0)?curIns:(prevIns>=0?prevIns:-1);
                if (prPreviewTimer>0&&prPreviewChan>=0) e->noteOff(prPreviewChan);
                e->noteOn(prChan,insToUse>=0?insToUse:0,existNote-60);
                prPreviewTimer=15; prPreviewChan=prChan;
                prepareUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=true;
              } else {

                prPainting=true; prErasing=false;
                prSelRow0=prSelRow1=-1; prSelN0=prSelN1=-1;
                prPaintNote=existNote; prPaintStartRow=mrow;
                prepareUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=true;
              }
            } else {

              if (hasSel) { prSelRow0=prSelRow1=-1; prSelN0=prSelN1=-1; }
              prPainting=true; prErasing=false;
              prPaintStartRow=mrow;
              if (prMode==0) {
                prPaintNote=prSnapScale(mnote);
                {
                  int insToUse=(curIns>=0)?curIns:(prevIns>=0?prevIns:-1);
                  if (prPreviewTimer>0&&prPreviewChan>=0) e->noteOff(prPreviewChan);
                  e->noteOn(prChan,insToUse>=0?insToUse:0,prPaintNote-60);
                  prPreviewTimer=15; prPreviewChan=prChan;
                }
              } else {
                int defNote=prSnapScale(mnote);
                if (ord>0) {
                  int prevPatIdx=e->curSubSong->orders.ord[prChan][ord-1];
                  DivPattern* prevPat=e->curPat[prChan].getPattern(prevPatIdx,false);
                  if (prevPat) {
                    for (int rr=mrow;rr>=0;rr--) {
                      short pn=prevPat->newData[rr][DIV_PAT_NOTE];
                      if (pn>=0&&pn<NOTES&&!prIsSpecial(pn)) { defNote=pn; break; }
                    }
                  }
                }
                if (defNote==prSnapScale(mnote)&&prLastNote>=0&&prLastNote<NOTES) defNote=prLastNote;
                prPaintNote=defNote;
              }
              prepareUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=true;
            }
          }
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)&&!prPainting&&!prResizing&&!prDragging&&!prDragMaybe) {
          if (ImGui::GetIO().KeyShift) {
            prErasing=true; prepareUndo(GUI_UNDO_PATTERN_EDIT);
          } else {
            short noteHere=pat->newData[mrow][DIV_PAT_NOTE];
            bool onNote=(noteHere>=0&&noteHere<NOTES&&!prIsSpecial(noteHere));
            bool onSelNote=hasSel&&mrow>=selR0&&mrow<=selR1&&onNote&&noteHere>=selN0&&noteHere<=selN1;
            if (onNote||onSelNote||hasSel) {
              prCtxRow=mrow; prCtxNote=mnote;
              ImGui::OpenPopup("##prCtx");
            } else {
              prErasing=true; prepareUndo(GUI_UNDO_PATTERN_EDIT);
            }
          }
        }

        if (prDragMaybe&&ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          float ddx=mp.x-prDragMouseStart.x, ddy=mp.y-prDragMouseStart.y;
          if (ddx*ddx+ddy*ddy>81.0f*(float)dpiScale*(float)dpiScale) {
            prDragMaybe=false; prDragging=true; prDragHasCopy=false;
            prDragBuf.clear();
            if (hasSel) {
              int chFirst=prChan, chLast=(prPolyEnabled?prChanEnd:prChan);
              for (int ch=chFirst;ch<=chLast;ch++) {
                int cpIdx=e->curSubSong->orders.ord[ch][ord];
                DivPattern* cp=e->curPat[ch].getPattern(cpIdx,true);
                if (!cp) continue;
                for (int r=selR0;r<=selR1;r++) {
                  short nv=cp->newData[r][DIV_PAT_NOTE];
                  if (nv<0||nv>=NOTES||prIsSpecial(nv)) continue;
                  if (nv<selN0||nv>selN1) continue;
                  int dur=prInferDuration(cp,r,patLen);
                  prDragBuf.push_back({r,r+dur,ch,nv,cp->newData[r][DIV_PAT_INS],cp->newData[r][DIV_PAT_VOL]});
                }
              }
            } else {
              int srcCh=prChan;
              DivPattern* srcPat=pat;
              if (prPolyEnabled&&prChanEnd>prChan) {
                for (int tc=prChan;tc<=prChanEnd;tc++) {
                  int tpIdx=e->curSubSong->orders.ord[tc][ord];
                  DivPattern* tp=e->curPat[tc].getPattern(tpIdx,false);
                  if (tp&&tp->newData[prDragStartR][DIV_PAT_NOTE]==prDragStartN) { srcCh=tc; srcPat=tp; break; }
                }
              }
              int dur=prInferDuration(srcPat,prDragStartR,patLen);
              prDragBuf.push_back({prDragStartR,prDragStartR+dur,srcCh,(short)prDragStartN,
                srcPat->newData[prDragStartR][DIV_PAT_INS],srcPat->newData[prDragStartR][DIV_PAT_VOL]});
            }
          }
        }


        if (prDragging&&ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          prDragDeltaR=mrow-prDragStartR;
          prDragDeltaN=mnote-prDragStartN;
          if (!prDragBuf.empty()) {
            int previewNote=prSnapScale(ImClamp((int)prDragBuf[0].note+prDragDeltaN,0,NOTES-1));
            if (previewNote!=prPaintHeld) {
              int insToUse=(curIns>=0)?curIns:(prevIns>=0?prevIns:-1);
              if (prPaintHeld>=0) e->noteOff(prChan);
              e->noteOn(prChan,insToUse>=0?insToUse:0,previewNote-60);
              prPaintHeld=previewNote;
            }
            float oBase=ox+pianoW+(float)ord*totalW;
            for (auto& dn:prDragBuf) {
              int nr=ImClamp(dn.row+prDragDeltaR,0,patLen-1);
              int nn=prSnapScale(ImClamp((int)dn.note+prDragDeltaN,0,NOTES-1));
              int nEnd=ImClamp(dn.endRow+prDragDeltaR,0,patLen);
              float gx0=oBase+nr*rowW+1;
              float gy0=oy+(NOTES-1-nn)*noteH+1;
              float gx1=oBase+nEnd*rowW-1;
              float gy1=gy0+noteH-2;
              dl->AddRectFilled(ImVec2(gx0,gy0),ImVec2(gx1,gy1),IM_COL32(255,255,255,55));
              dl->AddRect(ImVec2(gx0,gy0),ImVec2(gx1,gy1),IM_COL32(255,255,255,160),0.0f,0,1.5f);
            }
          }
        }

        if (prPainting&&ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          int pn=prSnapScale(mnote);
          int insToUse=(curIns>=0)?curIns:(prevIns>=0?prevIns:-1);
          int paintCh=prChan;
          if (prPolyEnabled&&prChanEnd>prChan) {

            for (int tc=prChan;tc<=prChanEnd;tc++) {
              int tpIdx=e->curSubSong->orders.ord[tc][ord];
              DivPattern* tp=e->curPat[tc].getPattern(tpIdx,true);
              if (tp&&tp->newData[mrow][DIV_PAT_NOTE]==-1) { paintCh=tc; break; }
            }
            prPaintChan=paintCh;
          }
          int ppIdx=e->curSubSong->orders.ord[paintCh][ord];
          DivPattern* pp=e->curPat[paintCh].getPattern(ppIdx,true);
          if (pp) {
            pp->newData[mrow][DIV_PAT_NOTE]=(short)pn;
            prLastNote=pn;
            if (pp->newData[mrow][DIV_PAT_INS]==-1&&insToUse>=0)
              pp->newData[mrow][DIV_PAT_INS]=(short)insToUse;
            if (pp->newData[mrow][DIV_PAT_VOL]==-1)
              pp->newData[mrow][DIV_PAT_VOL]=(short)volMax;
            {
              int noteEnd=mrow+prPaintLen;
              for (int rr=mrow+1;rr<noteEnd&&rr<patLen;rr++)
                if (pp->newData[rr][DIV_PAT_NOTE]==DIV_NOTE_OFF)
                  pp->newData[rr][DIV_PAT_NOTE]=-1;
              if (noteEnd<patLen&&(pp->newData[noteEnd][DIV_PAT_NOTE]==-1||pp->newData[noteEnd][DIV_PAT_NOTE]==DIV_NOTE_OFF))
                pp->newData[noteEnd][DIV_PAT_NOTE]=DIV_NOTE_OFF;
            }
            if (prPolyEnabled&&prChanEnd>prChan) {
              for (int tc=prChan;tc<=prChanEnd;tc++) {
                if (tc==paintCh) continue;
                int tpIdx2=e->curSubSong->orders.ord[tc][ord];
                DivPattern* tp2=e->curPat[tc].getPattern(tpIdx2,true);
                if (!tp2) continue;
                if (pp->newData[mrow][DIV_PAT_VOL]>=0)
                  tp2->newData[mrow][DIV_PAT_VOL]=pp->newData[mrow][DIV_PAT_VOL];
                for (int ei=0;ei<effectCols;ei++) {
                  tp2->newData[mrow][DIV_PAT_FX(ei)]=pp->newData[mrow][DIV_PAT_FX(ei)];
                  tp2->newData[mrow][DIV_PAT_FXVAL(ei)]=pp->newData[mrow][DIV_PAT_FXVAL(ei)];
                }
              }
            }
          }
          if (pn!=prPaintHeld) {
            if (prPaintHeld>=0) e->noteOff(paintCh);
            e->noteOn(paintCh,insToUse>=0?insToUse:0,pn-60);
            prPaintHeld=pn;
          }
          MARK_MODIFIED;
        }

        if (prResizing&&ImGui::IsMouseDown(ImGuiMouseButton_Left)&&prResizeRow>=0) {
          int oldEnd=prResizeRow+1;
          for (int rr=prResizeRow+1;rr<patLen;rr++) {
            short sv=pat->newData[rr][DIV_PAT_NOTE];
            if (sv==DIV_NOTE_OFF) { oldEnd=rr; break; }
            if (sv!=-1) { oldEnd=rr; break; }
            oldEnd=rr+1;
          }
          int newEnd=ImMax(mrow+1,prResizeRow+1);
          int clearTo=ImMax(oldEnd,newEnd);
          for (int rr=prResizeRow+1;rr<=clearTo&&rr<patLen;rr++) {
            short sv=pat->newData[rr][DIV_PAT_NOTE];
            if (sv==-1||sv==DIV_NOTE_OFF) pat->newData[rr][DIV_PAT_NOTE]=-1;
            else if (rr>newEnd) break;
          }
          if (newEnd<patLen) {
            short sv=pat->newData[newEnd][DIV_PAT_NOTE];
            if (sv==-1||sv==DIV_NOTE_OFF) pat->newData[newEnd][DIV_PAT_NOTE]=DIV_NOTE_OFF;
          }
          prPaintLen=newEnd-prResizeRow;
          MARK_MODIFIED;
        }

        if (prSelecting&&ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          int snappedSel=prSnapScale(mnote);
          prSelR1=mrow; prSelN1=snappedSel;
          if (prDragSelStartR>=0) {
            prSelRow0=ImMin(prDragSelStartR,mrow);
            prSelRow1=ImMax(prDragSelStartR,mrow);
            prSelN0=ImMin(prDragSelStartN,snappedSel);
            prSelN1=ImMax(prDragSelStartN,snappedSel);
          }
        }

        float hcx=ox+pianoW+mrow*rowW;
        float hcy=oy+(NOTES-1-prSnapScale(mnote))*noteH;
        dl->AddRectFilled(ImVec2(hcx,hcy),ImVec2(hcx+rowW,hcy+noteH),IM_COL32(255,255,255,28));

        if (prErasing&&ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          short noteHere=pat->newData[mrow][DIV_PAT_NOTE];
          if (noteHere>=0&&noteHere<NOTES&&!prIsSpecial(noteHere)) {
            pat->newData[mrow][DIV_PAT_NOTE]=-1;
            for (int ei=1;ei<DIV_MAX_COLS;ei++) pat->newData[mrow][ei]=-1;
            for (int rr=mrow+1;rr<patLen;rr++) {
              if (pat->newData[rr][DIV_PAT_NOTE]==DIV_NOTE_OFF) { pat->newData[rr][DIV_PAT_NOTE]=-1; break; }
              if (pat->newData[rr][DIV_PAT_NOTE]!=-1) break;
            }

            bool prevActive=false;
            for (int rr=mrow-1;rr>=0;rr--) {
              short pn=pat->newData[rr][DIV_PAT_NOTE];
              if (pn==DIV_NOTE_OFF||pn==DIV_NOTE_REL) break;
              if (pn>=0&&pn<NOTES&&!prIsSpecial(pn)) { prevActive=true; break; }
            }
            if (prevActive) pat->newData[mrow][DIV_PAT_NOTE]=DIV_NOTE_OFF;
            e->noteOff(prChan);
            MARK_MODIFIED;
          }
        }
      }

      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (prDragMaybe) {
          prDragMaybe=false;
          int dur=prInferDuration(pat,prDragStartR,patLen);
          prSelRow0=prDragStartR; prSelRow1=ImMin(prDragStartR+dur-1,patLen-1);
          prSelN0=prDragStartN; prSelN1=prSelN0;
          if (prNoteUndoOpen) { makeUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=false; }
          prDragClickN=-1;
        }
        if (prDragging) {
          prDragging=false;
          if (prDragDeltaR!=0||prDragDeltaN!=0) {
            for (auto& dn:prDragBuf) {
              int cpIdx=e->curSubSong->orders.ord[dn.chan][ord];
              DivPattern* cp=e->curPat[dn.chan].getPattern(cpIdx,true);
              if (!cp) continue;
              cp->newData[dn.row][DIV_PAT_NOTE]=-1;
              if (dn.endRow<patLen&&cp->newData[dn.endRow][DIV_PAT_NOTE]==DIV_NOTE_OFF)
                cp->newData[dn.endRow][DIV_PAT_NOTE]=-1;
            }
            for (auto& dn:prDragBuf) {
              int nr=ImClamp(dn.row+prDragDeltaR,0,patLen-1);
              int nn=prSnapScale(ImClamp((int)dn.note+prDragDeltaN,0,NOTES-1));
              int cpIdx=e->curSubSong->orders.ord[dn.chan][ord];
              DivPattern* cp=e->curPat[dn.chan].getPattern(cpIdx,true);
              if (!cp) continue;
              cp->newData[nr][DIV_PAT_NOTE]=(short)nn;
              cp->newData[nr][DIV_PAT_INS]=dn.ins;
              cp->newData[nr][DIV_PAT_VOL]=dn.vol;
              int nEnd=ImClamp(dn.endRow+prDragDeltaR,0,patLen);
              if (nEnd<patLen&&nEnd>nr) cp->newData[nEnd][DIV_PAT_NOTE]=DIV_NOTE_OFF;
            }
            if (hasSel) {
              prSelRow0=ImClamp(selR0+prDragDeltaR,0,patLen-1);
              prSelRow1=ImClamp(selR1+prDragDeltaR,0,patLen-1);
              prSelN0=ImClamp(selN0+prDragDeltaN,0,NOTES-1);
              prSelN1=ImClamp(selN1+prDragDeltaN,0,NOTES-1);
            }
            MARK_MODIFIED;
          }
          prDragBuf.clear(); prDragDeltaR=0; prDragDeltaN=0; prDragClickN=-1; prDragHasCopy=false;
          if (prNoteUndoOpen) { makeUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=false; }
        }
        if (prNoteUndoOpen) { makeUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=false; }
        if (prSelecting) {
          prSelecting=false;
          prSelRow0=ImMin(prSelR0,prSelR1); prSelRow1=ImMax(prSelR0,prSelR1);
          prSelN0  =ImMin(prSelN0,prSelN1); prSelN1  =ImMax(prSelN0,prSelN1);
          prDragSelStartR=-1; prDragSelStartN=-1;
        }
        if (prPaintHeld>=0) { e->noteOff(prPaintChan>=0?prPaintChan:prChan); prPaintHeld=-1; }
        prPainting=prResizing=false; prResizeRow=-1; prPaintNote=-1; prPaintChan=-1; prPaintStartRow=-1;
        if (prPianoHeld>=0&&!hov) { e->noteOff(prChan); prPianoHeld=-1; }
      }
      if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)&&prErasing)
        { makeUndo(GUI_UNDO_PATTERN_EDIT); prErasing=false; }
    } else {
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (prDragMaybe) { prDragMaybe=false; prDragClickN=-1; }
        if (prDragging) {
          prDragging=false; prDragBuf.clear(); prDragDeltaR=0; prDragDeltaN=0; prDragClickN=-1;
        }
        if (prNoteUndoOpen) { makeUndo(GUI_UNDO_PATTERN_EDIT); prNoteUndoOpen=false; }
        if (prPaintHeld>=0) { e->noteOff(prPaintChan>=0?prPaintChan:prChan); prPaintHeld=-1; }
        prPainting=prResizing=prSelecting=false; prResizeRow=-1; prPaintNote=-1; prPaintChan=-1;
        if (prPianoHeld>=0) { e->noteOff(prChan); prPianoHeld=-1; }
      }
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)&&prErasing)
        { makeUndo(GUI_UNDO_PATTERN_EDIT); prErasing=false; }
    }

    if (ImGui::BeginPopup("##prCtx")) {
      int mr=(prCtxRow>=0)?prCtxRow:0;
      int mn=(prCtxNote>=0)?prCtxNote:60;
      bool onNote=(pat->newData[mr][DIV_PAT_NOTE]>=0&&!prIsSpecial(pat->newData[mr][DIV_PAT_NOTE]));
      ImGui::TextDisabled("Row %d  |  %s",mr,noteNames[mn]);
      ImGui::Separator();
      if (ImGui::MenuItem("Note On")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        int pn=prSnapScale(mn);
        pat->newData[mr][DIV_PAT_NOTE]=(short)pn;
        int insToUse2=(curIns>=0)?curIns:(prevIns>=0?prevIns:-1);
        if (pat->newData[mr][DIV_PAT_INS]==-1&&insToUse2>=0) pat->newData[mr][DIV_PAT_INS]=(short)insToUse2;
        if (pat->newData[mr][DIV_PAT_VOL]==-1) pat->newData[mr][DIV_PAT_VOL]=(short)volMax;
        prLastNote=pn;
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      if (ImGui::MenuItem("Note Off")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        pat->newData[mr][DIV_PAT_NOTE]=DIV_NOTE_OFF;
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      if (ImGui::MenuItem("Note Release")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        pat->newData[mr][DIV_PAT_NOTE]=DIV_NOTE_REL;
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      if (ImGui::MenuItem("Macro Release")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        pat->newData[mr][DIV_PAT_NOTE]=DIV_MACRO_REL;
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Set Instrument",nullptr,false,onNote||(hasSel&&selR0>=0))) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        int insToSet=(curIns>=0)?curIns:0;
        if (hasSel) {
          for (int r=selR0;r<=selR1;r++) {
            short nv=pat->newData[r][DIV_PAT_NOTE];
            if (nv>=0&&nv<NOTES&&!prIsSpecial(nv)) pat->newData[r][DIV_PAT_INS]=(short)insToSet;
          }
        } else if (onNote) {
          pat->newData[mr][DIV_PAT_INS]=(short)insToSet;
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      ImGui::Separator();
      if (hasSel&&ImGui::BeginMenu("Transpose")) {
        if (ImGui::MenuItem("+1 semitone")) {
          prepareUndo(GUI_UNDO_PATTERN_EDIT);
          for (int r=selR0;r<=selR1;r++) {
            short nv=pat->newData[r][DIV_PAT_NOTE];
            if (nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)&&nv+1<NOTES)
              pat->newData[r][DIV_PAT_NOTE]=nv+1;
          }
          prSelN0=ImClamp(prSelN0+1,0,NOTES-1); prSelN1=ImClamp(prSelN1+1,0,NOTES-1);
          makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        }
        if (ImGui::MenuItem("-1 semitone")) {
          prepareUndo(GUI_UNDO_PATTERN_EDIT);
          for (int r=selR0;r<=selR1;r++) {
            short nv=pat->newData[r][DIV_PAT_NOTE];
            if (nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)&&nv-1>=0)
              pat->newData[r][DIV_PAT_NOTE]=nv-1;
          }
          prSelN0=ImClamp(prSelN0-1,0,NOTES-1); prSelN1=ImClamp(prSelN1-1,0,NOTES-1);
          makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        }
        if (ImGui::MenuItem("+1 octave")) {
          prepareUndo(GUI_UNDO_PATTERN_EDIT);
          for (int r=selR0;r<=selR1;r++) {
            short nv=pat->newData[r][DIV_PAT_NOTE];
            if (nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)&&nv+12<NOTES)
              pat->newData[r][DIV_PAT_NOTE]=nv+12;
          }
          prSelN0=ImClamp(prSelN0+12,0,NOTES-1); prSelN1=ImClamp(prSelN1+12,0,NOTES-1);
          makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        }
        if (ImGui::MenuItem("-1 octave")) {
          prepareUndo(GUI_UNDO_PATTERN_EDIT);
          for (int r=selR0;r<=selR1;r++) {
            short nv=pat->newData[r][DIV_PAT_NOTE];
            if (nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)&&nv-12>=0)
              pat->newData[r][DIV_PAT_NOTE]=nv-12;
          }
          prSelN0=ImClamp(prSelN0-12,0,NOTES-1); prSelN1=ImClamp(prSelN1-12,0,NOTES-1);
          makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        }
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (hasSel&&ImGui::MenuItem("Copy Selection")) {
        prClipboard.clear();
        prClipRows=selR1-selR0+1;
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=0&&nv<NOTES&&nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)) {
            PrClipEntry ce;
            ce.rowOff=r-selR0; ce.note=nv;
            ce.ins=pat->newData[r][DIV_PAT_INS];
            ce.vol=pat->newData[r][DIV_PAT_VOL];
            prClipboard.push_back(ce);
          }
        }
      }
      if (hasSel&&ImGui::MenuItem("Cut Selection")) {
        prClipboard.clear();
        prClipRows=selR1-selR0+1;
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=0&&nv<NOTES&&nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)) {
            PrClipEntry ce;
            ce.rowOff=r-selR0; ce.note=nv;
            ce.ins=pat->newData[r][DIV_PAT_INS];
            ce.vol=pat->newData[r][DIV_PAT_VOL];
            prClipboard.push_back(ce);
            pat->newData[r][DIV_PAT_NOTE]=-1;
          }
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        prSelRow0=prSelRow1=prSelN0=prSelN1=-1;
      }
      if (!prClipboard.empty()&&ImGui::MenuItem("Paste")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        int baseRow=cursor.y;
        for (int r=baseRow;r<ImMin(baseRow+prClipRows,patLen);r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=0&&nv<NOTES&&!prIsSpecial(nv)) pat->newData[r][DIV_PAT_NOTE]=-1;
        }
        for (auto& ce:prClipboard) {
          int r=baseRow+ce.rowOff;
          if (r>=0&&r<patLen) {
            pat->newData[r][DIV_PAT_NOTE]=ce.note;
            if (ce.ins>=0) pat->newData[r][DIV_PAT_INS]=ce.ins;
            if (ce.vol>=0) pat->newData[r][DIV_PAT_VOL]=ce.vol;
          }
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      if (hasSel&&ImGui::MenuItem("Duplicate Ahead")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        int span=selR1-selR0+1;
        int base=selR1+1;
        for (int r=selR0;r<=selR1;r++) {
          int dr=base+(r-selR0);
          if (dr>=patLen) break;
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=selN0&&nv<=selN1&&nv>=0&&nv<NOTES&&!prIsSpecial(nv)) {
            pat->newData[dr][DIV_PAT_NOTE]=nv;
            pat->newData[dr][DIV_PAT_INS]=pat->newData[r][DIV_PAT_INS];
            pat->newData[dr][DIV_PAT_VOL]=pat->newData[r][DIV_PAT_VOL];
          }
        }
        prSelRow0=base; prSelRow1=ImMin(base+span-1,patLen-1);
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      if (ImGui::MenuItem("Select All")) {
        prSelRow0=0; prSelRow1=patLen-1; prSelN0=0; prSelN1=NOTES-1;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Erase Note")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        int noteStart=mr;
        short sv=pat->newData[mr][DIV_PAT_NOTE];
        if (sv==-1||sv==DIV_NOTE_OFF) {
          for (int rr=mr-1;rr>=0;rr--) {
            short bv=pat->newData[rr][DIV_PAT_NOTE];
            if (bv==-1||bv==DIV_NOTE_OFF) continue;
            if (!prIsSpecial(bv)) { noteStart=rr; break; }
            break;
          }
        }
        for (int col=0;col<DIV_MAX_COLS;col++) pat->newData[noteStart][col]=-1;
        for (int rr=noteStart+1;rr<patLen;rr++) {
          short nv2=pat->newData[rr][DIV_PAT_NOTE];
          if (nv2==DIV_NOTE_OFF) { pat->newData[rr][DIV_PAT_NOTE]=-1; break; }
          if (nv2!=-1) break;
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }
      if (hasSel&&ImGui::MenuItem("Erase Selection")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=selN0&&nv<=selN1&&nv>=0&&nv<NOTES&&!prIsSpecial(nv))
            pat->newData[r][DIV_PAT_NOTE]=-1;
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        prSelRow0=prSelRow1=prSelN0=prSelN1=-1;
      }
      if (hasSel&&ImGui::MenuItem("Erase Row Data")) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=selN0&&nv<=selN1&&!prIsSpecial(nv))
            for (int col=0;col<DIV_MAX_COLS;col++) pat->newData[r][col]=-1;
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        prSelRow0=prSelRow1=prSelN0=prSelN1=-1;
      }
      ImGui::EndPopup();
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
      if (ImGui::IsKeyPressed(ImGuiKey_A,false)&&ImGui::GetIO().KeyCtrl)
        { prSelRow0=0; prSelRow1=patLen-1; prSelN0=0; prSelN1=NOTES-1; }

      if (ImGui::IsKeyPressed(ImGuiKey_C,false)&&ImGui::GetIO().KeyCtrl&&hasSel) {
        prClipboard.clear();
        prClipRows=selR1-selR0+1;
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=0&&nv<NOTES&&nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)) {
            PrClipEntry ce;
            ce.rowOff=r-selR0; ce.note=nv;
            ce.ins=pat->newData[r][DIV_PAT_INS];
            ce.vol=pat->newData[r][DIV_PAT_VOL];
            prClipboard.push_back(ce);
          }
        }
      }

      if (ImGui::IsKeyPressed(ImGuiKey_X,false)&&ImGui::GetIO().KeyCtrl&&hasSel) {
        prClipboard.clear();
        prClipRows=selR1-selR0+1;
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=0&&nv<NOTES&&nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)) {
            PrClipEntry ce;
            ce.rowOff=r-selR0; ce.note=nv;
            ce.ins=pat->newData[r][DIV_PAT_INS];
            ce.vol=pat->newData[r][DIV_PAT_VOL];
            prClipboard.push_back(ce);
            pat->newData[r][DIV_PAT_NOTE]=-1;
          }
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        prSelRow0=prSelRow1=prSelN0=prSelN1=-1;
      }

      if (ImGui::IsKeyPressed(ImGuiKey_V,false)&&ImGui::GetIO().KeyCtrl&&!prClipboard.empty()) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        int baseRow=cursor.y;
        for (int r=baseRow;r<ImMin(baseRow+prClipRows,patLen);r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=0&&nv<NOTES&&!prIsSpecial(nv)) pat->newData[r][DIV_PAT_NOTE]=-1;
        }
        for (auto& ce:prClipboard) {
          int r=baseRow+ce.rowOff;
          if (r>=0&&r<patLen) {
            pat->newData[r][DIV_PAT_NOTE]=ce.note;
            if (ce.ins>=0) pat->newData[r][DIV_PAT_INS]=ce.ins;
            else if (prevIns>=0) pat->newData[r][DIV_PAT_INS]=(short)prevIns;
            if (ce.vol>=0) pat->newData[r][DIV_PAT_VOL]=ce.vol;
            else pat->newData[r][DIV_PAT_VOL]=(short)volMax;
          }
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
      }

      if (!ImGui::GetIO().KeyCtrl&&!ImGui::GetIO().KeyShift) {
        if (ImGui::IsKeyPressed(ImGuiKey_D,false)) prMode=0;
        if (ImGui::IsKeyPressed(ImGuiKey_S,false)) prMode=1;
        if (ImGui::IsKeyPressed(ImGuiKey_P,false)) prMode=2;
      }
      if (hasSel) {
        int tDir=0;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow,false)&&ImGui::GetIO().KeyShift)
          tDir=ImGui::GetIO().KeyCtrl?12:1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow,false)&&ImGui::GetIO().KeyShift)
          tDir=ImGui::GetIO().KeyCtrl?-12:-1;
        if (tDir!=0) {
          int ns0=selN0+tDir, ns1=selN1+tDir;
          if (ns0>=0&&ns1<NOTES) {
            prepareUndo(GUI_UNDO_PATTERN_EDIT);
            for (int r=selR0;r<=selR1;r++) {
              short nv=pat->newData[r][DIV_PAT_NOTE];
              if (nv>=selN0&&nv<=selN1&&!prIsSpecial(nv)) {
                int nn=nv+tDir;
                if (nn>=0&&nn<NOTES) pat->newData[r][DIV_PAT_NOTE]=(short)nn;
              }
            }
            prSelN0=ns0; prSelN1=ns1;
            makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
          }
        }
        int rDir=0;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow,true)&&ImGui::GetIO().KeyShift&&!ImGui::GetIO().KeyCtrl)
          rDir=-1;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow,true)&&ImGui::GetIO().KeyShift&&!ImGui::GetIO().KeyCtrl)
          rDir=1;
        if (rDir!=0) {
          int nr0=selR0+rDir, nr1=selR1+rDir;
          if (nr0>=0&&nr1<patLen) {
            prepareUndo(GUI_UNDO_PATTERN_EDIT);
            struct ShiftNote { int r; short note, ins, vol; bool hasOff; };
            std::vector<ShiftNote> toMove;
            if (rDir<0) {
              for (int r=selR0;r<=selR1;r++) {
                short nv=pat->newData[r][DIV_PAT_NOTE];
                if (nv>=selN0&&nv<=selN1&&nv>=0&&nv<NOTES&&!prIsSpecial(nv)) {
                  int dur=prInferDuration(pat,r,patLen);
                  int offRow=r+dur;
                  bool hasOff=(offRow<patLen&&pat->newData[offRow][DIV_PAT_NOTE]==DIV_NOTE_OFF);
                  toMove.push_back({r,nv,pat->newData[r][DIV_PAT_INS],pat->newData[r][DIV_PAT_VOL],hasOff});
                }
              }
              for (auto& sn:toMove) {
                if (sn.hasOff) {
                  int oldOff=sn.r+prInferDuration(pat,sn.r,patLen);
                  if (oldOff<patLen&&pat->newData[oldOff][DIV_PAT_NOTE]==DIV_NOTE_OFF)
                    pat->newData[oldOff][DIV_PAT_NOTE]=-1;
                }
                for (int col=0;col<DIV_MAX_COLS;col++) pat->newData[sn.r][col]=-1;
              }
              for (auto& sn:toMove) {
                int dr=sn.r+rDir;
                pat->newData[dr][DIV_PAT_NOTE]=sn.note;
                pat->newData[dr][DIV_PAT_INS]=sn.ins;
                pat->newData[dr][DIV_PAT_VOL]=sn.vol;
                if (sn.hasOff) {
                  int newOff=dr+prInferDuration(pat,dr,patLen);
                  if (newOff<patLen&&(pat->newData[newOff][DIV_PAT_NOTE]==-1||pat->newData[newOff][DIV_PAT_NOTE]==DIV_NOTE_OFF))
                    pat->newData[newOff][DIV_PAT_NOTE]=DIV_NOTE_OFF;
                }
              }
            } else {
              for (int r=selR1;r>=selR0;r--) {
                short nv=pat->newData[r][DIV_PAT_NOTE];
                if (nv>=selN0&&nv<=selN1&&nv>=0&&nv<NOTES&&!prIsSpecial(nv)) {
                  int dur=prInferDuration(pat,r,patLen);
                  int offRow=r+dur;
                  bool hasOff=(offRow<patLen&&pat->newData[offRow][DIV_PAT_NOTE]==DIV_NOTE_OFF);
                  toMove.push_back({r,nv,pat->newData[r][DIV_PAT_INS],pat->newData[r][DIV_PAT_VOL],hasOff});
                }
              }
              for (auto& sn:toMove) {
                if (sn.hasOff) {
                  int oldOff=sn.r+prInferDuration(pat,sn.r,patLen);
                  if (oldOff<patLen&&pat->newData[oldOff][DIV_PAT_NOTE]==DIV_NOTE_OFF)
                    pat->newData[oldOff][DIV_PAT_NOTE]=-1;
                }
                for (int col=0;col<DIV_MAX_COLS;col++) pat->newData[sn.r][col]=-1;
              }
              for (auto& sn:toMove) {
                int dr=sn.r+rDir;
                pat->newData[dr][DIV_PAT_NOTE]=sn.note;
                pat->newData[dr][DIV_PAT_INS]=sn.ins;
                pat->newData[dr][DIV_PAT_VOL]=sn.vol;
                if (sn.hasOff) {
                  int newOff=dr+prInferDuration(pat,dr,patLen);
                  if (newOff<patLen&&(pat->newData[newOff][DIV_PAT_NOTE]==-1||pat->newData[newOff][DIV_PAT_NOTE]==DIV_NOTE_OFF))
                    pat->newData[newOff][DIV_PAT_NOTE]=DIV_NOTE_OFF;
                }
              }
            }
            prSelRow0=nr0; prSelRow1=nr1;
            makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
          }
        }
      }

      if (ImGui::IsKeyPressed(ImGuiKey_Delete,false)&&hasSel) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT);
        for (int r=selR0;r<=selR1;r++) {
          short nv=pat->newData[r][DIV_PAT_NOTE];
          if (nv>=selN0&&nv<=selN1&&nv>=0&&nv<NOTES&&!prIsSpecial(nv)) {

            for (int col=0;col<DIV_MAX_COLS;col++) pat->newData[r][col]=-1;
            for (int rr=r+1;rr<patLen;rr++) {
              if (pat->newData[rr][DIV_PAT_NOTE]==DIV_NOTE_OFF) { pat->newData[rr][DIV_PAT_NOTE]=-1; break; }
              if (pat->newData[rr][DIV_PAT_NOTE]!=-1) break;
            }

            bool prevNoteActive=false;
            for (int rr=r-1;rr>=0;rr--) {
              short pn=pat->newData[rr][DIV_PAT_NOTE];
              if (pn==DIV_NOTE_OFF||pn==DIV_NOTE_REL) break;
              if (pn>=0&&pn<NOTES&&!prIsSpecial(pn)) { prevNoteActive=true; break; }
            }
            if (prevNoteActive) pat->newData[r][DIV_PAT_NOTE]=DIV_NOTE_OFF;
          }
        }
        makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
        prSelRow0=prSelRow1=prSelN0=prSelN1=-1;
      }
      if (ImGui::IsKeyPressed(ImGuiKey_Escape,false)) prSelRow0=prSelRow1=prSelN0=prSelN1=-1;
    }

  }

  ImGui::EndChild();
  ImGui::PopStyleVar();

  {
    ImGui::InvisibleButton("##prSplit",ImVec2(noteAreaW,splitterH));
    bool splHov=ImGui::IsItemHovered();
    bool splAct=ImGui::IsItemActive();
    if (splHov||splAct) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (splAct) {
      prEffectLaneH-=ImGui::GetIO().MouseDelta.y/(float)dpiScale;
      prEffectLaneH=ImClamp(prEffectLaneH,40.0f,500.0f);
    }
    ImDrawList* sdl=ImGui::GetWindowDrawList();
    ImVec2 smin=ImGui::GetItemRectMin(), smax=ImGui::GetItemRectMax();
    float smid=smin.y+(smax.y-smin.y)*0.5f;
    sdl->AddRectFilled(smin,smax,prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],splAct?2.0f:1.6f));
    sdl->AddLine(ImVec2(smin.x+4,smid),ImVec2(smax.x-4,smid),cGridHi1,1.0f);
    for (int i=0;i<3;i++) {
      float dx=smax.x*0.5f-8+(float)i*8;
      sdl->AddLine(ImVec2(dx,smid-2),ImVec2(dx,smid+2),cGridHi2,1.5f);
    }
  }

  char dynNames[17][64];
  snprintf(dynNames[0],64,"Volume (0-%d)",volMax);
  for (int ei=0;ei<ImMin(effectCols,8);ei++) {
    snprintf(dynNames[1+ei*2],64,"FX%d",ei+1);
    snprintf(dynNames[1+ei*2+1],64,"FX%dV",ei+1);
  }

  int maxLane=ImMin(1+effectCols*2,17);
  if (prEffectLane>=maxLane) prEffectLane=0;

  {
    bool volSel=(prEffectLane==0);
    if (volSel) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    if (ImGui::Button("Vol")) prEffectLane=0;
    if (volSel) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Volume (0-%d)",volMax);
  }
  for (int ei=0;ei<ImMin(effectCols,8);ei++) {
    ImGui::SameLine(0,4);
    int fxLane=1+ei*2;
    int fvLane=2+ei*2;
    bool fxSel=(prEffectLane==fxLane);
    bool fvSel=(prEffectLane==fvLane);

    char fxBtnLbl[16],fvBtnLbl[16],fxTip[128],fvTip[128];
    int counts[256]={};
    for (int r=0;r<patLen;r++) {
      short fx=pat->newData[r][DIV_PAT_FX(ei)];
      if (fx>=0&&fx<=0xff) counts[(unsigned char)fx]++;
    }
    int maxFx=0,maxCnt=0;
    for (int i=0;i<256;i++) if (counts[i]>maxCnt) { maxCnt=counts[i]; maxFx=i; }
    if (maxCnt>0) {
      const char* d=e->getEffectDesc((unsigned char)maxFx,prChan,false);
      const char* nameStart=d?d:"";
      const char* colon=d?strchr(d,':'):NULL;
      if (colon&&colon[1]==' ') nameStart=colon+2;
      char sn[48]=""; int ni=0;
      while(nameStart[ni]&&nameStart[ni]!='('&&ni<46) { sn[ni]=nameStart[ni]; ni++; }
      while(ni>0&&sn[ni-1]==' ') ni--;
      sn[ni]=0;
      snprintf(fxBtnLbl,16,"FX%d",ei+1);
      snprintf(fxTip,128,"FX column %d  (most used: %02X - %s)",ei+1,(unsigned char)maxFx,sn[0]?sn:"?");
      snprintf(fvBtnLbl,16,"FX%dV",ei+1);
      snprintf(fvTip,128,"FX%d value  (effect: %02X - %s)",ei+1,(unsigned char)maxFx,sn[0]?sn:"?");
    } else {
      snprintf(fxBtnLbl,16,"FX%d",ei+1);
      snprintf(fxTip,128,"FX column %d  (no effects set)",ei+1);
      snprintf(fvBtnLbl,16,"FX%dV",ei+1);
      snprintf(fvTip,128,"FX%d value  (no effects set)",ei+1);
    }

    ImVec4 fxBtnCol=(maxCnt>0)
      ?uiColors[fxColors[(unsigned char)maxFx]]
      :ImGui::GetStyleColorVec4(ImGuiCol_Button);
    fxBtnCol.w=fxSel?1.0f:0.55f;
    ImGui::PushStyleColor(ImGuiCol_Button,fxBtnCol);
    fxBtnCol.w=1.0f;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,fxBtnCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,fxBtnCol);
    if (ImGui::Button(fxBtnLbl)) prEffectLane=fxLane;
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",fxTip);

    ImGui::SameLine(0,1);
    ImVec4 fvBtnCol=fxBtnCol; fvBtnCol.w=fvSel?1.0f:0.4f;
    ImGui::PushStyleColor(ImGuiCol_Button,fvBtnCol);
    fvBtnCol.w=1.0f;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,fvBtnCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,fvBtnCol);
    if (ImGui::Button(fvBtnLbl)) prEffectLane=fvLane;
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",fvTip);
  }
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
    int dw=-(int)ImGui::GetIO().MouseWheel;
    if (ImGui::GetIO().KeyCtrl&&dw) prEffectLane=ImClamp(prEffectLane+dw,0,maxLane-1);
  }
  ImGui::SameLine();
  bool canAddFX=(effectCols<8);
  if (!canAddFX) ImGui::BeginDisabled();
  if (ImGui::Button("+ FX")) {
    prepareUndo(GUI_UNDO_PATTERN_EDIT);
    e->curPat[prChan].effectCols=ImMin((int)e->curPat[prChan].effectCols+1,8);
    makeUndo(GUI_UNDO_PATTERN_EDIT);
    MARK_MODIFIED;
  }
  if (!canAddFX) ImGui::EndDisabled();
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip(canAddFX?"Add an FX column (max 8)":"Maximum amount of FX reached!");
  ImGui::SameLine();
  bool canRemFX=(effectCols>0);
  if (!canRemFX) ImGui::BeginDisabled();
  if (ImGui::Button("- FX")) {
    prepareUndo(GUI_UNDO_PATTERN_EDIT);
    e->curPat[prChan].effectCols=ImMax((int)e->curPat[prChan].effectCols-1,0);
    makeUndo(GUI_UNDO_PATTERN_EDIT);
    MARK_MODIFIED;
  }
  if (!canRemFX) ImGui::EndDisabled();
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip(canRemFX?"Remove an FX column":"No FX columns to remove!");
  ImGui::SameLine();
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted("FX Bar - how to use:");
    ImGui::Separator();
    ImGui::TextUnformatted("Lane selector: choose which data to view/edit");
    char volLaneDesc[64];
    snprintf(volLaneDesc,sizeof(volLaneDesc),"  Vol      - note volume per row (0 to %d)",volMax);
    ImGui::TextUnformatted(volLaneDesc);
    ImGui::TextUnformatted("  FX1..8   - effect code column (00 to FF)");
    ImGui::TextUnformatted("  FX1V..8V - effect value column (00 to FF)");
    ImGui::Separator();
    ImGui::TextUnformatted("Left-click / drag - draw values");
    ImGui::TextUnformatted("Shift + LMB drag  - erase values");
    ImGui::TextUnformatted("Right-click drag  - draw a slope between two rows");
    ImGui::TextUnformatted("  Mouse wheel while RMB held - adjust slope tension");
    ImGui::TextUnformatted("Mouse wheel on lane selector - switch lanes");
    ImGui::TextUnformatted("+ FX button - add an effect column (up to 8)");
    ImGui::TextUnformatted("- FX button - remove the last effect column");
    ImGui::EndTooltip();
  }

  bool isEffNum=(prEffectLane>0&&(prEffectLane&1)==1);
  int laneEffIdx=0, laneCol=DIV_PAT_VOL, laneMax=volMax;
  if (prEffectLane==0) {
    laneCol=DIV_PAT_VOL; laneMax=volMax;
  } else if (isEffNum) {
    laneEffIdx=(prEffectLane-1)/2;
    laneCol=DIV_PAT_FX(laneEffIdx); laneMax=0xff;
  } else {
    laneEffIdx=(prEffectLane-2)/2;
    laneCol=DIV_PAT_FXVAL(laneEffIdx); laneMax=0xff;
  }
  ImU32 laneBarColorBase=(prEffectLane==0)
    ?ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_VOL])
    :(isEffNum
      ?ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_NUM])
      :ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_VAL]));

  const float fxRowsPerLane=ImMax(20.0f*(float)dpiScale,fxRowH);
  const float fxRowsContentH=prFxRows?(fxRowsPerLane*(float)maxLane):effectLaneH;
  ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,prFxRows?ImGui::GetStyle().ScrollbarSize:0.0f);
  ImGui::SetNextWindowContentSize(ImVec2(pianoW+allW,fxRowsContentH));
  if (ImGui::BeginChild("##prFX",ImVec2(noteAreaW,effectLaneH),false,
      ImGuiWindowFlags_HorizontalScrollbar|(prFxRows?ImGuiWindowFlags_None:ImGuiWindowFlags_NoScrollWithMouse))) {
    if (settings.prFontScale!=1.0f) ImGui::SetWindowFontScale(settings.prFontScale);
    ImGui::SetScrollX(prSyncScrollX);
    float fxSX=ImGui::GetScrollX();
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 wp2=ImGui::GetWindowPos();
    float ox2=wp2.x-fxSX;
    float vx0=wp2.x,vx1=wp2.x+ImGui::GetWindowWidth();
    float sbSz=ImGui::GetStyle().ScrollbarSize;
    float lBot=wp2.y+ImGui::GetWindowHeight()-sbSz-2;
    float lTop=wp2.y+2;
    float lH=lBot-lTop;
    const float fxPad=4.0f*dpiScale;
    float lHpadded=lH+2.0f*fxPad;
    ImVec2 fSz=ImGui::CalcTextSize("FF");
    const float fxZeroH=ImMax(8.0f*(float)dpiScale,fSz.y*0.5f);
    float lBarBot=lBot-fxZeroH;
    float lBarH=lBarBot-lTop;

    for (int vi=0;vi<ordersLen;vi++) {
      float fxBase=ox2+pianoW+(float)vi*totalW;
      if (fxBase+totalW<vx0||fxBase>vx1) continue;
      bool viCur=(vi==ord);
      float ghostAlpha=viCur?1.0f:0.45f;

      int viPatIdx=e->curSubSong->orders.ord[prChan][vi];
      DivPattern* viPat=e->curPat[prChan].getPattern(viPatIdx,viCur);
      if (!viPat) continue;

      dl->AddRectFilled(ImVec2(fxBase,wp2.y),ImVec2(fxBase+totalW,wp2.y+effectLaneH),
        prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],viCur?0.85f:0.6f));
      for (int r=0;r<=patLen;r++) {
        float cx=fxBase+r*rowW;
        if (cx<vx0-rowW||cx>vx1+rowW) continue;
        ImU32 gc=(r%hiB==0)?cGridHi2:(r%hiA==0)?cGridHi1:cGrid;
        if (!viCur) { ImU32 a=(gc>>24)&0xFF; gc=(gc&0x00FFFFFF)|((ImU32)(a*0.4f)<<24); }
        dl->AddLine(ImVec2(cx,wp2.y),ImVec2(cx,wp2.y+effectLaneH),gc);
      }
      dl->AddLine(ImVec2(fxBase,wp2.y),ImVec2(fxBase,wp2.y+effectLaneH),IM_COL32(255,255,255,viCur?80:30),2.0f);

      if (!prFxRows) {

        if (viCur) {
          dl->AddLine(ImVec2(fxBase,lBarBot),ImVec2(fxBase+totalW,lBarBot),IM_COL32(255,255,255,30));
          dl->AddRectFilled(ImVec2(fxBase,lBarBot),ImVec2(fxBase+totalW,lBot),IM_COL32(0,0,0,40));
          { char qlbl[8];
            if (prEffectLane==0) snprintf(qlbl,8,"0");
            else snprintf(qlbl,8,"00");
            dl->AddText(ImVec2(fxBase+2,lBarBot+1),IM_COL32(140,140,140,80),qlbl); }
          for (int q=1;q<4;q++) {
            float qy=lBarBot-lBarH*(q/4.0f);
            dl->AddLine(ImVec2(fxBase,qy),ImVec2(fxBase+totalW,qy),(q==2)?cGridHi1:cGrid);
            char qlbl[8];
            int qv=(int)(laneMax*q/4.0f+0.5f);
            if (prEffectLane==0) snprintf(qlbl,8,"%d",qv);
            else snprintf(qlbl,8,"%02X",(unsigned char)qv);
            dl->AddText(ImVec2(fxBase+2,qy-fSz.y*0.5f),IM_COL32(140,140,140,100),qlbl);
          }
        }
        dl->PushClipRect(ImVec2(fxBase,lTop),ImVec2(fxBase+totalW,lBot+fxPad),true);
        for (int r=0;r<patLen;r++) {
          short val=viPat->newData[r][laneCol];
          if (val<0) continue;
          short cv=(short)ImClamp((int)val,0,laneMax);
          float bx0=fxBase+r*rowW+1;
          float bx1=fxBase+(r+1)*rowW-1;
          if (bx1<vx0||bx0>vx1) continue;
          float norm=(laneMax>0)?(float)cv/(float)laneMax:0.0f;
          float bw=bx1-bx0;

          ImU32 laneBarColor=laneBarColorBase;
          if (isEffNum) {
            laneBarColor=ImGui::ColorConvertFloat4ToU32(uiColors[fxColors[(unsigned char)cv]]);
          } else if (!isEffNum&&prEffectLane>0) {
            short fxCode=viPat->newData[r][DIV_PAT_FX(laneEffIdx)];
            if (fxCode>=0) laneBarColor=ImGui::ColorConvertFloat4ToU32(uiColors[fxColors[(unsigned char)fxCode]]);
          }

          if (!viCur) {
            ImVec4 cv4=ImGui::ColorConvertU32ToFloat4(laneBarColor);
            cv4.w*=ghostAlpha;
            laneBarColor=ImGui::ColorConvertFloat4ToU32(cv4);
          }

          if (cv==0) {
            dl->AddRectFilled(ImVec2(bx0,lBarBot),ImVec2(bx1,lBot),laneBarColor,1.0f);
            if (viCur&&bw>=fSz.x+2) {
              if (isEffNum) {
                const char* desc=e->getEffectDesc(0,prChan,false);
                const char* nameStart=desc?desc:"";
                const char* colon=desc?strchr(desc,':'):NULL;
                if (colon&&colon[1]==' ') nameStart=colon+2;
                char sd[32]=""; int ni=0;
                while(nameStart[ni]&&nameStart[ni]!='('&&ni<30) { sd[ni]=nameStart[ni]; ni++; }
                while(ni>0&&sd[ni-1]==' ') ni--;
                sd[ni]=0;
                dl->AddText(ImVec2(bx0+1,lBarBot+1),IM_COL32(255,255,255,180),sd[0]?sd:"00");
              } else {
                dl->AddText(ImVec2(bx0+1,lBarBot+1),IM_COL32(255,255,255,180),"00");
              }
            }
          } else {
            float bh=norm*lBarH;
            float by0=lBarBot-bh;
            dl->AddRectFilled(ImVec2(bx0,by0),ImVec2(bx1,lBarBot),laneBarColor,1.5f);
            dl->AddRect(ImVec2(bx0,by0),ImVec2(bx1,lBarBot),IM_COL32(255,255,255,viCur?25:10),1.5f);
            if (viCur&&bw>=fSz.x+2&&bh>=fSz.y) {
              char vs[8];
              if (prEffectLane==0) snprintf(vs,8,"%d",(int)cv);
              else snprintf(vs,8,"%02X",(unsigned char)cv);
              dl->AddText(ImVec2(bx0+1,by0+1),IM_COL32(255,255,255,200),vs);
            }
            if (viCur&&isEffNum&&bw>=fSz.x+2) {
              const char* desc=e->getEffectDesc((unsigned char)cv,prChan,false);
              if (desc&&desc[0]) {
                const char* nameStart=desc;
                const char* colon=strchr(desc,':');
                if (colon&&colon[1]==' ') nameStart=colon+2;
                char sd[32]=""; int ni=0;
                while(nameStart[ni]&&nameStart[ni]!='('&&ni<30) { sd[ni]=nameStart[ni]; ni++; }
                while(ni>0&&sd[ni-1]==' ') ni--;
                sd[ni]=0;
                if (ni>0) dl->AddText(ImVec2(bx0+1,lBarBot-fSz.y-1),IM_COL32(255,255,255,180),sd);
              }
            }
          }
        }
        dl->PopClipRect();

        if (viCur&&prFxViewAll) {
          dl->PushClipRect(ImVec2(fxBase,lTop),ImVec2(fxBase+totalW,lBot+fxPad),true);
          for (int li=0;li<maxLane;li++) {
            if (li==prEffectLane) continue;
            bool liIsEffNum=(li>0&&(li&1)==1);
            int liEffIdx=0;
            int liCol=DIV_PAT_VOL, liMax=volMax;
            if (li==0) { liCol=DIV_PAT_VOL; liMax=volMax; }
            else if (liIsEffNum) { liEffIdx=(li-1)/2; liCol=DIV_PAT_FX(liEffIdx); liMax=0xff; }
            else { liEffIdx=(li-2)/2; liCol=DIV_PAT_FXVAL(liEffIdx); liMax=0xff; }
            ImU32 ghostBase=(li==0)
              ?ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_VOL])
              :(liIsEffNum
                ?ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_NUM])
                :ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_VAL]));
            for (int r=0;r<patLen;r++) {
              short val=viPat->newData[r][liCol];
              if (val<0) continue;
              float bx0=fxBase+r*rowW+1;
              float bx1=fxBase+(r+1)*rowW-1;
              if (bx1<vx0||bx0>vx1) continue;
              short cv=(short)ImClamp((int)val,0,liMax);
              float norm=(liMax>0)?(float)cv/(float)liMax:0.0f;
              float bh=ImMax(norm*lHpadded,2.0f);
              float by0=(lBot+fxPad)-bh;
              ImU32 gc=ghostBase;
              if (liIsEffNum) gc=ImGui::ColorConvertFloat4ToU32(uiColors[fxColors[(unsigned char)cv]]);
              else if (!liIsEffNum&&li>0) {
                short fxCode=viPat->newData[r][DIV_PAT_FX(liEffIdx)];
                if (fxCode>=0) gc=ImGui::ColorConvertFloat4ToU32(uiColors[fxColors[(unsigned char)fxCode]]);
              }
              ImVec4 gcv=ImGui::ColorConvertU32ToFloat4(gc); gcv.w*=0.35f;
              dl->AddRectFilled(ImVec2(bx0,by0),ImVec2(bx1,lBot+fxPad),ImGui::ColorConvertFloat4ToU32(gcv));
            }
          }
          dl->PopClipRect();
        }
      } else {
        float rowsTop=wp2.y-ImGui::GetScrollY();
        float rH=fxRowsPerLane;
        for (int li=0;li<maxLane;li++) {
          bool liIsEffNum=(li>0&&(li&1)==1);
          int liEffIdx=0;
          int liCol=DIV_PAT_VOL, liMax=volMax;
          if (li==0) { liCol=DIV_PAT_VOL; liMax=volMax; }
          else if (liIsEffNum) { liEffIdx=(li-1)/2; liCol=DIV_PAT_FX(liEffIdx); liMax=0xff; }
          else { liEffIdx=(li-2)/2; liCol=DIV_PAT_FXVAL(liEffIdx); liMax=0xff; }

          float ry0=rowsTop+li*rH;
          float ry1=ry0+rH;
          bool isActive=(li==prEffectLane);

          if (viCur&&isActive)
            dl->AddRectFilled(ImVec2(fxBase,ry0),ImVec2(fxBase+totalW,ry1),IM_COL32(255,255,255,18));
          dl->AddLine(ImVec2(fxBase,ry1),ImVec2(fxBase+totalW,ry1),cGrid);

          ImU32 rowBarColor=(li==0)
            ?ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_VOL])
            :(liIsEffNum
              ?ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_NUM])
              :ImGui::ColorConvertFloat4ToU32(uiColors[GUI_COLOR_PIANO_ROLL_FX_VAL]));

          dl->PushClipRect(ImVec2(fxBase,ry0),ImVec2(fxBase+totalW,ry1),true);
          for (int r=0;r<patLen;r++) {
            short val=viPat->newData[r][liCol];
            if (val<0) continue;
            float bx0=fxBase+r*rowW+1;
            float bx1=fxBase+(r+1)*rowW-1;
            if (bx1<vx0||bx0>vx1) continue;
            short cv=(short)ImClamp((int)val,0,liMax);
            float norm=(liMax>0)?(float)cv/(float)liMax:0.0f;
            float fillW=ImMax(norm*(bx1-bx0),2.0f);
            ImU32 bc=rowBarColor;
            if (liIsEffNum) bc=ImGui::ColorConvertFloat4ToU32(uiColors[fxColors[(unsigned char)cv]]);
            else if (!liIsEffNum&&li>0) {
              short fxCode=viPat->newData[r][DIV_PAT_FX(liEffIdx)];
              if (fxCode>=0) bc=ImGui::ColorConvertFloat4ToU32(uiColors[fxColors[(unsigned char)fxCode]]);
            }
            if (!viCur) {
              ImVec4 bc4=ImGui::ColorConvertU32ToFloat4(bc); bc4.w*=ghostAlpha;
              bc=ImGui::ColorConvertFloat4ToU32(bc4);
            }
            float insetY=ry0+1;
            float insetY1=ry1-1;
            dl->AddRectFilled(ImVec2(bx0,insetY),ImVec2(bx0+fillW,insetY1),bc);
            if (viCur&&rH>=fSz.y+2) {
              char vs[8];
              if (li==0) snprintf(vs,8,"%d",(int)cv);
              else snprintf(vs,8,"%02X",(unsigned char)cv);
              dl->AddText(ImVec2(bx0+2,insetY+1),IM_COL32(255,255,255,200),vs);
            }
          }
          dl->PopClipRect();
        }
      }
    }

    if (e->isPlaying()&&playOrder==ord) {
      float phx=ox2+pianoW+(float)playOrder*totalW+oldRow*rowW;
      dl->AddLine(ImVec2(phx,wp2.y),ImVec2(phx,wp2.y+effectLaneH),cHead,2.0f);
    }

    dl->AddRectFilled(wp2,ImVec2(wp2.x+pianoW,wp2.y+effectLaneH),
      prColorMulAlpha(uiColors[GUI_COLOR_PIANO_ROLL_BG],1.25f));
    dl->PushClipRect(wp2,ImVec2(wp2.x+pianoW,wp2.y+effectLaneH),true);
    if (prFxRows) {
      float rowsTop=wp2.y-ImGui::GetScrollY();
      float rH=fxRowsPerLane;
      for (int li=0;li<maxLane;li++) {
        float ry0=rowsTop+li*rH;
        bool isActive=(li==prEffectLane);
        if (isActive) dl->AddRectFilled(ImVec2(wp2.x,ry0),ImVec2(wp2.x+pianoW,ry0+rH),IM_COL32(255,255,255,18));
        dl->AddLine(ImVec2(wp2.x,ry0+rH),ImVec2(wp2.x+pianoW,ry0+rH),cGrid);
        if (rH>=fSz.y) {
          ImU32 tc=isActive?IM_COL32(230,230,230,230):IM_COL32(160,160,160,160);
          dl->AddText(ImVec2(wp2.x+3,ry0+(rH-fSz.y)*0.5f),tc,dynNames[li]);
        }
      }
    } else {
      char topLbl[16],btmLbl[8];
      if (prEffectLane==0) { snprintf(topLbl,16,"%d",laneMax); snprintf(btmLbl,8,"0"); }
      else { snprintf(topLbl,16,"FF"); snprintf(btmLbl,8,"00"); }
      dl->AddText(ImVec2(wp2.x+3,lTop),IM_COL32(160,160,160,220),topLbl);
      dl->AddText(ImVec2(wp2.x+3,lBot-fSz.y),IM_COL32(160,160,160,220),btmLbl);
      const char* nm=dynNames[prEffectLane];
      ImVec2 nsz=ImGui::CalcTextSize(nm);
      float midY=(lTop+fSz.y+4+lBot-fSz.y-4)*0.5f-nsz.y*0.5f;
      dl->PushClipRect(ImVec2(wp2.x,lTop+fSz.y+4),ImVec2(wp2.x+pianoW,lBot-fSz.y-4),true);
      dl->AddText(ImVec2(wp2.x+3,midY),IM_COL32(230,230,230,200),nm);
      dl->PopClipRect();
    }
    dl->PopClipRect();
    dl->AddLine(ImVec2(wp2.x+pianoW,wp2.y),ImVec2(wp2.x+pianoW,wp2.y+effectLaneH),cKeyBrd,1.5f);

    float fxBase=ox2+pianoW+(float)ord*totalW;
    if (!prFxRows&&prFxSlopeActive&&prFxSlopeR0>=0&&prFxSlopeR1>=0) {
      int sr0=ImMin(prFxSlopeR0,prFxSlopeR1), sr1=ImMax(prFxSlopeR0,prFxSlopeR1);
      int sv0=(prFxSlopeR0<=prFxSlopeR1)?prFxSlopeV0:prFxSlopeV1;
      int sv1=(prFxSlopeR0<=prFxSlopeR1)?prFxSlopeV1:prFxSlopeV0;
      int span=ImMax(sr1-sr0,1);
      for (int rr=sr0;rr<sr1;rr++) {
        float t0=(float)(rr-sr0)/span, t1=(float)(rr+1-sr0)/span;
        float c0=prFxCurve(t0,prFxSlopeTension), c1=prFxCurve(t1,prFxSlopeTension);
        float pv0=sv0+(sv1-sv0)*c0, pv1=sv0+(sv1-sv0)*c1;
        float x0=fxBase+rr*rowW+rowW*0.5f;
        float x1=fxBase+(rr+1)*rowW+rowW*0.5f;
        float y0=(lBot+fxPad)-(pv0/ImMax((float)laneMax,1.0f))*lHpadded;
        float y1=(lBot+fxPad)-(pv1/ImMax((float)laneMax,1.0f))*lHpadded;
        if (x1>=vx0&&x0<=vx1)
          dl->AddLine(ImVec2(x0,y0),ImVec2(x1,y1),IM_COL32(255,180,60,230),2.0f);
      }
      char tlbl[20]; snprintf(tlbl,sizeof(tlbl),"T:%.1f",prFxSlopeTension);
      dl->AddText(ImVec2(wp2.x+pianoW+4,lTop+2),IM_COL32(255,220,100,230),tlbl);
    }

    ImGui::SetCursorPos(ImVec2(0,0));
    ImGui::InvisibleButton("##prFXClick",ImVec2(pianoW+allW,effectLaneH));
    bool fxHov=ImGui::IsItemHovered();

    ImVec2 fxMp=ImGui::GetMousePos();
    float fxLx=fxMp.x-wp2.x+fxSX;
    float fxLy=fxMp.y;
    int fxAbsRow=(int)((fxLx-pianoW)/rowW);
    int fxRow=ImClamp(fxAbsRow-(ord*patLen),0,patLen-1);


    if (prFxRows&&fxHov&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      float rowsTop=wp2.y-ImGui::GetScrollY();
      float rH=fxRowsPerLane;
      int clickedLane=ImClamp((int)((fxLy-rowsTop)/rH),0,maxLane-1);
      prEffectLane=clickedLane;
      isEffNum=(prEffectLane>0&&(prEffectLane&1)==1);
      if (prEffectLane==0) { laneEffIdx=0; laneCol=DIV_PAT_VOL; laneMax=volMax; }
      else if (isEffNum) { laneEffIdx=(prEffectLane-1)/2; laneCol=DIV_PAT_FX(laneEffIdx); laneMax=0xff; }
      else { laneEffIdx=(prEffectLane-2)/2; laneCol=DIV_PAT_FXVAL(laneEffIdx); laneMax=0xff; }
    }

    float fxCellLeft=fxBase+fxRow*rowW;
    float fxCellWidth=rowW;
    float fxNorm;
    if (prFxRows) {
      fxNorm=ImClamp((fxMp.x-fxCellLeft)/fxCellWidth,0.0f,1.0f);
    } else {

      fxNorm=ImClamp(1.0f-(fxLy-lTop)/lBarH,0.0f,1.0f);
    }
    int fxPv=(int)(fxNorm*(float)laneMax+0.5f);

    if (fxHov||prFxSlopeActive) {
      if (!prFxSlopeActive) {
        if (prFxRows) {
          float rowsTop=wp2.y-ImGui::GetScrollY();
          float rH=fxRowsPerLane;
          float ry0=rowsTop+prEffectLane*rH;
          float ry1=ry0+rH;
          float pvx=fxCellLeft+fxNorm*fxCellWidth;
          dl->AddLine(ImVec2(pvx,ry0),ImVec2(pvx,ry1),IM_COL32(255,255,100,160),1.5f);
        } else {
          float pvy=(fxPv==0)?lBarBot:(lBarBot-((float)fxPv/ImMax(laneMax,1))*lBarH);
          dl->AddLine(ImVec2(fxBase,pvy),ImVec2(fxBase+totalW,pvy),IM_COL32(255,255,100,110),1.0f);
        }
        char tip[160];
        if (prEffectLane==0) snprintf(tip,sizeof(tip),"Vol: %d / %d",fxPv,laneMax);
        else if (isEffNum) {
          const char* d=e->getEffectDesc((unsigned char)fxPv,prChan,false);
          snprintf(tip,sizeof(tip),"FX: %02X  %s",(unsigned char)fxPv,d?d:"");
        } else {
          short pf=pat->newData[fxRow][DIV_PAT_FX(laneEffIdx)];
          const char* d=(pf>=0)?e->getEffectDesc((unsigned char)pf,prChan,false):nullptr;
          snprintf(tip,sizeof(tip),"Val: %02X  (FX %02X: %s)",
            (unsigned char)fxPv,(pf>=0?(unsigned char)pf:0),d?d:"");
        }
        ImGui::SetTooltip("%s",tip);
        if (fxPv!=prFxPreviewLast) {
          prFxPreviewLast=fxPv;
          if (prEffectLane==0) {
            e->dispatchCmd(DivCommand(DIV_CMD_VOLUME,prChan,fxPv));
          } else {
            unsigned char previewCode, previewVal;
            if (isEffNum) {
              previewCode=(unsigned char)fxPv;
              short ev=pat->newData[fxRow][DIV_PAT_FXVAL(laneEffIdx)];
              previewVal=(ev>=0)?(unsigned char)ev:0;
            } else {
              short ec=pat->newData[fxRow][DIV_PAT_FX(laneEffIdx)];
              previewCode=(ec>=0)?(unsigned char)ec:0;
              previewVal=(unsigned char)fxPv;
            }
            e->previewEffect(prChan,previewCode,previewVal);
          }
        }
      }

      if (!prFxSlopeActive) {
        bool fxErase=ImGui::GetIO().KeyShift&&ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          if (isEffNum&&!fxErase) {

            prFxPickerRow=fxRow;
            prFxPickerEffIdx=laneEffIdx;
            prFxPickerSearch[0]='\0';
            prFxPickerList.clear();
            for (int ci=0;ci<256;ci++) {
              const char* d=e->getEffectDesc((unsigned char)ci,prChan,false);
              if (d&&d[0]) {
                PrFxEntry ent;
                ent.code=(unsigned char)ci;
                snprintf(ent.label,sizeof(ent.label),"%02X  %s",(unsigned char)ci,d);
                prFxPickerList.push_back(ent);
              }
            }
            prFxPickerOpen=true;
            ImGui::OpenPopup("##prFxPicker");
          } else {
            prepareUndo(GUI_UNDO_PATTERN_EDIT); prFxUndoOpen=true;
            prFxLastDragRow=fxRow;
          }
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)&&prFxUndoOpen) {
          int r0=(prFxLastDragRow>=0)?ImMin(prFxLastDragRow,fxRow):fxRow;
          int r1=(prFxLastDragRow>=0)?ImMax(prFxLastDragRow,fxRow):fxRow;
          if (fxErase) {
            for (int rr=r0;rr<=r1;rr++) {
              pat->newData[rr][laneCol]=-1;
              if (!isEffNum&&prEffectLane>0) pat->newData[rr][DIV_PAT_FX(laneEffIdx)]=-1;
            }
          } else {
            int vStart=(prFxLastDragRow>=0&&prFxLastDragRow!=fxRow)
              ?((int)(pat->newData[r0][laneCol]))
              :fxPv;
            for (int rr=r0;rr<=r1;rr++) {
              float t=(r1>r0)?(float)(rr-r0)/(r1-r0):0.0f;
              short vv=(short)ImClamp((int)(vStart+(fxPv-vStart)*t+0.5f),0,laneMax);
              pat->newData[rr][laneCol]=vv;
              if (!isEffNum&&prEffectLane>0&&pat->newData[rr][DIV_PAT_FX(laneEffIdx)]==-1)
                pat->newData[rr][DIV_PAT_FX(laneEffIdx)]=0;
            }
          }
          prFxLastDragRow=fxRow;
          MARK_MODIFIED;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)&&prFxUndoOpen) {
          makeUndo(GUI_UNDO_PATTERN_EDIT); prFxUndoOpen=false; prFxLastDragRow=-1;
        }
      }

      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)&&!prFxSlopeActive) {
        prepareUndo(GUI_UNDO_PATTERN_EDIT); prFxUndoOpen=true;
        prFxSlopeActive=true;
        prFxSlopeR0=prFxSlopeR1=fxRow;
        prFxSlopeV0=prFxSlopeV1=fxPv;
        prFxSlopeTension=0.0f;
      }
      if (prFxSlopeActive) {
        prFxSlopeR1=fxRow; prFxSlopeV1=fxPv;
        float wheel=ImGui::GetIO().MouseWheel;
        if (wheel!=0) prFxSlopeTension=ImClamp(prFxSlopeTension+wheel*0.2f,-3.0f,3.0f);
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          int sr0=ImMin(prFxSlopeR0,prFxSlopeR1), sr1=ImMax(prFxSlopeR0,prFxSlopeR1);
          int sv0=(prFxSlopeR0<=prFxSlopeR1)?prFxSlopeV0:prFxSlopeV1;
          int sv1=(prFxSlopeR0<=prFxSlopeR1)?prFxSlopeV1:prFxSlopeV0;
          int span=ImMax(sr1-sr0,1);
          for (int rr=sr0;rr<=sr1;rr++) {
            float t=(sr0==sr1)?0.5f:(float)(rr-sr0)/span;
            float cv=prFxCurve(t,prFxSlopeTension);
            short vv=(short)ImClamp((int)(sv0+(sv1-sv0)*cv+0.5f),0,laneMax);
            pat->newData[rr][laneCol]=vv;
            if (!isEffNum&&prEffectLane>0&&pat->newData[rr][DIV_PAT_FX(laneEffIdx)]==-1)
              pat->newData[rr][DIV_PAT_FX(laneEffIdx)]=0;
          }
          MARK_MODIFIED;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
          if (prFxUndoOpen) { makeUndo(GUI_UNDO_PATTERN_EDIT); prFxUndoOpen=false; }
          prFxSlopeActive=false; prFxLastDragRow=-1;
        }
      }
    } else {
      prFxPreviewLast=-1;
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)&&prFxUndoOpen) {
        makeUndo(GUI_UNDO_PATTERN_EDIT); prFxUndoOpen=false; prFxLastDragRow=-1;
      }
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)&&prFxSlopeActive) {
        if (prFxUndoOpen) { makeUndo(GUI_UNDO_PATTERN_EDIT); prFxUndoOpen=false; }
        prFxSlopeActive=false;
      }
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();

  if (prFxPickerOpen) {
    ImGui::SetNextWindowSize(ImVec2(300.0f*(float)dpiScale,320.0f*(float)dpiScale),ImGuiCond_Always);
    if (ImGui::BeginPopup("##prFxPicker",ImGuiWindowFlags_NoMove)) {
      ImGui::TextDisabled("Select effect (row %d)",prFxPickerRow);
      ImGui::Separator();
      ImGui::SetNextItemWidth(-1);
      bool searchChanged=ImGui::InputTextWithHint("##fxSearch","Search...",prFxPickerSearch,sizeof(prFxPickerSearch));
      (void)searchChanged;
      ImGui::BeginChild("##fxList",ImVec2(0,0),false);
      for (auto& ent: prFxPickerList) {
        if (prFxPickerSearch[0]) {

          char haystack[64]; char needle[128];
          int hi=0; const char* src=ent.label;
          while (*src&&hi<63) { char c=*src++; haystack[hi++]=(c>='A'&&c<='Z')?(c+32):c; }
          haystack[hi]='\0';
          int ni=0; const char* ns=prFxPickerSearch;
          while (*ns&&ni<127) { char c=*ns++; needle[ni++]=(c>='A'&&c<='Z')?(c+32):c; }
          needle[ni]='\0';
          if (!strstr(haystack,needle)) continue;
        }
        if (ImGui::Selectable(ent.label,false)) {
          prepareUndo(GUI_UNDO_PATTERN_EDIT);
          pat->newData[prFxPickerRow][DIV_PAT_FX(prFxPickerEffIdx)]=(short)ent.code;
          makeUndo(GUI_UNDO_PATTERN_EDIT); MARK_MODIFIED;
          prFxPickerOpen=false;
          ImGui::CloseCurrentPopup();
        }
      }
      ImGui::EndChild();
      ImGui::EndPopup();
    } else {
      prFxPickerOpen=false;
    }
  }

  ImGui::End();
}
