#include <clap/clap.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
int main(int argc,char** argv){
 assert(argc==2);
#ifdef _WIN32
 auto lib=LoadLibraryA(argv[1]);assert(lib);
 auto entry=(const clap_plugin_entry_t*)GetProcAddress(lib,"clap_entry");
#else
 auto lib=dlopen(argv[1],RTLD_NOW);assert(lib);
 auto entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");
#endif
 assert(entry&&entry->init(".") );
 auto f=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
 assert(f&&f->get_plugin_count(f)==1);
 auto desc=f->get_plugin_descriptor(f,0);assert(desc);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="Extension Probe";host.vendor="PrimeEight";host.version="1";
 auto p=f->create_plugin(f,&host,desc->id);assert(p&&p->init(p));
 assert(p->get_extension(p,CLAP_EXT_PARAMS));
 assert(p->get_extension(p,CLAP_EXT_AUDIO_PORTS));
 assert(p->get_extension(p,CLAP_EXT_STATE));
 assert(p->get_extension(p,CLAP_EXT_GUI)==nullptr);
 assert(p->get_extension(p,CLAP_EXT_NOTE_PORTS)==nullptr);
 p->destroy(p);entry->deinit();
#ifdef _WIN32
 FreeLibrary(lib);
#else
 dlclose(lib);
#endif
 std::puts("extension negotiation OK");
}
