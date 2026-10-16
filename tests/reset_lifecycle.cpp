#include "PrimeEightProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace primeeight;
int main(){
 Processor p(48000); p.set(Parameter::Mix,1);p.set(Parameter::Tap1Ms,10);p.set(Parameter::Tap1Level,1);
 float input[4096]={},output[4096]={};input[0]=1;const float* src[]={input};float* dst[]={output};
 p.process(src,dst,1,4096);
 bool nonzero=false;for(float x:output)if(std::abs(x)>1e-8f)nonzero=true;assert(nonzero);
 const auto before=p.get(Parameter::Tap1Ms);
 p.reset();assert(p.get(Parameter::Tap1Ms)==before);
 float silent[4096]={};const float* zero[]={silent};p.process(zero,dst,1,4096);
 for(float x:output)assert(std::abs(x)<1e-8f);
 p.set(Parameter::Multiply,3);p.reset();assert(p.engine().multiply()==8);
 std::cout<<"Reset clears both engines without recreating processor; parameters preserved\n";
}
