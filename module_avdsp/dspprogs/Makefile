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

LIBS = -L../encoder -lavdspencoder

INCLUDES = -I../encoder -I../runtime
CFLAGS += -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)

ifeq ($(uname_S), Darwin)

PROGS = oktodac.dylib crossover2x2lfe.dylib crossoverLV6.dylib testfunction.dylib

%.dylib:	%.c
	$(CC) $(CFLAGS) -dynamiclib $^ -o $*.dylib $(LIBS)
	@rm -f -r *.dSYM

all:	 $(PROGS)

clean:
	@rm -f $(PROGS) 

else
ifeq ($(uname_S), Windows)

#nothing done her, see windows folder

else	
#linux
PROGS = oktodac.so crossover2x2lfe.so crossoverLV6.so REWgenericEQ.so testfunction.so

all:	$(PROGS)

%.so:	%.o
	$(CC) $(CFLAGS) -shared $^ -o $*.so
	
clean:
	@rm -f *.so
	
endif
endif

