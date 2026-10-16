#pragma once
#include <clap/clap.h>

struct PrimeEightJuceBridge;

PrimeEightJuceBridge* primeeightJuceBridgeCreate();
void primeeightJuceBridgeDelete(PrimeEightJuceBridge*);
bool primeeightJuceGuiCreate(PrimeEightJuceBridge*, const char*, bool);
void primeeightJuceGuiDestroy(PrimeEightJuceBridge*);
bool primeeightJuceGuiParent(PrimeEightJuceBridge*, const clap_window_t*);
bool primeeightJuceGuiScale(PrimeEightJuceBridge*, double);
bool primeeightJuceGuiSize(PrimeEightJuceBridge*, uint32_t*, uint32_t*);
bool primeeightJuceGuiSetSize(PrimeEightJuceBridge*, uint32_t, uint32_t);
bool primeeightJuceGuiShow(PrimeEightJuceBridge*);
bool primeeightJuceGuiHide(PrimeEightJuceBridge*);
