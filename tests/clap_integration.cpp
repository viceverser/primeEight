#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>
#include <algorithm>
static const void* host_ext(const clap_host_t*,const char*) {return nullptr;}
static void noop(const clap_host_t*){}
struct Bytes {std::vector<uint8_t> data;size_t offset=0;};
static int64_t write_bytes(const clap_ostream_t* s,const void* p,uint64_t n){auto& b=*static_cast<Bytes*>(s->ctx);auto* q=static_cast<const uint8_t*>(p);b.data.insert(b.data.end(),q,q+n);return static_cast<int64_t>(n);}
static int64_t read_bytes(const clap_istream_t* s,void* p,uint64_t n){auto& b=*static_cast<Bytes*>(s->ctx);auto amount=std::min<size_t>(n,b.data.size()-b.offset);std::memcpy(p,b.data.data()+b.offset,amount);b.offset+=amount;return static_cast<int64_t>(amount);}
struct Events {std::vector<clap_event_param_value_t> events;};
static uint32_t ev_size(const clap_input_events_t* l){return static_cast<uint32_t>(static_cast<Events*>(l->ctx)->events.size());}
static const clap_event_header_t* ev_get(const clap_input_events_t* l,uint32_t i){auto& e=static_cast<Events*>(l->ctx)->events;return i<e.size()?&e[i].header:nullptr;}
int main(int argc,char** argv){
 assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=reinterpret_cast<const clap_plugin_entry_t*>(dlsym(lib,"clap_entry"));assert(entry&&entry->init(argv[1]));
 auto* f=static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));assert(f);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="integration";host.vendor="PrimeEight";host.url="";host.version="1";host.get_extension=host_ext;host.request_restart=noop;host.request_process=noop;host.request_callback=noop;
 auto* desc=f->get_plugin_descriptor(f,0);auto* p=f->create_plugin(f,&host,desc->id);assert(p&&p->init(p));
 auto* params=static_cast<const clap_plugin_params_t*>(p->get_extension(p,CLAP_EXT_PARAMS));
 auto* state=static_cast<const clap_plugin_state_t*>(p->get_extension(p,CLAP_EXT_STATE));assert(params&&state);
 clap_event_param_value_t mix{};mix.header.size=sizeof(mix);mix.header.space_id=CLAP_CORE_EVENT_SPACE_ID;mix.header.type=CLAP_EVENT_PARAM_VALUE;mix.header.time=32;mix.param_id=6;mix.value=0;
 Events events;events.events.push_back(mix);clap_input_events_t input_events{};input_events.ctx=&events;input_events.size=ev_size;input_events.get=ev_get;
 assert(p->activate(p,48000,16,128));assert(p->start_processing(p));
 float left[64],right[64],ol[64]={},orr[64]={};std::fill_n(left,64,0.4f);std::fill_n(right,64,0.4f);
 float* indata[]={left,right};float* outdata[]={ol,orr};clap_audio_buffer_t in{};in.data32=indata;in.channel_count=2;clap_audio_buffer_t out{};out.data32=outdata;out.channel_count=2;
 clap_process_t block{};block.frames_count=64;block.audio_inputs=&in;block.audio_inputs_count=1;block.audio_outputs=&out;block.audio_outputs_count=1;block.in_events=&input_events;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 double actual=99;assert(params->get_value(p,6,&actual)&&actual==0);
 for(int i=32;i<64;++i)assert(std::abs(ol[i]-0.4f)<1e-5f&&std::abs(orr[i]-0.4f)<1e-5f);
 // Host flush while processing must publish parameters without touching audio-owned DSP.
 mix.value=0.6;mix.header.time=0;events.events[0]=mix;
 params->flush(p,&input_events,nullptr);
 assert(params->get_value(p,6,&actual)&&std::abs(actual-0.6)<1e-10);
 block.in_events=nullptr;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 assert(params->get_value(p,6,&actual)&&std::abs(actual-0.6)<1e-10);
 p->stop_processing(p);p->deactivate(p);
 Bytes bytes;clap_ostream_t output_stream{};output_stream.ctx=&bytes;output_stream.write=write_bytes;assert(state->save(p,&output_stream));assert(!bytes.data.empty());
 mix.value=0.75;mix.header.time=0;events.events[0]=mix;params->flush(p,&input_events,nullptr);assert(params->get_value(p,6,&actual)&&actual==0.75);
 clap_istream_t input_stream{};input_stream.ctx=&bytes;input_stream.read=read_bytes;assert(state->load(p,&input_stream));assert(params->get_value(p,6,&actual)&&std::abs(actual-0.6)<1e-10);
 // Flush while stopped updates the published value; reactivation applies it to DSP.
 mix.value=0.33;events.events[0]=mix;params->flush(p,&input_events,nullptr);
 assert(params->get_value(p,6,&actual)&&std::abs(actual-0.33)<1e-10);
 assert(p->activate(p,48000,16,128));assert(p->start_processing(p));
 block.in_events=nullptr;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 assert(params->get_value(p,6,&actual)&&std::abs(actual-0.33)<1e-10);
 p->stop_processing(p);p->deactivate(p);
 p->destroy(p);entry->deinit();dlclose(lib);std::puts("CLAP timed automation and state round-trip: PASS");
}
