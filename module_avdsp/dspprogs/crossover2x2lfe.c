
#include "dsp_encoder.h"

int fcross = 1000;  // default crossover frequency for the demo
int distance = 100; // defaut distance between low and high. (positive means high in front)
int flfe = 80;
int subdelay = 745;

/*
 * this is the DSP user program.
 * each call to the below function will generate propers op codes, or list of data
 * the returned value is the total size of the program including header and final code (0)
 * if -1 then an error occured and has probably been printed on the console
 */

void prefilterLowpass(int in, int mem, int flow){
    dsp_PARAM_NUM(in*2);
    int defaultGain =
        dspGain_Default(1.0);

    int prefilter = dspBiquad_Sections(6);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dspGain_Default(1.0);
        dsp_LP_BUT4(flow);

        dsp_dumpParameterNum(prefilter,30,"BQ6_PRE_FILTER",in);

    dsp_LOAD(in);
    dsp_GAIN(defaultGain);
    dsp_BIQUADS(prefilter);
    dsp_STORE_MEM(mem);
}

void crossOver2ways(int in,int outlow, int outhigh, int f, int dist, float highgain){

    dsp_PARAM_NUM(in*2+1);

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

    dsp_dumpParameterNum(lowpass, 10,"BQ2_LOWPASS",in);
    dsp_dumpParameterNum(highpass,10,"BQ2_HIGHPASS",in);
    dsp_dumpParameterNum(delayline,1,"DELAY_HIGH_LOW",in);

    dsp_LOAD(in);
    dsp_COPYXY();
    dsp_BIQUADS(lowpass);
    if (dist>0) dsp_DELAY(delayline);
    dsp_STORE(outlow);

    dsp_SWAPXY();
    dsp_BIQUADS(highpass);
    dsp_GAIN0DB_Fixed(highgain);
    if (dist<0) dsp_DELAY(delayline);
    dsp_STORE(outhigh);
}

void LFEChannel(int mem1,int mem2, int out, int dist){
    dsp_PARAM(10);
    int filterlfe = dspBiquad_Sections(4);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);

    int delayline =
        dspDelay_MilliMeter_Max_Default(1000, dist, 340);

    dsp_dumpParameterNum(filterlfe,20,"BQ4_EQ_LFE",-1);
    dsp_dumpParameterNum(delayline,1,"DELAY_LFE",-1);

    dsp_LOAD_MEM(mem1);
    dsp_LOAD_MEM(mem2);
    dsp_ADDXY();
    dsp_BIQUADS(filterlfe);
    dsp_DELAY(delayline);
    dsp_STORE(out);
}

int StereoCrossOverLFE(int left, int right, int outs, int fx, int dist, int flfe){
    dsp_PARAM();
    int mem1 = dspMem_Location(1);
    int mem2 = dspMem_Location(1);

    dsp_dumpParameterNum(mem1,2,"MEM",1);
    dsp_dumpParameterNum(mem2,2,"MEM",2);

    dsp_CORE();
    prefilterLowpass(left,  mem1, flfe);
    prefilterLowpass(right, mem2, flfe);
    crossOver2ways(left, outs+0,outs+1, fx, dist, 0.8);
    crossOver2ways(right,outs+2,outs+3, fx, dist, 0.8);
    LFEChannel(mem1, mem2, outs+4, 0);
    return dsp_END_OF_CODE();
}

