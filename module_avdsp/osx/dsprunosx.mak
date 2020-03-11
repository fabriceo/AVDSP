#make -f dsprunosx.mak all

all:
	gcc -g -o dsprunint dsprunosx.c ../runtime/dsp_*.c ../encoder/dsp_fileaccess.c -I../runtime -I../encoder -DDSP_FORMAT=2 #see dsp_header.h 
