
/*
 * this program is for testing the dsp_engine
 * can be compiled directly on any platfform with gcc (install mingw64 for windows) with :
 * gcc -o testdsp testdsp.c dsp_engine.c
 */
#include <stdio.h>      // import printf functions

#include "dsp_runtime.h"

#define opcodesMax 10000
#define inputOutputMax 32

dspSample_t inputOutput[inputOutputMax];        // table containing the input an output samples treated by the dsp_engine

#include "dsp_fileaccess.h" // for loading the opcodes in memory
#include <string.h>
#include <stdlib.h>

// print the fatal error message and quit encoder
void dspFatalError(char *msg) {
    dspprintf("FATAL ERROR: %s\n",msg);
    exit(1);    // from stdlib.h
}

// special macro for s.31 format including saturation and special treatment for +1.0 recognition (nomally +1.0 is not possible!)
#define DSP_Q31_MAX     (0x000000007fffffffULL)
#define DSP_Q31_MIN     (0xFFFFFFFF80000000ULL)
#define DSP_2P31F       (2147483648.0)
#define DSP_2P31F_INV   (1.0/DSP_2P31F)
#define DSP_F31(x)      ( (x == DSP_Q31_MAX) ? 1.0 : (double)(x)/DSP_2P31F )


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

    if (size < 0) 
        dspFatalError("Cant load opcode file.");
 
    // verify frequency compatibility with header, and at least 1 core is defined, and checksum ok
    int result = dspRuntimeInit(codePtr, 96000, opcodesMax, 0, 23);

    if (result < 0) 
        dspFatalError("Problem with opcode header or compatibility.");
    
    int * dataPtr = (int*)codePtr + result;           // runing data area just after the program code

    int nbcores;
    opcode_t *codeStart[8];
    opcode_t * corePtr;
    for(nbcores=0;nbcores<8; nbcores++) {
            corePtr = dspFindCore(codePtr, nbcores+1);  // find the DSP_CORE instruction
            corePtr = dspFindCoreBegin(corePtr);    // skip any useless opcode to reach the real core begin
            codeStart[nbcores] = corePtr;
            if (corePtr == 0) break;
    }

#if DSP_SAMPLE_FLOAT
    inputOutput[8] = -0.3;
    printf("io(8) = %f\n",inputOutput[8]);
    inputOutput[9] = 0.5;
    printf("io(9) = %f\n",inputOutput[9]);
#else
    inputOutput[8] = DSP_QM32(-0.3,31);
    printf("io(8) = 0x%X\n",inputOutput[8]);
    inputOutput[9] = DSP_QM32(0.5,31);
    printf("io(9) = 0x%X\n",inputOutput[9]);
#endif

    for(int nc=0; nc<nbcores; nc++) 
        DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], inputOutput, 0);    // simulate 1 fs cycle in the first task

#if DSP_SAMPLE_FLOAT
    dspprintf("io(10) = %f\n",inputOutput[10]);
#else
    dspprintf("io(10) = %f\n",DSP_F31(inputOutput[10]));
#endif
    return 0;
}
