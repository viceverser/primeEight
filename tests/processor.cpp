#include "../src/PrimeEightProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 primeeight::Processor p;
 assert(primeeight::parameterInfo.size()==static_cast<size_t>(primeeight::Parameter::Count));
 p.set(primeeight::Parameter::Multiply,3);assert(p.engine().internalClock()==4000);
 p.set(primeeight::Parameter::Feedback,5);assert(p.get(primeeight::Parameter::Feedback)==0.95);
 p.set(primeeight::Parameter::Tap1Ms,100);p.set(primeeight::Parameter::Hold,0);
 float left[512]{},right[512]{},outL[512]{},outR[512]{};left[0]=1;
 const float* in[]{left,right};float* out[]{outL,outR};
 for(int block=0;block<300;block++){
  p.process(in,out,2,512);
  for(int i=0;i<512;i++)assert(std::isfinite(outL[i])&&std::isfinite(outR[i]));
  left[0]=0;
 }
 p.set(primeeight::Parameter::Hold,1);
 const auto frozen=p.engine().debugMemory();
 const auto frozenRight=p.rightEngine().debugMemory();
 for(int i=0;i<100;i++)p.process(in,out,2,512);
 assert(p.engine().debugMemory()==frozen);
 assert(p.rightEngine().debugMemory()==frozenRight);
 p.set(primeeight::Parameter::ModRate,2.5);
 p.set(primeeight::Parameter::ModDepthMs,4);
 p.process(in,out,2,512);
 for(float v:outL)assert(std::isfinite(v));
 std::cout<<"PASS parameter bridge, multiply, stereo buffers, bounds, hold, modulation\n";
}
