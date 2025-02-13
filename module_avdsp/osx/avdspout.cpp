//welcome to cplusplus

#include "avdspinclude.hpp"


void dsp_CORE1() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
   dsp_LOAD_STORE(0,0); //TODO missing parameters
   dsp_LOAD(16);
   dsp_VALUEY(2.000000);
   dsp_SWAPXY();
   dsp_DIVYX();
   dsp_SWAPXY();
   dsp_STORE(25);
} //end of core 1
int dspDataSpace1[32];
