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

int dspProg_test1(int dither){  // test noise, tpdf, white, dither, dither_ns2, distribution, dirac-bessel

    float noiseshaper[] = {
            2.51758, -2.01206, 0.57800,           //44.1
            2.56669, -2.04479, 0.57800,           //48
            2.75651, -2.50072, 0.77760,           //88.2
            2.76821, -2.51152, 0.77760,           //96
            2.78567, -2.58690, 0.80595,           //176
            2.78695, -2.59168, 0.80757  };        //192

    dsp_PARAM();
    int nscoefs = dspDataTableFloat(noiseshaper, 3*6);

    int lowpass1 = dspBiquad_Sections_Flexible();
        dsp_LP_BES4(1000);

    int lowpass2 = dspBiquad_Sections_Flexible();
        dsp_LP_BES4(1000);

    dsp_CORE();
    dsp_TPDF(dither);           // generate triangular noise for dithering, ALU X contains 1bit noise at "dither" position,
   // use either swap or sat0db here below for 2 different example:
    dsp_SAT0DB();               // reduce precision to int32 and provide the 1bit vaue at dither prosition
    //dsp_SWAPXY();             // ALU Y contains triangular noise at Full scale as int32
    dsp_STORE( USBIN(0) );      // result is -96dBFS rms flat noise curve with SAT0DB or -5.5dBFS rms with SWAPXY

    dsp_SWAPXY();               // ALU Y contains triangular noise at Full scale as int32
    dsp_DISTRIB(512);           // prepare a table of 512 value spreading the original -1..+1 noise around the 512 bins
    dsp_STORE( USBIN(2) );      // show perfect triangular noise distribution with REW Scope function

    dsp_LOAD( USBOUT(1) );       // loopback rew, no treatments
    dsp_STORE( USBIN(1) );

    dsp_GAIN_Fixed( 1.0 );      // apply a gain on the previous LOAD above to bring it to dual precision
    // try 3 combinations below, either clip(0.5) or clip(0.0) or clip(0.0) and swapxy togetehr
    dsp_CLIP_Fixed( 0.5 );      // clip the signal below -0.5 and above +0.5
    //dsp_CLIP_Fixed(0.0);        // special case to generate a synchronized dirac impulse overloading ALU X
    //dsp_SWAPXY();             // ALU Y contains a square wave between 0..+1.0 if using clip(0.0)
    dsp_BIQUADS(lowpass1);      // show filtered response of a clipped signal, or dirac impulse, or square
    dsp_SAT0DB();
    dsp_STORE( USBIN(3) );

    dsp_WHITE();                // provide white noise as an int32
    dsp_STORE( USBIN(4) );      // full white noise measured with REW -2.5dBFS rms

    dsp_DIRAC_Fixed( 100, 1.0 );// generate 100 pulse per seconds, max value is 0.8 in the -1..+1 range
    // try swapxy to replace dirac impulse by a square between -0.5 ... +0.5
    dsp_SWAPXY();             // ALU Y contains a square signal at 100hz then. too much energy with 1.0 for biquads!
    //dsp_BIQUADS(lowpass2);
    dsp_SAT0DB();
    dsp_STORE( USBIN(5) );      // show filter impulse response in REW with Scope

    dsp_LOAD_GAIN_Fixed( USBIN(1) , 1.0 );
    dsp_SAT0DB_TPDF();          // show effect of triangular dithering no noise shaping
    dsp_STORE( USBIN(6) );      // measuring -94dBFS with no signal for 16bits dither

    dsp_LOAD_GAIN_Fixed( USBIN(1) , 1.0);
    dsp_DITHER_NS2(nscoefs);    // noise shape the input signal with special coeficient declared above
    dsp_SAT0DB();
    dsp_STORE( USBIN(7) );      // see shaping curve with REW FFT: -85.5dBFS for dither 16bits, +3db curve@2800hz, +12db/octave

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
    }
    return dsp_END_OF_CODE();
}
