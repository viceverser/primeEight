#include <clap/clap.h>
#include "PluginContract.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <new>
#include <cstdarg>
#include <mutex>
#include <chrono>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#endif

namespace {
using namespace primeeight;
// Diagnostics only on CLAP lifecycle/metadata paths; never write from process().
// A fresh line is flushed per call so a host abort leaves useful evidence.
void trace(const char* fmt, ...){
 static std::mutex lock;
 std::lock_guard<std::mutex> guard(lock);
 const char* temp=std::getenv("TEMP");
 if(!temp || !*temp)temp=std::getenv("TMP");
 if(!temp || !*temp)temp=".";
 char path[1024]{};
 std::snprintf(path,sizeof(path),"%s\\PrimeEight_CLAP_Diagnostic.log",temp);
 FILE* file=std::fopen(path,"a");
 if(!file)return;
 const auto now=std::chrono::system_clock::now().time_since_epoch();
 const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
 const auto tid=std::hash<std::thread::id>{}(std::this_thread::get_id());
 std::fprintf(file,"[%lld ms thread=%llu] ",static_cast<long long>(ms),static_cast<unsigned long long>(tid));
 va_list args;va_start(args,fmt);std::vfprintf(file,fmt,args);va_end(args);
 std::fputc('\n',file);std::fclose(file);
}

constexpr const char* pluginId = "org.codeyvybez.primeeight";
const char* features[] = { CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr };
const clap_plugin_descriptor_t descriptor = {CLAP_VERSION_INIT,pluginId,"PrimeEight","Codey Vybez","","","","0.100.0","Prime Time-inspired digital delay prototype",features};
struct Instance {
 uint64_t serial=0;
 clap_plugin_t plugin{};
 const clap_host_t* host{};
 std::unique_ptr<Processor> processor;
 double sampleRate=48000;
 uint32_t maxFrames=0;
 bool active=false;
 bool processing=false;
#ifdef _WIN32
 HWND guiWindow=nullptr;
 bool guiCreated=false;
 bool guiFloating=false;
 std::array<std::atomic<double>,parameterInfo.size()> guiPending{};
 std::atomic<uint32_t> guiDirty{0};
 int dragParam=-1;
 int dragY=0;
 double dragStart=0;
#endif
 std::array<std::atomic<double>,parameterInfo.size()> published{};
 explicit Instance(const clap_host_t* h):host(h),processor(new Processor(48000)){
  for(size_t i=0;i<parameterInfo.size();++i){published[i].store(parameterInfo[i].initial,std::memory_order_relaxed);
#ifdef _WIN32
 guiPending[i].store(parameterInfo[i].initial,std::memory_order_relaxed);
#endif
 }
 }
 void synchronize(){
  for(size_t i=0;i<parameterInfo.size();++i){
   const double v=published[i].load(std::memory_order_relaxed);
   if(processor->get(static_cast<Parameter>(i))!=v)processor->set(static_cast<Parameter>(i),v);
  }
 }

};
Instance* self(const clap_plugin_t* p){return static_cast<Instance*>(p->plugin_data);}
double canonical(uint32_t id,double value){
 const auto& p=parameterInfo[id];
 value=std::clamp(value,p.minimum,p.maximum);
 if(id==0 || id==7)value=std::round(value);
 return value;
}
void event(Instance* s,const clap_event_header_t* h,bool audioThread){
 if(!h||h->size<sizeof(clap_event_param_value_t)||h->space_id!=CLAP_CORE_EVENT_SPACE_ID||h->type!=CLAP_EVENT_PARAM_VALUE)return;
 const auto* e=reinterpret_cast<const clap_event_param_value_t*>(h);
 if(e->param_id<parameterInfo.size() && std::isfinite(e->value)){
  const auto id=static_cast<Parameter>(e->param_id);
  const double v=canonical(e->param_id,e->value);
  s->published[e->param_id].store(v,std::memory_order_relaxed);
  if(audioThread)s->processor->set(id,v);
 }
}
uint32_t portCount(const clap_plugin_t*,bool input){trace("audio_ports.count input=%u",unsigned(input));return 1;}
bool portGet(const clap_plugin_t*,uint32_t index,bool input,clap_audio_port_info_t* info){
 trace("audio_ports.get index=%u input=%u",index,unsigned(input));
 if(index||!info)return false;
 std::memset(info,0,sizeof(*info));info->id=0;
 std::snprintf(info->name,sizeof(info->name),"%s",input?"Stereo Input":"Stereo Output");
 info->channel_count=2;info->flags=CLAP_AUDIO_PORT_IS_MAIN;info->port_type=CLAP_PORT_STEREO;info->in_place_pair=0;return true;
}
const clap_plugin_audio_ports_t ports={portCount,portGet};
uint32_t paramCount(const clap_plugin_t*){trace("params.count");return static_cast<uint32_t>(parameterInfo.size());}
bool paramInfo(const clap_plugin_t*,uint32_t index,clap_param_info_t* info){
 trace("params.get_info index=%u",index);
 if(!info||index>=parameterInfo.size())return false;
 std::memset(info,0,sizeof(*info));const auto& p=parameterInfo[index];
 info->id=index;std::snprintf(info->name,sizeof(info->name),"%s",p.name);
 info->min_value=p.minimum;info->max_value=p.maximum;info->default_value=p.initial;
 info->flags=CLAP_PARAM_IS_AUTOMATABLE;
 if(index==0||index==7)info->flags|=CLAP_PARAM_IS_STEPPED;
 return true;
}
bool paramValue(const clap_plugin_t* plugin,clap_id id,double* value){
 if(!value||id>=parameterInfo.size())return false;
 *value=self(plugin)->published[id].load(std::memory_order_relaxed);return true;
}
bool valueText(const clap_plugin_t*,clap_id id,double value,char* text,uint32_t capacity){
 if(!text||!capacity||id>=parameterInfo.size())return false;
 if(!std::isfinite(value))return false;
 value=canonical(id,value);
 const int n=(id==0||id==7)?std::snprintf(text,capacity,"%.0f",value):std::snprintf(text,capacity,"%.9g",value);
 return n>=0 && static_cast<uint32_t>(n)<capacity;
}
bool textValue(const clap_plugin_t*,clap_id id,const char* text,double* value){
 if(!text||!value||id>=parameterInfo.size())return false;
 errno=0;char* end=nullptr;const double v=std::strtod(text,&end);
 if(end==text||errno==ERANGE||!std::isfinite(v))return false;
 while(*end==' '||*end=='\t'||*end=='\n')++end;
 if(*end!='\0')return false;
 *value=canonical(id,v);return true;
}
void flush(const clap_plugin_t* plugin,const clap_input_events_t* in,const clap_output_events_t* out){
 auto* s=self(plugin);
 if(in&&in->size&&in->get)for(uint32_t i=0;i<in->size(in);++i)event(s,in->get(in,i),false);
#ifdef _WIN32
 const uint32_t dirty=s->guiDirty.exchange(0,std::memory_order_acq_rel);
 for(uint32_t i=0;i<parameterInfo.size();++i)if(dirty&(1u<<i)){
  const double value=s->guiPending[i].load(std::memory_order_relaxed);
  clap_event_param_value_t ev{};
  ev.header.size=sizeof(ev);ev.header.time=0;ev.header.space_id=CLAP_CORE_EVENT_SPACE_ID;
  ev.header.type=CLAP_EVENT_PARAM_VALUE;ev.param_id=i;ev.value=value;
  if(out&&out->try_push&&out->try_push(out,&ev.header))
   s->published[i].store(value,std::memory_order_relaxed);
  else s->guiDirty.fetch_or(1u<<i,std::memory_order_release);
 }
#endif
}
const clap_plugin_params_t params={paramCount,paramInfo,paramValue,valueText,textValue,flush};
bool save(const clap_plugin_t* plugin,const clap_ostream_t* stream){
 if(!stream||!stream->write)return false;
 // State serialization reads atomic published values, never the audio-owned DSP.
 Processor snapshot(48000);
 for(size_t i=0;i<parameterInfo.size();++i)snapshot.set(static_cast<Parameter>(i),self(plugin)->published[i].load(std::memory_order_relaxed));
 const auto bytes=saveState(snapshot);size_t offset=0;
 while(offset<bytes.size()){
  const int64_t n=stream->write(stream,bytes.data()+offset,bytes.size()-offset);
  if(n<=0||static_cast<size_t>(n)>bytes.size()-offset)return false;
  offset+=static_cast<size_t>(n);
 }return true;
}
bool load(const clap_plugin_t* plugin,const clap_istream_t* stream){
 if(!stream||!stream->read)return false;
 std::array<uint8_t,stateBytes> bytes{};size_t offset=0;
 while(offset<bytes.size()){
  const int64_t n=stream->read(stream,bytes.data()+offset,bytes.size()-offset);
  if(n<=0||static_cast<size_t>(n)>bytes.size()-offset)return false;
  offset+=static_cast<size_t>(n);
 }
 // Decode completely before publishing; audio engine synchronizes at next process.
 Processor snapshot(48000);
 if(!loadState(snapshot,bytes.data(),bytes.size()))return false;
 auto* s=self(plugin);
 for(size_t i=0;i<parameterInfo.size();++i)s->published[i].store(snapshot.get(static_cast<Parameter>(i)),std::memory_order_relaxed);
 return true;
}
const clap_plugin_state_t state={save,load};
#ifdef _WIN32
// Minimal native Win32 CLAP editor for MuLab host-compatibility diagnosis.
// This is a real embeddable HWND, not a claimed GUI with no window.
// Functional native editor based on the approved flat PrimeEight panel.
constexpr uint32_t guiWidth=940,guiHeight=490;
struct UiControl {int x,y,w,h;int param;const char* label;};
constexpr UiControl uiControls[]={
 {24,174,210,88,1,"TAP A TIME"}, {24,276,210,88,2,"TAP B TIME"},
 {248,174,210,88,5,"FEEDBACK"}, {248,276,210,88,7,"REPEAT HOLD"},
 {472,174,210,88,9,"MOD DEPTH"}, {472,276,210,88,8,"MOD RATE"},
 {696,174,220,88,6,"MIX"}, {696,276,220,88,10,"SMOOTHING"},
 {24,390,210,70,0,"MULTIPLY"}, {248,390,210,70,3,"TAP A LEVEL"},
 {472,390,210,70,4,"TAP B LEVEL"}
};
void label(HDC dc,int x,int y,const char* t){TextOutA(dc,x,y,t,int(std::strlen(t)));}
void drawPanel(HDC dc,int x,int y,int w,int h){
 RECT r{x,y,x+w,y+h};HBRUSH fill=CreateSolidBrush(RGB(19,20,20));
 FillRect(dc,&r,fill);DeleteObject(fill);
 HPEN edge=CreatePen(PS_SOLID,1,RGB(113,109,102));
 HGDIOBJ old=SelectObject(dc,edge);SelectObject(dc,GetStockObject(NULL_BRUSH));
 Rectangle(dc,x,y,x+w,y+h);SelectObject(dc,old);DeleteObject(edge);
 HPEN red=CreatePen(PS_SOLID,3,RGB(233,39,30));old=SelectObject(dc,red);
 MoveToEx(dc,x+2,y+2,nullptr);LineTo(dc,x+w-2,y+2);SelectObject(dc,old);DeleteObject(red);
}
void submitGuiValue(Instance* s,int idx,double value){
 if(!s||idx<0||idx>=int(parameterInfo.size()))return;
 value=canonical(uint32_t(idx),value);
 s->guiPending[idx].store(value,std::memory_order_relaxed);
 s->guiDirty.fetch_or(1u<<idx,std::memory_order_release);
 const auto* hp=s->host&&s->host->get_extension?
  static_cast<const clap_host_params_t*>(s->host->get_extension(s->host,CLAP_EXT_PARAMS)):nullptr;
 if(hp&&hp->request_flush)hp->request_flush(s->host);
}
LRESULT CALLBACK editorProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 auto* s=reinterpret_cast<Instance*>(GetWindowLongPtrA(hwnd,GWLP_USERDATA));
 if(msg==WM_NCCREATE){
  auto* cs=reinterpret_cast<CREATESTRUCTA*>(lp);
  SetWindowLongPtrA(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
  return TRUE;
 }
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){
  PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);
  RECT r{};GetClientRect(hwnd,&r);
  HBRUSH bg=CreateSolidBrush(RGB(14,15,15));FillRect(dc,&r,bg);DeleteObject(bg);
  SetBkMode(dc,TRANSPARENT);
  HFONT headline=CreateFontA(39,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,"Arial");
  HGDIOBJ old=SelectObject(dc,headline);
  SetTextColor(dc,RGB(232,218,195));label(dc,24,26,"Prime");
  SetTextColor(dc,RGB(237,48,37));label(dc,140,26,"Eight");
  SelectObject(dc,old);DeleteObject(headline);
  SetTextColor(dc,RGB(184,182,175));label(dc,25,79,"A N A L O G   D E L A Y   P R O C E S S O R");
  SetTextColor(dc,RGB(237,75,47));label(dc,695,31,"PRIMEEIGHT");
  SetTextColor(dc,RGB(170,170,163));label(dc,695,55,"NATIVE CLAP EDITOR");
  drawPanel(dc,24,112,892,48);
  SetTextColor(dc,RGB(223,215,202));label(dc,39,127,"DELAY ENGINE");
  SetTextColor(dc,RGB(239,73,42));label(dc,405,127,"1x     2x     4x     8x");
  for(const auto& c:uiControls){
   drawPanel(dc,c.x,c.y,c.w,c.h);
   SetTextColor(dc,RGB(216,214,205));label(dc,c.x+12,c.y+13,c.label);
   double v=s?(s->guiDirty.load(std::memory_order_relaxed)&(1u<<c.param)?
    s->guiPending[c.param].load(std::memory_order_relaxed):
    s->published[c.param].load(std::memory_order_relaxed)):parameterInfo[c.param].initial;
   char val[64]{};
   if(c.param==0)std::snprintf(val,sizeof(val),"%dx",1<<int(std::round(v)));
   else if(c.param==7)std::snprintf(val,sizeof(val),"%s",v>=0.5?"ON":"OFF");
   else if(c.param==1||c.param==2||c.param==9||c.param==10)std::snprintf(val,sizeof(val),"%.1f ms",v);
   else if(c.param==8)std::snprintf(val,sizeof(val),"%.2f Hz",v);
   else std::snprintf(val,sizeof(val),"%.2f",v);
   HFONT valueFont=CreateFontA(24,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,"Arial");
   old=SelectObject(dc,valueFont);SetTextColor(dc,RGB(246,72,43));
   label(dc,c.x+12,c.y+37,val);
   SelectObject(dc,old);DeleteObject(valueFont);
   // Flat vector dial: native GDI geometry, no gradients or textures.
   const int cx=c.x+c.w-35,cy=c.y+42;
   const auto& info=parameterInfo[c.param];
   const double fraction=info.maximum>info.minimum?
    std::clamp((v-info.minimum)/(info.maximum-info.minimum),0.0,1.0):0.0;
   HPEN ring=CreatePen(PS_SOLID,3,RGB(92,91,86));
   HGDIOBJ previous=SelectObject(dc,ring);
   HGDIOBJ previousBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
   Ellipse(dc,cx-20,cy-20,cx+20,cy+20);
   SelectObject(dc,previous);DeleteObject(ring);
   HPEN pointer=CreatePen(PS_SOLID,4,RGB(242,71,44));
   previous=SelectObject(dc,pointer);
   const double angle=(135.0+270.0*fraction)*3.14159265358979323846/180.0;
   MoveToEx(dc,cx,cy,nullptr);
   LineTo(dc,cx+int(std::cos(angle)*16.0),cy+int(std::sin(angle)*16.0));
   SelectObject(dc,previous);DeleteObject(pointer);
   SelectObject(dc,previousBrush);
  }
  EndPaint(hwnd,&ps);return 0;
 }
 if(msg==WM_LBUTTONDOWN||msg==WM_RBUTTONDOWN){
  if(!s)return 0;
  const int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);
  for(const auto& c:uiControls)if(x>=c.x&&x<c.x+c.w&&y>=c.y&&y<c.y+c.h){
   const int idx=c.param;
   const auto& p=parameterInfo[idx];
   const double old=s->guiDirty.load(std::memory_order_relaxed)&(1u<<idx)?
    s->guiPending[idx].load(std::memory_order_relaxed):
    s->published[idx].load(std::memory_order_relaxed);
   if(idx==7){
    submitGuiValue(s,idx,old>=0.5?0:1);
   }else if(msg==WM_RBUTTONDOWN){
    submitGuiValue(s,idx,p.initial);
   }else{
    s->dragParam=idx;s->dragY=y;s->dragStart=old;SetCapture(hwnd);
    const double direction=x>=c.x+c.w/2?1.0:-1.0;
    const double step=idx==0?1.0:(p.maximum-p.minimum)/100.0;
    submitGuiValue(s,idx,old+direction*step);
    s->dragStart=s->guiPending[idx].load(std::memory_order_relaxed);
   }
   InvalidateRect(hwnd,nullptr,FALSE);return 0;
  }
  return 0;
 }
 if(msg==WM_MOUSEMOVE&&s&&s->dragParam>=0&&(wp&MK_LBUTTON)){
  const int idx=s->dragParam;
  const auto& p=parameterInfo[idx];
  const double pixels=double(s->dragY-GET_Y_LPARAM(lp));
  const double delta=idx==0?std::round(pixels/35.0):
   pixels*(p.maximum-p.minimum)/240.0;
  submitGuiValue(s,idx,s->dragStart+delta);
  InvalidateRect(hwnd,nullptr,FALSE);return 0;
 }
 if(msg==WM_LBUTTONUP||msg==WM_CAPTURECHANGED){
  if(s)s->dragParam=-1;
  if(msg==WM_LBUTTONUP&&GetCapture()==hwnd)ReleaseCapture();
  return 0;
 }
 return DefWindowProcA(hwnd,msg,wp,lp);
}
const char* editorClass="PrimeEightV133Editor";
bool registerEditor(){
 static ATOM cls=[](){
  WNDCLASSA wc{};wc.lpfnWndProc=editorProc;wc.hInstance=GetModuleHandleA(nullptr);
  wc.lpszClassName=editorClass;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
  return RegisterClassA(&wc);
 }();
 return cls!=0||GetLastError()==ERROR_CLASS_ALREADY_EXISTS;
}
bool guiSupported(const clap_plugin_t*,const char* api,bool floating){
 return !floating && api && std::strcmp(api,CLAP_WINDOW_API_WIN32)==0;
}
bool guiPreferred(const clap_plugin_t*,const char** api,bool* floating){
 if(!api||!floating)return false;
 *api=CLAP_WINDOW_API_WIN32;*floating=false;return true;
}
bool guiCreate(const clap_plugin_t* p,const char* api,bool floating){
 auto* s=self(p);
 trace("gui.create instance=%llu api=%s floating=%u",static_cast<unsigned long long>(s->serial),api?api:"(null)",unsigned(floating));
 if(s->guiCreated||!guiSupported(p,api,floating))return false;
 // The host supplies its parent later through set_parent(). Create a hidden
 // top-level window first; Win32 rejects WS_CHILD with no parent.
 if(!registerEditor())return false;
 s->guiWindow=CreateWindowExA(0,editorClass,"PrimeEight",
  WS_POPUP,0,0,int(guiWidth),int(guiHeight),
  nullptr,nullptr,GetModuleHandleA(nullptr),s);
 if(!s->guiWindow){trace("gui.create Win32 error=%lu",GetLastError());return false;}
 s->guiCreated=true;s->guiFloating=false;return true;
}
void guiDestroy(const clap_plugin_t* p){
 auto* s=self(p);trace("gui.destroy instance=%llu",static_cast<unsigned long long>(s->serial));
 if(s->guiWindow)DestroyWindow(s->guiWindow);
 s->guiWindow=nullptr;s->guiCreated=false;
}
bool guiScale(const clap_plugin_t*,double scale){return std::isfinite(scale)&&scale>0;}
bool guiSize(const clap_plugin_t* p,uint32_t* w,uint32_t* h){
 if(!self(p)->guiCreated||!w||!h)return false;
 *w=guiWidth;*h=guiHeight;return true;
}
bool guiCanResize(const clap_plugin_t*){return false;}
bool guiResizeHints(const clap_plugin_t*,clap_gui_resize_hints_t*){return false;}
bool guiAdjust(const clap_plugin_t*,uint32_t* w,uint32_t* h){
 if(!w||!h)return false;*w=guiWidth;*h=guiHeight;return true;
}
bool guiSetSize(const clap_plugin_t* p,uint32_t w,uint32_t h){
 return self(p)->guiCreated&&w==guiWidth&&h==guiHeight;
}
bool guiParent(const clap_plugin_t* p,const clap_window_t* parent){
 auto* s=self(p);
 trace("gui.set_parent instance=%llu api=%s",static_cast<unsigned long long>(s->serial),parent&&parent->api?parent->api:"(null)");
 if(!s->guiCreated||!s->guiWindow||!parent||!parent->api||
  std::strcmp(parent->api,CLAP_WINDOW_API_WIN32)||!parent->win32)return false;
 HWND hwnd=static_cast<HWND>(parent->win32);
 if(!IsWindow(hwnd))return false;
 SetLastError(0);
 // Win32 requires updating the window style when embedding a top-level HWND.
 LONG_PTR style=GetWindowLongPtrA(s->guiWindow,GWL_STYLE);
 SetWindowLongPtrA(s->guiWindow,GWL_STYLE,(style & ~WS_POPUP)|WS_CHILD);
 SetLastError(0);
 HWND old=SetParent(s->guiWindow,hwnd);
 if(!old&&GetLastError()!=0){
  SetWindowLongPtrA(s->guiWindow,GWL_STYLE,style);
  trace("gui.set_parent Win32 error=%lu",GetLastError());return false;
 }
 SetWindowPos(s->guiWindow,nullptr,0,0,int(guiWidth),int(guiHeight),
  SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
 return true;
}
bool guiTransient(const clap_plugin_t*,const clap_window_t*){return false;}
void guiTitle(const clap_plugin_t*,const char*){}
bool guiShow(const clap_plugin_t* p){
 auto* s=self(p);trace("gui.show instance=%llu",static_cast<unsigned long long>(s->serial));
 if(!s->guiCreated||!s->guiWindow)return false;
 ShowWindow(s->guiWindow,SW_SHOW);return true;
}
bool guiHide(const clap_plugin_t* p){
 auto* s=self(p);trace("gui.hide instance=%llu",static_cast<unsigned long long>(s->serial));
 if(!s->guiCreated||!s->guiWindow)return false;
 ShowWindow(s->guiWindow,SW_HIDE);return true;
}
const clap_plugin_gui_t gui={guiSupported,guiPreferred,guiCreate,guiDestroy,
 guiScale,guiSize,guiCanResize,guiResizeHints,guiAdjust,guiSetSize,
 guiParent,guiTransient,guiTitle,guiShow,guiHide};
#endif
bool init(const clap_plugin_t* p){trace("plugin.init instance=%llu -> true host=%s",static_cast<unsigned long long>(self(p)->serial),self(p)->host&&self(p)->host->name?self(p)->host->name:"(null)");return true;}
void destroy(const clap_plugin_t* p){auto* s=self(p);trace("plugin.destroy instance=%llu active=%u processing=%u",static_cast<unsigned long long>(s->serial),unsigned(s->active),unsigned(s->processing));
#ifdef _WIN32
 if(s->guiCreated)guiDestroy(p);
#endif
 delete s;}
bool activate(const clap_plugin_t* p,double sampleRate,uint32_t minFrames,uint32_t maxFrames){
 trace("plugin.activate instance=%llu rate=%.2f min=%u max=%u",static_cast<unsigned long long>(self(p)->serial),sampleRate,minFrames,maxFrames);
 auto* s=self(p);
 if(s->active||s->processing||!std::isfinite(sampleRate)||sampleRate<=0||maxFrames<minFrames||maxFrames>static_cast<uint32_t>(INT32_MAX))return false;
 s->sampleRate=sampleRate;s->maxFrames=maxFrames;
 auto next=std::make_unique<Processor>(sampleRate);
 for(uint32_t i=0;i<parameterInfo.size();++i)next->set(static_cast<Parameter>(i),s->published[i].load(std::memory_order_relaxed));
 s->processor=std::move(next);s->active=true;trace("plugin.activate instance=%llu -> true",static_cast<unsigned long long>(s->serial));return true;
}
void deactivate(const clap_plugin_t* p){trace("plugin.deactivate instance=%llu",static_cast<unsigned long long>(self(p)->serial));auto* s=self(p);s->processing=false;s->active=false;s->maxFrames=0;}
bool start(const clap_plugin_t* p){trace("plugin.start_processing instance=%llu",static_cast<unsigned long long>(self(p)->serial));auto* s=self(p);if(!s->active||s->processing)return false;s->processing=true;return true;}
void stop(const clap_plugin_t* p){trace("plugin.stop_processing instance=%llu",static_cast<unsigned long long>(self(p)->serial));self(p)->processing=false;}
void reset(const clap_plugin_t* p){auto* s=self(p);if(s->active && s->processor)s->processor->reset();}

clap_process_status process(const clap_plugin_t* p,const clap_process_t* block){
 auto* s=self(p);if(!s->active||!s->processing||!block||block->frames_count>s->maxFrames||block->audio_outputs_count!=1||!block->audio_outputs||block->audio_inputs_count!=1||!block->audio_inputs)return CLAP_PROCESS_ERROR;
 auto& out=block->audio_outputs[0];
 if(out.channel_count!=2)return CLAP_PROCESS_ERROR;
 const clap_audio_buffer_t* in=&block->audio_inputs[0];
 if((block->audio_inputs_count==1&&!block->audio_inputs)||(in&&in->channel_count!=2))return CLAP_PROCESS_ERROR;
 // Select a single host-provided sample format; never allocate or convert on the audio thread.
 const bool use64=out.data64&&out.data64[0]&&out.data64[1]&&(!in||(in->data64&&in->data64[0]&&in->data64[1]));
 const bool use32=out.data32&&out.data32[0]&&out.data32[1]&&(!in||(in->data32&&in->data32[0]&&in->data32[1]));
 if(!use64&&!use32)return CLAP_PROCESS_ERROR;
 // CLAP constant_mask marks input channels represented by a single sample.
 // We always render complete output buffers, so clear their constant flags.
 out.constant_mask=0;
 // Deliver GUI-originated parameter changes through the process output event stream
 // when the host does not call params.flush while audio is running.
#ifdef _WIN32
 const uint32_t guiChanges=s->guiDirty.exchange(0,std::memory_order_acq_rel);
 for(uint32_t i=0;i<parameterInfo.size();++i)if(guiChanges&(1u<<i)){
  clap_event_param_value_t ev{};
  ev.header.size=sizeof(ev);ev.header.time=0;
  ev.header.space_id=CLAP_CORE_EVENT_SPACE_ID;ev.header.type=CLAP_EVENT_PARAM_VALUE;
  ev.param_id=i;ev.value=canonical(i,s->guiPending[i].load(std::memory_order_relaxed));
  if(block->out_events&&block->out_events->try_push&&block->out_events->try_push(block->out_events,&ev.header)){
   s->published[i].store(ev.value,std::memory_order_relaxed);
  }else s->guiDirty.fetch_or(1u<<i,std::memory_order_release);
 }
#endif
 s->synchronize();
 // Cache constant inputs before any output writes: in-place output may overwrite sample zero.
 const uint64_t constantMask=in->constant_mask & 3u;
 const double fixed64[2]={use64?in->data64[0][0]:0.0,use64?in->data64[1][0]:0.0};
 const float fixed32[2]={use32?in->data32[0][0]:0.0f,use32?in->data32[1][0]:0.0f};
 const auto* events=block->in_events;
 if(events&&(!events->size||!events->get))return CLAP_PROCESS_ERROR;
 const uint32_t count=events?events->size(events):0;
 auto render=[&](uint32_t begin,uint32_t end){
  if(begin>=end)return;
  const uint64_t constant=constantMask;
  if(use64){
   if(constant){
    for(uint32_t n=begin;n<end;++n){
     const double* src[2]={(constant&1u)?&fixed64[0]:in->data64[0]+n,(constant&2u)?&fixed64[1]:in->data64[1]+n};
     double* dst[2]={out.data64[0]+n,out.data64[1]+n};
     s->processor->process(src,dst,2,1);
    }
   }else{
    const double* src[2]={in->data64[0]+begin,in->data64[1]+begin};
    double* dst[2]={out.data64[0]+begin,out.data64[1]+begin};
    s->processor->process(src,dst,2,static_cast<int>(end-begin));
   }
  }else{
   if(constant){
    for(uint32_t n=begin;n<end;++n){
     const float* src[2]={(constant&1u)?&fixed32[0]:in->data32[0]+n,(constant&2u)?&fixed32[1]:in->data32[1]+n};
     float* dst[2]={out.data32[0]+n,out.data32[1]+n};
     s->processor->process(src,dst,2,1);
    }
   }else{
    const float* src[2]={in->data32[0]+begin,in->data32[1]+begin};
    float* dst[2]={out.data32[0]+begin,out.data32[1]+begin};
    s->processor->process(src,dst,2,static_cast<int>(end-begin));
   }
  }
 };
 uint32_t pos=0;
 for(uint32_t i=0;i<count;++i){
  const auto* h=events->get(events,i);if(!h || h->size < sizeof(clap_event_header_t) || h->time >= block->frames_count && !(block->frames_count==0 && h->time==0))continue;
  const uint32_t at=h->time;
  if(at<pos)continue;
  render(pos,at);pos=at;
  event(s,h,true);
 }
 render(pos,block->frames_count);
 return CLAP_PROCESS_CONTINUE;
}
const void* extension(const clap_plugin_t* p,const char* id){
 const void* result=nullptr;
 if(id){
  if(!std::strcmp(id,CLAP_EXT_AUDIO_PORTS))result=&ports;
  else if(!std::strcmp(id,CLAP_EXT_PARAMS))result=&params;
  else if(!std::strcmp(id,CLAP_EXT_STATE))result=&state;
#ifdef _WIN32
  else if(!std::strcmp(id,CLAP_EXT_GUI))result=&gui;
#endif
 }
 trace("plugin.get_extension instance=%llu id=%s supported=%u",static_cast<unsigned long long>(self(p)->serial),id?id:"(null)",unsigned(result!=nullptr));
 return result;
}
void mainThread(const clap_plugin_t*){}
const clap_plugin_t* create(const clap_plugin_factory_t*,const clap_host_t* host,const char* id){
 trace("factory.create id=%s host=%s vendor=%s version=%s clap=%u.%u.%u",id?id:"(null)",host&&host->name?host->name:"(null)",host&&host->vendor?host->vendor:"(null)",host&&host->version?host->version:"(null)",host?host->clap_version.major:0,host?host->clap_version.minor:0,host?host->clap_version.revision:0);
 if(!host||!id||!clap_version_is_compatible(host->clap_version)||std::strcmp(id,pluginId))return nullptr;
 auto* s=new(std::nothrow) Instance(host);if(!s)return nullptr;
 static std::atomic<uint64_t> nextSerial{1};s->serial=nextSerial.fetch_add(1,std::memory_order_relaxed);
 s->plugin.desc=&descriptor;s->plugin.plugin_data=s;
 s->plugin.init=init;s->plugin.destroy=destroy;s->plugin.activate=activate;s->plugin.deactivate=deactivate;
 s->plugin.start_processing=start;s->plugin.stop_processing=stop;s->plugin.reset=reset;s->plugin.process=process;
 s->plugin.get_extension=extension;s->plugin.on_main_thread=mainThread;
 trace("factory.create -> instance=%llu",static_cast<unsigned long long>(s->serial));return &s->plugin;
}
uint32_t count(const clap_plugin_factory_t*){trace("factory.count");return 1;}
const clap_plugin_descriptor_t* describe(const clap_plugin_factory_t*,uint32_t index){trace("factory.describe index=%u id=%s features=audio-effect,stereo",index,pluginId);return index==0?&descriptor:nullptr;}
const clap_plugin_factory_t factory={count,describe,create};
bool entryInit(const char* path){trace("entry.init path=%s",path?path:"(null)");return true;}
void entryDeinit(){trace("entry.deinit");}
const void* getFactory(const char* id){trace("entry.get_factory id=%s",id?id:"(null)");return id&&!std::strcmp(id,CLAP_PLUGIN_FACTORY_ID)?&factory:nullptr;}
}
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry={CLAP_VERSION_INIT,entryInit,entryDeinit,getFactory};
