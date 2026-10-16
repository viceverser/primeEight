#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstdio>
static const void* ext(const clap_host_t*,const char*){return nullptr;}
static void noop(const clap_host_t*){}
struct Events {clap_event_param_value_t e{};};
static uint32_t count(const clap_input_events_t*){return 1;}
static const clap_event_header_t* get(const clap_input_events_t* l,uint32_t){return &static_cast<Events*>(l->ctx)->e.header;}
int main(int argc,char**argv){
 assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(argv[1]));
 auto* f=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(f);
 clap_host_t h{};h.clap_version=CLAP_VERSION;h.name="zero-frame";h.vendor="test";h.url="";h.version="1";h.get_extension=ext;h.request_restart=noop;h.request_process=noop;h.request_callback=noop;
 auto* p=f->create_plugin(f,&h,f->get_plugin_descriptor(f,0)->id);assert(p&&p->init(p));
 auto* params=(const clap_plugin_params_t*)p->get_extension(p,CLAP_EXT_PARAMS);assert(params);
 assert(p->activate(p,48000,1,64)&&p->start_processing(p));
 float a[1]={},b[1]={},c[1]={},d[1]={};float* ins[]={a,b};float* outs[]={c,d};
 clap_audio_buffer_t in{};in.channel_count=2;in.data32=ins;clap_audio_buffer_t out{};out.channel_count=2;out.data32=outs;
 Events e{};e.e.header.size=sizeof(e.e);e.e.header.space_id=CLAP_CORE_EVENT_SPACE_ID;e.e.header.type=CLAP_EVENT_PARAM_VALUE;e.e.header.time=0;e.e.param_id=6;e.e.value=.321;
 clap_input_events_t events{};events.ctx=&e;events.size=count;events.get=get;
 clap_process_t block{};block.frames_count=0;block.audio_inputs=&in;block.audio_inputs_count=1;block.audio_outputs=&out;block.audio_outputs_count=1;block.in_events=&events;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 double value=0;assert(params->get_value(p,6,&value)&&std::abs(value-.321)<1e-9);
 p->stop_processing(p);p->deactivate(p);p->destroy(p);entry->deinit();dlclose(lib);
 std::puts("CLAP zero-frame parameter events: PASS");
}
