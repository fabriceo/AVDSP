
/*
 * this program is for testing the encode with a user dsp program
 * can be compiled directly on any platfform with gcc (install mingw64 for windows)
 * launch make -f dspcreate.mak all
 *
 */

#include "dsp_encoder.h"

#define DACOUT(x) (x)
#define ADCIN(x)  (8  + x )
#define USBOUT(x) (16 + x)
#define USBIN(x)  (16 + 8 + x)

int oktoProg2(){
    dsp_PARAM();

    int lowpass = dspBiquad_Sections(2);
        dsp_LP_BES6(1000.0);


    dsp_CORE();  // first core

    dsp_LOAD(USBOUT(0));
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(200);
    dsp_SWAPXY();
    dsp_BIQUADS(lowpass);
    dsp_STORE(DACOUT(0));
    dsp_SWAPXY();
    dsp_SUBXY();
    dsp_STORE(DACOUT(1));

    dsp_LOAD(USBOUT(1));
    dsp_STORE(USBIN(1));

    dsp_LOAD(ADCIN(0));
    dsp_STORE(USBIN(0));

    dsp_LOAD(ADCIN(1));
    dsp_STORE(USBIN(2));
    return dsp_END_OF_CODE();
}


int dspProg(int argc,char **argv){
    return oktoProg2();
}

