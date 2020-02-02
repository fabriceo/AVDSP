#make -f dsprun.mak all

all:
	gcc -g -o dsprunint    ../runtime/dsprun.c ../runtime/dsp_runtime.c ../encoder/dsp_fileaccess.c -I../runtime -I../encoder -DDSP_PRINTF=2 -DDSP_FORMAT=2 #see dsp_header.h 
	gcc -g -o dsprunfloat  ../runtime/dsprun.c ../runtime/dsp_runtime.c ../encoder/dsp_fileaccess.c -I../runtime -I../encoder -DDSP_PRINTF=2 -DDSP_FORMAT=3 #see dsp_header.h 
	gcc -g -o dsprundouble ../runtime/dsprun.c ../runtime/dsp_runtime.c ../encoder/dsp_fileaccess.c -I../runtime -I../encoder -DDSP_PRINTF=2 -DDSP_FORMAT=4 #see dsp_header.h 