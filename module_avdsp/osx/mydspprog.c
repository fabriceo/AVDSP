#include <stdlib.h>
#include <string.h>
#include "dsp_encoder.h"
// this file describe a dsp program to be used on the OKTO RESEARCH DAC8PRO

#define DACOUT(x) (x)                 // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + (x) )         // SPDIF receiver reception stored with an offset of 8
#define USBOUT(x) (16 + (x))          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + (x))      // the samples going to the USB Host are offseted by 24

// list of global variable

static int myDspProg(int dither) {

    // fixed parameter section
    dsp_PARAM();

    // params go here

    dsp_CORE();  // first core

    if (dither>=0) {
        printf("dithering enabled for %d usefull bits\n",dither);
        dsp_TPDF_CALC(dither); }

    // codes go here


    dsp_CORE();  // second core

    // codes go here

    return dsp_END_OF_CODE();
}


// entry point for the DSP program, called by dspcreate
int dspProg(int argc,char **argv){

    int dither = -1;    // no dithering by default

    //setSerialHash(0x9ADD2096);  // for serial number 0
    //setSerialHash(0xCAC47719);  // for serial number 16
    //setSerialHash(0x01C978EB);  // for serial number 64

    for(int i=0 ; i<argc;i++) {
        // parses USER'S command line parameters

        if (strcmp(argv[i],"-dither") == 0) {
            dither = 0;
             if (argc>=i) {
                 i++;
                 dither = strtol(argv[i], NULL, 10); }
            dspprintf("add dithering %d bits \n",dither);
            continue; }

        if (strcmp(argv[i],"-hash") == 0) {
             if (argc>=i) {
                 i++;
                 unsigned serialHash = strtol(argv[i], NULL, 16);
                 setSerialHash(serialHash); }
            continue; }

        // other params here

    }


    return myDspProg(dither);


}
