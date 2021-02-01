
INCLUDES = -I. -I../encoder -I../runtime -I../dspprogs
LIBS     = -L. -lavdspencoder
CFLAGS   = -DOSX -DDSP_PRINTF=1 -Ofast -fPIC -Wall $(INCLUDES)

#define your binary dsp code here, and for each, add a section below as per example

DSPPROGS = dac8prodsp.h dacfabriceo.bin LXmini.bin LXminisub.bin LXminisubmono.bin LXminilv6.bin mydspcode.bin

all:	$(DSPPROGS)

# this code is used to generate the main encoder library
libavdspencoder.dylib:	../encoder/dsp_*.c
	$(CC) $(CFLAGS) -dynamiclib -o libavdspencoder.dylib ../encoder/dsp_*.c
	@rm -rf *.dSYM
	@rm -R *.dSYM

#this code is used to generate the dspcreate executable
dspcreate:	libavdspencoder.dylib ../encoder/dspcreate.c
	$(CC) $(CFLAGS) -o dspcreate $(LIBS) ../encoder/dspcreate.c

#code for the library containing fabriceo dsp program
oktodac_fabriceo.dylib:	../dspprogs/oktodac_fabriceo.c libavdspencoder.dylib
	$(CC) $(CFLAGS) -dynamiclib ../dspprogs/oktodac_fabriceo.c -o oktodac_fabriceo.dylib $(LIBS)

dacfabriceo.bin: oktodac_fabriceo.dylib dspcreate
	./dspcreate -dspprog oktodac_fabriceo.dylib -binfile dacfabriceo.bin -dspformat 2 -fsmax 96000 -fx 800 -dither 23 -microslow 740 -gcomp 0.35

#code for the library containing factory dsp programs
oktodac.dylib:	../dspprogs/oktodac.c libavdspencoder.dylib
	$(CC) $(CFLAGS) -dynamiclib ../dspprogs/oktodac.c -o oktodac.dylib $(LIBS)

dac8prodsp.h: oktodac.dylib dspcreate
	./dspcreate -dspprog oktodac.dylib -hexfile dac8prodsp.h -dspformat 2  -dac8prodsp -dither 24
	

#code for the library containing LXmini crossover programs
oktodac_LX.dylib:	../dspprogs/oktodac_LX.c libavdspencoder.dylib
	$(CC) $(CFLAGS) -dynamiclib ../dspprogs/oktodac_LX.c -o oktodac_LX.dylib $(LIBS)

LXmini.bin: oktodac_LX.dylib dspcreate
	./dspcreate -dspprog oktodac_LX.dylib -binfile LXmini.bin -dspformat 2  -lxmini -dither 23
	
LXminisub.bin: oktodac_LX.dylib dspcreate
	./dspcreate -dspprog oktodac_LX.dylib -binfile LXminisub.bin -dspformat 2  -lxmini -sub 2 -dither 23
	
LXminisubmono.bin: oktodac_LX.dylib dspcreate
	./dspcreate -dspprog oktodac_LX.dylib -binfile LXminisubmono.bin -dspformat 2  -lxmini -sub 1 -dither 23
	
LXminilv6.bin: oktodac_LX.dylib dspcreate
	./dspcreate -dspprog oktodac_LX.dylib -binfile LXminilv6.bin -dspformat 2  -lxmini -lv6 -dither 23
	


#this code generates the library containing your dsp program
#
mydspprog.dylib:	mydspprog.c libavdspencoder.dylib
	$(CC) $(CFLAGS) -dynamiclib mydspprog.c -o mydspprog.dylib $(LIBS)

#
#list your binary(ies) dsp code here with dependency on the required dylib and dspcreate executable
#
mydspcode.bin:	mydspprog.dylib dspcreate
# put the code to generate the .bin files here
	./dspcreate -dspprog mydspprog.dylib -binfile mydspcode.bin -dspformat 2 -fs 1000

clean:
	@rm -rf *.dylib
	@rm -rf dspcreate
	@rm -rf *.bin
	