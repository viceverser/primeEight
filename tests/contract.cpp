#include "PluginContract.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace primeeight;
 Processor a;for(uint32_t i=0;i<parameterInfo.size();i++){
  assert(std::abs(normalizedToPlain(i,plainToNormalized(i,parameterInfo[i].initial))-parameterInfo[i].initial)<1e-8);
 }
 a.set(Parameter::Multiply,3);a.set(Parameter::Feedback,0.7);a.set(Parameter::Hold,1);
 auto b=saveState(a);Processor c;assert(loadState(c,b.data(),b.size()));
 assert(c.get(Parameter::Multiply)==3 && c.get(Parameter::Feedback)==0.7 && c.get(Parameter::Hold)==1);
 auto corrupt=b;corrupt[0]=0;assert(!loadState(c,corrupt.data(),corrupt.size()));
 corrupt=b;corrupt[4]=2;assert(!loadState(c,corrupt.data(),corrupt.size()));
 assert(!loadState(c,b.data(),b.size()-1));
 std::cout<<"Shared plugin contract tests passed\n";
}
