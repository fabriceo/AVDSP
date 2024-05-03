
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

extern int minidspCreateParameters(char * xmlName);
extern int dspbasicCreate(char * fileName, int argc, char ** argv);

#define opcodesMax 10000                 // just to define a maximum, could be up to 65000 (=256kbytes)

#define inputOutputMax 32               // size of the future table containg samples of each IO accessible by LOAD & STORE

int freqMin = DSP_DEFAULT_MIN_FREQ;     // default value from header.h
int freqMax = DSP_DEFAULT_MAX_FREQ;

const char * hexBegin =
        "#define DSP_CODESIZE %d\n"
        "#define DSP_DATASIZE %d\n"
        "#define DSP_NUMCORES %d\n"
        "const unsigned int __attribute__((aligned(8))) dspCodeArray[ DSP_CODESIZE ] = {\n";
const char * hexEnd =
        "};\n"
        " unsigned int __attribute__((aligned(8))) dspDataArray[ DSP_DATASIZE ];\n\t";

void usage() {
    fprintf(stderr,"command line options:\n");
    fprintf(stderr,"-dspprog   <libfilename> \n");
    fprintf(stderr,"-dsptext   <textfilename> \n");
    fprintf(stderr,"-dspformat <2..6> \n");
    fprintf(stderr,"-binfile   <filename> -hexfile <filename> -dumpfile <filename>\n");
    fprintf(stderr,"[dsprogs paramaters ...] \n");
}

int main(int argc, char **argv) {
    char *perr;
    char * binFileName  = "dspcreate.bin";
    char * hexFileName  = "dspcreate.hex";
    char * asmFileName  = "dspcreate.asm";

//    char *outFileName = NULL;
    char *dumpFileName = NULL;
    char *dspProgName = NULL;
    char *minidspProgName = NULL;
    char *dspbasicProgName = NULL;
    int outFileType = 0;
    int defaultType = DSP_FORMAT_FLOAT;
    int i,size;

    opcode_t opcodes[opcodesMax];       // temporary table for dsp code
    const int max = opcodesMax;

    int (*dspProg)(int argc, char **argv);
    void *dspproglib;

    for (i=1; i<argc; i++) {
        // basic parameters handling for file outputs and encoder format
        if (strcmp(argv[i],"-minidsp") == 0) {
                i++;
                if (argc>i) {
                     minidspProgName = argv[i];
                     continue; } }

        if (strcmp(argv[i],"-dsptext") == 0) {
                i++;
                if (argc>i) {
                     dspbasicProgName = argv[i];
                     continue; } }

        if (strcmp(argv[i],"-dspprog") == 0) {
                i++;
                if (argc>i) {
			         dspProgName = argv[i];
			         continue; } }
        if (strcmp(argv[i],"-binfile") == 0) {
                i++;
                if (argc>i) {
                    binFileName = argv[i];
                    outFileType |= 1;
                    continue; } }
        if (strcmp(argv[i],"-hexfile") == 0) {
            i++;
            if (argc>i) {
                hexFileName = argv[i];
                outFileType |= 2;
                continue; } }
        if (strcmp(argv[i],"-asmfile") == 0) {
            i++;
            if (argc>i) {
                asmFileName = argv[i];
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
        if (strcmp(argv[i],"-fsmin") == 0) {
            i++;
            if (argc>i) {
                int f = strtol(argv[i], &perr,10);
                int res = dspConvertFrequencyToIndex(f);
                if (res >= FMAXpos) {
                    fprintf(stderr,"Could not find this sampling rate %d\n",f);
                    exit(-1); }
                freqMin = res;
                continue; } }
        if (strcmp(argv[i],"-fsmax") == 0) {
            i++;
            if (argc>i) {
                int f = strtol(argv[i], &perr,10);
                int res = dspConvertFrequencyToIndex(f);
                if (res >= FMAXpos) {
                    fprintf(stderr,"Could not find this sampling rate %d\n",f);
                    exit(-1); }
                fprintf(stderr,"sampling rate max = %d (%d)\n",f,res);
                freqMax = res;
                continue; } }
	    break;
    }

    /*
        if( dumpFileName == NULL) {
            fprintf(stderr,"-dumpfile name Needed\n\n");
        usage();
        exit(-1);
        }
    */

    if (minidspProgName != NULL)    //experimental
        if (minidspCreateParameters(minidspProgName)) exit(-1);

    if (dspbasicProgName != NULL) {
        dspEncoderInit( opcodes,            // table where we store the generated opcodes
                        max,                // max number of words in this table
                        defaultType,        // format of the dsp : int64, float or double (see runtime.h)
                        freqMin, freqMax,   // list of frequencies treated by runtime. used to pre-generate biquad coef and delay lines
                        inputOutputMax);    // number of I/O that can be used in the Load & Store instruction (represent ADC + DAC)

        size = dspbasicCreate(dspbasicProgName, argc-i,&argv[i]);
        if (size <0) exit(size);
    } else {

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
                    freqMin, freqMax,   // list of frequencies treated by runtime. used to pre-generate biquad coef and delay lines
                    inputOutputMax);    // number of I/O that can be used in the Load & Store instruction (represent ADC + DAC)

	size = dspProg(argc-i,&argv[i]);     
    }

    if (size > 0) {
        dspprintf("DSP program file successfully generated \n");
        if (outFileType & 1) {
            dspCreateBuffer(binFileName, (int*)opcodes, size); // write bin file
            dspprintf("stored in-> %s\n",binFileName);
        }
        if (outFileType & 2) {
            dspHeader_t *head = (dspHeader_t*)opcodes;
            dspCreateIntFile(hexFileName, (int*)opcodes, size, head->dataSize, head->numCores, (char*)hexBegin, (char*)hexEnd);
            dspprintf("stored in-> %s\n",hexFileName);
        }
        if (outFileType & 4) {
            dspCreateAssemblyFile(asmFileName, (int*)opcodes, size);
            dspprintf("stored in-> %s\n",asmFileName);
        }

        if (outFileType & 8) dspprintf("Dump file -> %s\n",dumpFileName);
    } else {
        dspprintf("... problem...\n");
    	return -1;
    }
    return 0;
}

