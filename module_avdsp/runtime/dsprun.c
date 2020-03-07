
/*
 * this program is for testing the dsp_engine
 * can be compiled directly on any platfform with gcc (install mingw64 for windows) with :
 * gcc -o testdsp testdsp.c dsp_engine.c
 */
#include <stdio.h>      // import printf functions
#include <string.h>
#include <stdlib.h>
#include <sndfile.h>
#include "alsa.h"

#include "dsp_fileaccess.h" // for loading the opcodes in memory

#define DSP_FORMAT DSP_FORMAT_INT64

#include "dsp_runtime.h"

#define opcodesMax 10000
#define inputOutputMax 32

static dspSample_t inputOutput[inputOutputMax];         // table containing the input an output samples treated by the dsp_engine
static opcode_t opcodes[opcodesMax];           		// table for dsp code

static void usage() {
	fprintf(stderr,"dsprun alsainname alsaoutname nbchin nbchout dspprog.bin fs\n");
	fprintf(stderr,"or : \ndsprun -i impulsefilename nbchin nbchout dspprog.bin fs\n");
	exit(-1);
}

int main(int argc, char **argv) {

    char* dspfilename;
    char* alsainname,*alsaoutname;
    char* impulsename = NULL ;
    int nbchin, nbchout;
    int fs;

    int size,result;
    int nbcores;
    int *dataPtr;
    opcode_t *codeStart[4];

    int nc,n,ch;

    // parse and check args 
    if(argc<7) usage();

    if(strcmp(argv[1],"-i") == 0 )  {
	impulsename = argv[2];
    } else {
    	alsainname=argv[1];
    	alsaoutname=argv[2];
    }
    nbchin=atoi(argv[3]); if(nbchin <=0) usage();
    nbchout=atoi(argv[4]); if(nbchin <=0) usage();
    dspfilename=argv[5];
    fs=atoi(argv[6]); if(fs<=0) usage();

    // load dsp prog
    size = dspReadBuffer(dspfilename, (int*)opcodes, opcodesMax);
    if (size < 0) {
        dspprintf("FATAL ERROR trying to load opcode.\n");
        exit(-1);
    }

    // verify frequency compatibility with header, and at least 1 core is defined, and checksum ok
    result = dspRuntimeInit(opcodes, size, fs);
    if (result < 0) {
        dspprintf("FATAL ERROR: problem with opcode header or compatibility\n");
        exit(-1);
    }
    // runing data area just after the program code
    dataPtr = (int*)opcodes + result;

    // find cores
    for(nbcores=0;nbcores<4; nbcores++) {
       codeStart[nbcores] = dspFindCore(opcodes, nbcores+1);
       if (codeStart[nbcores]==0)
	break;
    }

    if(impulsename) {
	 int n;
	 SNDFILE *outsnd;
	 SF_INFO infsnd;

         infsnd.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
         infsnd.samplerate = fs;
         infsnd.channels = nbchout;

         outsnd= sf_open(impulsename, SFM_WRITE, &infsnd);
         if (outsnd == NULL) {
               	fprintf(stderr, "could not open %s\n ",impulsename);
               	exit(1);
         }

	// impulse
        for(ch=0;ch<nbchin;ch++) {
    		inputOutput[ch] = INT32_MAX/2;
	}
	
	for(nc=0;nc<nbcores;nc++) 
    		DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], dataPtr, inputOutput); 

    	sf_write_int(outsnd,&(inputOutput[8]),nbchout);

	// then fs/2 0 samples
        for(ch=0;ch<nbchin;ch++)
    		inputOutput[ch] = 0;
	
	for(n=0;n<fs/2;n++) {
		for(nc=0;nc<nbcores;nc++) 
    			DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], dataPtr, inputOutput); 

    		sf_write_int(outsnd,&(inputOutput[8]),nbchout);
	}
	sf_close(outsnd);

    } else {
	int *inbuffer,*outbuffer;

    	if(initAlsaIO(alsainname, nbchin, alsaoutname, nbchout, fs)) {
        	dspprintf("Alsa init error\n");
        	exit(-1);
    	}

	inbuffer=(int*)malloc(sizeof(int)*nbchout*8192);
	outbuffer=(int*)malloc(sizeof(int)*nbchout*8192);

    	// infinite loop
    	while(1) {
		int sz;

		sz=(int)readAlsa(inbuffer , 8192) ;
		if(sz<0) break;

		for (n=0;n < sz ; n++) {

        		// input 0-7 
        		for(ch=0;ch<nbchin;ch++)
    				inputOutput[ch] = inbuffer[n*nbchin+ch];
		
			for(nc=0;nc<nbcores;nc++) 
    				DSP_RUNTIME_FORMAT(dspRuntime)(codeStart[nc], dataPtr, inputOutput); 

        		// outputs 8-15
        		for(ch=0;ch<nbchout;ch++)
    				outbuffer[n*nbchout+ch]=inputOutput[ch+8] ;
		}

		writeAlsa(outbuffer , sz) ;
    	}
    }

    return 0;
}

