//welcome to cplusplus

#include "avdspinclude.hpp"


// dsp_PARAM section start
const float F_FRONT_HP[][6] = {
   { 79, 36.000000, 0.750000, 45.000000, 0.750000, 1.000000 }, //LT
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
const float F_FRONT_EQ[][6] = {
   { 74, 400.000000, 1.000000, 1.000000, 0, 0 }, //PEAK
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
const float F_FRONT_SUB[][6] = {
   { 51, 35.000000, 1.000000, 1.000000, 0, 0 }, //LPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float F_CENTRE_HP[][6] = {
   { 59, 60.000000, 1.000000, 1.000000, 0, 0 }, //HPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float F_CENTRE_EQ[][6] = {
   { 74, 1000.000000, 1.000000, 1.000000, 0, 0 }, //PEAK
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
const float F_CENTRE_SUB[][6] = {
   { 51, 60.000000, 1.000000, 1.000000, 0, 0 }, //LPLR4
   { 0, 0, 0, 0, 0, 0 },
   { 74, 400.000000, 1.000000, 1.000000, 0, 0 }, //PEAK
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //3 biquad cell(s) provided
const float F_SUR_HP[][6] = {
   { 59, 80.000000, 1.000000, 1.000000, 0, 0 }, //HPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float F_SUR_EQ[][6] = {
   { 74, 1000.000000, 1.000000, 1.000000, 0, 0 }, //PEAK
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
const float F_SUR_SUB[][6] = {
   { 51, 80.000000, 1.000000, 1.000000, 0, 0 }, //LPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float F_SURtm_HP[][6] = {
   { 59, 100.000000, 1.000000, 1.000000, 0, 0 }, //HPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float F_SURtm_EQ[][6] = {
   { 74, 1000.000000, 1.000000, 1.000000, 0, 0 }, //PEAK
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
const float F_SURtm_SUB[][6] = {
   { 51, 100.000000, 1.000000, 1.000000, 0, 0 }, //LPLR4
   { 0, 0, 0, 0, 0, 0 },
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //2 biquad cell(s) provided
const float F_SUB[][6] = {
   { 33, 1000.000000, 1.000000, 1.000000, 0, 0 }, //LPBU2
{ 0,0,0,0,0,0}, }; //1 biquad cell(s) provided
void dsp_CORE1() {
   if (0==dsp_CORE(0x3,0x0)) return;
   dsp_LOAD_STORE(0,0); //TODO missing parameters
   dsp_LOAD(22);
   dsp_BIQUADS(&F_SURtm_HP,2,32,12); //TODO
   dsp_DELAY(100,44,19);
   dsp_STORE(7686);
   dsp_LOAD(23);
   dsp_BIQUADS(&F_SURtm_HP,2,64,12); //TODO
   dsp_DELAY(100,76,19);
   dsp_STORE(7);
} //end of core 1

void dsp_CORE2() {
   if (0==dsp_CORE(0x8,0x0)) return;
   dsp_LOAD_STORE(0,0); //TODO missing parameters
} //end of core 2

void dsp_CORE3() {
   if (0==dsp_CORE(0xf,0x0)) return;
   dsp_LOAD(18);
   dsp_BIQUADS(&F_CENTRE_SUB,3,96,18); //TODO
   dsp_STORE_GAIN(26,0.501187);
//dsp_MIXER(); //TODO list of parameters
   dsp_BIQUADS(&F_FRONT_HP,1,114,6); //TODO
   dsp_DELAY(100,120,19);
   dsp_STORE(6400);
//dsp_MIXER(); //TODO list of parameters
   dsp_BIQUADS(&F_FRONT_HP,1,140,6); //TODO
   dsp_DELAY(100,146,19);
   dsp_STORE(1);
} //end of core 3

void dsp_CORE4() {
   if (0==dsp_CORE(0x0,0x0)) return;
   dsp_LOAD(16);
   dsp_BIQUADS(&F_FRONT_HP,1,166,6); //TODO
   dsp_DELAY(100,172,19);
   dsp_STORE(6400);
   dsp_LOAD(17);
   dsp_BIQUADS(&F_FRONT_HP,1,192,6); //TODO
   dsp_DELAY(100,198,19);
   dsp_STORE(1);
} //end of core 4

void dsp_CORE5() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
   dsp_LOAD(20);
   dsp_BIQUADS(&F_SUR_HP,2,218,12); //TODO
   dsp_DELAY(100,230,19);
   dsp_STORE(7428);
   dsp_LOAD(21);
   dsp_BIQUADS(&F_SUR_HP,2,250,12); //TODO
   dsp_DELAY(100,262,19);
   dsp_STORE(5);
} //end of core 5

void dsp_CORE6() {
   if (0==dsp_CORE(0xf,0x0)) return;
   dsp_LOAD(18);
   dsp_BIQUADS(&F_CENTRE_HP,2,282,12); //TODO
   dsp_DELAY(100,294,19);
   dsp_STORE(2);
//dsp_MIXER(); //TODO list of parameters
   dsp_BIQUADS(&F_FRONT_SUB,2,314,12); //TODO
   dsp_DELAY(3000,326,575);
//dsp_STORE_X_MEM(); //TODO
} //end of core 6

void dsp_CORE7() {
   if (0==dsp_CORE(0x0,0x0)) return;
//dsp_MIXER(); //TODO list of parameters
   dsp_BIQUADS(&F_FRONT_SUB,2,902,12); //TODO
   dsp_DELAY(3000,914,575);
   dsp_COPYXY();
   dsp_LOAD(18);
   dsp_BIQUADS(&F_CENTRE_SUB,3,1490,18); //TODO
   dsp_ADDXY();
   dsp_STORE(27);
//dsp_STORE_X_MEM(); //TODO
   dsp_LOAD(18);
   dsp_BIQUADS(&F_CENTRE_HP,2,1508,12); //TODO
   dsp_DELAY(100,1520,19);
   dsp_STORE(2);
} //end of core 7

void dsp_CORE8() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
//dsp_MIXER(); //TODO list of parameters
   dsp_BIQUADS(&F_SUR_SUB,2,1540,12); //TODO
//dsp_LOAD_Y_MEM(); //TODO
   dsp_ADDYX();
//dsp_MIXER(); //TODO list of parameters
   dsp_BIQUADS(&F_SURtm_SUB,2,1552,12); //TODO
   dsp_ADDYX();
//dsp_MIXER(); //TODO list of parameters
   dsp_ADDXY();
   dsp_BIQUADS(&F_SUB,1,1564,6); //TODO
   dsp_DELAY(100,1570,19);
   dsp_STORE(7939);
} //end of core 8
int dspDataSpace1[1590];
