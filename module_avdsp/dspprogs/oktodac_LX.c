
#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + (x) )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + (x))          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + (x))      // the samples going to the USB Host are offseted by 24


const int leftin  = USBOUT(0);  // get the left input sample from the USB out channel 0
const int rightin = USBOUT(1);

const int rightlow  = 2;
const int rightmid  = 3;
const int leftlow   = 4;
const int leftmid   = 5;

const int rightsub  = 6;
const int leftsub   = 7;

int dither = 0;

// these gains are applied in the very last stage of the dsp, before storing result to dac
int gainsubleft  = 1.0;
int gainsubright = 1.0;
int gainlow = 1.0;
int gainmid = 1.0;

int fx = 700;
int delaymid = 60;  // midrange delyed by 60us by default
int sub = 0;
int delaysubleft  = 0;
int delaysubright = 0;

int dspProg_LXmini(){

        setSerialHash(0x9ADD2096);  // serial number 0
        //setSerialHash(0xCAC47719);  // serial number 16

    const float attn  = dB2gain(-8.0); // to avoid any saturation in biquads (due to 50hz bass boost in fact).

    dsp_PARAM();

    // upfront equalization on the stereo channels.
    int frontEQ = dspBiquad_Sections_Flexible();
        //dsp_filter(FHP2,     10, 0.7, 1.0);    // optional high pass to protect xmax
        dsp_filter(FPEAK,  1800,  7.0, dB2gain(+3.5));
        dsp_filter(FPEAK,  2500,  2.0, dB2gain( -5.0));
        dsp_filter(FPEAK, 10000,  4.0, dB2gain( +2.0));
        dsp_filter(FPEAK, 16200,  5.0, dB2gain( +0.5));

    int lowEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLP2,     fx, 0.5,  1.0 ); //dB2gain(-11.2) ); // lowpass LR2
        dsp_filter(FPEAK,    50, 0.7,  dB2gain( sub ? 0.0 : +7.0));    // bass boost only if no sub
        dsp_filter(FPEAK,  5100, 6.0,  dB2gain(-14.0));   // breakup and raisonance ?

    int rightmidEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FHP2,   fx,    0.5, -dB2gain(-9.8) ); // highpass LR2 inverted and reduced by 10db
        dsp_filter(FLS2,   1000,  0.5, dB2gain(+16.0));
        dsp_filter(FHS2,   8000,  0.7, dB2gain( +8.0));

    int leftmidEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FHP2,   fx,    0.5, -dB2gain(-9.8) ); // highpass LR2 inverted and reduced by 10db
        dsp_filter(FLS2,   1000,  0.5, dB2gain(+16.0));
        dsp_filter(FHS2,   8000,  0.7, dB2gain( +8.0));
        dsp_filter(FPEAK,  6000,  0.3, dB2gain(1.8));

    // subwoofer for left or mono
    int leftsubEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLP2,   60, 0.5, -1.0);    // LR2 @ 60hz, inverted
        dsp_filter(FPEAK,  50, 1.0, dB2gain(0.0));

    // subwoofer for right only
    int rightsubEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLP2,   60, 0.5, -1.0);    // LR2 @ 60hz, inverted
        dsp_filter(FPEAK,  50, 2.0, dB2gain(0.0));


    int leftmem  = dspMem_Location();
    int rightmem = dspMem_Location();

    // used to average Left and Right in case of 1 subwoofer.
    int avgLR = dspLoadMux_Inputs(0);
        dspLoadMux_Data(leftin,  0.5); // gain can be adjusted lower
        dspLoadMux_Data(rightin, 0.5);

dsp_CORE();  // first core, stereo conditioning

    // basic transfers
    dsp_LOAD_STORE();
        dspLoadStore_Data( leftin,    DACOUT(0) );      // headphones
        dspLoadStore_Data( rightin,   DACOUT(1) );
        dspLoadStore_Data( ADCIN(0),  USBIN(0) );       // spdif in passtrough
        dspLoadStore_Data( ADCIN(1),  USBIN(1) );
        dspLoadStore_Data( rightin,   USBIN(1) );       // loopback for REW during tests only

    if (dither>=0) dsp_TPDF_CALC(dither);        // this calculate a tpdf white noise value at each cycle/sample

    dsp_LOAD_GAIN_Fixed(leftin, attn);           // load input and apply a gain on the left channel
    dsp_BIQUADS(frontEQ);                        // compute EQs
    dsp_STORE_MEM(leftmem);                      // store in temporary location for second core

    dsp_LOAD_GAIN_Fixed(rightin, attn);
    dsp_BIQUADS(frontEQ);
    dsp_STORE_MEM(rightmem);                        // store in temporary location for third core

dsp_CORE();  // second core crossover low channel
    
    //low channel
    dsp_LOAD_MEM(leftmem);
    dsp_BIQUADS(lowEQ);   //compute lowpass filter
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainlow);
        else dsp_SAT0DB_GAIN_Fixed( gainlow);
    dsp_STORE( USBIN(leftlow) );
    dsp_STORE( DACOUT(leftlow));

    dsp_LOAD_MEM(rightmem);
    dsp_BIQUADS(lowEQ);   //compute lowpass filter
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainlow);
        else dsp_SAT0DB_GAIN_Fixed( gainlow);
    dsp_STORE( USBIN(rightlow) );
    dsp_STORE( DACOUT(rightlow));

dsp_CORE();  // third core midrange channel

    // mid channel
    dsp_LOAD_MEM(leftmem);
    dsp_BIQUADS(leftmidEQ);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainmid);
        else dsp_SAT0DB_GAIN_Fixed( gainmid);
    dsp_DELAY_FixedMicroSec(delaymid);
    //dsp_STORE( USBIN(leftmid) );
    dsp_STORE( DACOUT(leftmid));

    dsp_LOAD_MEM(rightmem);
    dsp_BIQUADS(rightmidEQ);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainmid);
        else dsp_SAT0DB_GAIN_Fixed( gainmid);
    dsp_DELAY_FixedMicroSec(delaymid);
    //dsp_STORE( USBIN(rightmid) );
    dsp_STORE( DACOUT(rightmid));

if ( sub ) {

    dsp_CORE();  // 4th core for subwoofers

    if (sub == 2)
         dsp_LOAD_MEM(rightmem);
    else dsp_LOAD_MUX(avgLR);        // load and mix left+right
    dsp_BIQUADS(rightsubEQ);
    if (sub == 1) dsp_COPYXY();     // temporary store EQed
    if (delaysubright) dsp_DELAY_FixedMicroSec(delaysubright);
    dsp_SAT0DB_GAIN_Fixed( gainsubright );
    dsp_STORE( USBIN(rightsub) );
    dsp_STORE( DACOUT(rightsub));

    if (sub == 2) {
        dsp_LOAD_MEM(leftmem);
        dsp_BIQUADS(leftsubEQ);
    } else dsp_COPYYX();            // retreive stored value
    if (delaysubleft) dsp_DELAY_FixedMicroSec(delaysubleft);
    dsp_SAT0DB_GAIN_Fixed( gainsubleft );
    dsp_STORE( USBIN( leftsub ));
    dsp_STORE( DACOUT( leftsub ));
}

    return dsp_END_OF_CODE();
}

int dspProg(int argc,char **argv){
   int prog = 0;
    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

         if (strcmp(argv[i],"-lxmini") == 0) {
            dspprintf("generating dsp code for LXmini with 50hz bass boost, no subwoofer\n");
            prog = 1; sub = 0;
            continue; }

         if (strcmp(argv[i],"-lxminisub") == 0) {
            dspprintf("generating dsp code for LXmini for 2 subwoofers LR2@60hz\n");
            prog = 1; sub = 1;
            continue; }

         if (strcmp(argv[i],"-lxminisubmono") == 0) {
            dspprintf("generating dsp code for LXmini with only 1 subwoofer LR2@60hz\n");
            prog = 1; sub = 2;
            continue; }

         if (strcmp(argv[i],"-dither") == 0) {
             dither = 0;
              if (argc>=i) {
                  i++;
                  dither = strtol(argv[i], NULL,10); }
             dspprintf("add dithering %d bits \n",dither);
             continue; }

         if (strcmp(argv[i],"-fx") == 0) {
              if (argc>=i) {
                  i++;
                  fx = strtol(argv[i], NULL,10); }
             dspprintf("crossover frequency %dhz\n",fx);
             continue; }

         if (strcmp(argv[i],"-delaymid") == 0) {
              if (argc>=i) {
                  i++;
                  delaymid = strtol(argv[i], NULL,10); }
             dspprintf("delayed midrange by %dus\n",delaymid);
             continue; }


        // .. put the other line options here.
    }

    switch (prog) {
    case 1:  return dspProg_LXmini();
    case 2:
    case 3:
    case 4:
    case 5:
    default:  return dsp_END_OF_CODE();
    }
}

