#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"


int filter(int fcross, int subdelay){
    dsp_PARAM();

    int lowpass = dspBiquad_Sections(3);
        dsp_LP_BES6(fcross);

    dsp_CORE();  // first core

    dsp_LOAD(0);
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(subdelay);
    dsp_SWAPXY();   // move delayed signa in Y and restore original sample
    dsp_BIQUADS(lowpass);
    dsp_STORE(8);
    dsp_SWAPXY();
    dsp_SUBXY();            // 12db/oct High pass, with LP BES6
    dsp_STORE(9);

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

    return filter(fcross,subdelay);
}

