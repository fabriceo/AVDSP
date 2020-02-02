#make -f dspcreate.mak all

all:
	gcc -o dspcreate ../encoder/dspcreate.c ../dspprogs/crossover2x2lfe.c ../encoder/dsp_encoder.c ../encoder/dsp_filters.c ../encoder/dsp_fileaccess.c -I../encoder -I../runtime -DDSP_PRINTF=3 #see dsp_header.h