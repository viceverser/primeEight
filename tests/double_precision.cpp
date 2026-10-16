#include "PrimeEightProcessor.h"
#include <array>
#include <cmath>
#include <cstdio>
int main(){
 primeeight::Processor p(48000);
 p.set(primeeight::Parameter::Mix,0);
 std::array<double,128> l{},r{},ol{},orr{};
 l[0]=0.75;r[0]=-0.25;
 const double* in[2]={l.data(),r.data()};double* out[2]={ol.data(),orr.data()};
 p.process(in,out,2,128);
 for(size_t i=0;i<128;++i)if(std::abs(ol[i]-l[i])>1e-12||std::abs(orr[i]-r[i])>1e-12)return 1;
 p.set(primeeight::Parameter::Mix,1);
 p.reset();p.process(in,out,2,128);
 for(double v:ol)if(!std::isfinite(v))return 2;
 for(double v:orr)if(!std::isfinite(v))return 3;
 std::puts("double precision processing passed");return 0;
}
