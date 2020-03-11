INCLUDES = -I../encoder -I../runtime -L../encoder
CFLAGS = -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)

SLIBS = oktoprog.dylib oktodac.dylib crossover2x2lfe.dylib

%.dylib:	%.c
	$(CC) $(CFLAGS) -dynamiclib $^ -o $*.dylib -lavdspencoder #-shared
	
all:	 $(SLIBS)

clean:
	@rm -f $(SLIBS) 

