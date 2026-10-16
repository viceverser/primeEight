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
#endif
 std::array<std::atomic<double>,parameterInfo.size()> published{};
 explicit Instance(const clap_host_t* h):host(h),processor(new Processor(48000)){
  for(size_t i=0;i<parameterInfo.size();++i)published[i].store(parameterInfo[i].initial,std::memory_order_relaxed);
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
void flush(const clap_plugin_t* plugin,const clap_input_events_t* in,const clap_output_events_t*){
 if(!in||!in->size||!in->get)return;for(uint32_t i=0;i<in->size(in);++i)event(self(plugin),in->get(in,i),false);
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
constexpr uint32_t guiWidth=420,guiHeight=170;
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
 s->guiWindow=CreateWindowExA(0,"STATIC","PrimeEight - CLAP compatibility editor",
  WS_CHILD|SS_CENTER|SS_CENTERIMAGE,0,0,int(guiWidth),int(guiHeight),
  nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
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
 HWND old=SetParent(s->guiWindow,hwnd);
 if(!old&&GetLastError()!=0){trace("gui.set_parent Win32 error=%lu",GetLastError());return false;}
 SetWindowPos(s->guiWindow,nullptr,0,0,int(guiWidth),int(guiHeight),
  SWP_NOZORDER|SWP_NOACTIVATE);
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
