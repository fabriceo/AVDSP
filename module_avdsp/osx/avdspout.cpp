//welcome to cplusplus

#include "avdspinclude.hpp"


// dsp_PARAM section start
const float lp[][6] = {
   { 51, 400.000000, 1.000000, 1.000000, 0, 0 }, //LPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float hp[][6] = {
   { 59, 400.000000, 1.000000, 1.000000, 0, 0 }, //HPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float lpbe8[][6] = {
   { 7, 400.000000, 1.000000, 1.000000, 0, 0 }, //LPBE8
   { 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //4 biquad cell(s) provided
const float hp2[][6] = {
   { 67, 400.000000, 0.770000, 1.000000, 0, 0 }, //HP2
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
void dsp_CORE1() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
   dsp_TPDF_CALC(26);
   dsp_LOAD_STORE(0,0); //TODO missing parameters
   dsp_SINE(43000,0.891251);
   dsp_STORE_TPDF(31);
} //end of core 1
int dspDataSpace1[36];
