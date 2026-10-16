#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cstring>
#include <cstdio>
static const void* host_ext(const clap_host_t*,const char*) {return nullptr;}
static void host_restart(const clap_host_t*) {}
static void host_process(const clap_host_t*) {}
static void host_callback(const clap_host_t*) {}
int main(int argc,char**argv){
 assert(argc==2);
 void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!lib){std::fprintf(stderr,"%s\n",dlerror());return 1;}
 auto* entry=reinterpret_cast<const clap_plugin_entry_t*>(dlsym(lib,"clap_entry"));assert(entry);
 assert(clap_version_is_compatible(entry->clap_version));assert(entry->init(argv[1]));
 auto* factory=static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));assert(factory&&factory->get_plugin_count(factory)==1);
 auto* desc=factory->get_plugin_descriptor(factory,0);assert(desc&&std::strcmp(desc->id,"org.codeyvybez.primeeight")==0);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="PrimeEight test host";host.vendor="Codey Vybez";host.url="";host.version="1";host.get_extension=host_ext;host.request_restart=host_restart;host.request_process=host_process;host.request_callback=host_callback;
 const auto* plugin=factory->create_plugin(factory,&host,desc->id);assert(plugin&&plugin->init(plugin));
 auto* ports=static_cast<const clap_plugin_audio_ports_t*>(plugin->get_extension(plugin,CLAP_EXT_AUDIO_PORTS));assert(ports&&ports->count(plugin,true)==1&&ports->count(plugin,false)==1);
 auto* params=static_cast<const clap_plugin_params_t*>(plugin->get_extension(plugin,CLAP_EXT_PARAMS));assert(params&&params->count(plugin)==11);
 assert(plugin->activate(plugin,48000,16,256));assert(plugin->start_processing(plugin));
 float left[64]={},right[64]={},outl[64]={},outr[64]={};left[0]=1;right[0]=1;
 float* indata[]={left,right};float* outdata[]={outl,outr};
 clap_audio_buffer_t in{};in.data32=indata;in.channel_count=2;
 clap_audio_buffer_t out{};out.data32=outdata;out.channel_count=2;
 clap_process_t process{};process.frames_count=64;process.audio_inputs=&in;process.audio_inputs_count=1;process.audio_outputs=&out;process.audio_outputs_count=1;
 assert(plugin->process(plugin,&process)==CLAP_PROCESS_CONTINUE);
 plugin->stop_processing(plugin);plugin->deactivate(plugin);plugin->destroy(plugin);entry->deinit();dlclose(lib);
 std::puts("CLAP factory, lifecycle, ports, parameters, audio processing: PASS");
}
