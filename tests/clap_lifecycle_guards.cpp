#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cstdio>
#include <cmath>
static const void* ext(const clap_host_t*,const char*){return nullptr;}
static void noop(const clap_host_t*){}
struct OneEvent {clap_event_param_value_t value{};};
static uint32_t size(const clap_input_events_t*){return 1;}
static const clap_event_header_t* get(const clap_input_events_t* l,uint32_t){return &static_cast<OneEvent*>(l->ctx)->value.header;}
int main(int argc,char**argv){
 assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(argv[1]));
 auto* factory=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(factory);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="lifecycle";host.vendor="test";host.url="";host.version="1";host.get_extension=ext;host.request_restart=noop;host.request_process=noop;host.request_callback=noop;
 auto* p=factory->create_plugin(factory,&host,factory->get_plugin_descriptor(factory,0)->id);assert(p&&p->init(p));
 auto* params=(const clap_plugin_params_t*)p->get_extension(p,CLAP_EXT_PARAMS);assert(params);
 assert(!p->start_processing(p));assert(!p->activate(p,NAN,16,128));
 assert(p->activate(p,48000,16,128));assert(p->start_processing(p));assert(!p->start_processing(p));
 float l[16]={},r[16]={},ol[16]={},orr[16]={};float* src[]={l,r};float* dst[]={ol,orr};
 clap_audio_buffer_t input{};input.data32=src;input.channel_count=2;
 clap_audio_buffer_t output{};output.data32=dst;output.channel_count=2;
 clap_process_t block{};block.frames_count=16;block.audio_inputs=&input;block.audio_inputs_count=1;block.audio_outputs=&output;block.audio_outputs_count=1;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 input.channel_count=1;assert(p->process(p,&block)==CLAP_PROCESS_ERROR);input.channel_count=2;
 OneEvent event{};event.value.header.size=sizeof(clap_event_header_t);event.value.header.space_id=CLAP_CORE_EVENT_SPACE_ID;event.value.header.type=CLAP_EVENT_PARAM_VALUE;event.value.param_id=6;event.value.value=0.99;
 clap_input_events_t events{};events.ctx=&event;events.size=size;events.get=get;
 double before=0,after=0;assert(params->get_value(p,6,&before));params->flush(p,&events,nullptr);assert(params->get_value(p,6,&after)&&before==after);
 event.value.header.size=sizeof(event.value);event.value.value=NAN;params->flush(p,&events,nullptr);assert(params->get_value(p,6,&after)&&before==after);
 p->stop_processing(p);assert(p->process(p,&block)==CLAP_PROCESS_ERROR);
 p->deactivate(p);assert(!p->start_processing(p));p->destroy(p);entry->deinit();dlclose(lib);
 std::puts("CLAP lifecycle and malformed input guards: PASS");
}
