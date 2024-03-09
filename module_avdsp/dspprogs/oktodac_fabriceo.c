#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"
// this file describe a program to be used on the OKTO RESEARCH DAC8PRO

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + (x) )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + (x))          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + (x))      // the samples going to the USB Host are offseted by 24

int modeoppo = 0;
int centerhilbert=0;

const int leftin   = USBOUT(0);     // get the left input sample from the USB out channel 0
const int rightin  = USBOUT(1);
const int centerin   = ADCIN(2);    // spdif center+lfe channel connected to "coax1" on my dac8stereo
const int lfein      = ADCIN(3);
const int suroundLeftin  = ADCIN(4);  // spdif surround   channel connected to "aes" on my dac8stereo
const int suroundRightin = ADCIN(5);
const float zerodB = 1.0;


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

    dsp_LOAD_X_MEM(in);
    dsp_BIQUADS(HPF1);
    dsp_COPYXY();
    dsp_BIQUADS(HBPF2);
    dsp_STORE_X_MEM(memHBPF);
    dsp_NEGX();
    dsp_ADDXY();
    dsp_STORE_X_MEM(memHPF);

    dsp_LOAD_X_MEM(in);
    dsp_BIQUADS(LPF1);
    dsp_COPYXY();
    dsp_BIQUADS(LBPF2);
    dsp_STORE_X_MEM(memLBPF);
    dsp_NEGX();
    dsp_ADDXY();
    dsp_LOAD_X_MEM(memHBPF);
    dsp_ADDXY();
    dsp_STORE_X_MEM(memLPF);
    // low is ready
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( defaultGain);
    else dsp_SAT0DB_GAIN_Fixed( defaultGain);
    dsp_STORE( USBIN(outlow) );     // feedback to computer for measurements
    if (microslow>0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer (ahead of compression) will be delayed by %d us\n",microslow); }
    dsp_STORE( DACOUT(outlow) );


    dsp_LOAD_X_MEM(memHPF);
    dsp_COPYXY();
    dsp_LOAD_X_MEM(memLBPF);
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
void crossoverLV(int freq, int gd, int dither, int gain, float gaincomp, int microslow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_BES6(freq,1.0);

    if (gd == 0) gd = 752000/freq;  // group delay of the bessel6
    //if (gd == 0) gd = 986000/freq;  // group delay of the bessel8

    // equalization for 18s 2" comp 2080 on Horn XR2064
    int compEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FHP2,200,0.7,zerodB);   // extra protection to remove lower freq
        //dsp_filter(FPEAK,1000,5,dB2gain(+4.0));
        dsp_filter(FPEAK,1700,3,dB2gain(-2.0));
        dsp_filter(FPEAK,7400,3,dB2gain(+3.0));


    dsp_LOAD_X_MEM(in);
    dsp_COPYXY();
    dsp_DELAY_DP_FixedMicroSec(gd);
    dsp_SWAPXY();
    dsp_BIQUADS(lowpass);       //compute lowpass filter in X
    dsp_SUBYX();                // compute high pass in Y

    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( gain);
    else dsp_SAT0DB_GAIN_Fixed( gain);
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
         dsp_SAT0DB_TPDF_GAIN_Fixed( gain );
    else dsp_SAT0DB_GAIN_Fixed( gain );
    dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements

    //dsp_NEGX(); // invert phase during tests to test allignement
    dsp_STORE( DACOUT(outhigh) );
}


void dspSuroundEQ(int source, int dest, int gain, int dither) {

    dsp_PARAM();
    // surround channel equalization (JBL LS 40)
    int suroundEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FPEAK, 100, 1.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 200, 2.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 400, 2.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 800, 2.0, dB2gain(-0.01));

    const float attSuround = dB2gain(-1.0);   // to compensate above potential gains and avoid any saturation in first biquads.

    dsp_LOAD_GAIN_Fixed( source, attSuround );
    dsp_BIQUADS( suroundEQ );
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( gain );
    else dsp_SAT0DB_GAIN_Fixed( gain );
    dsp_STORE( dest );
}

void dspHeadphoneEQ(int source, int dest, int gain, int dither) {

    dsp_PARAM();
    int headphoneEQ = dspBiquad_Sections_Flexible();    //3 filters compatible with 96k on XU216
    dsp_filter(FPEAK, 100, 1.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 200, 2.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 400, 2.0, dB2gain(-0.01));

    const float attHeadphone = dB2gain(-1.0);   // to compensate above potential gains and avoid any saturation in first biquads.

    dsp_LOAD_GAIN_Fixed( source, attHeadphone );
    dsp_BIQUADS( headphoneEQ );
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( gain );
    else dsp_SAT0DB_GAIN_Fixed( gain );
    dsp_STORE( dest );
}

void dspCenterEQ(int source, int dest, int gain, int dither){
    dsp_PARAM();
    // center channel equalization (JBL LS Center)
    int centerEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FPEAK, 100, 2.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 200, 2.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 400, 2.0, dB2gain(-0.01));
    dsp_filter(FPEAK, 800, 2.0, dB2gain(-0.01));

    const float attCenter  = dB2gain(-3.0);   // to compensate above potential gains and avoid any saturation in first biquads.

    int hilbertEQ = dspBiquad_Sections_Flexible();
    dsp_Hilbert( 4, 160.0, 0 );

    int hilbertEQ90 = dspBiquad_Sections_Flexible();
    dsp_Hilbert( 4, 160.0, 90 );            // (3, 120) is also acceptable

    // treat center channel
    if (modeoppo == 0) {
        // create a center channel from left & right with 90° phase shift (studder method)
        dsp_LOAD_GAIN_Fixed(leftin, attCenter);
        dsp_DELAY_1();  // delay by 1 sample.
        dsp_BIQUADS(hilbertEQ);
        dsp_LOAD_GAIN_Fixed(rightin, attCenter); // previous result in ALU X is automatically transfered in Y
        dsp_BIQUADS(hilbertEQ90);
        dsp_SWAPXY();
        dsp_SUBXY();
    } else {
        dsp_LOAD_GAIN_Fixed( source, attCenter ); // load center channel and apply gain to avoid saturation in biquad
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(centerEQ);
    }
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( gain );
    else dsp_SAT0DB_GAIN_Fixed( gain );
    dsp_STORE( dest );

}

// main program for the crossover of a 2 way stystem based on 12LW1400 and 2" comp
int dspProgDACFABRICEO(int fx, int gd, int dither, float gaincomp, int microslow, int mono){
    dspprintf("program for the dac belonging to the author\n");

    dsp_PARAM();

    int leftmem  = dspMem_Location();   //storage location for one sample to be passed by first core to crossover core
    int rightmem = dspMem_Location();

    // woofer equalization
    int rightEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FPEAK, 230, 0.3, dB2gain(-3.0));     //according to room measurement with both speakers
    dsp_filter(FPEAK,  40, 2.0, dB2gain(-3.0));
    dsp_filter(FHP2,   10, 0.7, zerodB);            // high pass to protect xmax
    dsp_filter(FPEAK, 120, 1.5, dB2gain(+2.0));
    dsp_filter(FHS2, 9000,0.6,dB2gain(+5.0));


    int leftEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FPEAK, 230, 0.3, dB2gain(-3.0));
    dsp_filter(FPEAK,  40, 2.0, dB2gain(-3.0));
    dsp_filter(FHP2,   10, 0.7, zerodB);
    dsp_filter(FPEAK, 110, 2.0, dB2gain(+3.0));
    dsp_filter(FHS2, 9000,0.6,dB2gain(+5.0));       //comp...


    const float attRight   = dB2gain(-3.0);   // to compensate above potential gains and avoid any saturation in first biquads.
    const float attLeft    = dB2gain(-3.0);

    int avgLR = dspLoadMux_Inputs(0);
        dspLoadMux_Data(leftin, 0.5 * attLeft);
        dspLoadMux_Data(rightin,0.5 * attRight);

dsp_CORE();  // first core

    if (dither>=0) {
        printf("dithring enabled for %d usefull bits\n",dither);
        dsp_TPDF_CALC(dither); }

    if (modeoppo) {
        dsp_LOAD_STORE();
        dspLoadStore_Data( ADCIN(0),  USBIN(0) );       // coax 2 sent to host for displaying sound presence
        dspLoadStore_Data( ADCIN(1),  USBIN(1) );       // and to monitor spdif (in AES mode on dac8pro)
    } else {
        dsp_LOAD_STORE();
        dspLoadStore_Data( rightin,    USBIN(1) );       //loopback for monitoring in REW
    }


    // prepare left and right channels with upfront EQ,
    if (mono) {
        dsp_LOAD_MUX(avgLR);        // load and mix left+right
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(rightEQ);
        dsp_STORE_X_MEM(leftmem);
        dsp_STORE_X_MEM(rightmem);

    } else {

        dsp_LOAD_GAIN_Fixed(leftin, attLeft);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(leftEQ);
        dsp_STORE_X_MEM(leftmem);     // store EQ result in memory, for treatment by the crossover pogram in other dspcore

        dsp_LOAD_GAIN_Fixed(rightin, attRight);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(rightEQ);
        dsp_STORE_X_MEM(rightmem);
    }

    if (centerhilbert) {
        dspCenterEQ( centerin, USBOUT(6), zerodB, dither );
        dsp_DELAY_1();  // to compensate delay inherent to crossover being in other cores
        //possibility to monitor EQ on USB6
        dsp_STORE( DACOUT(6) );
    }


dsp_CORE();
    crossoverLV(fx, gd, dither, zerodB , gaincomp, microslow, leftmem, 4, 5);
    //crossoverLR6acoustic(fx, gd, dither, 1.0 , gaincomp, microslow, leftmem, 2, 3);

    if (modeoppo)
        //consider spdif 3 as suround channel
        dspSuroundEQ( suroundLeftin, DACOUT(0), zerodB, dither );
    else
        dspHeadphoneEQ( leftin, DACOUT(0), zerodB, dither );




dsp_CORE();
    crossoverLV(fx, gd, dither, zerodB , gaincomp, microslow, rightmem, 2, 3);
    //crossoverLR6acoustic(fx, gd, dither, 1.0 , gaincomp, microslow, rightmem, 4, 5);

    //consider spdif 3 as suround channel
    if (modeoppo)
        dspSuroundEQ(  suroundRightin,  DACOUT(1), zerodB, dither );
    else
        dspHeadphoneEQ( rightin,  DACOUT(1), zerodB, dither );

    return dsp_END_OF_CODE();
}


// generic entry point for dspcreate utility
int dspProg(int argc,char **argv){

    setSerialHash(0x9ADD2096);  // serial number 0
    //setSerialHash(0xCAC47719);  // serial number 16

   int dither = -1; // no dithering by default
   int fx = 800;
   int gd = 0;
   float gaincomp = 0.35;
   int microslow = 740;  //250mm ahead of compression driver
   int mono = 0;

    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

         if (strcmp(argv[i],"-dither") == 0) {
             dither = 24;
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

        if (strcmp(argv[i],"-oppo") == 0) {
             if (argc>=i) {
                 i++;
                 modeoppo = 1; }
            dspprintf("mode Oppo (output surround/coax3 on dac 0 and 1 and center/coax1 on dac 6 and 7)\n");
            continue; }

        if (strcmp(argv[i],"-centerhilbert") == 0) {
             if (argc>=i) {
                 i++;
                 centerhilbert = 1; }
            dspprintf("center channel created with hilbert transform\n");
            continue; }
    }

    return dspProgDACFABRICEO(fx, gd, dither, gaincomp, microslow, mono);

}

