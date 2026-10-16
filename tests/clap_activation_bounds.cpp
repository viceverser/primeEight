#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cstdio>
#include <cmath>
static const void* ext(const clap_host_t*,const char*){return nullptr;}
static void noop(const clap_host_t*){}
int main(int argc,char**argv){
 assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(argv[1]));
 auto* factory=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(factory);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="activation bounds";host.vendor="test";host.url="";host.version="1";host.get_extension=ext;host.request_restart=noop;host.request_process=noop;host.request_callback=noop;
 auto* p=factory->create_plugin(factory,&host,factory->get_plugin_descriptor(factory,0)->id);assert(p&&p->init(p));
 assert(!p->activate(p,48000,0,64));assert(!p->activate(p,48000,128,64));assert(!p->activate(p,48000,1,0xffffffff));
 assert(p->activate(p,48000,1,64));assert(!p->activate(p,48000,1,64));assert(p->start_processing(p));
 float l[65]={},r[65]={},ol[65]={},orr[65]={};float* src[]={l,r};float* dst[]={ol,orr};
 clap_audio_buffer_t in{};in.data32=src;in.channel_count=2;
 clap_audio_buffer_t out{};out.data32=dst;out.channel_count=2;
 clap_process_t block{};block.audio_inputs=&in;block.audio_inputs_count=1;block.audio_outputs=&out;block.audio_outputs_count=1;
 block.frames_count=65;assert(p->process(p,&block)==CLAP_PROCESS_ERROR);
 block.frames_count=64;assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 block.audio_outputs_count=2;assert(p->process(p,&block)==CLAP_PROCESS_ERROR);block.audio_outputs_count=1;
 block.audio_inputs_count=2;assert(p->process(p,&block)==CLAP_PROCESS_ERROR);block.audio_inputs_count=1;
 block.audio_inputs=nullptr;assert(p->process(p,&block)==CLAP_PROCESS_ERROR);block.audio_inputs=&in;
 clap_input_events_t malformed{};block.in_events=&malformed;assert(p->process(p,&block)==CLAP_PROCESS_ERROR);block.in_events=nullptr;
 p->stop_processing(p);p->deactivate(p);
 assert(p->activate(p,44100,1,128));assert(p->start_processing(p));block.frames_count=65;assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 p->stop_processing(p);p->deactivate(p);p->destroy(p);entry->deinit();dlclose(lib);
 std::puts("CLAP activation and block size bounds: PASS");
}
