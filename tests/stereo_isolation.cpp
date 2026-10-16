#include "../src/PrimeEightProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace primeeight;
int main(){
 Processor p(48000);p.set(Parameter::Mix,1);p.set(Parameter::Tap1Ms,20);
 float l[256]{},r[256]{},ol[256]{},orr[256]{};l[0]=1;
 const float* in[]={l,r};float* out[]={ol,orr};
 double leftEnergy=0,rightEnergy=0;
 for(int b=0;b<30;b++){
  p.process(in,out,2,256);
  for(int i=0;i<256;i++){
   assert(std::isfinite(ol[i])&&std::isfinite(orr[i]));
   leftEnergy+=std::abs(ol[i]);rightEnergy+=std::abs(orr[i]);
  }
  l[0]=0;
 }
 assert(leftEnergy>0.00001);assert(rightEnergy==0);
 // Verify a right-only impulse never contaminates left.
 Processor q(48000);q.set(Parameter::Tap1Ms,20);
 r[0]=1;double right=0,left=0;
 for(int b=0;b<30;b++){
  q.process(in,out,2,256);
  for(int i=0;i<256;i++){left+=std::abs(ol[i]);right+=std::abs(orr[i]);}
  r[0]=0;
 }
 assert(right>0.00001);assert(left==0);
 std::cout<<"PASS independent stereo delay memories and channel isolation\n";
}
