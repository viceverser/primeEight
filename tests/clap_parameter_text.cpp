#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
int main(int argc,char**argv){
 assert(argc==2);
 void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(nullptr));
 auto* factory=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(factory);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="parameter-text-test";host.vendor="test";host.url="";host.version="1";
 auto* plugin=factory->create_plugin(factory,&host,"org.codeyvybez.primeeight");assert(plugin&&plugin->init(plugin));
 auto* params=(const clap_plugin_params_t*)plugin->get_extension(plugin,CLAP_EXT_PARAMS);assert(params);
 clap_param_info_t info{};assert(params->get_info(plugin,0,&info));assert(info.flags&CLAP_PARAM_IS_STEPPED);
 assert(params->get_info(plugin,7,&info));assert(info.flags&CLAP_PARAM_IS_STEPPED);
 char buf[64]{};double v=-1;
 assert(params->value_to_text(plugin,0,2.7,buf,sizeof(buf)));assert(!strcmp(buf,"3"));
 assert(params->text_to_value(plugin,0,"2.7",&v)&&v==3);
 assert(params->text_to_value(plugin,7,"0.6",&v)&&v==1);
 assert(params->text_to_value(plugin,7,"0.4",&v)&&v==0);
 assert(!params->text_to_value(plugin,1,"50junk",&v));
 assert(!params->text_to_value(plugin,1,"nan",&v));
 assert(!params->text_to_value(plugin,1,"inf",&v));
 assert(!params->text_to_value(plugin,1,"1e9999",&v));
 assert(params->text_to_value(plugin,1,"  50.25  ",&v)&&v==50.25);
 assert(!params->value_to_text(plugin,1,50,buf,2));
 plugin->destroy(plugin);entry->deinit();dlclose(lib);
 std::cout<<"CLAP parameter text and stepped values passed\\n";
}
