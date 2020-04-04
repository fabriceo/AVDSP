
/*
 * this program is for testing the dsp_engine
 * can be compiled directly on any platfform with gcc (install mingw64 for windows) with :
 * gcc -o testdsp testdsp.c dsp_engine.c
 */
#include <stdio.h>      // import printf functions
#include <string.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include "alsa.h"

#include "dsp_fileaccess.h" // for loading the opcodes in memory

#include "dsp_runtime.h"

#define opcodesMax 10000
#define inputOutputMax 32
#define nbCoreMax 8

static void usage() {
	fprintf(stderr,"dsprun alsainname alsaoutname dspprog.bin fs\n");
	fprintf(stderr,"or : \ndsprun -i impulsefilename  dspprog.bin fs\n");
	exit(-1);
}

int main(int argc, char **argv) {

    char* dspfilename;
    char* alsainname,*alsaoutname;
    char* filename = NULL ;
    int fs;

    dspSample_t inputOutput[inputOutputMax];
    int		nbchin, nbchout;
    int 	inputMap[inputOutputMax];
    int 	outputMap[inputOutputMax];

    opcode_t opcodes[opcodesMax];
    int *dataPtr;
    dspHeader_t *hptr;

    opcode_t *codeStart[nbCoreMax];
    int nbcores;

    int size,result;
    int nc,n,ch,o;
    int impulse=0,sine=0;

    // parse and check args 
    if(argc<5) usage();

    if(strcmp(argv[1],"-i") == 0 )  {
	filename = argv[2];
	impulse=1;
    } else 
      if(strcmp(argv[1],"-s") == 0 )  {
	filename = argv[2];
	sine=1;
      } else {
    		alsainname=argv[1];
    		alsaoutname=argv[2];
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
    result = dspRuntimeInit(opcodes, size, fs, 0);	// 0 for random seed to be changed by a gettime type of function
    if (result < 0) {
        dspprintf("FATAL ERROR: problem with opcode header or compatibility\n");
        exit(-1);
    }
    // runing data area just after the program code
    dataPtr = (int*)opcodes + result;

    // find cores
    for(nbcores=0;nbcores<nbCoreMax; nbcores++) {
       codeStart[nbcores] = dspFindCore(opcodes, nbcores+1);
       if (codeStart[nbcores]==0)
		break;
    }

    // compute nbchin / nbchout and imput/output map
    nbchin=nbchout=0;
    hptr=(dspHeader_t*)opcodes;
    for(ch=0;ch<32;ch++) {
	if(hptr->usedInputs & (1<<ch)) {
		inputMap[nbchin]=ch;
		nbchin++;
	}
	if(hptr->usedOutputs & (1<<ch)) {
		outputMap[nbchout]=ch;
		nbchout++;
	}
    }
    
    if(filename) {
	 SNDFILE *outsnd;
	 SF_INFO infsnd;
    	 dspSample_t Outputs[inputOutputMax];

         infsnd.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
         infsnd.samplerate = fs;
         infsnd.channels = nbchout;

         outsnd= sf_open(filename, SFM_WRITE, &infsnd);
         if (outsnd == NULL) {
               	fprintf(stderr, "could not open %s\n ",filename);
               	exit(1);
         }

	for(n=0;n<fs*10;n++) {
        	for(ch=0;ch<nbchin;ch++) {
    			inputOutput[inputMap[ch]] = 0;
			if(impulse && n==0) 
    				inputOutput[inputMap[ch]] = INT32_MAX/2;
			if(sine) 
    				inputOutput[inputMap[ch]] = round((double)INT32_MAX/2.0*sin(2.0*M_PI*1000.0*(double)n/(double)fs));
		}
	
		for(nc=0;nc<nbcores;nc++) 
    			DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], dataPtr, inputOutput); 

       		for(ch=0;ch<nbchout;ch++) {
    			Outputs[ch]=inputOutput[outputMap[ch]];
		}

    		sf_write_int(outsnd,Outputs,nbchout);

	}

	sf_close(outsnd);

    } else {
	int *inbuffer,*outbuffer;

    	if(initAlsaIO(alsainname, nbchin, alsaoutname, nbchout, fs)) {
        	dspprintf("Alsa init error\n");
        	exit(-1);
    	}

	inbuffer=(int*)malloc(sizeof(int)*nbchin*8192);
	outbuffer=(int*)malloc(sizeof(int)*nbchout*8192);

    	// infinite loop
    	while(1) {
		int sz;

		sz=(int)readAlsa(inbuffer , 8192) ;
		if(sz<0) break;

		for (n=0;n < sz ; n++) {

        		// input 
        		for(ch=0;ch<nbchin;ch++) {
    				inputOutput[inputMap[ch]] = inbuffer[n*nbchin+ch];
			}
		
			for(nc=0;nc<nbcores;nc++) 
    				DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], dataPtr, inputOutput); 

        		// outputs 
        		for(ch=0;ch<nbchout;ch++) {
    				outbuffer[n*nbchout+ch]=inputOutput[outputMap[ch]] ;
			}

    		}

		writeAlsa(outbuffer , sz) ;
         }
  } 
  return 0;
}

