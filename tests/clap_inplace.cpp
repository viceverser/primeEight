#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <array>
static const void* ext(const clap_host_t*,const char*){return nullptr;}
static void noop(const clap_host_t*){}
int main(int argc,char** argv){
 assert(argc==2);
 void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=reinterpret_cast<const clap_plugin_entry_t*>(dlsym(lib,"clap_entry"));assert(entry&&entry->init(argv[1]));
 auto* factory=static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));assert(factory);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="inplace";host.vendor="PrimeEight";host.url="";host.version="1";host.get_extension=ext;host.request_restart=noop;host.request_process=noop;host.request_callback=noop;
 const auto* d=factory->get_plugin_descriptor(factory,0);
 auto* p=factory->create_plugin(factory,&host,d->id);assert(p&&p->init(p));
 const auto* ports=static_cast<const clap_plugin_audio_ports_t*>(p->get_extension(p,CLAP_EXT_AUDIO_PORTS));assert(ports);
 clap_audio_port_info_t info{};assert(ports->get(p,0,true,&info));assert(info.in_place_pair==0);
 assert(ports->get(p,0,false,&info));assert(info.in_place_pair==0);
 assert(p->activate(p,48000,1,128));assert(p->start_processing(p));
 // Compare non-aliased and aliased buffers after an explicit engine reset.
 std::array<float,64> left{},right{},outL{},outR{};
 for(int i=0;i<64;++i){left[i]=(i%9==0)?0.7f:0.1f;right[i]=(i%7==0)?-0.5f:0.2f;}
 auto origL=left,origR=right;
 float* input[]={left.data(),right.data()};float* output[]={outL.data(),outR.data()};
 clap_audio_buffer_t in{};in.data32=input;in.channel_count=2;
 clap_audio_buffer_t out{};out.data32=output;out.channel_count=2;
 clap_process_t block{};block.frames_count=64;block.audio_inputs=&in;block.audio_inputs_count=1;block.audio_outputs=&out;block.audio_outputs_count=1;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 p->reset(p);
 input[0]=origL.data();input[1]=origR.data();out.data32=input;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 for(int i=0;i<64;++i){assert(std::isfinite(origL[i])&&std::isfinite(origR[i]));assert(std::fabs(origL[i]-outL[i])<1e-6f);assert(std::fabs(origR[i]-outR[i])<1e-6f);}
 p->stop_processing(p);p->deactivate(p);p->destroy(p);entry->deinit();dlclose(lib);
 std::puts("CLAP in-place processing: PASS");
}
