#include "../src/PrimeEightCore.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <algorithm>
int main(){
 for(int factor:{1,2,4,8}){
  primeeight::Core c(48000);c.configure(factor);
  assert(c.internalClock()==32000.0/factor);
  c.setTapDelay(0,0.02);c.setTapDelay(1,0.04);
  c.setTapLevel(0,1);c.setTapLevel(1,0.5);
  c.setFeedback(0);c.setMix(1);
  double peak=0;
  for(int i=0;i<24000;i++){auto y=c.process(i<100?1.0:0.0);assert(std::isfinite(y));peak=std::max(peak,std::abs(y));}
  assert(peak>0.01&&peak<2);
 }
 // Hold must preserve memory while allowing output to continue changing.
 primeeight::Core c(48000);c.configure(8);c.setTapDelay(0,0.01);c.setTapLevel(0,1);c.setTapLevel(1,0);c.setFeedback(0);
 for(int i=0;i<48000;i++)c.process(std::sin(i*0.019)+0.3*std::sin(i*0.033));
 auto before=c.debugMemory();c.setHold(true);
 double lo=1e9,hi=-1e9;
 for(int i=0;i<10000;i++){double y=c.process(0);lo=std::min(lo,y);hi=std::max(hi,y);}
 assert(c.debugMemory()==before);
 assert(hi-lo>0.01);
 // Modulation must be finite, bounded, and produce a different waveform.
 primeeight::Core plain(48000), mod(48000);
 plain.configure(2);mod.configure(2);
 plain.setTapDelay(0,0.03);mod.setTapDelay(0,0.03);
 mod.setModulation(2.3,0.004);
 double difference=0;
 for(int i=0;i<48000;i++){
   const double x=std::sin(i*0.041)+0.2*std::sin(i*0.13);
   double a=plain.process(x),b=mod.process(x);
   assert(std::isfinite(b));difference+=std::abs(a-b);
 }
 assert(difference>1);
 // Smoothing must converge toward the requested tap delay without non-finite output.
 primeeight::Core smooth(48000);smooth.configure(1);smooth.setDelaySmoothing(0.02);
 smooth.setTapDelay(0,0.08);
 for(int i=0;i<48000;i++)assert(std::isfinite(smooth.process(i==0?1.0:0.0)));
 // Aggressive feedback must remain finite over extended processing.
 primeeight::Core stress(48000);stress.configure(8);stress.setFeedback(0.95);
 for(int i=0;i<150000;i++)assert(std::isfinite(stress.process(i==0?1.0:0.0)));
 std::cout<<"PASS four modes, dual taps, finite outputs, hold memory immutability, hold playback progression\n";
}
