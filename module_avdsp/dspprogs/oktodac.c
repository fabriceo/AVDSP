#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"
// this file describe some DSP programs to be used on the OKTO DAC (8PRO or STEREO)

#define DACOUT(x) (x)                  // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + (x) )          // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + (x) )          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + (x) )      // the samples going to the USB Host are offseted by 24



// basic program providing a passtrough between USB and DAC/ADC
int dspProgDAC8PRODSP(int dither){

    return dsp_END_OF_CODE();

    dsp_CORE();  

    dsp_LOAD_STORE();   // cpu optimized version for a multiple LOAD and STORE sequence
    for (int i=0; i<2; i++)
        dspLoadStore_Data( USBOUT(i), DACOUT(i) );
        
    dsp_LOAD_STORE();   // cpu optimized version for a multiple LOAD and STORE sequence
    for (int i=0; i<2; i++)
    	dspLoadStore_Data( ADCIN(i), USBIN(i) );

    dsp_LOAD_STORE();
    for (int i=2; i<8; i++)
        dspLoadStore_Data( USBOUT(i & 1), USBIN(i) );   // simply loop back stereo to each pair

    if (dither>=0) dsp_TPDF_CALC(dither);
    
 // Left channels       
    dsp_CORE();  
	for (int i=2; i<8; i+=2) {
        dsp_LOAD_GAIN_Fixed( USBOUT(0) , 0.0631 );  // -24db as a security
        if (dither>=0) dsp_SAT0DB_TPDF();
        else dsp_SAT0DB();
        dsp_STORE( DACOUT(i) );
	}
        	 
 // right channels
    dsp_CORE();  
	for (int i=3; i<8; i+=2) {
        dsp_LOAD_GAIN_Fixed( USBOUT(1) , 0.0631 );
        if (dither>=0) dsp_SAT0DB_TPDF();
        else dsp_SAT0DB();
        dsp_STORE( DACOUT(i) );
	}

    return dsp_END_OF_CODE();
}

// passtrough program between USB and DAC, considering 2/4/6/8 channels, and between SPDIF-AES and USB
int dspProgDACSTEREO(int outs, int dither){

    dsp_CORE();  // first and unique core

    if (dither>=0) {
        dsp_TPDF_CALC(dither);  // calculate tpdf value for dithering

        for (int i=0; i<2; i++) {
            dsp_LOAD_GAIN_Fixed( ADCIN(i) , 1.0 );
            dsp_SAT0DB_TPDF();
            dsp_STORE( USBIN(i) );
        }
    } else {
        dsp_LOAD_STORE();   // cpu optimized version for a multiple LOAD and STORE sequence
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
        dsp_LOAD_STORE();   // cpu optimized version for a multiple LOAD and STORE sequence
        for (int i=0; i<8; i++)
            dspLoadStore_Data( USBOUT(i), DACOUT(i) );
        break;}
    }

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


int dspNoProg(){
    return dsp_END_OF_CODE();
}

// this basic program create a loop between usb out<->in (useless!)
int dspProgUsbLoopBack(int outs, int dither){
    if (dither>=0) {
		dsp_TPDF_CALC(dither);	// calculate tpdf value for dithering

	    for (int i=0; i<outs; i++) {
	    	dsp_LOAD_GAIN_Fixed( USBOUT(i) , 1.0 );
	    	dsp_SAT0DB_TPDF();
	    	dsp_STORE( USBIN(i) );
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
        
#if 0 // to avoir warning "not used"
        int testBQ = dspBiquad_Sections_Flexible();
        dsp_LP_BUT4(2000);
        dsp_filter(FPEAK, 400,  1.0 , dB2gain(+6.0));
        dsp_filter(FPEAK, 100,  1.0, dB2gain( -6.0));

    	int mux = dspLoadMux_Inputs(2);
        dspLoadMux_Data(USBOUT(0), 0.5);  // possibility to mix any of 2 inputs
        dspLoadMux_Data(USBOUT(0), 0.5);  // and to adapt gain between -1.0 .. +1.0 for each

        int allpass = dspBiquad_Sections_Flexible();
        dsp_filter(FAP2, 1800, 0.57 , 1.0);
        dsp_filter(FAP2, 1800, 0.57 , 1.0);
        dsp_filter(FAP2, 1800, 0.57 , 1.0);
#endif

        int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_BES6(800,1.0);



    int mem1 = dspMem_Location();

dsp_CORE();
        //dsp_LOAD_MUX(mux);
        dsp_LOAD_GAIN_Fixed( USBOUT(0) , 0.5 );
        dsp_BIQUADS(lowpass);
        dsp_STORE_X_MEM(mem1);
        dsp_SAT0DB_GAIN_Fixed(2.0);
        dsp_STORE( USBIN(0) );

        //dsp_COPYYX();
        dsp_LOAD_GAIN_Fixed( USBOUT(0) , 0.5 );
        dsp_DELAY_DP_FixedMicroSec(752000/800);
        //dsp_BIQUADS(allpass);
        dsp_LOAD_X_MEM(mem1);
        dsp_SUBXY();
        dsp_NEGX();
        dsp_SAT0DB_GAIN_Fixed(2.0);
        dsp_STORE( USBIN(1) );

        dsp_LOAD_STORE();
        dspLoadStore_Data( USBOUT(1), USBIN(7) );   // loopback for REW

    return dsp_END_OF_CODE();
}

int dspProg(int argc,char **argv){
   int prog = 0;
   int dither = -1; // no dithering by default
   int outs;
   	for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

                 if (strcmp(argv[i],"-dac8prodsp") == 0) {
                 	dspprintf("factory dsp program for dac8prodsp\n");
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
                    dspprintf("test program for dac8prodsp and rew\n");
                    prog = 4;
                    outs = 8;
                    continue; }


                 if (strcmp(argv[i],"-dacstereodsp4") == 0) {
                    dspprintf("program for dac stereo with 4 dsp_CORE basic filtering+delay\n");
                    prog = 5;
                    outs = 4;
                    continue; }


                 if (strcmp(argv[i],"-dither") == 0) {
                     dither = 0;
                      if (argc>=i) {
                          i++;
                          dither = strtol(argv[i], NULL,10); }
                     dspprintf("add dithering %d bits \n",dither);
                     continue; }

    }

	switch (prog) {
	case 1:  return dspProgDAC8PRODSP(dither);
	case 2:  return dspProgDACSTEREO(outs, dither);
	case 3:  return dspProgUsbLoopBack(outs, dither);
    case 4:  return dspProgTest();
    case 5:  return dspDACStereoDsp4channels(outs);
	default: return dspNoProg();
	}
}

