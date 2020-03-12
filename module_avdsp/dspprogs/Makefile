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

ifeq ($(uname_S), Darwin)
LIBS = -L../encoder -lavdspencoder

PROGS = oktoprog.dylib oktodac.dylib crossover2x2lfe.dylib crossoverLV6.dylib

%.dylib:	%.c
	$(CC) $(CFLAGS) -dynamiclib $^ -o $*.dylib $(LIBS)
	@rm -f -r *.dSYM

all:	 $(PROGS)

clean:
	@rm -f $(PROGS) 

else($(uname_S), Windows)
all:

clean:

else	#linux
PROGS = oktoprog.so oktodac.so crossover2x2lfe.so crossoverLV6.so

all:	$(PROGS)

%.so:	%.o
	$(CC) $(CFLAGS) -shared $^ -o $*.so
	
clean:
	@rm -f *.so
	
endif

