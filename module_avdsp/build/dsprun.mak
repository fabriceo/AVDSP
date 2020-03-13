
INCLUDES = -I../runtime -I../encoder
CFLAGS = -Ofast   -Wall $(INCLUDES)
LIBS = -lm -lasound -lsndfile
OBJS = ../runtime/dsp_biquadSTD.o  ../runtime/dsp_firSTD.o  ../runtime/dsp_runtime.o ../encoder/dsp_fileaccess.o ../linux/alsa.o  ../linux/dsprun.o 

dsprun:	 $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

clean:
	@rm -f $(OBJS) dsprun
