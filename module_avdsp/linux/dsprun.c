
/*
 * this program is for testing the dsp_engine
 * can be compiled directly on any platfform with gcc (install mingw64 for windows) with :
 * gcc -o testdsp testdsp.c dsp_engine.c
 */
#include <stdio.h>      // import printf functions
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sndfile.h>
#include <math.h>

#include "dsp_fileaccess.h" // for loading the opcodes in memory
#include "dsp_runtime.h"

#define opcodesMax 5000
#define nbCoreMax 8

// ouput from 0 to 7, input from 8 to 15
#define inputOutputMax 16
#define OUTOFFSET 0
#define INOFFSET 8

static void usage() {
	fprintf(stderr,"or : \ndsprun -i|-r|-s outwavefile  dspprog.bin fs\n");
	exit(-1);
}

typedef struct {
    int		nbchin, nbchout;
    int 	inputMap[inputOutputMax];
    int 	outputMap[inputOutputMax];
} coreio_t;

int main(int argc, char **argv) {

    char* dspfilename;
    char* alsainname,*alsaoutname;
    char* filename = NULL ;
    int fs;

    int nbcores;
    coreio_t coreio[nbCoreMax];
    int maxnbchin,maxnbchout;

    dspSample_t inputOutput[inputOutputMax];

    opcode_t opcodes[opcodesMax];
    opcode_t *codeStart[nbCoreMax];

    int *dataPtr;

    int size,result;
    int nc,n,ch,o;
    int inmode=0;

    SNDFILE *outsnd;
    SF_INFO infsnd;
    dspSample_t *Outputs;

    // parse and check args 
    if(argc<5) usage();

    if(strcmp(argv[1],"-i") == 0 )  {
	filename = argv[2];
	inmode=1;
    } else 
      if(strcmp(argv[1],"-s") == 0 )  {
	filename = argv[2];
	inmode=2;
    } else 
      if(strcmp(argv[1],"-r") == 0 )  {
	filename = argv[2];
	inmode=3;
      } else {
	fprintf(stderr,"error nedd -i | -r | -s\n");
	exit(-1);
      }

    dspfilename=argv[3];
    fs=atoi(argv[4]); if(fs<=0) usage();

    // load dsp prog
    size = dspReadBuffer(dspfilename, (int*)opcodes, opcodesMax);
    if (size < 0) {
        dspprintf("FATAL ERROR trying to load opcode.\n");
        exit(-1);
    }

    // verify frequency compatibility with header, and at least 1 core is defined, and checksum ok
    result = dspRuntimeInit(opcodes, size, fs, 0, 31);
    if (result < 0) {
        dspprintf("FATAL ERROR: problem with opcode header or compatibility\n");
        exit(-1);
    }
    // runing data area just after the program code
    dataPtr = (int*)opcodes + result;

    // find cores and compute iomaps
    maxnbchin=0,maxnbchout=0;
    for(nbcores=0;nbcores<nbCoreMax; nbcores++) {

    	   opcode_t *corePtr = dspFindCore(opcodes, nbcores+1);  // find the DSP_CORE instruction
           if (corePtr) {
	       unsigned int usedInputs,usedOutputs;
               int * IOPtr = (int *)corePtr+1;             // point on DSP_CORE parameters

               usedInputs  = *IOPtr++;
               usedOutputs = *IOPtr;

               corePtr = dspFindCoreBegin(corePtr);    // skip any useless opcode to reach the real core begin

    		// compute nbchin / nbchout and input/output map by core
		coreio[nbcores].nbchin=coreio[nbcores].nbchout=0;
    		for(ch=0;ch<inputOutputMax;ch++) {
			if(usedInputs & (1<<ch)) {
				coreio[nbcores].inputMap[coreio[nbcores].nbchin]=ch;
				coreio[nbcores].nbchin++;
				if((ch-INOFFSET+1)>maxnbchin) maxnbchin=ch-INOFFSET+1;
			}
			if(usedOutputs & (1<<ch)) {
				coreio[nbcores].outputMap[coreio[nbcores].nbchout]=ch;
				coreio[nbcores].nbchout++;
				if((ch-OUTOFFSET+1)>maxnbchout) maxnbchout=ch-OUTOFFSET+1;
			}
    		}
    
            }
            codeStart[nbcores] = corePtr;
            if (corePtr == 0) break;
    }


         infsnd.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
         infsnd.samplerate = fs;
         infsnd.channels = maxnbchout;

         outsnd= sf_open(filename, SFM_WRITE, &infsnd);
         if (outsnd == NULL) {
               	fprintf(stderr, "could not open %s\n ",filename);
               	exit(1);
         }

	Outputs=malloc(fs*5*maxnbchout*sizeof(unsigned int));

	for(nc=0;nc<nbcores;nc++)  {

	   for(n=0;n<10;n++) {

        	for(ch=0;ch<coreio[nc].nbchin;ch++) {
    			inputOutput[coreio[nc].inputMap[ch]] = 0;
			switch(inmode) {
			case 1 :
				if(n==0) inputOutput[coreio[nc].inputMap[ch]] = INT32_MAX;
				break;
			case 2 :
    				inputOutput[coreio[nc].inputMap[ch]] = round((double)INT32_MAX*sin(2.0*M_PI*40.0*(double)n/(double)fs));
				break;
			case 3 :
    				inputOutput[coreio[nc].inputMap[ch]] = RAND_MAX/16-(int)(rand()/8);
				break;
			}
		}
	
    		DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], inputOutput, 0);

       		for(ch=0;ch<coreio[nc].nbchout;ch++) {
    			Outputs[n*maxnbchout+(coreio[nc].outputMap[ch]-OUTOFFSET)]=inputOutput[coreio[nc].outputMap[ch]];
		}

	    }
	}

	for(n=0;n<fs*5;n++) 
       		sf_write_int(outsnd,&(Outputs[n*maxnbchout]),maxnbchout);

	sf_close(outsnd);

  return 0;
}

