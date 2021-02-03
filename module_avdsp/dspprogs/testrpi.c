#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)
#define DACIN(x)  (8+(x))

void dspcodes() {

    dsp_PARAM();
    int filterHeadphones = dspBiquad_Sections_Flexible();
        dsp_filter(FPEAK,  100,  1.0,  1.0 );
        dsp_filter(FPEAK,  500,  1.0,  1.0 );
        dsp_filter(FPEAK,  1000, 1.0,  1.0 );
        dsp_filter(FPEAK,  2000, 1.0,  1.0 );

    int filterLow = dspBiquad_Sections_Flexible();
        dsp_LP_LR4(400);
        dsp_filter(FHP1,   10,  1.0,   1.0 );
        dsp_filter(FPEAK,  80,  1.0,   1.0 );
        dsp_filter(FPEAK,  100, 1.0,   1.0 );
        dsp_filter(FPEAK,  150, 1.0,   1.0 );

    int filterMid = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(400);
        dsp_LP_LR4(2500);
        dsp_filter(FPEAK,  500,  1.0,   1.0 );
        dsp_filter(FPEAK,  800,  1.0,   1.0 );
        dsp_filter(FPEAK,  1200, 1.0,   1.0 );
        dsp_filter(FPEAK,  1500, 1.0,   1.0 );

    int filterHigh = dspBiquad_Sections_Flexible();
        dsp_HP_LR4(2500);
        dsp_filter(FPEAK,  4000,  1.0,   1.0 );
        dsp_filter(FPEAK,  5000,  1.0,   1.0 );
        dsp_filter(FPEAK, 10000,  1.0,   1.0 );
        dsp_filter(FLP1,  15000, 1.0,   1.0 );

    dsp_CORE();
    dsp_TPDF_CALC(23);

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterHeadphones);
    dsp_SAT0DB_TPDF();
    dsp_STORE(DACOUT(0));

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterHeadphones);
    dsp_SAT0DB_TPDF();
    dsp_STORE(DACOUT(1));

    dsp_CORE(); // left channel

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterLow);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(2));

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterMid);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(3));

    dsp_LOAD_GAIN_Fixed(DACIN(0), 1.0);
    dsp_BIQUADS(filterHigh);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(4));

    dsp_CORE(); //right

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterLow);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(5));

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterMid);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(6));

    dsp_LOAD_GAIN_Fixed(DACIN(1), 1.0);
    dsp_BIQUADS(filterHigh);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(100);
    dsp_STORE(DACOUT(7));

}


int dspProg(int argc,char **argv){

    dspcodes();

    return dsp_END_OF_CODE();

}

