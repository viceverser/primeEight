#pragma once
#include "PrimeEightProcessor.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
namespace primeeight {
// Shared contract for CLAP and VST3 adapters. Host values are normalized [0,1].
inline double normalizedToPlain(uint32_t id,double normalized){
 if(id>=parameterInfo.size() || !std::isfinite(normalized))return 0;
 const auto& p=parameterInfo[id];
 normalized=std::clamp(normalized,0.0,1.0);
 return p.minimum+(p.maximum-p.minimum)*normalized;
}
inline double plainToNormalized(uint32_t id,double plain){
 if(id>=parameterInfo.size() || !std::isfinite(plain))return 0;
 const auto& p=parameterInfo[id];
 return std::clamp((plain-p.minimum)/(p.maximum-p.minimum),0.0,1.0);
}
inline constexpr uint32_t stateMagic=0x38455050; // PPE8, little-endian
inline constexpr uint32_t stateVersion=1;
inline constexpr size_t stateBytes=8+parameterInfo.size()*8;
// Explicit little-endian, platform-independent, no raw C++ struct serialization.
inline void put32(uint8_t* b,uint32_t v){for(int i=0;i<4;i++)b[i]=uint8_t(v>>(i*8));}
inline uint32_t get32(const uint8_t* b){uint32_t v=0;for(int i=0;i<4;i++)v|=uint32_t(b[i])<<(i*8);return v;}
inline std::array<uint8_t,stateBytes> saveState(const Processor& p){
 std::array<uint8_t,stateBytes> b{};put32(b.data(),stateMagic);put32(b.data()+4,stateVersion);
 for(size_t i=0;i<parameterInfo.size();i++){
  const double d=p.get(static_cast<Parameter>(i));uint64_t u=0;std::memcpy(&u,&d,8);
  for(int j=0;j<8;j++)b[8+i*8+j]=uint8_t(u>>(j*8));
 }return b;
}
inline bool loadState(Processor& p,const uint8_t* b,size_t n){
 if(!b||n!=stateBytes||get32(b)!=stateMagic||get32(b+4)!=stateVersion)return false;
 std::array<double,parameterInfo.size()> decoded{};
 for(size_t i=0;i<decoded.size();i++){
  uint64_t u=0;for(int j=0;j<8;j++)u|=uint64_t(b[8+i*8+j])<<(j*8);
  std::memcpy(&decoded[i],&u,8);
  if(!std::isfinite(decoded[i])||decoded[i]<parameterInfo[i].minimum||decoded[i]>parameterInfo[i].maximum)return false;
 }
 for(size_t i=0;i<decoded.size();i++)p.set(static_cast<Parameter>(i),decoded[i]);return true;
}
}
