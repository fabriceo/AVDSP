#include "dsp_encoder.h"

static int fcross;  // default crossover frequency for the demo
static int subdelay;

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24



int dspProg_testallfunction(){
    dspprintf("test program \n");

    dsp_PARAM();

    int eq1 = dspBiquad_Sections(1);
        dsp_Filter2ndOrder(FLS2,1000, 0.8, 2.0);

    int lowpass1 = dspBiquad_Sections(1);
    dsp_LP_LR2(fcross);
    //dsp_Filter2ndOrder(FPEAK,1000, 1, 1.0);
    //dsp_Filter2ndOrder(FPEAK,1000, 1, 1.0);

    int lowpass2 = dspBiquad_Sections(2);
        dsp_LP_LR4(1000);

    int lowpass3 = dspBiquad_Sections(3);
        dsp_LP_BES6(1000);

    int highpass1 = dspBiquad_Sections(1);
        dsp_HP_LR2(fcross);

    int highpass2 = dspBiquad_Sections(2);
        dsp_HP_LR4(fcross);

    int highpass3 = dspBiquad_Sections(2);
        dsp_HP_LR4(fcross);

    int mux1 = dspLoadMux_Inputs(2);
        dspLoadMux_Data(USBOUT(0), 0.25);
        dspLoadMux_Data(USBOUT(0), 0.25);

    int sine192 = dspGeneratorSine(192);

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
    dsp_BIQUADS(lowpass3);   //compute lowpass filter
    dsp_SUBYX();
    //dsp_LOAD_MUX(mux1);   // tested OK
    //dsp_VALUE_Fixed(0.5); //tested OK
    //dsp_SHIFT(3); // tested OK
    //dsp_ADDXY();  // tested ok
    //dsp_MULXY();  // tested ok
    //dsp_GAIN_Fixed(1.0); // tested OK
    //dsp_LOAD_GAIN_Fixed( USBOUT(0), 1.0 ); // tested OK
    //dsp_COPYXY();
    //dsp_SWAPXY();
    //dsp_DELAY_DP_FixedMicroSec(750);
    dsp_SAT0DB_TPDF(); // tested OK
    dsp_STORE( USBIN(0) );
    dsp_SWAPXY();
    dsp_SAT0DB_TPDF(); // tested OK
    //dsp_SHIFT(-28); // tested OK
    //dsp_SAT0DB(); // tested OK
    //dsp_SAT0DB_GAIN_Fixed(2.0); // tested OK
    //dsp_SAT0DB_TPDF(); // tested OK
    //dsp_DATA_TABLE(sine192, 1.0, 2, 192); //tested ok
    //dsp_GAIN_Fixed(0.5); // tested OK
    //dsp_SAT0DB_TPDF_GAIN_Fixed(1.0 );  // tested OK
    dsp_STORE( USBIN(2) );

    dsp_CORE();  // third core
    dsp_LOAD_GAIN_Fixed( USBOUT(0), 1.0 );
    dsp_BIQUADS(lowpass3);   //compute lowpass filter
    dsp_SAT0DB_TPDF_GAIN_Fixed( 1.0 );
    dsp_STORE( USBIN(3) );


    return dsp_END_OF_CODE();
}

