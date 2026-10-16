#include "HostProcessing.h"
#include <cassert>
#include <cmath>
#include <array>
using namespace primeeight;
int main(){
 Processor p(48000);std::array<float,256> in{},out{};in.fill(0.5f);
 const float* ins[]={in.data()};float* outs[]={out.data()};
 ParameterEvent ev[]={{0,6,0.0},{128,6,1.0}};
 assert(processEvents(p,ins,outs,1,256,ev,2));
 for(int i=0;i<128;i++)assert(std::abs(out[i]-0.5f)<1e-6);
 assert(p.get(Parameter::Mix)==1.0);
 auto before=p.get(Parameter::Feedback);
 ParameterEvent invalid[]={{80,5,0.3},{40,5,0.5}};
 assert(!processEvents(p,ins,outs,1,256,invalid,2));
 assert(p.get(Parameter::Feedback)==before);
 ParameterEvent bad[]={{256,5,0.2}};
 assert(!processEvents(p,ins,outs,1,256,bad,1));
 assert(processEvents(p,ins,outs,1,256,nullptr,0));
 auto saved=saveState(p);Processor restored;assert(loadState(restored,saved.data(),saved.size()));
 assert(restored.get(Parameter::Mix)==p.get(Parameter::Mix));
}
