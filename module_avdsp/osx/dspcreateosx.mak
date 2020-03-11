INCLUDES = -I../encoder -I../runtime -I../dspprogs ../dspprogs/_*.c
CFLAGS = -DDSP_PRINTF=3 -Ofast  -fPIC  -Wall $(INCLUDES)
LIBS = -lm -ldl
OBJS = ../encoder/dspcreate.o ../encoder/dsp_encoder.o ../encoder/dsp_filters.o ../encoder/dsp_fileaccess.o 

all:	dspcreate dspprogs

dspcreate:	 $(OBJS)
#	$(CC) $^ -Wl,--export-dynamic -o $@ $(LIBS)
	$(CC) $^ -rdynamic -o $@ $(LIBS)

dspprogs:
	@cd ../dspprogs; make

clean:
	@rm -f $(OBJS) dspcreate
	@cd ../dspprogs ; make clean
