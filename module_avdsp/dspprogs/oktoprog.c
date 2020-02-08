#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)
#define ADCIN(x)  (8  + x )
#define USBOUT(x) (16 + x)
#define USBIN(x)  (16 + 8 + x)

int oktoProg(int fcross, int subdelay){
    dsp_PARAM();

    int lowpass = dspBiquad_Sections(3);
        dsp_LP_BES6(fcross);
        //dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        //dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);

    dsp_CORE();  // first core

    dsp_LOAD(USBOUT(0));
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(subdelay);
    dsp_SWAPXY();   // move delayed signa in Y and restore original sample
    dsp_BIQUADS(lowpass);
    dsp_STORE(USBIN(0));
    dsp_SWAPXY();
    dsp_SUBXY();            // 12db/oct High pass, with LP BES6
    dsp_STORE(USBIN(2));


    dsp_LOAD(USBOUT(1));
    dsp_STORE(USBIN(1));

    return dsp_END_OF_CODE();
}


int dspProg(int argc,char **argv){
   int fcross = 1000;  
   int subdelay = 745; 

   for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

                 if (strcmp(argv[i],"-fx") == 0) {
                     i++;
                     if (argc>i) {
                         fcross = strtol(argv[i], NULL,10);
                         printf("crossover fx = %d\n",fcross);
                         continue; } }

                 if (strcmp(argv[i],"-subdelay") == 0) {
                     i++;
                     if (argc>i) {
                         subdelay = strtol(argv[i], NULL,10);
                         printf("subdelay = %d\n",subdelay);
                         continue; } }

        }

    return oktoProg(fcross,subdelay);
}

