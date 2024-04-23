#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"


#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24


int dspProg_base(){

    dsp_CORE();

    dsp_LOAD( USBIN(0) );
    dsp_STORE( DACOUT(0) );
    dsp_STORE( USBIN(0) );

    dsp_LOAD( USBIN(1) );
    dsp_STORE( DACOUT(1) );
    dsp_STORE( USBIN(1) );

    return dsp_END_OF_CODE();
}
float noiseshaper[] = {
        2.51758, -2.01206, 0.57800,           //44.1
        2.56669, -2.04479, 0.57800,           //48
        2.75651, -2.50072, 0.77760,           //88.2
        2.76821, -2.51152, 0.77760,           //96
        2.78567, -2.58690, 0.80595,           //176
        2.78695, -2.59168, 0.80757  };        //192
float noiseshaper2[] = {
        1.93281,  -1.32009,   0.32468,
        1.87690,  -1.24188,   0.29376,
        2.27740,  -1.78748,   0.48375,
        2.26413,  -1.76302,   0.47216,
        2.59434,  -2.26443,   0.66580,
        2.64541,  -2.34913,   0.70107 };

int dspProg_test1(int dither){  // test noise, tpdf, white, dither, dither_ns2, distribution, dirac-bessel


    dsp_PARAM();
    int nscoefs = dspDataTableFloat(noiseshaper, 3*6);

    int lowpass1 = dspBiquad_Sections_Flexible();
        dsp_LP_BES2(100,1.0);

    int lowpass2 = dspBiquad_Sections_Flexible();
        dsp_LP_BES4(1000,1.0);

    dsp_CORE();
//    dsp_TPDF_CALC(dither);           // generate triangular noise for dithering, ALU X contains 1bit noise at "dither" position,

//    dsp_LOAD( USBOUT(0) );       // loopback rew, no treatments
//    dsp_DISTRIB(USBIN(0), 960);  // prepare a table of 960 value spreading the original -1..+1 noise around the 960 bins
                                  // and show perfect triangular noise distribution with REW Scope function

    dsp_LOAD( USBOUT(1) );       // loopback rew, no treatments
    dsp_STORE( USBIN(1) );
#if 0
    dsp_GAIN_Fixed( 1.0 );      // apply a gain on the previous LOAD above to bring it to dual precision
    dsp_CLIP_Fixed( 0.75 );     // clip the signal below -0.75 and above +0.75
    dsp_BIQUADS(lowpass1);      // show filtered response of a clipped signal, or squared
    dsp_SAT0DB();
    dsp_STORE( USBIN(3) );

    dsp_WHITE();                // provide white noise as an int32
    dsp_STORE( USBIN(4) );      // full white noise measured with REW -2.5dBFS rms

    dsp_SQUAREWAVE_Fixed( 100, 1.0 );// generate a 100hz square
    //dsp_DIRAC_Fixed( 100, 1.0 );    // generate 100 value per seconds of dirac impulse
    dsp_BIQUADS(lowpass2);
    dsp_SAT0DB();
    dsp_STORE( USBIN(5) );      // show filter impulse response in REW with Scope

    dsp_LOAD_GAIN_Fixed( USBOUT(1) , 1.0 );
    dsp_SAT0DB_TPDF();          // show effect of triangular dithering no noise shaping
    dsp_STORE( USBIN(6) );      // measuring -94dBFS with no signal for 16bits dither

    dsp_LOAD_GAIN_Fixed( USBOUT(1) , 1.0);
    dsp_DITHER_NS2(nscoefs);    // noise shape the input signal with special coeficient declared above
    dsp_SAT0DB();
    dsp_STORE( USBIN(7) );      // see shaping curve with REW FFT: -85.5dBFS for dither 16bits, +3db curve@2800hz, +12db/octave
#endif
    dsp_SINE_Fixed(1000, 0.5);
    //dsp_SAT0DB();
    //dsp_SAT0DB_TPDF();          // show effect of triangular dithering no noise shaping
    dsp_STORE( USBIN(7) );      // see shaping curve with REW FFT: -85.5dBFS for dither 16bits, +3db curve@2800hz, +12db/octave

    return dsp_END_OF_CODE();
}


int dspProg_testFloat(int dither){
    //to test float, use the "-testfloat option and configure the runtime in DSP_FORMAT 3 or 4

    dsp_PARAM();
    int nscoefs = dspDataTableFloat(noiseshaper2, 3*6);
    int lowpass1 = dspBiquad_Sections_Flexible();
        dsp_LP_BES2(1000,1.0);


    dsp_CORE();
    //dsp_TPDF_CALC(dither);

    dsp_LOAD( USBOUT(1) );       // loopback rew, no treatments
    dsp_STORE( USBIN(1) );

    //dsp_DIRAC_Fixed(100, 1.0);
    //dsp_LOAD( USBOUT(0) );
    //dsp_LOAD_GAIN_Fixed( USBOUT(0) , 1.0 );
    //dsp_CLIP_Fixed(0.25);
    //dsp_SAT0DB_TPDF();
    dsp_SINE_Fixed(750,0.95);
    dsp_STORE( USBIN(0) );
    return dsp_END_OF_CODE();

    dsp_TPDF(20);
    dsp_SAT0DB_TPDF();
    dsp_STORE( USBIN(2) );
    //dsp_CLIP_Fixed(0.5);
    //dsp_VALUE_Fixed(0.03125);
    //dsp_ADDXY();
    //dsp_BIQUADS(lowpass1);
    //dsp_SAT0DB_TPDF();
    //dsp_DITHER();
    //dsp_DITHER_NS2(nscoefs);
    //dsp_SAT0DB();
    //dsp_DCBLOCK(100);

    dsp_LOAD_STORE();
    dspLoadStore_Data( USBOUT(1), USBIN(1) );

    return dsp_END_OF_CODE();


}

int dspProg(int argc,char **argv){
   int prog = 0;
   int dither = 0;
    for(int i=0 ; i<argc;i++) {
        // parse USER'S command line parameters


                 if (strcmp(argv[i],"-test1") == 0) {
                    dspprintf("test program for dac8pro and rew\n");
                    prog = 1;

                    continue; }

                 if (strcmp(argv[i],"-testfloat") == 0) {
                    dspprintf("test program for dac8pro and rew\n");
                    prog = 2;

                    continue; }
                 if (strcmp(argv[i],"-dither") == 0) {
                     dither = 24;
                      if (argc>=i) {
                          i++;
                          dither = strtol(argv[i], NULL,10); }
                     dspprintf("add dithering %d\n",dither);
                     continue; }


    }

    switch (prog) {
    case 0:  return dspProg_base();
    case 1:  return dspProg_test1(dither);
    case 2:  return dspProg_testFloat(dither);
    }
    return dsp_END_OF_CODE();
}
