#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"


#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24

int dspProgDAC8PRO(int dither){

    dsp_CORE();  // first and unique core

    dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
    for (int i=0; i<8; i++)
	    dspLoadStore_Data( USBOUT(i), DACOUT(i) );

    if (dither) {
    	dsp_TPDF(dither);	// calculate tpdf value for dithering
   
	    for (int i=0; i<8; i++) {
	    	dsp_LOAD_GAIN_Fixed( ADCIN(i) , 1.0 );
	    	dsp_SAT0DB_TPDF();
	    	dsp_STORE( USBIN(i) );
		}
	} else {
	    dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
	    for (int i=0; i<8; i++)
		    dspLoadStore_Data( ADCIN(i), USBIN(i) );
	}

    return dsp_END_OF_CODE();
}

int dspProgDACSTEREO(int outs, int dither){

    dsp_CORE();  // first and unique core

    if (dither) {
		dsp_TPDF(dither);	// calculate tpdf value for dithering

	    for (int i=0; i<2; i++) {
	    	dsp_LOAD_GAIN_Fixed( ADCIN(i) , 1.0 );
	    	dsp_SAT0DB_TPDF();
	    	dsp_STORE( USBIN(i) );
		}
	} else {
		dsp_LOAD_STORE();	// cpu optimized version for a multiple LOAD and STORE sequence
		    dspLoadStore_Data( ADCIN(0), USBIN(0) );
		    dspLoadStore_Data( ADCIN(1), USBIN(1) );
	}

	switch (outs) {
		case 2: {
	    dsp_LOAD_STORE();
	        dspLoadStore_Data( USBOUT(0), DACOUT(0) );
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
	        dspLoadStore_Data( USBOUT(0), DACOUT(0) );
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
	        dspLoadStore_Data( USBOUT(0), DACOUT(0) );
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
    	for (int i=0; i<8; i++)
	    	dspLoadStore_Data( USBOUT(i), DACOUT(i) );
		break;}
	}

    return dsp_END_OF_CODE();
}

int dspNoProg(){
    return dsp_END_OF_CODE();
}

int dspProgUsbLoopBack(int outs, int dither){
    if (dither) {
		dsp_TPDF(dither);	// calculate tpdf value for dithering

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


int dspProgTest(){

        dsp_PARAM();
        //int mem0 = dspMem_Location();
        //int mem1 = dspMem_Location();

        int lr4 = dspBiquad_Sections(2);
                  dsp_LP_LR4(1000);

        dsp_TPDF(24);   // calculate tpdf value for dithering

        dsp_LOAD_GAIN_Fixed( USBOUT(0) , 1.0 );
        dsp_BIQUADS(lr4);
        dsp_SAT0DB_TPDF();
        dsp_STORE( USBIN(0) );
        dsp_RMS_MilliSec(100,20);
        dsp_STORE( USBIN(2) );

        dsp_LOAD_STORE();
        dspLoadStore_Data( USBOUT(1), USBIN(1) );   // loopback REW

    return dsp_END_OF_CODE();
}

int mono = 0;

const int left  = USBOUT(0);
const int right = USBOUT(1);

void crossoverLV6(int freq, int defaultGain, int gd, float gaincomp, int distlow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpassLV6 = dspBiquad_Sections_Flexible();
        dsp_LP_BES6(freq);
        dsp_filter(FPEAK,200,3,dB2gain(4.0));

    int compEQ = dspBiquad_Sections_Flexible();
        dsp_filter(FPEAK,1200,4,dB2gain(4.0));
        dsp_filter(FHS2,3000,1,dB2gain(4.0));

    int avgLR = dspLoadMux_Inputs(2);
        dspLoadMux_Data(left,0.5 * defaultGain);
        dspLoadMux_Data(right,0.5 * defaultGain);


#if 0
    if (mono) {
        dsp_LOAD(left);
        dsp_LOAD(right);
        dsp_AVGXY();
    } else
        dsp_LOAD(in);
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(gd);
    dsp_GAIN_Fixed( defaultGain * 0.5 );
    dsp_SWAPXY();
    dsp_GAIN_Fixed( defaultGain * 0.5 );
    dsp_BIQUADS(lowpassLV6);    //compute lowpass filter
    dsp_SUBYX();                // compute high pass
#else
    if (mono)
         dsp_LOAD_MUX(avgLR);
    else dsp_LOAD_GAIN_Fixed(in, defaultGain);
    dsp_COPYXY();
    dsp_DELAY_DP_FixedMicroSec(gd);
    dsp_SWAPXY();
    dsp_BIQUADS(lowpassLV6);    //compute lowpass filter
    dsp_SUBYX();                // compute high pass
#endif
    dsp_SAT0DB_TPDF_GAIN_Fixed( defaultGain / 0.5 );
    if (distlow) dsp_DELAY_FixedMilliMeter(distlow,340.0);
    dsp_STORE( DACOUT(outlow) ); // low driver
    //dsp_STORE( USBIN(outlow) );
    dsp_SWAPXY();
    dsp_NEGX();
    dsp_BIQUADS(compEQ);
    dsp_SAT0DB_TPDF_GAIN_Fixed( gaincomp * ( defaultGain / 0.5 ) );
    dsp_STORE( DACOUT(outhigh) );
    //dsp_STORE( USBIN(outhigh) );
}

void crossoverLR4(int freq, int defaultGain, int gd, float gaincomp, int distlow, int in, int outlow, int outhigh){

    dsp_PARAM();
    int lowpass = dspBiquad_Sections_Flexible();
        dsp_LP_LR4(freq);
        dsp_filter(FPEAK,200,3,dB2gain(4.0));


    int highpass = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(freq);
        dsp_filter(FPEAK,1200,4,dB2gain(4.0));
        dsp_filter(FHS2,3000,1,dB2gain(4.0));

    int avgLR = dspLoadMux_Inputs(2);
        dspLoadMux_Data(left,0.5 * defaultGain);
        dspLoadMux_Data(right,0.5 * defaultGain);


    if (mono)
         dsp_LOAD_MUX(avgLR);
    else dsp_LOAD_GAIN_Fixed(in, defaultGain);

    dsp_COPYXY();
    dsp_BIQUADS(lowpass);   //compute lowpass filter
    dsp_SAT0DB_TPDF();
    if (distlow) dsp_DELAY_FixedMilliMeter(distlow,340.0);
    dsp_STORE( DACOUT(outlow) ); // low driver
    //dsp_STORE( USBIN(outlow) );
    dsp_SWAPXY();
    dsp_BIQUADS(highpass);   //compute lowpass filter
    dsp_NEGX();
    dsp_SAT0DB_TPDF_GAIN_Fixed(gaincomp * defaultGain);
    dsp_STORE( DACOUT(outhigh) );
    //dsp_STORE( USBIN(outhigh) );
}


int dspProgDACFABRICEO(int fx, int gd, float gaincomp, int distlow, int dither){
    dspprintf("program for the dac belonging to the author\n");

    dsp_PARAM();

    int avgLR = dspLoadMux_Inputs(0);
        dspLoadMux_Data(left,0.5);
        dspLoadMux_Data(right,0.5);

    dsp_CORE();  // first core

    if (dither == 0) dither = 24;
    dsp_TPDF(dither);   // returns nTh bit noise like the one used in SAT0DB_TPDF

    dsp_LOAD_STORE();
        dspLoadStore_Data( left,  DACOUT(0) );   // headphones
        dspLoadStore_Data( right, DACOUT(1) );
        //dspLoadStore_Data( ADCIN(0),  USBIN(0) );    // spdif in
        //dspLoadStore_Data( ADCIN(1),  USBIN(1) );
        dspLoadStore_Data( right, USBIN(1) );    // loopback REW
/*
    dsp_LOAD(left);
    dsp_STORE( DACOUT(2) ); // low driver
    dsp_STORE( USBIN(2) );  // low driver
    dsp_LOAD(right);
    dsp_STORE( DACOUT(3) ); // high driver
    dsp_STORE( USBIN(3) );  // high driver
     */
    crossoverLV6(fx, 1.0 , gd, gaincomp, distlow, left, 2, 3);
    //crossoverLR4(fx, 1.0 , gd, gaincomp, distlow, left, 2, 3);


    dsp_CORE();  // second core for test

    dsp_LOAD_MUX(avgLR);
    dsp_DITHER();
    dsp_SAT0DB();

    //dsp_LOAD(left);
    //dsp_LOAD(right);
    //dsp_AVGXY();

    dsp_STORE(DACOUT(6));   // center
    //dsp_STORE(USBIN(6));
    dsp_STORE(DACOUT(7));   // lfe
    //dsp_STORE(USBIN(7));

    crossoverLV6(fx, 1.0 , gd, gaincomp, distlow, right, 4, 5);
    //crossoverLR4(fx, 1.0 , gd, gaincomp, distlow, right, 4, 5);

    return dsp_END_OF_CODE();
}



int dspProg(int argc,char **argv){
   int prog = 0;
   int dither = 0;
   int outs;
   int fx = 800;
   int gd = 930;    // typicall groupdelay for BES6 @ 800hz
   float gaincomp = 1.0;
   int distlow = 300;  //300mm ahead of compression driver
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

                 if (strcmp(argv[i],"-dither") == 0) {
                     dither = 24;
                      if (argc>=i) {
                          i++;
                          dither = strtol(argv[i], NULL,10); }
                     dspprintf("add dithering %d \n",dither);
                     continue; }

                 if (strcmp(argv[i],"-dacfabriceo") == 0) {
                    dspprintf("cross over program for dac8pro and rew\n");
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
                    dspprintf("substractive delay %dus\n",gd);
                    continue; }

                if (strcmp(argv[i],"-distlow") == 0) {
                     if (argc>=i) {
                         i++;
                         distlow = strtol(argv[i], NULL,10); }
                    dspprintf("low driver ahead %dmm\n",distlow);
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
    case 5:  return dspProgDACFABRICEO(fx, gd, gaincomp, distlow,dither);
	default: return dspNoProg();
	}
}

