INCLUDES = -I../encoder -I../runtime
CFLAGS = -DDSP_PRINTF=3 -Ofast  -fPIC  -Wall $(INCLUDES)
LIBS = -lm -ldl
LIBS2 = $(LIBS) -L../encoder -lavdspencoder
OBJS = ../encoder/dspcreate.o ../encoder/dsp_encoder.o ../encoder/dsp_filters.o ../encoder/dsp_HilbertDesign.o ../encoder/dsp_fileaccess.o ../encoder/dsp_nanosharcxml.o

AT = 

all:	libencoder libdspprogs libcopy dspcreate

libencoder:
	$(AT)cd ../encoder; make

libdspprogs:
	$(AT)cd ../dspprogs; make

libcopy:
	@cp -f ../encoder/*.dylib ../osx
	@cp -f ../dspprogs/*.dylib ../osx

dspcreate:	 $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS2)
	@rm -f -r *.dSYM
	
clean:
	@rm -f $(OBJS) dspcreate *.dylib
	@cd ../dspprogs ; make clean
	@cd ../encoder ; make clean
