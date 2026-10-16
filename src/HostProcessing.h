#pragma once
#include "PluginContract.h"
#include <cstdint>
#include <cmath>
namespace primeeight {
struct ParameterEvent { uint32_t sampleOffset; uint32_t parameterId; double plainValue; };
// Host-independent event processing. Events must be sorted by sampleOffset.
// The adapter converts CLAP/VST3 events to this format, preserving sample positions.
inline bool processEvents(Processor& processor,const float* const* inputs,float* const* outputs,
                          int channels,uint32_t frames,const ParameterEvent* events,size_t eventCount){
 if(channels<1||channels>2||!outputs||(!events&&eventCount))return false;
 uint32_t previous=0;
 for(size_t i=0;i<eventCount;i++){
  const auto& e=events[i];
  if(e.sampleOffset>=frames||e.sampleOffset<previous||e.parameterId>=parameterInfo.size()||!std::isfinite(e.plainValue))return false;
  previous=e.sampleOffset;
 }
 uint32_t pos=0;
 auto render=[&](uint32_t start,uint32_t end){
  if(end==start)return;
  const float* in[2]={nullptr,nullptr};float* out[2]={nullptr,nullptr};
  for(int c=0;c<channels;c++){
   in[c]=(inputs&&inputs[c])?inputs[c]+start:nullptr;
   out[c]=outputs[c]?outputs[c]+start:nullptr;
  }
  processor.process(in,out,channels,static_cast<int>(end-start));
 };
 for(size_t i=0;i<eventCount;i++){
  const auto& e=events[i];render(pos,e.sampleOffset);pos=e.sampleOffset;
  processor.set(static_cast<Parameter>(e.parameterId),e.plainValue);
 }
 render(pos,frames);return true;
}
}
