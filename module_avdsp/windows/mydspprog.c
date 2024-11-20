#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"
 
// this file describe some DSP programs to be used on the OKTO DAC (8PRO or STEREO)

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24

// basic program providing a passtrough between USB and DAC/ADC
int dspProgDAC8PRO(int dither){

    dsp_CORE();  // first and unique core

    dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
    for (int i=0; i<8; i++)
	    dspLoadStore_Data( USBOUT(i), DACOUT(i) );

    if (dither>=0) {
    	dsp_TPDF_CALC(dither);	// calculate tpdf value for dithering
   
	    for (int i=0; i<8; i++) {
	    	dsp_LOAD_GAIN_Fixed( ADCIN(i) , 1.0 );
	    	dsp_SAT0DB();
	    	dsp_STORE_TPDF( USBIN(i) );
		}
	} else {
	    dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
	    for (int i=0; i<8; i++)
		    dspLoadStore_Data( ADCIN(i), USBIN(i) );
	}

    return dsp_END_OF_CODE();
}

// passtrough program between USB and DAC, considering 2/4/6/8 channels, and between SPDIF-AES and USB
int dspProgDACSTEREO(int outs, int dither){

    dsp_CORE();  // first and unique core

    if (dither>=0) {
		dsp_TPDF_CALC(dither);	// calculate tpdf value for dithering

	    for (int i=0; i<2; i++) {
	    	dsp_LOAD_GAIN_Fixed( ADCIN(i) , 1.0 );
	    	dsp_SAT0DB();
	    	dsp_STORE_TPDF( USBIN(i) );
		}
	} else {
		dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
		    dspLoadStore_Data( ADCIN(0), USBIN(0) );
		    dspLoadStore_Data( ADCIN(1), USBIN(1) );
	}

	switch (outs) {
		case 2: {
	    dsp_LOAD_STORE();
	        dspLoadStore_Data( USBOUT(0), DACOUT(0) );  // the stereo signal goes to 4 pairs of DACs in paralell
	        dspLoadStore_Data( USBOUT(1), DACOUT(1) );
	        dspLoadStore_Data( USBOUT(0), DACOUT(2) );
	        dspLoadStore_Data( USBOUT(1), DACOUT(3) );
	        dspLoadStore_Data( USBOUT(0), DACOUT(4) );
	        dspLoadStore_Data( USBOUT(1), DACOUT(5) );
	        dspLoadStore_Data( USBOUT(0), DACOUT(6) );
	        dspLoadStore_Data( USBOUT(1), DACOUT(7) );
		break;}
		case 4: {
	    dsp_LOAD_STORE();
	        dspLoadStore_Data( USBOUT(0), DACOUT(0) );  // considered as a quad dac with 4567 in // of 0123
	        dspLoadStore_Data( USBOUT(1), DACOUT(1) );
	        dspLoadStore_Data( USBOUT(2), DACOUT(2) );
	        dspLoadStore_Data( USBOUT(3), DACOUT(3) );
	        dspLoadStore_Data( USBOUT(0), DACOUT(4) );
	        dspLoadStore_Data( USBOUT(1), DACOUT(5) );
	        dspLoadStore_Data( USBOUT(2), DACOUT(6) );
	        dspLoadStore_Data( USBOUT(3), DACOUT(7) );
		break;}
		case 6: {
	    dsp_LOAD_STORE();
	        dspLoadStore_Data( USBOUT(0), DACOUT(0) );  // considered as a 6 channel dac
	        dspLoadStore_Data( USBOUT(1), DACOUT(1) );
	        dspLoadStore_Data( USBOUT(2), DACOUT(2) );
	        dspLoadStore_Data( USBOUT(3), DACOUT(3) );
	        dspLoadStore_Data( USBOUT(4), DACOUT(4) );
	        dspLoadStore_Data( USBOUT(5), DACOUT(5) );
	        dspLoadStore_Data( USBOUT(0), DACOUT(6) );
	        dspLoadStore_Data( USBOUT(1), DACOUT(7) );
		break;}
		case 8: {
		dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
    	for (int i=0; i<8; i++) { dspLoadStore_Data( USBOUT(i), DACOUT(i) ); }
		break;}
	}

    return dsp_END_OF_CODE();
}

int dspNoProg(){
    return dsp_END_OF_CODE();
}

// basic and generic DSP program for the DAC stereo, able to generate 4 outputs out of a stereo signal
int dspDACStereoDsp4channels(int outs){
    const int in1  = USBOUT(0);
    const int in2  = USBOUT(1);
    const int in3  = USBOUT(2);
    const int in4  = USBOUT(3);
    const int out1 = DACOUT(0);
    const int out2 = DACOUT(1);
    const int out3 = DACOUT(2);
    const int out4 = DACOUT(3);

    dsp_PARAM();
    int mux1 = dspLoadMux_Inputs(2);
        dspLoadMux_Data(in1, 0.5);  // possibility to mix any of 2 inputs
        dspLoadMux_Data(in1, 0.5);  // and to adapt gain between -1.0 .. +1.0 for each
    int mux2 = dspLoadMux_Inputs(2);
        dspLoadMux_Data(in2, 0.5);
        dspLoadMux_Data(in2, 0.5);
    int mux3 = dspLoadMux_Inputs(2);
        dspLoadMux_Data(in3, 0.5);
        dspLoadMux_Data(in3, 0.5);
    int mux4 = dspLoadMux_Inputs(2);
        dspLoadMux_Data(in4, 0.5);
        dspLoadMux_Data(in4, 0.5);

    int delay1 = dspDelay_MicroSec_Max_Default(5000,0);    // max 5ms = 170cm
    int delay2 = dspDelay_MicroSec_Max_Default(5000,0);
    int delay3 = dspDelay_MicroSec_Max_Default(5000,0);
    int delay4 = dspDelay_MicroSec_Max_Default(5000,0);

    int fbank1 = dspBiquad_Sections(12);    // max 12 biquad cell, can be combinaison of any filters
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);

    int fbank2 = dspBiquad_Sections(12);    // 12 biquad cells at 192K is OK
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);

    int fbank3 = dspBiquad_Sections(12);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);

    int fbank4 = dspBiquad_Sections(12);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);
    dsp_filter(FPEAK, 1000, 0.7, 1.0);

    dsp_CORE();
    dsp_LOAD_MUX(mux1);
    dsp_BIQUADS(fbank1);
    dsp_SAT0DB();
    dsp_DELAY(delay1);
    dsp_STORE(out1);
    dsp_STORE(USBIN(0));

    dsp_CORE();
    dsp_LOAD_MUX(mux2);
    dsp_BIQUADS(fbank2);
    dsp_SAT0DB();
    dsp_DELAY(delay2);
    dsp_STORE(out2);
    dsp_STORE(USBIN(1));

    dsp_CORE();
    dsp_LOAD_MUX(mux3);
    dsp_BIQUADS(fbank3);
    dsp_SAT0DB();
    dsp_DELAY(delay3);
    dsp_STORE(out3);
    dsp_STORE(USBIN(2));

    dsp_CORE();
    dsp_LOAD_MUX(mux4);
    dsp_BIQUADS(fbank4);
    dsp_SAT0DB();
    dsp_DELAY(delay4);
    dsp_STORE(out4);
    dsp_STORE(USBIN(3));

    return dsp_END_OF_CODE();
}

// this basic program create a loop between usb out<->in (useless!)
int dspProgUsbLoopBack(int outs, int dither){
    if (dither>=0) {
		dsp_TPDF_CALC(dither);	// calculate tpdf value for dithering

	    for (int i=0; i<outs; i++) {
	    	dsp_LOAD_GAIN_Fixed( USBOUT(i) , 1.0 );
	    	dsp_SAT0DB();
	    	dsp_STORE_TPDF( USBIN(i) );
		}
	} else {
		dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
		for (int i=0; i<outs; i++)
			dspLoadStore_Data( USBOUT(i), USBIN(i) );
	}
    return dsp_END_OF_CODE();
}

// used by the author to test each dsp_function with a little program...
int dspProgTest(){

        dsp_PARAM();
        int testFir = dspFir_Impulses();
        dspFir_ImpulseFile("../dspprogs/fir3.txt",3);   //44k
        dspFir_ImpulseFile("../dspprogs/fir3.txt",3);
        dspFir_ImpulseFile("../dspprogs/fir3.txt",3);
        dspFir_ImpulseFile("../dspprogs/fir3.txt",3);   //96

        int testBQ = dspBiquad_Sections_Flexible();
        dsp_filter(FAP2,1000, 0.58, 1.0);
/*
        dsp_LP_BUT4(1000,1.0);
        dsp_filter(FPEAK, 400,  1.0, dB2gain(6.0));
        dsp_filter(FPEAK, 100,  1.0, dB2gain(-6.0));
*/

dsp_CORE();
        //dsp_TPDF_CALC(22);   // calculate tpdf value for dithering

        dsp_LOAD_GAIN_Fixed( USBOUT(0) , 1.0 );
        //dsp_FIR(testFir);
        dsp_BIQUADS(testBQ);
        dsp_SAT0DB();
        dsp_STORE( USBIN(0) );

        dsp_LOAD_STORE();
        dspLoadStore_Data( USBOUT(1), USBIN(7) );   // loopback for REW

    return dsp_END_OF_CODE();
}

#if 1
// LR type of crossover for the 2 way system of the Author
void crossoverLR6acoustic(int freq, int gd, int dither, int defaultGain, float gaincomp, int microslow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_BUT4(532,1.0);
        dsp_filter(FHS1,  500,  0.5, dB2gain(-5.0));
        dsp_filter(FPEAK, 180,  2.5, dB2gain(-1.9));
        dsp_filter(FPEAK, 544,  4.0, dB2gain(-2.0));

    int highpass = dspBiquad_Sections_Flexible();
        dsp_HP_BUT3(822,1.0);
        dsp_filter(FPEAK, 1710, 2.0, dB2gain(-4.8));
        dsp_filter(FPEAK,  917, 5.0, dB2gain(+3.0));
        dsp_filter(FHS2, 11500, 0.7, dB2gain(+6.0));


    dsp_LOAD_X_MEM(in);
    dsp_BIQUADS(lowpass);   //compute lowpass filter

    dsp_SAT0DB_GAIN_Fixed( defaultGain);
    if (dither>=0) dsp_STORE_TPDF( USBIN(outlow) );
    else dsp_STORE( USBIN(outlow) );
    if (microslow > 0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer delayed by %d us\n",microslow);
    }
    dsp_STORE( DACOUT(outlow) ); // low driver

    dsp_LOAD_X_MEM(in);
    dsp_BIQUADS(highpass);   //compute lowpass filter
    dsp_SAT0DB_GAIN_Fixed( gaincomp * defaultGain );
    if (dither>=0)
        dsp_STORE_TPDF( USBIN(outhigh) );
    else dsp_STORE( USBIN(outhigh) );
    if (microslow < 0) {
        dsp_DELAY_FixedMicroSec(-microslow);
        printf("compression driver delayed by %d us\n",-microslow); // not relevant !-)
    }
    //dsp_NEGX();   // change polarity to check driver allignements
    dsp_STORE( DACOUT(outhigh) );
}
#endif

// special crossover for test, substractive and using Notch filters. Not really finished/working
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
    dsp_SAT0DB_GAIN_Fixed( defaultGain);
    if (dither>=0)
        dsp_STORE_TPDF( USBIN(outlow) );     // feedback to computer for measurements
    else
        dsp_STORE( USBIN(outlow) );     // feedback to computer for measurements
    if (microslow>0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer (ahead of compression) will be delayed by %d us\n",microslow); }
    dsp_STORE( DACOUT(outlow) );


    dsp_LOAD_X_MEM(memHPF);
    dsp_LOAD_X_MEM(memLBPF);
    dsp_ADDXY();
    // high ready

    dsp_BIQUADS(compEQ);
    //dsp_NEGX(); // invert phase due to cable mismatch ?
    dsp_SAT0DB_GAIN_Fixed( gaincomp * defaultGain );
    if (dither>=0)
        dsp_STORE_TPDF( USBIN(outhigh) );    // feedback to computer for measurements
    else dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements
    if (microslow<0) {
        dsp_DELAY_FixedMicroSec(-microslow);
        printf("compression (behind woofer) will be additionally delayed by %d us\n",-microslow); }
    dsp_STORE( DACOUT(outhigh) );
}



// lipsitch vanderkoy crossover, using delay and substraction
void crossoverLV(int freq, int gd, int dither, int defaultGain, float gaincomp, int microslow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_BES6(freq, 1.0);

    if (gd == 0) gd = 752000/freq;  // group delay of the bessel6
    //if (gd == 0) gd = 986000/freq;  // group delay of the bessel8

    // equalization for 18s 2" comp 2080 on Horn XR2064
    int compEQ = dspBiquad_Sections_Flexible();
		dsp_filter(FHP2,200,0.7,1.0);   // extra protection to remove lower freq        	dsp_filter(FPEAK,1000,5,dB2gain(+4.0));
        dsp_filter(FPEAK,1700,3,dB2gain(-3.0));
        dsp_filter(FHS2, 9000,0.6,dB2gain(5.0));


    dsp_LOAD_X_MEM(in);
    dsp_COPYXY();
    dsp_DELAY_DP_FixedMicroSec(gd);
    dsp_SWAPXY();
    dsp_BIQUADS(lowpass);       //compute lowpass filter in X
    dsp_SUBYX();                // compute high pass in Y

    dsp_SAT0DB_GAIN_Fixed( defaultGain);
    if (dither>=0)
        dsp_STORE_TPDF( USBIN(outlow) );     // feedback to computer for measurements
    else dsp_STORE( USBIN(outlow) );     // feedback to computer for measurements
    if (microslow>0) {
        dsp_DELAY_FixedMicroSec(microslow);
        printf("woofer (ahead of compression) will be delayed by %d us\n",microslow); }
    dsp_STORE( DACOUT(outlow) );

    dsp_SWAPXY();               // get highpass
    dsp_SHIFT_FixedInt(-100);   // by default -100 means DSP_MANT
    dsp_GAIN_Fixed(gaincomp);
    dsp_BIQUADS(compEQ);
    dsp_SAT0DB_GAIN_Fixed( defaultGain );
    if (dither>=0)
         dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements
    else dsp_STORE( USBIN(outhigh) );    // feedback to computer for measurements

    //dsp_NEGX(); // invert phase during tests to test allignement
    dsp_STORE( DACOUT(outhigh) );
}


int mono = 0;
const int leftin  = USBOUT(0);
const int rightin = USBOUT(1);

// main program for the crossover of a 2 way stystem based on 12LW1400 and 2" comp
int dspProgDACFABRICEO(int fx, int gd, int dither, float gaincomp, int microslow){
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
        printf("ditehring enable for %d usefull bits\n",dither);
        dsp_TPDF_CALC(dither); }

    dsp_LOAD_STORE();
        dspLoadStore_Data( leftin,    DACOUT(0) );      // headphones
        dspLoadStore_Data( rightin,   DACOUT(1) );
        dspLoadStore_Data( rightin,   USBIN(7) );       // loopback for REW
        dspLoadStore_Data( ADCIN(0),  USBIN(0) );       // spdif in passtrough
        dspLoadStore_Data( ADCIN(1),  USBIN(1) );

    if (mono) {
        dsp_LOAD_MUX(avgLR);        // load and mix left+right
        dsp_STORE_X_MEM(avgLRmem);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(rightEQ);
        dsp_STORE_X_MEM(leftmem);
        dsp_STORE_X_MEM(rightmem);

        dsp_LOAD_X_MEM(avgLRmem);
    } else {

        dsp_LOAD_GAIN_Fixed(leftin, attLeft);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(leftEQ);
        dsp_STORE_X_MEM(leftmem);

        dsp_LOAD_GAIN_Fixed(rightin, attRight);
        //dsp_DCBLOCK(10);
        dsp_BIQUADS(rightEQ);
        dsp_STORE_X_MEM(rightmem);

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



int dspProg(int argc,char **argv){
   int prog = 0;
   int dither = -1; // no dithering by default
   int outs = 0;
   int fx = 800;
   int gd = 0;
   float gaincomp = 0.35;
   int microslow = 740;  //250mm ahead of compression driver
   	for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

                 if (strcmp(argv[i],"-dac8pro") == 0) {
                 	dspprintf("factory dsp program for dac8pro\n");
                    prog = 1;
                    outs = 8;
                    continue; }

                 if (strcmp(argv[i],"-dacstereo") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 2;
                    outs = 2;
                    continue; }
                 
                 if (strcmp(argv[i],"-dacstereo4") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 2;
                    outs = 4;
                    continue; }
                 
                 if (strcmp(argv[i],"-dacstereo6") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 2;
                    outs = 6;
                    continue; }
                 
                 if (strcmp(argv[i],"-dacstereo8") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 2;
                    outs = 8;
                    continue; }
                 
                 if (strcmp(argv[i],"-usbloopback2") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 3;
                    outs = 2;
                    continue; }
                 
                 if (strcmp(argv[i],"-usbloopback4") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 3;
                    outs = 4;
                    continue; }

                 if (strcmp(argv[i],"-usbloopback6") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 3;
                    outs = 6;
                    continue; }

                 if (strcmp(argv[i],"-usbloopback8") == 0) {
                    dspprintf("factory dsp program for dacstereo\n");
                    prog = 3;
                    outs = 8;
                    continue; }

                 if (strcmp(argv[i],"-testrew") == 0) {
                    dspprintf("test program for dac8pro and rew\n");
                    prog = 4;
                    outs = 8;
                    continue; }
 

                 if (strcmp(argv[i],"-dacstereodsp4") == 0) {
                    dspprintf("program for dac stereo with 4 dsp_CORE basic filtering+delay\n");
                    prog = 6;
                    outs = 4;
                    continue; }


                 if (strcmp(argv[i],"-dither") == 0) {
                     dither = 0;
                      if (argc>=i) {
                          i++;
                          dither = strtol(argv[i], NULL,10); }
                     dspprintf("add dithering %d bits \n",dither);
                     continue; }

                 if (strcmp(argv[i],"-dacfabriceo") == 0) {
                    dspprintf("cross over program for oktodac and rew\n");
                    prog = 5;
                    outs = 8;
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

	switch (prog) {
	case 1:  return dspProgDAC8PRO(dither);
	case 2:  return dspProgDACSTEREO(outs, dither);
	case 3:  return dspProgUsbLoopBack(outs, dither);
    case 4:  return dspProgTest();
    case 5:  return dspProgDACFABRICEO(fx, gd, dither, gaincomp, microslow);
    case 6:  return dspDACStereoDsp4channels(outs);
	default: return dspNoProg();
	}
}

