
INCLUDES = -I. -I../encoder -I../runtime
#LIBS     = -L. -lavdspencoder
LIBS     = -L.
CFLAGS   = -DWIN32 -DDSP_PRINTF=1 -O3 -fPIC -Wall $(INCLUDES)

#define your binary dsp code here, and for each, add a section below as per exampl

all:	dspcreate.exe libavdspencoder.dll

# this code is used to generate the main encoder library
libavdspencoder.dll:	../encoder/dsp_*.c ../runtime/dsp_header.c
	gcc $(CFLAGS) -shared -o libavdspencoder.dll $^

#this code is used to generate the dspcreate executable
dspcreate.exe:	libavdspencoder.dll ../encoder/dspcreate.c
#	gcc $(CFLAGS) -o dspcreate $(LIBS) ../encoder/dspcreate.c dlfcn.c
	gcc $(CFLAGS) -o $@ ../encoder/dspcreate.c ../encoder/dsp_*.c ../runtime/dsp_header.c -DNODYLIB $(LIBS) 
