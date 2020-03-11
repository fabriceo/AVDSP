INCLUDES = -I../encoder -I../runtime -L../encoder
CFLAGS = -DDSP_PRINTF=3 -Ofast  -fPIC  -Wall $(INCLUDES)
LIBS = -lm -ldl -lavdspencoder
OBJS = ../encoder/dspcreate.o

all:	avdspencoder dspprogs dspcreate

avdspencoder:
	@cd ../encoder; make

dspprogs:
	@cd ../dspprogs; make

dspcreate:	 $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	@rm -f $(OBJS) dspcreate
	@cd ../dspprogs ; make clean
	@cd ../encoder ; make clean
