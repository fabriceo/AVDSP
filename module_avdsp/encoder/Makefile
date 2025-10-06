ifeq ($(OS),Windows_NT)
    uname_S := Windows
else
    uname_S := $(shell uname -s)
endif

ifeq ($(uname_S), Windows)
    CFLAGS = -DWIN32
else 
ifeq ($(uname_S), Darwin)
    CFLAGS = -DOSX
	# tested successfully on imac 2010 intel with high sierra
	OSXX86 = -target x86_64-apple-macos10.13
	# so far all ARM silicon product support sequoia macos15
	OSXARM = -target arm64-apple-macos15 
else
	CFLAGS = -DLINUX -std=gnu99
endif
endif


INCLUDES = -I../encoder -I../runtime
CFLAGS += -DDSP_PRINTF=1 -O3 -fPIC -Wall $(INCLUDES)
#LIBS = -L. -lavdspencoder -lm -ldl  
LIBS = -L. -lm -ldl  

FILESENCODER = ../encoder/dsp_encoder.c \
		../encoder/dsp_encoder.h \
		../encoder/dsp_fileaccess.c \
		../encoder/dsp_fileaccess.h \
		../encoder/dsp_filters.c \
		../encoder/dsp_filters.h \
		../encoder/dsp_HilbertDesign.c \
		../encoder/dsp_nanosharcxml.c \
		../encoder/dsp_basiccreate.c \
		../encoder/dsp_mipsXS2.c \
		../runtime/dsp_header.h \
		../runtime/dsp_header.c

ifeq ($(uname_S), Darwin)
all:	libavdspencoder.dylib dspcreate

libavdspencoder.dylib: $(FILESENCODER)
	$(CC) $(CFLAGS) -dynamiclib  -o $@ ../encoder/dsp_*.c ../runtime/dsp_header.c 
	@rm -rf *.dSYM

dspcreate_arm:	 ../encoder/dspcreate.c $(FILESENCODER)
	$(CC) $(CFLAGS) -o $@ -DNODYLIB ../encoder/dspcreate.c ../encoder/dsp_*.c ../runtime/dsp_header.c $(LIBS) $(OSXARM)
	@rm -f -r *.dSYM

dspcreate_x86:	 ../encoder/dspcreate.c $(FILESENCODER)
	$(CC) $(CFLAGS) -o $@ -DNODYLIB ../encoder/dspcreate.c ../encoder/dsp_*.c ../runtime/dsp_header.c $(LIBS) $(OSXX86)
	@rm -f -r *.dSYM

dspcreate:	dspcreate_x86 dspcreate_arm
	@lipo -create -output $@ dspcreate_x86 dspcreate_arm
	@echo dual architecture $@ combined sucessfully

clean:
	@rm -rf dspcreate libavdspencoder.dylib *.o

else 
ifeq ($(uname_S), Windows)

#windows compile here with MinGW
#nothing done here, see windows folder
	
else
all:	libavdspencoder.so dspcreate

libavdspencoder.so:	$(FILESENCODER)
	$(CC) $(CFLAGS) -shared -o $@ ../encoder/dsp_*.c ../runtime/dsp_header.c

dspcreate:	 ../encoder/dspcreate.c libavdspencoder.so
#	$(CC) $(CFLAGS) ../encoder/dspcreate.c -o $@ $(LIBS) -lavdspencoder
	$(CC) $(CFLAGS) -o $@ -DNODYLIB ../encoder/dspcreate.c ../encoder/dsp_*.c ../runtime/dsp_header.c $(LIBS)

clean:
	@rm -f dspcreate libavdspencoder.so *.o
endif
endif
