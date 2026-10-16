#include <clap/clap.h>
#include <dlfcn.h>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <array>
struct Host { clap_host_t host{}; Host(){host.clap_version=CLAP_VERSION;host.name="PrimeEight stress";host.vendor="test";host.url="";host.version="1";host.get_extension=[](const clap_host_t*,const char*)->const void*{return nullptr;};host.request_restart=[](const clap_host_t*){};host.request_process=[](const clap_host_t*){};host.request_callback=[](const clap_host_t*){};} };
struct Input {clap_input_events_t iface{};clap_event_param_value_t event{};Input(){iface.ctx=this;iface.size=[](const clap_input_events_t*)->uint32_t{return 1;};iface.get=[](const clap_input_events_t* i,uint32_t n)->const clap_event_header_t*{return n?nullptr:&static_cast<Input*>(i->ctx)->event.header;};event.header.size=sizeof(event);event.header.time=0;event.header.space_id=CLAP_CORE_EVENT_SPACE_ID;event.header.type=CLAP_EVENT_PARAM_VALUE;event.param_id=6;event.value=.25;}};
int main(int argc,char** argv){assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(""));auto* factory=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(factory&&factory->get_plugin_count(factory)==1);Host host;auto* p=factory->create_plugin(factory,&host.host,"org.codeyvybez.primeeight");assert(p&&p->init(p));auto* params=(const clap_plugin_params_t*)p->get_extension(p,CLAP_EXT_PARAMS);assert(params);assert(p->activate(p,48000,1,128)&&p->start_processing(p));
 std::atomic<bool> running{true};std::atomic<int> reads{0};
 std::thread reader([&]{while(running.load(std::memory_order_acquire)){for(clap_id i=0;i<11;i++){double v;assert(params->get_value(p,i,&v));assert(std::isfinite(v));}++reads;}});
 std::array<float,128> il{},ir{},ol{},orr{};float* inptr[]={il.data(),ir.data()},*outptr[]={ol.data(),orr.data()};clap_audio_buffer_t in{},out{};in.data32=inptr;in.channel_count=2;out.data32=outptr;out.channel_count=2;
 Input events;clap_process_t block{};block.frames_count=128;block.audio_inputs=&in;block.audio_inputs_count=1;block.audio_outputs=&out;block.audio_outputs_count=1;block.in_events=&events.iface;
 for(int i=0;i<20000;i++){events.event.value=(i%100)/100.0;assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);for(float x:ol)assert(std::isfinite(x));}
 running.store(false,std::memory_order_release);reader.join();assert(reads>0);p->stop_processing(p);p->deactivate(p);p->destroy(p);entry->deinit();dlclose(lib);std::cout<<"CLAP concurrent parameter reads: "<<reads<<" cycles, 20000 audio blocks passed\n";}
