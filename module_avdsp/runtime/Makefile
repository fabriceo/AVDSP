INCLUDES = -I../runtime
CFLAGS = -Ofast -fPIC -Wall $(INCLUDES)

all:
	$(CC) -dynamiclib $(CFLAGS) -DDSP_FORMAT=2 -o libavdspruntimeint.dylib   dsp_*.c
	$(CC) -dynamiclib $(CFLAGS) -DDSP_FORMAT=3 -o libavdspruntimefloat.dylib dsp_*.c

clean:
	@rm -f *.dylib 

