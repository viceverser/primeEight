#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace primeeight {
// Functional prototype: timing and filters are NOT authenticated Model 93 hardware.
class Core {
public:
 explicit Core(double sampleRate=48000):fs(sampleRate>0?sampleRate:48000){configure(1);}
 void configure(int multiply){factor=(multiply==1||multiply==2||multiply==4||multiply==8)?multiply:1;clock=32000.0/factor;phase=0;cursor=0;modPhase=0;if(memory.size()!=4096)memory.assign(4096,0);else std::fill(memory.begin(),memory.end(),0.0);held={0,0};lowpass=0;smoothedDelays=delays;updateSlots();}
 // Change the internal clock without clearing the circulating delay memory.
 // This is a transport-speed change, not a reset.
 void setMultiply(int multiply) noexcept {
  const int next=(multiply==1||multiply==2||multiply==4||multiply==8)?multiply:1;
  if(next==factor)return;
  factor=next;clock=32000.0/factor;updateSlots();
 }
 void reset() noexcept { phase=0;cursor=0;modPhase=0;std::fill(memory.begin(),memory.end(),0.0);held={0,0};lowpass=0;smoothedDelays=delays; }
 void setTapDelay(int tap,double seconds){if(tap>=0&&tap<2){delays[tap]=std::clamp(seconds,1.0/clock,4095.0/clock);updateSlots();}}
 void setDelay(double seconds){setTapDelay(0,seconds);}
 void setTapLevel(int tap,double level){if(tap>=0&&tap<2)levels[tap]=std::clamp(level,-2.0,2.0);}
 void setFeedback(double f){feedback=std::clamp(f,-0.95,0.95);}
 void setMix(double m){mix=std::clamp(m,0.0,1.0);}
 void setHold(bool h){hold=h;}
 void setDelaySmoothing(double seconds){smoothingSeconds=std::clamp(seconds,0.0,0.5);}
 void setModulation(double rateHz,double depthSeconds){modRate=std::clamp(rateHz,0.0,20.0);modDepth=std::clamp(depthSeconds,0.0,0.02);}
 double process(double input){
  if(!std::isfinite(input))input=0;
  const double cutoff=std::min(0.4*clock,0.45*fs);
  const double a=1.0-std::exp(-6.283185307179586*cutoff/fs);
  lowpass+=a*(input-lowpass);
  modPhase+=6.283185307179586*modRate/fs; if(modPhase>=6.283185307179586)modPhase=std::fmod(modPhase,6.283185307179586);
  phase+=clock/fs;
  while(phase>=1.0){
   phase-=1.0;
   for(int t=0;t<2;t++){const double coeff=smoothingSeconds<=0?1.0:1.0-std::exp(-1.0/(smoothingSeconds*clock));
    smoothedDelays[t]+=coeff*(delays[t]-smoothedDelays[t]);
    const double offset=std::clamp(smoothedDelays[t]*clock+modDepth*clock*std::sin(modPhase),1.0,4095.0);
    const size_t whole=(size_t)offset; const double frac=offset-whole;
    const double a0=memory[(cursor+memory.size()-whole)%memory.size()];
    const double a1=memory[(cursor+memory.size()-whole-1)%memory.size()];
    held[t]=a0+(a1-a0)*frac;}
   if(!hold)memory[cursor]=std::clamp(lowpass+feedback*held[0],-4.0,4.0);
   cursor=(cursor+1)%memory.size(); // playback advances even while memory is frozen
  }
  return (1-mix)*input+mix*(levels[0]*held[0]+levels[1]*held[1]);
 }
 double internalClock()const{return clock;}
 int multiply()const{return factor;}
 const std::vector<double>& debugMemory()const{return memory;}
private:
 void updateSlots(){for(int t=0;t<2;t++)slots[t]=std::clamp((int)std::lround(delays[t]*clock),1,4095);}
 double fs,clock=32000,phase=0,lowpass=0,feedback=0,mix=1;
 double modPhase=0,modRate=0,modDepth=0,smoothingSeconds=0.015;
 int factor=1; bool hold=false;size_t cursor=0;
 std::array<double,2> delays{0.05,0.1},smoothedDelays{0.05,0.1},levels{1,0},held{0,0};
 std::array<int,2> slots{1600,3200};
 std::vector<double> memory;
};
}
