
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

int prog = 0;
int dither = 0;

// these gains are applied in the very last stage of the dsp, before storing result to dac
float gainsubleft  = 1.0;
float gainsubright = 1.0;
float gainlow = 1.0;
float gainmid = 1.0;
float attn;
float lowattn;


int ftype = LPLR2;	// default crossover LR2
int fx = 700;
int gd = 0;
int delaymid = 55;  // midrange delyed by 60us by default rounded to 3 samples at 44k1
int sub = 0;
int delaysubleft  = 0;
int delaysubright = 0;



// lipsitch vanderkoy crossover, using delay and substraction
void crossoverLV(int lowpass, int loweq, int mideq, int in, int outlow, int outhigh){

    dsp_LOAD_MEM(in);
    dsp_COPYXY();
    dsp_DELAY_DP_FixedMicroSec( gd );
    dsp_SWAPXY();
    dsp_BIQUADS( lowpass );        //compute lowpass filter in X
    dsp_SUBYX();                   // compute high pass in Y
    dsp_BIQUADS(loweq);
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( lowattn);
    else dsp_SAT0DB_GAIN_Fixed( lowattn);
    dsp_STORE( USBIN(outlow) );   // feedback to computer for measurements
    dsp_STORE( DACOUT(outlow) );

    dsp_SWAPXY();                 // get highpass
    dsp_BIQUADS(mideq);
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( 1.0 );
    else dsp_SAT0DB_GAIN_Fixed( 1.0 );
    if (delaymid>0) dsp_DELAY_FixedMicroSec(delaymid);
    dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements

    //dsp_NEGX(); // invert phase during tests to test allignement
    dsp_STORE( DACOUT(outhigh) );
}

void crossoverLR2(int lowpass, int loweq, int highpass, int mideq, int in, int outlow, int outmid){

    dsp_LOAD_MEM(in);
    dsp_BIQUADS(lowpass);   //compute lowpass filter
    dsp_BIQUADS(loweq);
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( lowattn );
    else dsp_SAT0DB_GAIN_Fixed( lowattn );			
    dsp_STORE( USBIN(outlow) );
    dsp_STORE( DACOUT(outlow));

    dsp_LOAD_MEM(in);
    dsp_BIQUADS(highpass);
    dsp_BIQUADS(mideq);
    if (dither>=0)
         dsp_SAT0DB_TPDF_GAIN_Fixed( 1.0 );
    else dsp_SAT0DB_GAIN_Fixed( 1.0 );
    if (delaymid) dsp_DELAY_FixedMicroSec(delaymid);
    dsp_STORE( USBIN(outmid) );
    dsp_STORE( DACOUT(outmid));

}

int dspProg_LXmini(){

        setSerialHash(0x9ADD2096);  // serial number 0
        //setSerialHash(0xCAC47719);  // serial number 16

attn     = dB2gain(-8.0); // to avoid any saturation in biquads
lowattn  = dB2gain( -1.2); // to compendate hypex buffer excess gain compared to mid channel

 
int lowpass;
int highpass;
int lowEQ;
int rightmidEQ;
int leftmidEQ;
int leftsubEQ;
int rightsubEQ;

    dsp_PARAM();

// upfront equalization on the stereo channels.
int frontEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FHP1,    10, 0.5, 1.0);    // optional high pass to protect xmax
        dsp_filter(FHS2,   400, 1.0, dB2gain( -2.0));

    //int lowEQ = dspBiquad_Sections_Flexible();
    //    dsp_filter(FLP2,     fx, 0.5,  1.0 ); //dB2gain(-11.2) ); // lowpass LR2
    //    dsp_filter(FPEAK,    50, 0.7,  dB2gain( sub ? 0.0 : +7.0));    // bass boost only if no sub
    //    dsp_filter(FPEAK,  5100, 6.0,  dB2gain(-14.0));   // breakup and raisonance ?
if (ftype == LPLR2) {

	 lowpass = dspBiquad_Sections_Flexible();
        dsp_filter(FLP2,   fx, 0.5,  1.0);

	 highpass = dspBiquad_Sections_Flexible();
        dsp_filter(FHP2,   fx, 0.5, -1.0 ); // highpass LR2 inverted 
        
} else if ((ftype == LPBE4)||(ftype == LPBE6)||(ftype == LPBE8)) {

	int freq;
    lowpass = dspBiquad_Sections_Flexible();
	switch (ftype) {
		case LPBE4 : freq = fx * 1.111;  gd =  526140/freq; dsp_LP_BES4(freq); break;
		case LPBE6 : freq = fx * 1.2563; gd =  759230/freq; dsp_LP_BES6(freq); break;
		case LPBE8 : freq = fx * 1.391;  gd = 1020994/freq; dsp_LP_BES8(freq); break;
	}	
}

     lowEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FPEAK,   50,  0.7,    dB2gain( sub ? 0.0 : +7.0));    // bass boost only if no sub
        //dsp_filter(FPEAK,   68,  6.0,    dB2gain( +2.0));
        dsp_filter(FPEAK,   150, 1.0,    dB2gain( -2.0));
        dsp_filter(FPEAK,   230, 4.0,    dB2gain( -4.0));
        dsp_filter(FPEAK,  5000, 5.0,    dB2gain(-13.0));

     rightmidEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLS2,   1000,  0.5, dB2gain(+16.0));
        dsp_filter(FPEAK,  1900,  4.0, dB2gain( +3.0));;
        dsp_filter(FPEAK,  2500,  2.0, dB2gain( -5.0));
        dsp_filter(FHS2,   8000,  0.7, dB2gain( +5.0));
        dsp_filter(FPEAK, 15500,  1.0, dB2gain( +4.0));

     leftmidEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLS2,   1000,  0.5, dB2gain(+16.0));
        dsp_filter(FPEAK,  1900,  4.0, dB2gain( +3.0));;
        dsp_filter(FPEAK,  2500,  2.0, dB2gain( -5.0));
        dsp_filter(FPEAK,  6000,  0.3, dB2gain( +1.8));   // driver defect ...
        dsp_filter(FHS2,   8000,  0.7, dB2gain( +5.0));
        dsp_filter(FPEAK, 15500,  1.0, dB2gain( +4.0));
        
if (sub) {
    // subwoofer for left or mono
     leftsubEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLP2,   60, 0.5, -1.0);    // LR2 @ 60hz, inverted
        dsp_filter(FPEAK,  50, 1.0, dB2gain(0.0));

    // subwoofer for right only
     rightsubEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FLP2,   60, 0.5, -1.0);    // LR2 @ 60hz, inverted
        dsp_filter(FPEAK,  50, 2.0, dB2gain(0.0));
}

    int leftmem  = dspMem_Location();
    int rightmem = dspMem_Location();

    // used to average Left and Right in case of 1 subwoofer.
    int avgLR = dspLoadMux_Inputs(0);
        dspLoadMux_Data(leftin,  0.5); // gain can be adjusted lower
        dspLoadMux_Data(rightin, 0.5);

// end of dsp_PARAM

dsp_CORE();  // first core, stereo conditioning

    // basic transfers
    dsp_LOAD_STORE();
        dspLoadStore_Data( leftin,    DACOUT(0) );      // headphones
        dspLoadStore_Data( rightin,   DACOUT(1) );
        dspLoadStore_Data( ADCIN(0),  USBIN(0) );       // spdif in passtrough
        dspLoadStore_Data( ADCIN(1),  USBIN(1) );
        dspLoadStore_Data( rightin,   USBIN(1) );       // loopback for REW during tests only

    if (dither>=0) dsp_TPDF_CALC(dither);        // this calculate a tpdf white noise value at each cycle/sample

    dsp_LOAD_GAIN_Fixed(leftin, attn);           // load input and apply a gain 
    dsp_BIQUADS(frontEQ);                        // compute EQs
    dsp_STORE_MEM(leftmem);                      // store in temporary location for second core

    dsp_LOAD_GAIN_Fixed(rightin, attn);
    dsp_BIQUADS(frontEQ);
    dsp_STORE_MEM(rightmem);                        // store in temporary location for third core

if (ftype == LPLR2) {

dsp_CORE();  // second core left channel
	crossoverLR2(lowpass, lowEQ, highpass, leftmidEQ,  leftmem,  4, 5);

dsp_CORE();  // third core right channel
    // right channel
    crossoverLR2(lowpass, lowEQ, highpass, rightmidEQ, rightmem, 2, 3);
    
} else if ((ftype == LPBE4)||(ftype == LPBE6)||(ftype == LPBE8)) {

dsp_CORE();
    crossoverLV(lowpass, lowEQ, leftmidEQ,  leftmem,  4, 5);

dsp_CORE();
    crossoverLV(lowpass, lowEQ, rightmidEQ, rightmem, 2, 3);

}
if ( sub ) {

    dsp_CORE();  // 4th core for subwoofers

    if (sub == 2)
         dsp_LOAD_MEM(rightmem);
    else dsp_LOAD_MUX(avgLR);        // load and mix left+right
    dsp_BIQUADS(rightsubEQ);
    if (sub == 1) dsp_COPYXY();     // temporary store EQed
    dsp_SAT0DB_GAIN_Fixed( gainsubright );
    if (delaysubright) dsp_DELAY_FixedMicroSec(delaysubright);
    dsp_STORE( USBIN(rightsub) );
    dsp_STORE( DACOUT(rightsub));

    if (sub == 2) {
        dsp_LOAD_MEM(leftmem);
        dsp_BIQUADS(leftsubEQ);
    } else dsp_COPYYX();            // retreive stored value
    dsp_SAT0DB_GAIN_Fixed( gainsubleft );
    if (delaysubleft) dsp_DELAY_FixedMicroSec(delaysubleft);
    dsp_STORE( USBIN( leftsub ));
    dsp_STORE( DACOUT( leftsub ));
}

    return dsp_END_OF_CODE();
}

int dspProg(int argc,char **argv){
    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

         if (strcmp(argv[i],"-lxmini") == 0) {
            dspprintf("generating dsp code for LXmini with 50hz bass boost\n");
            prog = 1; 
            continue; }

         if (strcmp(argv[i],"-lv4") == 0) {
            ftype = LPBE4;
            continue; }

         if (strcmp(argv[i],"-lv6") == 0) {
            ftype = LPBE6;
            continue; }

         if (strcmp(argv[i],"-lv8") == 0) {
            ftype = LPBE8;
            continue; }

         if (strcmp(argv[i],"-sub") == 0) {
             sub = 0;
              if (argc>=i) {
                  i++;
                  sub = strtol(argv[i], NULL,10); }
             dspprintf("including %d subwoofer\n",sub);
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

