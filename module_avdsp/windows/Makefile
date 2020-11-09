
INCLUDES = -I. -I../encoder -I../runtime -I../dspprogs
LIBS     = -L. -lavdspencoder
CFLAGS   = -DWIN32 -DDSP_PRINTF=2 -Ofast -fPIC -Wall $(INCLUDES)

#define your binary dsp code here, and for each, add a section below as per example
DSPPROGS = mydspcode.bin

all:	$(DSPPROGS)

# this code is used to generate the main encoder library
libavdspencoder.dll:	../encoder/dsp_*.c
	gcc $(CFLAGS) -shared -o libavdspencoder.dll ../encoder/dsp_*.c

#this code is used to generate the dspcreate executable
dspcreate.exe:	libavdspencoder.dll ../encoder/dspcreate.c
	gcc $(CFLAGS) -o dspcreate $(LIBS) ../encoder/dspcreate.c dlfcn.c

#this code generate the library containing your dsp program
mydspprog.dll:	mydspprog.c libavdspencoder.dll
	gcc $(CFLAGS) -shared mydspprog.c -o mydspprog.dll $(LIBS)

#list your binary dsp code here with dependency on the required.dll and dspcreate executable
mydspcode.bin:	mydspprog.dll dspcreate.exe
# put the code to generate the .bin files here
	dspcreate -dspprog mydspprog.dll -binfile mydspcode.bin -dspformat 2 -dacfabriceo -fs 1000

