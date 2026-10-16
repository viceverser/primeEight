#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cstdio>
static const void* ext(const clap_host_t*,const char*){return nullptr;}
static void noop(const clap_host_t*){}
int main(int argc,char**argv){
 assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(argv[1]));
 auto* factory=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(factory);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="reset boundaries";host.vendor="test";host.url="";host.version="1";host.get_extension=ext;host.request_restart=noop;host.request_process=noop;host.request_callback=noop;
 auto* p=factory->create_plugin(factory,&host,factory->get_plugin_descriptor(factory,0)->id);assert(p&&p->init(p));
 p->reset(p); // Defensive check: no engine exists before activation.
 assert(p->activate(p,48000,1,64));p->reset(p);
 assert(p->start_processing(p));p->reset(p);
 p->stop_processing(p);p->reset(p);p->deactivate(p);p->reset(p);
 assert(p->activate(p,44100,1,128));p->reset(p);p->deactivate(p);
 p->destroy(p);entry->deinit();dlclose(lib);
 std::puts("CLAP reset callback boundaries: PASS");
}
