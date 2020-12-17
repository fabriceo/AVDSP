#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"
// this file describe a program to be used on the OKTO RESEARCH DAC8PRO

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + (x) )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + (x))          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + (x))      // the samples going to the USB Host are offseted by 24

#if 0
// special crossover for test, substractive and using Notch filters. Not really finished nor working
// inspired from https://www.diyaudio.com/forums/multi-way/6655-active-crossover-3.html#post1088722
void crossoverNTM(int freq, int gd, int dither, int defaultGain, float gaincomp, int microslow, int in, int outlow, int outhigh){
const float J = 1.0;
const float K = 0.6;
const float Q = 2.0;
    dsp_PARAM();
    int HPF1 = dspBiquad_Sections_Flexible();
        dsp_filter(FHP1, 1000/J, 0.5, 1.0);
    int HBPF2 = dspBiquad_Sections_Flexible();
        dsp_filter(FBP0DB, 1000/J*K, Q, 1.0);
    int LPF1 = dspBiquad_Sections_Flexible();
        dsp_filter(FLP1, 1000*J, 0.5, 1.0);
    int LBPF2 = dspBiquad_Sections_Flexible();
        dsp_filter(FBP0DB, 1000*J/K, Q, 1.0);

    int memHPF = dspMem_Location();
    int memLPF = dspMem_Location();
    int memHBPF = dspMem_Location();
    int memLBPF = dspMem_Location();

    int compEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FHP2,200,0.7,1.0);   // extra protection to remove lower freq
        dsp_filter(FPEAK,1700,3,dB2gain(-3.0)); // 2,dB2gain(-4.8));
        dsp_filter(FHS2,9000,0.6,dB2gain(6.0)); // 11500, 0.7,dB2gain(7.0)

    dsp_LOAD_MEM(in);
    dsp_BIQUADS(HPF1);
    dsp_COPYXY();
    dsp_BIQUADS(HBPF2);
    dsp_STORE_MEM(memHBPF);
    dsp_NEGX();
    dsp_ADDXY();
    dsp_STORE_MEM(memHPF);

    dsp_LOAD_MEM(in);
    dsp_BIQUADS(LPF1);
    dsp_COPYXY();
    dsp_BIQUADS(LBPF2);
    dsp_STORE_MEM(memLBPF);
    dsp_NEGX();
    dsp_ADDXY();
    dsp_LOAD_MEM(memHBPF);
    dsp_ADDXY();
    dsp_STORE_MEM(memLPF);
    // low is ready
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( defaultGain);
    else dsp_SAT0DB_GAIN_Fixed( defaultGain);
    dsp_STORE( USBIN(outlow) );     // feedback to computer for measurements
    if (microslow>0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer (ahead of compression) will be delayed by %d us\n",microslow); }
    dsp_STORE( DACOUT(outlow) );


    dsp_LOAD_MEM(memHPF);
    dsp_LOAD_MEM(memLBPF);
    dsp_ADDXY();
    // high ready

    dsp_BIQUADS(compEQ);
    //dsp_NEGX(); // invert phase due to cable mismatch ?
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( gaincomp * defaultGain );
    else dsp_SAT0DB_GAIN_Fixed( gaincomp * defaultGain );
    dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements
    if (microslow<0) {
        dsp_DELAY_FixedMicroSec(-microslow);
        printf("compression (behind woofer) will be additionally delayed by %d us\n",-microslow); }
    dsp_STORE( DACOUT(outhigh) );
}

#endif

// lipsitch vanderkoy crossover, using delay and substraction
void crossoverLV(int freq, int gd, int dither, int defaultGain, float gaincomp, int microslow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_BES6(freq);

    if (gd == 0) gd = 752000/freq;  // group delay of the bessel6
    //if (gd == 0) gd = 986000/freq;  // group delay of the bessel8

    // equalization for 18s 2" comp 2080 on Horn XR2064
    int compEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FHP2,200,0.7,1.0);   // extra protection to remove lower freq            dsp_filter(FPEAK,1000,5,dB2gain(+4.0));
        dsp_filter(FPEAK,1700,3,dB2gain(-3.0));
        dsp_filter(FHS2, 9000,0.6,dB2gain(5.0));


    dsp_LOAD_MEM(in);
    dsp_COPYXY();
    dsp_DELAY_DP_FixedMicroSec(gd);
    dsp_SWAPXY();
    dsp_BIQUADS(lowpass);       //compute lowpass filter in X
    dsp_SUBYX();                // compute high pass in Y

    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( defaultGain);
    else dsp_SAT0DB_GAIN_Fixed( defaultGain);
    dsp_STORE( USBIN(outlow) );     // feedback to computer for measurements
    if (microslow>0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer (ahead of compression) will be delayed by %d us\n",microslow); }
    dsp_STORE( DACOUT(outlow) );

    dsp_SWAPXY();               // get highpass
    dsp_SHIFT_FixedInt(-100);   // by default -100 means DSP_MANT
    dsp_GAIN_Fixed(gaincomp);
    dsp_BIQUADS(compEQ);
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( defaultGain );
    else dsp_SAT0DB_GAIN_Fixed( defaultGain );
    dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements

    //dsp_NEGX(); // invert phase during tests to test allignement
    dsp_STORE( DACOUT(outhigh) );
}


const int leftin  = USBOUT(0);  // get the left input sample from the USB out channel 0
const int rightin = USBOUT(1);


// main program for the crossover of a 2 way stystem based on 12LW1400 and 2" comp
int dspProgDACFABRICEO(int fx, int gd, int dither, float gaincomp, int microslow, int mono){
    dspprintf("program for the dac belonging to the author\n");

    dsp_PARAM();

    int leftmem  = dspMem_Location();
    int rightmem = dspMem_Location();
    int avgLRmem = dspMem_Location();

    // woofer equalization
    int rightEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FPEAK, 150, 1.0, dB2gain(-3.0));
    dsp_filter(FPEAK, 580, 2.0, dB2gain(-4.0));
    dsp_filter(FHP2,   10, 0.7, 1.0);               // high pass to protect xmax
    dsp_filter(FPEAK,  73, 0.9, dB2gain(+2.0));     // to compensate box tuning and over sized

    int leftEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FPEAK, 150, 1.0, dB2gain(-3.0));
    dsp_filter(FPEAK, 580, 2.0, dB2gain(-4.0));
    dsp_filter(FHP2,   10, 0.7, 1.0);
    dsp_filter(FPEAK,  73, 0.9, dB2gain(+1.0)); // 1db less than Right due to loudspeaker in a corder


    const float attRight = dB2gain(-3.0); // to compensate above potential gains and avoid any saturation in first biquads.
    const float attLeft  = dB2gain(-4.0); // to compensate above potential gains and avoid any saturation in first biquads.

    int avgLR = dspLoadMux_Inputs(0);
        dspLoadMux_Data(leftin, 0.5 * attLeft);
        dspLoadMux_Data(rightin,0.5 * attRight);


dsp_CORE();  // first core

    if (dither>=0) {
        printf("ditehring enabled for %d usefull bits\n",dither);
        dsp_TPDF_CALC(dither); }

    dsp_LOAD_STORE();
        dspLoadStore_Data( leftin,    DACOUT(0) );      // headphones
        dspLoadStore_Data( rightin,   DACOUT(1) );
        dspLoadStore_Data( rightin,   USBIN(7) );       // loopback for REW
        dspLoadStore_Data( ADCIN(0),  USBIN(0) );       // spdif in passtrough
        dspLoadStore_Data( ADCIN(1),  USBIN(1) );

    if (mono) {
        dsp_LOAD_MUX(avgLR);        // load and mix left+right
        dsp_STORE_MEM(avgLRmem);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(rightEQ);
        dsp_STORE_MEM(leftmem);
        dsp_STORE_MEM(rightmem);

        dsp_LOAD_MEM(avgLRmem);
    } else {

        dsp_LOAD_GAIN_Fixed(leftin, attLeft);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(leftEQ);
        dsp_STORE_MEM(leftmem);

        dsp_LOAD_GAIN_Fixed(rightin, attRight);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(rightEQ);
        dsp_STORE_MEM(rightmem);

        dsp_LOAD_MUX(avgLR);
    }
    dsp_SAT0DB();
    dsp_STORE(DACOUT(6));   // center = (left+right )/2
    dsp_STORE(USBIN(6));

    //dsp_STORE(DACOUT(7));   // lfe
    //dsp_STORE(USBIN(7));

dsp_CORE();
    crossoverLV(fx, gd, dither, 1.0 , gaincomp, microslow, leftmem, 4, 5);
    //crossoverLR6acoustic(fx, gd, dither, 1.0 , gaincomp, microslow, leftmem, 2, 3);

dsp_CORE();
    crossoverLV(fx, gd, dither, 1.0 , gaincomp, microslow, rightmem, 2, 3);
    //crossoverLR6acoustic(fx, gd, dither, 1.0 , gaincomp, microslow, rightmem, 4, 5);

    return dsp_END_OF_CODE();
}


// generic entry point for dspcreate utility
int dspProg(int argc,char **argv){

    //setSerialHash(0x9ADD2096);  // serial number 0
    setSerialHash(0xCAC47719);  // serial number 16

   int dither = -1; // no dithering by default
   int fx = 800;
   int gd = 0;
   float gaincomp = 0.35;
   int microslow = 740;  //250mm ahead of compression driver
   int mono = 0;

    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

         if (strcmp(argv[i],"-dither") == 0) {
             dither = 0;
              if (argc>=i) {
                  i++;
                  dither = strtol(argv[i], NULL,10); }
             dspprintf("add dithering %d bits \n",dither);
             continue; }

         if (strcmp(argv[i],"-mono") == 0) {
            dspprintf("mode mono Left+Right averaged\n");
            mono = 1;
            continue; }

        if (strcmp(argv[i],"-fx") == 0) {
             if (argc>=i) {
                 i++;
                 fx = strtol(argv[i], NULL,10); }
            dspprintf("crossover frequency %dhz\n",fx);
            continue; }

        if (strcmp(argv[i],"-gd") == 0) {
             if (argc>=i) {
                 i++;
                 gd = strtol(argv[i], NULL,10); }
            dspprintf("substractive delay forced to %dus\n",gd);
            continue; }

        if (strcmp(argv[i],"-microslow") == 0) {
             if (argc>=i) {
                 i++;
                 microslow = strtol(argv[i], NULL,10); }
            dspprintf("low driver delayed by %duSec\n",microslow);
            continue; }

        if (strcmp(argv[i],"-gcomp") == 0) {
             if (argc>=i) {
                 i++;
                 gaincomp = strtof(argv[i], NULL); }
            dspprintf("gain on compression driver %f\n",gaincomp);
            continue; }
    }

    return dspProgDACFABRICEO(fx, gd, dither, gaincomp, microslow, mono);

}

