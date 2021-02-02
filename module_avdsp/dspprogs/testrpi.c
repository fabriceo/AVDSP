#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)
#define DACIN(x)  (8+(x))

void dspcodes() {

    dsp_PARAM();
    int filter = dspBiquad_Sections_Flexible();
        dsp_LP_LR4(1000);
        dsp_filter(FPEAK,  100,  1.0,  1.0 );
        dsp_filter(FPEAK,  500,  1.0,  1.0 );
        dsp_filter(FPEAK,  1000, 1.0,  1.0 );
        dsp_filter(FPEAK,  2000, 1.0,  1.0 );

    dsp_CORE();
    dsp_TPDF_CALC(23);

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filter);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(0));

    dsp_CORE();
    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filter);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(1));

}


int dspProg(int argc,char **argv){

    dspcodes();

    return dsp_END_OF_CODE();

}

