// dsp_PARAM section start
const float [][6] = {
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //4 biquad cell(s) provided
const float [][6] = {
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //4 biquad cell(s) provided
void dsp_CORE1() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
   dsp_LOAD_STORE(0,0); //TODO missing parameters
   dsp_LOAD_GAIN(16,0.630957);
   dsp_BIQUADS(&,4,32,24); //TODO
//dsp_STORE_X_MEM(); //TODO
   dsp_LOAD_GAIN(17,0.707946);
   dsp_BIQUADS(&,4,56,24); //TODO
//dsp_STORE_X_MEM(); //TODO
//dsp_LOAD_MUX(&mux);  //TODO
   dsp_SAT0DB();
   dsp_STORE(6);
   dsp_STORE(30);
} //end of core 1

void dsp_CORE2() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
// dsp_PARAM section start
const float [][6] = {
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //3 biquad cell(s) provided
const float [][6] = {
//dsp_LOAD_X_MEM(); //TODO
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //3 biquad cell(s) provided
   dsp_COPYXY();
   dsp_DELAY_DP(940,83,180);
   dsp_SWAPXY();
   dsp_BIQUADS(&,3,444,18); //TODO
   dsp_SUBYX();
   dsp_SAT0DB_GAIN(1.000000);
   dsp_STORE(28);
   dsp_DELAY(740,462,142);
   dsp_STORE(4);
   dsp_SWAPXY();
   dsp_SHIFT(-100);
   dsp_GAIN(0.350000);
   dsp_BIQUADS(&,3,606,18); //TODO
   dsp_SAT0DB_GAIN(1.000000);
   dsp_STORE(29);
   dsp_STORE(5);
} //end of core 2

void dsp_CORE3() {
   if (0==dsp_CORE(0xffffffff,0x0)) return;
// dsp_PARAM section start
const float [][6] = {
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //3 biquad cell(s) provided
const float [][6] = {
//dsp_LOAD_X_MEM(); //TODO
{ 0,0,0,0,0,0},{ 0,0,0,0,0,0},{ 0,0,0,0,0,0}, }; //3 biquad cell(s) provided
   dsp_COPYXY();
   dsp_DELAY_DP(940,625,180);
   dsp_SWAPXY();
   dsp_BIQUADS(&,3,986,18); //TODO
   dsp_SUBYX();
   dsp_SAT0DB_GAIN(1.000000);
   dsp_STORE(26);
   dsp_DELAY(740,1004,142);
   dsp_STORE(2);
   dsp_SWAPXY();
   dsp_SHIFT(-100);
   dsp_GAIN(0.350000);
   dsp_BIQUADS(&,3,1148,18); //TODO
   dsp_SAT0DB_GAIN(1.000000);
   dsp_STORE(27);
   dsp_STORE(3);
} //end of core 3
int dspDataSpace1[1166];
