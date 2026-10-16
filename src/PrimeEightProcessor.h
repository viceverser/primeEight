#pragma once
#include "PrimeEightCore.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace primeeight {
// Host-independent parameter/processing layer. VST3 SDK adapter is a separate target.
enum class Parameter : uint32_t { Multiply, Tap1Ms, Tap2Ms, Tap1Level, Tap2Level, Feedback, Mix, Hold, ModRate, ModDepthMs, SmoothingMs, Count };
struct ParameterInfo {const char* name; double minimum,maximum,initial;};
inline constexpr std::array<ParameterInfo,11> parameterInfo{{
 {"Multiply",0,3,0},{"Tap 1 delay (ms)",0,1023,50},{"Tap 2 delay (ms)",0,1023,100},
 {"Tap 1 level",-2,2,1},{"Tap 2 level",-2,2,0},{"Feedback",-0.95,0.95,0},
 {"Mix",0,1,1},{"Repeat Hold",0,1,0},{"Mod rate (Hz)",0,20,0},
 {"Mod depth (ms)",0,20,0},{"Smoothing (ms)",0,500,15}}};
class Processor {
public:
 explicit Processor(double rate=48000):core(rate),rightCore(rate){for(size_t i=0;i<values.size();++i)values[i]=parameterInfo[i].initial; apply();}
 void set(Parameter p,double value){auto i=static_cast<size_t>(p);if(i>=values.size()||!std::isfinite(value))return;const double next=std::clamp(value,parameterInfo[i].minimum,parameterInfo[i].maximum);if(values[i]==next)return;values[i]=next;applyOne(i);}
 double get(Parameter p)const {auto i=static_cast<size_t>(p);return i<values.size()?values[i]:0;}
 template<typename Sample>
 void processTyped(const Sample* const* inputs,Sample* const* outputs,int channels,int frames){
  if(frames<=0||channels<=0||!outputs)return;
  // Two independent mono delay memories: no unintended L/R summing or crosstalk.
  // Mono uses the left engine; stereo uses both engines with identical parameters.
  for(int n=0;n<frames;n++){
   const double l=(inputs&&inputs[0])?inputs[0][n]:0.0;
   const Sample yl=static_cast<Sample>(core.process(l));
   if(outputs[0])outputs[0][n]=yl;
   if(channels>=2){
    const double r=(inputs&&inputs[1])?inputs[1][n]:0.0;
    const Sample yr=static_cast<Sample>(rightCore.process(r));
    if(outputs[1])outputs[1][n]=yr;
   }
  }
 }
 void process(const float* const* in,float* const* out,int channels,int frames){processTyped(in,out,channels,frames);}
 void process(const double* const* in,double* const* out,int channels,int frames){processTyped(in,out,channels,frames);}
 void reset() noexcept {core.reset();rightCore.reset();}
 Core& engine(){return core;}
 Core& rightEngine(){return rightCore;}
private:
 void apply(){for(size_t i=0;i<values.size();++i)applyOne(i);}
 // Apply only the changed control: no redundant parameter writes or delay-slot updates
 // for every sample-positioned automation event.
 void applyOne(size_t i){
  if(i==0){
   const int factors[]{1,2,4,8};
   const int mode=std::clamp(static_cast<int>(std::lround(values[0])),0,3);
   core.setMultiply(factors[mode]);rightCore.setMultiply(factors[mode]);
   // Re-evaluate delay times against the new clock's capacity.
   core.setTapDelay(0,values[1]/1000);rightCore.setTapDelay(0,values[1]/1000);
   core.setTapDelay(1,values[2]/1000);rightCore.setTapDelay(1,values[2]/1000);
   return;
  }
  for(Core* c : {&core,&rightCore}){
   switch(i){
    case 1:c->setTapDelay(0,values[1]/1000);break;
    case 2:c->setTapDelay(1,values[2]/1000);break;
    case 3:c->setTapLevel(0,values[3]);break;
    case 4:c->setTapLevel(1,values[4]);break;
    case 5:c->setFeedback(values[5]);break;
    case 6:c->setMix(values[6]);break;
    case 7:c->setHold(values[7]>=0.5);break;
    case 8:case 9:c->setModulation(values[8],values[9]/1000);break;
    case 10:c->setDelaySmoothing(values[10]/1000);break;
    default:break;
   }
  }
 }
 Core core,rightCore;std::array<double,11> values{};
};
}
