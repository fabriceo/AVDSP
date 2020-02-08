
/*
 * this program is for testing the encode with a user dsp program
 * can be compiled directly on any platfform with gcc (install mingw64 for windows)
 * launch make -f dspcreate.mak all
 *
 */

#include "dsp_encoder.h"
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>


#define opcodesMax 10000                 // just to define a maximum, could be up to 65000 (=256kbytes)

#define inputOutputMax 32               // size of the future table containg samples of each IO accessible by LOAD & STORE

#define myDspMin    F44100              // minimum frequency that is supported by the dsp runtime
#define myDspMax    F192000             // same with max. biquad coef and delay lines are all generated now accordingly.

const char * hexBegin = "const unsigned int dspFactory[] = {";
const char * hexEnd = "};";

void usage() {
    fprintf(stderr,"command line options:\n");
    fprintf(stderr,"-dspprog name \n");
    fprintf(stderr,"-dspformat <2..6> \n");
    fprintf(stderr,"-binfile <filename> -hexfile <filename> -dumpfile <filename>\n");
    fprintf(stderr,"[dsprogs paramaters ...] \n");
}

int main(int argc, char **argv) {
    char *perr;
    char *outFileName = NULL;
    char *dumpFileName = NULL;
    char *dspProgName = NULL;
    int outFileType = 0;
    int defaultType = DSP_FORMAT_INT64;
    int i,size;

    opcode_t opcodes[opcodesMax];       // temporary table for dsp code
    const int max = opcodesMax;

    int (*dspProg)(int argc, char **argv);
    void *dspproglib;

    for (i=1; i<argc; i++) {
        // basic parameters handling for file outputs and encoder format
        if (strcmp(argv[i],"-dspprog") == 0) {
                i++;
                if (argc>i) {
			dspProgName = argv[i];
			continue; } }
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
                    continue; } }
	    break;
    }

    if( dumpFileName == NULL) {
    	fprintf(stderr,"-dumpfile name Needed\n\n");
	usage();
	exit(-1);
    }

    if(dspProgName == NULL) {
    	fprintf(stderr,"-dspprog name Needed\n\n");
	usage();
	exit(-1);
    }

    if (!( dspproglib = dlopen (dspProgName, RTLD_LAZY))) {
    	fprintf(stderr,"Could not load %s\n",dspProgName);
     usage();
     exit (-1);
    }
    if (!(dspProg = dlsym (dspproglib, "dspProg"))) {
     fprintf(stderr,"%s linking problem\n",dspProgName);
     dlclose (dspproglib);
     exit (-1);
    }

    dspEncoderInit( opcodes,            // table where we store the generated opcodes
                    max,                // max number of words in this table
                    defaultType,        // format of the dsp : int64, float or double (see runtime.h)
                    myDspMin, myDspMax, // list of frequencies treated by runtime. used to pre-generate biquad coef and delay lines
                    inputOutputMax);    // number of I/O that can be used in the Load & Store instruction (represent ADC + DAC)

    size = dspProg(argc-i,&argv[i]); 

    if (size > 0) {
        dspprintf("DSP program file successfully generated \n");
        if (outFileType & 1) {
            dspCreateBuffer(outFileName, (int*)opcodes, size); // write bin file
        }
        if (outFileType & 2) {
            dspCreateIntFile(outFileName, (int*)opcodes, size,hexBegin,hexEnd);
        }
        if (outFileType & 4) {
            dspCreateAssemblyFile(outFileName, (int*)opcodes, size);
        }
        dspprintf("stored in-> %s\n",outFileName);

        if (outFileType & 8) dspprintf("Dump file -> %s\n",dumpFileName);
    } else {
        dspprintf("... problem...\n");
    	return -1;
    }
    return 0;
}

