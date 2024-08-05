#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)
#define DACIN(x)  (8+(x))

void dspcodesstereo() {

    dsp_CORE();

    dsp_PARAM();
    int filterHeadphones = dspBiquad_Sections_Flexible();
        dsp_filter(FPEAK,  100,  1.0,  1.0 );
        dsp_filter(FPEAK,  500,  1.0,  1.0 );
        dsp_filter(FPEAK,  1000, 1.0,  1.0 );
        dsp_filter(FPEAK,  2000, 1.0,  1.0 );

    dsp_TPDF_CALC(23);

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterHeadphones);
    dsp_STORE_TPDF(DACOUT(0));

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterHeadphones);
    dsp_STORE_TPDF(DACOUT(1));

}

void dspcodescrossover() {

    dsp_CORE(); // left channel

    dsp_PARAM(); // param section can be used by any core
    int filterLow = dspBiquad_Sections_Flexible();
        dsp_LP_LR4(400,1.0);
        dsp_filter(FHP1,   10,  1.0,   1.0 );
        dsp_filter(FPEAK,  80,  1.0,   1.0 );
        dsp_filter(FPEAK,  100, 1.0,   1.0 );
        dsp_filter(FPEAK,  150, 1.0,   1.0 );

    int filterMid = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(400,1.0);
        dsp_LP_LR4(2500,1.0);
        dsp_filter(FPEAK,  500,  1.0,   1.0 );
        dsp_filter(FPEAK,  800,  1.0,   1.0 );
        dsp_filter(FPEAK,  1200, 1.0,   1.0 );
        dsp_filter(FPEAK,  1500, 1.0,   1.0 );

    int filterHigh = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(2500,1.0);
        dsp_filter(FPEAK,  4000,  1.0,   1.0 );
        dsp_filter(FPEAK,  5000,  1.0,   1.0 );
        dsp_filter(FPEAK, 10000,  1.0,   1.0 );
        dsp_filter(FLP1,  15000, 1.0,   1.0 );

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterLow);
    dsp_SAT0DB();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(2));

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterMid);
    dsp_SAT0DB();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(3));

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterHigh);
    dsp_SAT0DB();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(4));

    dsp_CORE(); //right

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterLow);
    dsp_SAT0DB();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(5));

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterMid);
    dsp_SAT0DB();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(6));

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterHigh);
    dsp_SAT0DB();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(7));

}


int crossover = 0;
int dspProg(int argc,char **argv){

    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters

         if (strcmp(argv[i],"-crossover") == 0) {
            dspprintf("generating dsp code for 8 channels\n");
            crossover = 1;
            continue; }
    }

    dspcodesstereo();

    if (crossover) dspcodescrossover();

    return dsp_END_OF_CODE();

}

