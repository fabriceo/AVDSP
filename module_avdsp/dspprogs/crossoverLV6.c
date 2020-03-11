#include "dsp_encoder.h"

static int fcross;  // default crossover frequency for the demo
static int subdelay;

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24



int dspProg_test(){
    dspprintf("test program for okto dac\n");

    dsp_PARAM();

    int lowpass = dspBiquad_Sections(3);
        dsp_LP_BES6(1000);

    dsp_CORE();  // first core
    dsp_TPDF(24); // tested OK
    dsp_LOAD(USBOUT(1));    // loop back for minimum delay time reference
    dsp_STORE(USBIN(1));

    dsp_CORE();  // second core
    dsp_LOAD(USBOUT(0)); // tested OK
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(750);
    dsp_GAIN_Fixed(1.0);
    dsp_SWAPXY();
    dsp_GAIN_Fixed(1.0);
    dsp_BIQUADS(lowpass);   //compute lowpass filter
    dsp_SUBYX();
    dsp_SAT0DB_TPDF(); // tested OK
    dsp_STORE( USBIN(0) );
    dsp_SWAPXY();
    dsp_SAT0DB_TPDF(); // tested OK
    dsp_STORE( USBIN(2) );


    return dsp_END_OF_CODE();
}


