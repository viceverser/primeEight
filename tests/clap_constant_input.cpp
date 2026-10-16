#include <clap/clap.h>
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstdio>
static const void* ext(const clap_host_t*,const char*){return nullptr;}
static void noop(const clap_host_t*){}
int main(int argc,char**argv){
 assert(argc==2);void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
 auto* entry=(const clap_plugin_entry_t*)dlsym(lib,"clap_entry");assert(entry&&entry->init(argv[1]));
 auto* factory=(const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);assert(factory);
 clap_host_t host{};host.clap_version=CLAP_VERSION;host.name="constant input";host.vendor="test";host.url="";host.version="1";host.get_extension=ext;host.request_restart=noop;host.request_process=noop;host.request_callback=noop;
 auto* p=factory->create_plugin(factory,&host,factory->get_plugin_descriptor(factory,0)->id);assert(p&&p->init(p));
 assert(p->activate(p,48000,1,64));assert(p->start_processing(p));
 float left[64],right[64];for(int i=0;i<64;i++){left[i]=0.25f;right[i]=-0.5f;}
 float ol[64]={},orr[64]={};float* ins[2]={left,right};float* outs[2]={ol,orr};
 clap_audio_buffer_t in{};in.data32=ins;in.channel_count=2;in.constant_mask=3;
 clap_audio_buffer_t out{};out.data32=outs;out.channel_count=2;out.constant_mask=3;
 clap_process_t block{};block.frames_count=64;block.audio_inputs=&in;block.audio_inputs_count=1;block.audio_outputs=&out;block.audio_outputs_count=1;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);assert(out.constant_mask==0);
 float referenceL[64],referenceR[64];for(int i=0;i<64;i++){referenceL[i]=ol[i];referenceR[i]=orr[i];assert(std::isfinite(ol[i])&&std::isfinite(orr[i]));}
 p->reset(p);in.constant_mask=0;out.constant_mask=0;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 for(int i=0;i<64;i++){assert(std::fabs(ol[i]-referenceL[i])<1e-6f);assert(std::fabs(orr[i]-referenceR[i])<1e-6f);}
 // Constant inputs with in-place output must retain their original first sample.
 p->reset(p);in.constant_mask=3;out.constant_mask=0;
 float il[64],ir[64];for(int i=0;i<64;i++){il[i]=0.25f;ir[i]=-0.5f;}
 float* shared[2]={il,ir};in.data32=shared;out.data32=shared;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 for(int i=0;i<64;i++){assert(std::fabs(il[i]-referenceL[i])<1e-6f);assert(std::fabs(ir[i]-referenceR[i])<1e-6f);}
 in.data32=ins;out.data32=outs;
 // Mixed constant/nonconstant channel and 64-bit path.
 p->reset(p);in.constant_mask=1;
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);
 double dl[64],dr[64],yl[64]={},yr[64]={};for(int i=0;i<64;i++){dl[i]=0.25;dr[i]=-0.5;}
 double* din[2]={dl,dr};double* dout[2]={yl,yr};clap_audio_buffer_t in64{};in64.data64=din;in64.channel_count=2;in64.constant_mask=3;
 clap_audio_buffer_t out64{};out64.data64=dout;out64.channel_count=2;out64.constant_mask=3;
 block.audio_inputs=&in64;block.audio_outputs=&out64;p->reset(p);
 assert(p->process(p,&block)==CLAP_PROCESS_CONTINUE);assert(out64.constant_mask==0);
 for(int i=0;i<64;i++){assert(std::isfinite(yl[i])&&std::isfinite(yr[i]));}
 p->stop_processing(p);p->deactivate(p);p->destroy(p);entry->deinit();dlclose(lib);
 std::puts("CLAP constant input buffers: PASS");
}
