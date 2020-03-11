ifeq ($(OS),Windows_NT)
    uname_S := Windows
else
    uname_S := $(shell uname -s)
endif

ifeq ($(uname_S), Windows)
    CFLAGS = -DWIN32
endif
ifeq ($(uname_S), Darwin)
    CFLAGS = -DOSX
else
	CFLAGS = -DLINUX
endif

INCLUDES = -I../encoder -I../runtime
CFLAGS = -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)
CC = gcc -g
ENC_C = ../encoder/dsp_*.c

ifeq ($(uname_S), Darwin)
SLIBS = oktoprog.dylib oktodac.dylib crossover2x2lfe.dylib

%.dylib:	%.c
	$(CC) $(CFLAGS) -dynamiclib $^ -o $*.dylib $(ENC_C)
	
all:	 $(SLIBS)

clean:
	@rm -f $(SLIBS) 

else
ifeq ($(uname_S), Windows)
all:

clean:

else	#linux
SLIBS = oktoprog.so oktodac.so crossover2x2lfe.so

all:	$(SLIBS)

%.so:	%.c
	$(CC) $(CFLAGS) -shared $^ -o $*.so $(ENC_C)
	
clean:
	@rm -f *.so 
endif
endif

