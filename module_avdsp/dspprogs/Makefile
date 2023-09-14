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
    PL = .dylib
else
   CFLAGS = -DLINUX -std=gnu99
    PL = .so
endif

INCLUDES = -I../encoder -I../runtime
CFLAGS += -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)

ifeq ($(uname_S), Darwin)
LIBS = -L../encoder -lavdspencoder

PROGS = oktodac.dylib \
		crossover2x2lfe.dylib \
		crossoverLV6.dylib \
		testfunction.dylib \
		oktodac_fabriceo.dylib \
		oktodac_diy.dylib \
		oktodac_LX.dylib \
		testrpi.dylib

%.dylib:	../dspprogs/%.c
	$(CC) $(CFLAGS) -dynamiclib $^ -o $@ $(LIBS)
	@rm -f -r *.dSYM

all:	 $(PROGS)

clean:
	@rm -f $(PROGS) 

else
ifeq ($(uname_S), Windows)
all:

clean:

else	#linux
PROGS = oktodac.so \
		crossover2x2lfe.so \
		crossoverLV6.so \
		REWgenericEQ.so \
		testfunction.so \
		oktodac_fabriceo.so \
		oktodac_diy.so \
		oktodac_LX.so \
		testrpi.so

all:	$(PROGS)

%.so:	../dspprogs/%.o
	$(CC) $(CFLAGS) -shared $^ -o $@
	
clean:
	@rm -f $(PROGS)
	
endif
endif

