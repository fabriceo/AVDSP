#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"


#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24



int dspProgDAC8PRO(){

    dsp_CORE();  // first and unique core
    dsp_LOAD(USBOUT(0));
    dsp_STORE(DACOUT(0));
    dsp_LOAD(USBOUT(1));
    dsp_STORE(DACOUT(1));
    dsp_LOAD(ADCIN(0));
    dsp_STORE(USBIN(0));
    dsp_LOAD(ADCIN(1));
    dsp_STORE(USBIN(1));

    dsp_LOAD_STORE();
        dspLoadStore_Data( USBOUT(2), DACOUT(2) );
        dspLoadStore_Data( USBOUT(3), DACOUT(3) );
        dspLoadStore_Data( USBOUT(4), DACOUT(4) );
        dspLoadStore_Data( USBOUT(5), DACOUT(5) );
        dspLoadStore_Data( USBOUT(6), DACOUT(6) );
        dspLoadStore_Data( USBOUT(7), DACOUT(7) );

    dsp_LOAD_STORE();
        dspLoadStore_Data( ADCIN(2), USBIN(2) );
        dspLoadStore_Data( ADCIN(3), USBIN(3) );
        dspLoadStore_Data( ADCIN(4), USBIN(4) );
        dspLoadStore_Data( ADCIN(5), USBIN(5) );
        dspLoadStore_Data( ADCIN(6), USBIN(6) );
        dspLoadStore_Data( ADCIN(7), USBIN(7) );

    return dsp_END_OF_CODE();
}


int dspProgDACSTEREO(){

    dsp_CORE();  // first and unique core

    dsp_LOAD(USBOUT(0));
    dsp_STORE(DACOUT(0));
    dsp_LOAD(USBOUT(1));
    dsp_STORE(DACOUT(1));
    dsp_LOAD(ADCIN(0));
    dsp_STORE(USBIN(0));
    dsp_LOAD(ADCIN(1));
    dsp_STORE(USBIN(1));

    dsp_LOAD_STORE();
        dspLoadStore_Data( USBOUT(0), DACOUT(2) );
        dspLoadStore_Data( USBOUT(1), DACOUT(3) );
        dspLoadStore_Data( USBOUT(0), DACOUT(4) );
        dspLoadStore_Data( USBOUT(1), DACOUT(5) );
        dspLoadStore_Data( USBOUT(0), DACOUT(6) );
        dspLoadStore_Data( USBOUT(1), DACOUT(7) );

    return dsp_END_OF_CODE();
}

int dspNoProg(){
    return dsp_END_OF_CODE();
}

int dspProg(int argc,char **argv){
   int prog = 0;
   
   for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

                 if (strcmp(argv[i],"-dac8pro") == 0) {
                     i++;
                     if (argc>i) {
                         printf("factory dsp program for dac8pro\n");
                         prog = 1;
                         continue; } }

                 if (strcmp(argv[i],"-dacstereo") == 0) {
                     i++;
                     if (argc>i) {
                         printf("factory dsp program for dacstereo\n");
                         prog = 2;
                         continue; } }


        }

	switch (prog) {
	case 1:  return dspProgDAC8PRO();
	case 2:  return dspProgDACSTEREO();
	default: return dspNoProg();
	}
}

