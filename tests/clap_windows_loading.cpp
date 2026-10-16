// Windows native smoke test: verify that the .clap DLL exports a valid CLAP factory.
#include <clap/clap.h>
#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
 if(argc!=2)return 2;
 HMODULE dll=LoadLibraryA(argv[1]);
 if(!dll){std::fprintf(stderr,"LoadLibrary failed: %lu\n",GetLastError());return 1;}
 auto* entry=reinterpret_cast<const clap_plugin_entry_t*>(GetProcAddress(dll,"clap_entry"));
 if(!entry||!clap_version_is_compatible(entry->clap_version)||!entry->init(argv[1])){FreeLibrary(dll);return 1;}
 auto* factory=static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
 bool ok=factory&&factory->get_plugin_count(factory)==1;
 if(ok){const auto* d=factory->get_plugin_descriptor(factory,0);ok=d&&d->id&&std::strcmp(d->id,"org.codeyvybez.primeeight")==0;}
 entry->deinit();FreeLibrary(dll);
 std::puts(ok?"Windows CLAP factory: PASS":"Windows CLAP factory: FAIL");
 return ok?0:1;
}
#else
int main(){return 0;}
#endif
