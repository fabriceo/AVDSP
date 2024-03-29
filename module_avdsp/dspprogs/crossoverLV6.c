#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24



int dspProg_crossoverLV6(int fcross, int delay){
    dspprintf("dspProg_crossoverLV6 program prepared for okto dac\n");

    dsp_PARAM();

    int lowpass = dspBiquad_Sections(3);
        dsp_LP_BES6(fcross*1.25);
    int highpass = dspBiquad_Sections(1);
        dsp_HP_BUT2(fcross*0.75);

    if (delay == 0) delay = 752000/(fcross*1.25);  // group delay of the bessel6
    //if (delay == 0) delay = 986000/fcross;  // group delay of the bessel8

    dsp_CORE();  // first core
    dsp_TPDF_CALC(24);
    dsp_LOAD(USBOUT(1));    // loop back with minimum delay time for reference
    dsp_STORE(USBIN(1));

    //dsp_CORE();  // second core just for test
    dsp_LOAD(USBOUT(0)); 
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(delay); //single precios delay is good here
    dsp_GAIN_Fixed(1.0);
    dsp_SWAPXY();
    dsp_GAIN_Fixed(1.0);
    dsp_BIQUADS(lowpass);   //compute lowpass filter
    dsp_SUBYX();
    dsp_SAT0DB_TPDF(); 
    dsp_STORE( USBIN(2) );
    dsp_SWAPXY();
    //dsp_BIQUADS(highpass);   //compute lowpass filter
    dsp_SAT0DB_TPDF(); 
    dsp_STORE( USBIN(3) );

    dsp_CORE();

    dsp_PARAM();

    int LPLR4 = dspBiquad_Sections(2); dsp_LP_LR4(fcross);
    int HPLR4 = dspBiquad_Sections(2); dsp_HP_LR4(fcross);


    dsp_LOAD_GAIN_Fixed( USBOUT(0) , 1.0);
    dsp_COPYXY();
    dsp_BIQUADS(LPLR4);
    dsp_SAT0DB_TPDF();
    dsp_STORE( USBIN(4) );
    dsp_SWAPXY();
    dsp_BIQUADS(HPLR4);
    dsp_SAT0DB_TPDF();
    dsp_STORE( USBIN(5) );

    return dsp_END_OF_CODE();
}



int dspProg(int argc,char **argv){
   int fcross = 1000;  
   int delay = 0;

   for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

                 if (strcmp(argv[i],"-fx") == 0) {
                     i++;
                     if (argc>i) {
                         fcross = strtol(argv[i], NULL,10);
                         printf("crossover fx = %d\n",fcross);
                         continue; } }

                 if (strcmp(argv[i],"-delay") == 0) {
                     i++;
                     if (argc>i) {
                         delay = strtol(argv[i], NULL,10);
                         printf("delay = %d\n",delay);
                         continue; } }

        }

    return dspProg_crossoverLV6(fcross,delay);
}

