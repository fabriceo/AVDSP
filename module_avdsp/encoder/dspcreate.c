
/*
 * this program is for testing the encode with a user dsp program
 * can be compiled directly on any platfform with gcc (install mingw64 for windows)
 * launch make -f dspcreate.mak all
 *
 */

#include "dsp_encoder.h"
#include <string.h>
#include <stdlib.h>

#define opcodesMax 10000                 // just to define a maximum, could be up to 65000 (=256kbytes)

#define inputOutputMax 32               // size of the future table containg samples of each IO accessible by LOAD & STORE

#define myDspMin    F44100              // minimum frequency that is supported by the dsp runtime
#define myDspMax    F192000             // same with max. biquad coef and delay lines are all generated now accordingly.

#include "../dspprogs/crossover2x2lfe.h"


#define DACOUT(x) (x)
#define ADCIN(x)  (8  + x )
#define USBOUT(x) (16 + x)
#define USBIN(x)  (16 + 8 + x)

int oktoProg(){
    dsp_PARAM();

    int lowpass = dspBiquad_Sections(3);
        dsp_LP_BES6(fcross);
        //dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        //dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
    int highpass = dspBiquad_Sections(2);
        dsp_HP_LR4(fcross);
        //dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);
        //dsp_Filter2ndOrder(FPEAK,1000, 0.5, 1.0);

    dsp_CORE();  // first core

    dsp_LOAD(USBOUT(0));
    dsp_COPYXY();
    dsp_DELAY_FixedMicroSec(subdelay);
    dsp_SWAPXY();   // move delayed signa in Y and restore original sample
    dsp_BIQUADS(lowpass);
    dsp_STORE(USBIN(0));
    dsp_SWAPXY();
    dsp_SUBXY();            // 12db/oct High pass, with LP BES6
    dsp_STORE(USBIN(2));


    dsp_LOAD(USBOUT(1));
    dsp_STORE(USBIN(1));

    return dsp_END_OF_CODE();
}

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


int dspProgDACDXIO(){
    return oktoProg();
}

int dspProgDAC8PRO(){
    return oktoProg();
}

int dspProgDACSTEREO(){
    return oktoProg();
}

char * hexBegin = "const unsigned int dspFactory[] = {";
char * hexEnd = "};";


int main(int argc, char **argv) {
    char * perr = NULL;
    char * outFileName  = "";
    char * dumpFileName = "";
    int outFileType = 0;
    int defaultType = DSP_FORMAT_INT64;
    printf("command line options:\n");
    printf("-dspformat <2..6> -fx <crossover_frequency> -dist <distance mm>\n");
    printf("-binfile <filename> -hexfile <filename> -dumpfile <filename>\n");

    int progNum = 0;
    printf("-okto <x>\n");

    for (int i=1; i<argc; i++) {
        // basic parameters handling for file outputs and encoder format
        if (strcmp(argv[i],"-binfile") == 0) {
                i++;
                if (argc>i) {
                    outFileName = argv[i];
                    outFileType |= 1;
                    continue; } }
            if (strcmp(argv[i],"-hexfile") == 0) {
                i++;
                if (argc>i) {
                    outFileName = argv[i];
                    outFileType |= 2;
                    continue; } }
            if (strcmp(argv[i],"-asmfile") == 0) {
                i++;
                if (argc>i) {
                    outFileName = argv[i];
                    outFileType |= 4;
                    continue; } }
            if (strcmp(argv[i],"-dumpfile") == 0) {
                i++;
                if (argc>i) {
                    dumpFileName = argv[i];
                    outFileType |= 8;
                    dumpFileInit(dumpFileName);
                    continue; } }

            if (strcmp(argv[i],"-dspformat") == 0) {
                i++;
                if (argc>i) {
                    defaultType = strtol(argv[i], &perr,10);
                    printf("encoder format = %d\n",defaultType);
                    continue; } }

// specific parameters for OKTO RESEARCH, to generate default preloaded programs
            if (strcmp(argv[i],"-okto") == 0) {
                i++;
                if (argc>i) {
                    int num = strtol(argv[i], &perr,10);
                    if (num == 1) {
                        printf("OKTO basic program for DAC8PRO\n");
                        outFileName = "dspdac8pro.h";
                        outFileType |= 2;
                        progNum = 1;
                    } else
                    if (num == 2) {
                        printf("OKTO basic program for DACSTEREO\n");
                        outFileName = "dspdacstereo.h";
                        outFileType |= 2;
                        progNum = 2;
                    } else
                    if (num == 3) {
                        printf("OKTO basic program for DXIO\n");
                        outFileName = "dspdacdxio.h";
                        outFileType |= 2;
                        progNum = 3;
                    }
                    continue; } }

// USER'S command line parameters starts here
                 if (strcmp(argv[i],"-fx") == 0) {
                     i++;
                     if (argc>i) {
                         fcross = strtol(argv[i], &perr,10);
                         printf("crossover fx = %d\n",fcross);
                         continue; } }

                 if (strcmp(argv[i],"-dist") == 0) {
                     i++;
                     if (argc>i) {
                         distance = strtol(argv[i], &perr,10);
                         printf("distance = %d\n",distance);
                         continue; } }

                 if (strcmp(argv[i],"-delay") == 0) {
                     i++;
                     if (argc>i) {
                         subdelay = strtol(argv[i], &perr,10);
                         printf("crossover delay = %d\n",subdelay);
                         continue; } }

    }

    opcode_t opcodes[opcodesMax];       // temporary table for dsp code
    const int max = opcodesMax;

    opcode_t * codePtr;
    if ((unsigned long long)&opcodes[0] & 4) codePtr = &opcodes[1]; // force align8
    else codePtr = &opcodes[0];

    dspEncoderInit( codePtr,            // table where we store the generated opcodes
                    max,                // max number of words in this table
                    defaultType,        // format of the dsp : int64, float or double (see runtime.h)
                    myDspMin, myDspMax, // list of frequencies treated by runtime. used to pre-generate biquad coef and delay lines
                    inputOutputMax);    // number of I/O that can be used in the Load & Store instruction (represent ADC + DAC)

    int size;
    switch(progNum) {
    case 0: size = dspProg(); break;
    case 1: size = dspProgDAC8PRO(); break;
    case 2: size = dspProgDACSTEREO(); break;
    case 3: size = dspProgDACDXIO(); break;
    }

    if (size > 0) {
        dspprintf("DSP program file successfully generated \n");
        if (outFileType & 1) {
            dspCreateBuffer(outFileName, (int*)codePtr, size); // write bin file
        }
        if (outFileType & 2) {
            dspCreateIntFile(outFileName, (int*)codePtr, size,hexBegin,hexEnd);
        }
        if (outFileType & 4) {
            dspCreateAssemblyFile(outFileName, (int*)codePtr, size);
        }
        dspprintf("stored in-> %s\n",outFileName);

        if (outFileType & 8) dspprintf("Dump file -> %s\n",dumpFileName);
    }
    else
        dspprintf("... problem...\n");
    return 0;
}

