#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"


static void prefilterLowpass(int in, int mem, int flow){
    dsp_PARAM_NUM(in*2);
    int defaultGain =           // storing gains in this PARAM area will enable to access
        dspGain_Default(1.0);   // their value dynamically from host application later

    int prefilter = dspBiquad_Sections(6);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
//        dspGain_Default(1.0); test for error detection
        dsp_LP_BUT4(flow);

        dsp_dumpParameterNum(prefilter,4+6*6,"BQ6_PRE_FILTER",in); // generate an entry in the dumpfile

    dsp_LOAD_GAIN(in, defaultGain); // load input and apply a gain on it
    dsp_BIQUADS(prefilter);
    dsp_STORE_MEM(mem);             // result of the biquad calculation is now stored in a memory location
}

static void crossOver2ways(int in,int outlow, int outhigh, int f, int dist, float highgain){

    int paramnumIndex = in*2+1;
    dsp_PARAM_NUM(paramnumIndex);

    int lowpass = dspBiquad_Sections(4);
        dsp_LP_LR4(f);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);

    int highpass  = dspBiquad_Sections(4);
        dsp_HP_LR4(f);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);

    int delayline =
        dspDelay_MilliMeter_Max_Default(500, dist, 340); // max 50cm between drivers

    dsp_dumpParameterNum(lowpass, 4+4*6,"BQ2_LOWPASS",paramnumIndex);
    dsp_dumpParameterNum(highpass,4+4*6,"BQ2_HIGHPASS",paramnumIndex);
    dsp_dumpParameterNum(delayline,1,"DELAY_HIGH_LOW",paramnumIndex);

    dsp_LOAD_GAIN_Fixed(in,1.0);    //load sample in s.31 and simply convert it to dual precision
    dsp_COPYXY();                   // keep a copy for the input of the high pass filter
    dsp_BIQUADS(lowpass);
    dsp_SAT0DB_TPDF();              // convert to s.31 and add  dithering (for the purpose of the demo!)
    if (dist>0) dsp_DELAY(delayline);
    dsp_STORE(outlow);

    dsp_SWAPXY();                   // getback sample in s.31
    dsp_BIQUADS(highpass);
    dsp_GAIN_Fixed(highgain);
    dsp_SAT0DB_TPDF_GAIN_Fixed(highgain );  // convert to 0.31 apply gain and add 24 bits dithering
    if (dist<0) dsp_DELAY(delayline);
    dsp_STORE(outhigh);
}

static void LFEChannel(int mem1,int mem2, int out, int dist){
    dsp_PARAM(10);
    int filterlfe = dspBiquad_Sections(4);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);

    int delayline =
        dspDelay_MilliMeter_Max_Default(1000, dist, 340);

    dsp_dumpParameterNum(filterlfe,4+4*6,"BQ4_EQ_LFE",-1);
    dsp_dumpParameterNum(delayline,1,"DELAY_LFE",-1);

    dsp_LOAD_MEM(mem1);
    dsp_LOAD_MEM(mem2);
    dsp_ADDXY();
    dsp_BIQUADS(filterlfe);
    dsp_SAT0DB();
    dsp_DELAY(delayline);
    dsp_STORE(out);
}

static int StereoCrossOverLFE(int left, int right, int outs, int fx, int dist, int flfe){
    dsp_PARAM();
    int mem1 = dspMem_Location(1);
    int mem2 = dspMem_Location(1);

    dsp_dumpParameterNum(mem1,2,"MEM",1);
    dsp_dumpParameterNum(mem2,2,"MEM",2);

    dsp_CORE();
    dsp_TPDF_CALC(24);
    prefilterLowpass(left,  mem1, flfe);
    prefilterLowpass(right, mem2, flfe);
    crossOver2ways(left, outs+0,outs+1, fx, dist, 0.8);
    crossOver2ways(right,outs+2,outs+3, fx, dist, 0.8);
    LFEChannel(mem1, mem2, outs+4, 0);
    return dsp_END_OF_CODE();
}

/*
 * this is the DSP user program.
 * each call to the below function will generate propers op codes, or list of data
 * the returned value is the total size of the program including header and final code (0)
 * if -1 then an error occured and has probably been printed on the console
 */

int dspProg(int argc, char **argv){
   int fcross = 1000;  // default crossover frequency for the demo
   int distance = 100; // defaut distance between low and high. (positive means high in front)
   int flfe = 80;

   for(int i=0 ; i<argc;i++) {
	// parse USER'S command line parameters 

                 if (strcmp(argv[i],"-fx") == 0) {
                     i++;
                     if (argc>i) {
                         fcross = strtol(argv[i], NULL,10);
                         printf("crossover fx = %d\n",fcross);
                         continue; } }

                 if (strcmp(argv[i],"-dist") == 0) {
                     i++;
                     if (argc>i) {
                         distance = strtol(argv[i], NULL,10);
                         printf("distance = %d\n",distance);
                         continue; } }

                 if (strcmp(argv[i],"-lfe") == 0) {
                     i++;
                     if (argc>i) {
                         flfe = strtol(argv[i], NULL,10);
                         printf("crossover freq = %d\n",flfe);
                         continue; } }
	}

    return StereoCrossOverLFE(0,1,8,fcross,distance,flfe);

}
