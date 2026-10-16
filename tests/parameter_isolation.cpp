#include "PrimeEightProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace primeeight;
int main(){
 Processor p(48000);
 p.set(Parameter::Mix,1.0);
 p.set(Parameter::Tap1Ms,1000.0); // 1x limited by 4096-sample memory
 p.set(Parameter::Multiply,3.0); // 8x should allow nearly 1024ms
 assert(p.engine().multiply()==8);
 p.set(Parameter::Tap1Ms,500.0);
 p.set(Parameter::Tap1Level,0.8);
 p.set(Parameter::Feedback,0.4);
 p.set(Parameter::ModRate,2.0);
 p.set(Parameter::ModDepthMs,1.0);
 p.set(Parameter::Hold,1.0);
 assert(p.rightEngine().multiply()==8);
 for(int i=0;i<5000;i++){
  double x= i==0 ? 1.0 : 0.0;
  double y=p.engine().process(x);
  assert(std::isfinite(y));
 }
 // Repeated unchanged parameter publication must be inert.
 for(int i=0;i<10000;i++)p.set(Parameter::Mix,1.0);
 assert(p.get(Parameter::Mix)==1.0);
 std::cout<<"parameter isolation passed\n";
}
