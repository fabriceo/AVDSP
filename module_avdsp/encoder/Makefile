INCLUDES = -I../encoder -I../runtime
CFLAGS = -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)

all:
	$(CC) -dynamiclib $(CFLAGS) -o libavdspencoder.dylib dsp_*.c

clean:
	@rm -f *.dylib 

