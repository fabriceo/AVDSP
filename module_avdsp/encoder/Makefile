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
else
	CFLAGS = -DLINUX -std=gnu99
endif
endif


INCLUDES = -I../encoder -I../runtime
CFLAGS += -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)
LIBS = -L. -lavdspencoder -lm -ldl  

FILESENCODER = ../encoder/dsp_encoder.c \
		../encoder/dsp_encoder.h \
		../encoder/dsp_fileaccess.c \
		../encoder/dsp_fileaccess.h \
		../encoder/dsp_filters.c \
		../encoder/dsp_filters.h \
		../encoder/dsp_HilbertDesign.c \
		../encoder/dsp_nanosharcxml.c \
		../encoder/dsp_basiccreate.c \
		../runtime/dsp_header.h \
		../runtime/dsp_header.c

ifeq ($(uname_S), Darwin)
all:	libavdspencoder.dylib dspcreate

libavdspencoder.dylib: $(FILESENCODER)
	$(CC) $(CFLAGS) -dynamiclib  -o $@ ../encoder/dsp_*.c ../runtime/dsp_header.c 
	@rm -rf *.dSYM

dspcreate:	 ../encoder/dspcreate.c libavdspencoder.dylib
	$(CC) $(CFLAGS) ../encoder/dspcreate.c -o $@ $(LIBS) -Wl,-R.
	@rm -f -r *.dSYM

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
	$(CC) $(CFLAGS) ../encoder/dspcreate.c -o $@ $(LIBS)

clean:
	@rm -f dspcreate libavdspencoder.so *.o
endif
endif
