#include "PrimeEightProcessor.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
int main(){
 using namespace primeeight;
 Processor p(48000);
 p.set(Parameter::Mix,1.0);
 p.set(Parameter::Tap1Ms,15.0);
 p.set(Parameter::Feedback,0.5);
 float left[4096]={},right[4096]={},outL[4096]={},outR[4096]={};
 left[0]=1.0f;
 const float* in[]={left,right};float* out[]={outL,outR};
 p.process(in,out,2,4096);
 const auto& memory=p.engine().debugMemory();
 assert(std::any_of(memory.begin(),memory.end(),[](double v){return std::abs(v)>1e-12;}));
 const auto before=memory;
 for(int mode=1;mode<=3;++mode){
  p.set(Parameter::Multiply,mode);
  assert(p.engine().multiply()==(1<<mode));
  assert(p.rightEngine().multiply()==(1<<mode));
  assert(p.engine().debugMemory()==before);
 }
 p.reset();
 assert(std::all_of(memory.begin(),memory.end(),[](double v){return v==0.0;}));
 std::puts("Clock multiply retains memory; reset clears memory: PASS");
}
