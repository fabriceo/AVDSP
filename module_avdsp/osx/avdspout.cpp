//welcome to cplusplus

#include "avdspinclude.hpp"


// dsp_PARAM section start
const float lp[][6] = {
   { 51, 1000.000000, 1.000000, 1.000000, 0, 0 }, //LPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
void dsp_CORE1() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
   dsp_TPDF_CALC(25);
   dsp_LOAD_STORE(0,0); //TODO missing parameters
   dsp_LOAD_GAIN(16,0.707946);
   dsp_STORE(25);
} //end of core 1
int dspDataSpace1[32];
