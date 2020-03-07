
INCLUDES = -I../runtime -I../encoder
CFLAGS = -Ofast   -Wall $(INCLUDES)
LIBS = -lm -lasound -lsndfile
OBJS = ../runtime/alsa.o  ../runtime/dsp_biquadSTD.o  ../runtime/dsp_firSTD.o  ../runtime/dsprun.o  ../runtime/dsp_runtime.o ../encoder/dsp_fileaccess.o

dsprun:	 $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

clean:
	@rm -f $(OBJS) dsprun
