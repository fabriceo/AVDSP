
#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + (x) )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + (x))          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + (x))      // the samples going to the USB Host are offseted by 24


const int leftin  = USBOUT(0);  // get the left input sample from the USB out channel 0
const int rightin = USBOUT(1);

const int leftlow   = 2;
const int leftmid   = 4;
const int lefthigh  = 6;
const int rightlow  = 3;
const int rightmid  = 5;
const int righthigh = 7;

#if 0
// LR type of crossover for the 2 way system of the Author
void crossoverLR6acoustic(int freq, int gd, int dither, int defaultGain, float gaincomp, int microslow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_BUT4(532);
        dsp_filter(FHS1,  500,  0.5, dB2gain(-5.0));
        dsp_filter(FPEAK, 180,  2.5, dB2gain(-1.9));
        dsp_filter(FPEAK, 544,  4.0, dB2gain(-2.0));

    int highpass = dspBiquad_Sections_Flexible();
        dsp_HP_BUT3(822);
        dsp_filter(FPEAK, 1710, 2.0, dB2gain(-4.8));
        dsp_filter(FPEAK,  917, 5.0, dB2gain(+3.0));
        dsp_filter(FHS2, 11500, 0.7, dB2gain(+6.0));


    dsp_LOAD_MEM(in);
    dsp_BIQUADS(lowpass);   //compute lowpass filter

    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( defaultGain);
    else dsp_SAT0DB_GAIN_Fixed( defaultGain);
    dsp_STORE( USBIN(outlow) );
    if (microslow > 0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer delayed by %d us\n",microslow);
    }
    dsp_STORE( DACOUT(outlow) ); // low driver

    dsp_LOAD_MEM(in);
    dsp_BIQUADS(highpass);   //compute lowpass filter
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( gaincomp * defaultGain );
    else dsp_SAT0DB_GAIN_Fixed( gaincomp * defaultGain );
    dsp_STORE( USBIN(outhigh) );
    if (microslow < 0) {
        dsp_DELAY_FixedMicroSec(-microslow);
        printf("compression driver delayed by %d us\n",-microslow); // not relevant !-)
    }
    //dsp_NEGX();   // change polarity to check driver allignements
    dsp_STORE( DACOUT(outhigh) );
}
#endif

int dither = 0;
int hpdc   = 10;
int lpsub  = 0;
int hplow  = 10;
int lplow  = 400;
int hpmid  = 400;
int lpmid  = 2000;
int hphigh = 2000;

int delsub = 0;
int dellow = 40;
int delmid = 75;
int delhigh = 150;
int delveryhigh;
int gainsub = 1.0;
int gainlow = 1.0;
int gainmid = 1.0;
int gainhigh = 1.0;


int dspProg_3ways_LR4(){

    const float attRight = dB2gain(-3.0); // to avoid any saturation in first biquads.
    const float attLeft  = dB2gain(-3.0);

    dsp_PARAM();

    // upfront equalization of the stereo channels.
    int rightEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FHP2,   10, 0.7, 1.0);               // high pass to protect xmax
    dsp_filter(FPEAK, 100, 1.0, dB2gain(0.0));
    dsp_filter(FPEAK, 200, 1.0, dB2gain(0.0));
    dsp_filter(FPEAK, 400, 1.0, dB2gain(0.0));

    // suggest to have the exact same value than above to keep pahse accurate between left/right
    int leftEQ = dspBiquad_Sections_Flexible();
    dsp_filter(FHP2,   10, 0.7, 1.0);
    dsp_filter(FPEAK, 100, 1.0, dB2gain(0.0));
    dsp_filter(FPEAK, 200, 1.0, dB2gain(0.0));
    dsp_filter(FPEAK, 400, 1.0, dB2gain(0.0));

    int lplowbq = dspBiquad_Sections_Flexible();
        dsp_HP_BUT2(hpdc);	// to remove DC, could be replaced by lpsub to complement subwoofer
        dsp_LP_LR4(lplow);

    int midbq = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(hpmid);
        dsp_LP_LR4(lpmid);

    int hphighbq = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(hphigh);

    int leftmem  = dspMem_Location();
    int rightmem = dspMem_Location();

dsp_CORE();  // first core, stereo conditioning

    // basic transfers
    dsp_LOAD_STORE();
        dspLoadStore_Data( leftin,    DACOUT(0) );      // headphones
        dspLoadStore_Data( rightin,   DACOUT(1) );
        dspLoadStore_Data( ADCIN(0),  USBIN(0) );       // spdif in passtrough
        dspLoadStore_Data( ADCIN(1),  USBIN(1) );
        dspLoadStore_Data( rightin,   USBIN(1) );       // loopback for REW during tests only

    if (dither>=0) dsp_TPDF_CALC(dither);           // this calculate a tpdf white noise value at each cycle/sample

    dsp_LOAD_GAIN_Fixed(leftin, attLeft);           // load input and apply a gain on the left channel
    dsp_BIQUADS(leftEQ);                            // compute EQs
    dsp_STORE_MEM(leftmem);                         // store in temporary location for second core

    dsp_LOAD_GAIN_Fixed(rightin, attRight);
    dsp_BIQUADS(rightEQ);
    dsp_STORE_MEM(rightmem);                        // store in temporary location for third core

dsp_CORE();  // second core crossover low channel
    
    //low channel
    dsp_LOAD_MEM(leftmem);
    dsp_BIQUADS(lplowbq);   //compute lowpass filter
    if (dellow) dsp_DELAY_FixedMicroSec(dellow);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainlow);
        else dsp_SAT0DB_GAIN_Fixed( gainlow);
    dsp_STORE( USBIN(leftlow) );
    dsp_STORE( DACOUT(leftlow));

    dsp_LOAD_MEM(rightmem);
    dsp_BIQUADS(lplowbq);   //compute lowpass filter
    if (dellow) dsp_DELAY_FixedMicroSec(dellow);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainlow);
        else dsp_SAT0DB_GAIN_Fixed( gainlow);
    dsp_STORE( USBIN(rightlow) );
    dsp_STORE( DACOUT(rightlow));

dsp_CORE();  // third core crossover mid channel

    // mid channel
    dsp_LOAD_MEM(leftmem);
    dsp_BIQUADS(midbq);
    if (delmid) dsp_DELAY_FixedMicroSec(delmid);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainmid);
        else dsp_SAT0DB_GAIN_Fixed( gainmid);
    //dsp_STORE( USBIN(leftmid) );
    dsp_STORE( DACOUT(leftmid));

    dsp_LOAD_MEM(rightmem);
    dsp_BIQUADS(midbq);
    if (delmid) dsp_DELAY_FixedMicroSec(delmid);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainmid);
        else dsp_SAT0DB_GAIN_Fixed( gainmid);
    //dsp_STORE( USBIN(rightmid) );
    dsp_STORE( DACOUT(rightmid));

dsp_CORE();  // 4th core crossover high channel

    //high channel
    dsp_LOAD_MEM(leftmem);
    dsp_BIQUADS(hphighbq);
    if (delhigh) dsp_DELAY_FixedMicroSec(delhigh);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainhigh);
        else dsp_SAT0DB_GAIN_Fixed( gainhigh);
    dsp_STORE( USBIN(lefthigh) );
    dsp_STORE( DACOUT(lefthigh));

    dsp_LOAD_MEM(rightmem);
    dsp_BIQUADS(hphighbq);
    if (delhigh) dsp_DELAY_FixedMicroSec(delhigh);
    if (dither>=0)
        dsp_SAT0DB_TPDF_GAIN_Fixed( gainhigh);
        else dsp_SAT0DB_GAIN_Fixed( gainhigh);
    dsp_STORE( USBIN(lefthigh) );
    dsp_STORE( DACOUT(lefthigh));

    return dsp_END_OF_CODE();
}

int dspProg(int argc,char **argv){
   int prog = 0;
    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

         if (strcmp(argv[i],"-diy1") == 0) {
            dspprintf("first diy program made for ASR\n");
            prog = 1;
            continue; }

         if (strcmp(argv[i],"-dither") == 0) {
             dither = 0;
              if (argc>=i) {
                  i++;
                  dither = strtol(argv[i], NULL,10); }
             dspprintf("add dithering %d bits \n",dither);
             continue; }

        if (strcmp(argv[i],"-lpsub") == 0) {
             if (argc>=i) {
                 i++;
                 lpsub = strtol(argv[i], NULL,10); }
            dspprintf("low pass subwoofer %dhz\n",lpsub);
            continue; }

        // .. put the other line options here.
    }

    switch (prog) {
    case 1:  return dspProg_3ways_LR4();
    case 2:
    case 3:
    case 4:
    case 5:
    default:  return dsp_END_OF_CODE();
    }
}

