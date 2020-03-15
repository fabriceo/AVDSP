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
	    	dsp_RMS(60000,60);
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

int dspProg(int argc,char **argv){
   int prog = 0;
   int dither = 0;
   int outs;
   
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

                if (strcmp(argv[i],"-dithering") == 0) {
                 	dither = 24;
                     if (argc>=i) {
                 	     i++;
                         dither = strtol(argv[i], NULL,10); }
                 	dspprintf("add dithering %d on each spdif inputs\n",dither);
                    continue; }
    }

	switch (prog) {
	case 1:  return dspProgDAC8PRO(dither);
	case 2:  return dspProgDACSTEREO(outs, dither);
	case 3:  return dspProgUsbLoopBack(outs, dither);
	default: return dspNoProg();
	}
}

