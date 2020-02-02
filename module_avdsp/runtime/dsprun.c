
/*
 * this program is for testing the dsp_engine
 * can be compiled directly on any platfform with gcc (install mingw64 for windows) with :
 * gcc -o testdsp testdsp.c dsp_engine.c
 */
#include <stdio.h>      // import printf functions

#ifndef DSP_FORMAT
#warning "DSP_FORMAT not defined : by default the format will be INT64"
#endif

#include "dsp_runtime.h"

#define opcodesMax 10000
#define inputOutputMax 32

dspSample_t inputOutput[inputOutputMax];        // table containing the input an output samples treated by the dsp_engine


#include "dsp_fileaccess.h" // for loading the opcodes in memory
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    char * fileName = "dspprogram.bin";

    printf("command line options:\n");
    printf("-inputfile <filename>\n");
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i],"-inputfile") == 0) {
            i++;
            if (argc>i) {
                fileName = argv[i];
                continue; } }
    }
    printf("input file = %s\n",fileName);

    opcode_t opcodes[opcodesMax];           // table for dsp code

    opcode_t * codePtr;
    if ((unsigned long long)&opcodes[0] & 4) codePtr = &opcodes[1];
    else codePtr = &opcodes[0];

    int size = dspReadBuffer(fileName, (int*)codePtr, opcodesMax);

    if (size < 0) {
        dspprintf("FATAL ERROR trying to load opcode.\n");
        exit(-1);
    }

    int result = dspRuntimeInit(codePtr, opcodesMax, 96000);   // verify frequency compatibility with header, and at least 1 core is defined, and checksum ok

    if (result < 0) {
        dspprintf("FATAL ERROR: problem with opcode header or compatibility\n");
        exit(-1);
    }
    int * dataPtr = (int*)codePtr + result;           // runing data area just after the program code

    opcode_t * codeStart1 = dspFindCore(codePtr, 1);             // search for code Start for core 1
    if (codeStart1)
        dspprintf("First core starts at : %d\n",(int)((int*)codeStart1-(int*)codePtr))
    else {
        dspprintf("FATAL ERROR : no core found in dsp program.\n");
        exit(-1);
    }

    opcode_t * codeStart2 = dspFindCore(codePtr, 2);
    if (codeStart2)
        dspprintf("Second core starts at : %d\n",(int)((int*)codeStart2-(int*)codePtr));

#if DSP_SAMPLE_FLOAT
    samples[8] = -0.3;
    printf("io(8) = %f\n",samples[8]);
    samples[9] = 0.5;
    printf("io(9) = %f\n",samples[9]);
#else
    inputOutput[8] = DSP_Q31(-0.3);
    printf("io(8) = 0x%X\n",inputOutput[8]);
    inputOutput[9] = DSP_Q31(0.5);
    printf("io(9) = 0x%X\n",inputOutput[9]);
#endif

    DSP_RUNTIME_FORMAT(dspRuntime)(codeStart1, dataPtr, &inputOutput[0]);    // simulate 1 fs cycle in the first task
    if (codeStart2) {
        dspprintf("next core\n");
        DSP_RUNTIME_FORMAT(dspRuntime)(codeStart2, dataPtr, &inputOutput[0]);    // simulate 1 fs cycle in the second task
    }

#if DSP_SAMPLE_FLOAT
    dspprintf("io(10) = %f\n",inputOutput[10]);
#else
    dspprintf("io(10) = %f\n",DSP_F31(inputOutput[10]));
#endif
    return 0;
}

