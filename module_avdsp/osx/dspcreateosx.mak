INCLUDES = -I../encoder -I../runtime
CFLAGS = -DDSP_PRINTF=3 -Ofast  -fPIC  -Wall $(INCLUDES)
LIBS = -lm -ldl
LIB2 = -L../encoder -lavdspencoder
OBJS2 = ../encoder/dsp_encoder.o ../encoder/dsp_filters.c ../encoder/dsp_fileaccess.o
OBJS1 = ../encoder/dspcreate.o

all:	avdspencoder dspprogs dspcreate

avdspencoder:
	@cd ../encoder; make

dspprogs:
	@cd ../dspprogs; make

dspcreate:	 $(OBJS1)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS) $(LIB2)

clean:
	@rm -f $(OBJS) dspcreate
	@cd ../dspprogs ; make clean
	@cd ../encoder ; make clean
