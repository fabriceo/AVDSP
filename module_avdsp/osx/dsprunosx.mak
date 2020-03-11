#make -f dsprunosx.mak all

all:
	gcc -g -o dsprunint    dsprunosx.c ../runtime/*.c ../encoder/dsp_fileaccess.c -I. -I../runtime/ -I../encoder/ -DDSP_PRINTF=2 -DDSP_FORMAT=2 -DDSP_SINGLE_CORE=1 #see dsp_header.h 
	#gcc -g -o dsprunfloat  dsprunosx.c ../runtime/dsp_runtime.c dsp_fileaccess.c -I. -I../runtime/ -DDSP_PRINTF=2 -DDSP_FORMAT=3 #see dsp_header.h 
	#gcc -g -o dsprundouble dsprunosx.c ../runtime/dsp_runtime.c dsp_fileaccess.c -I. -I../runtime/ -DDSP_PRINTF=2 -DDSP_FORMAT=4 #see dsp_header.h 