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
#this libraries are not really needed. just to check compilation errors for each DSP_FORMAT

INCLUDES = -I../runtime
CFLAGS += -Ofast -fPIC -Wall $(INCLUDES)

ifeq ($(uname_S), Darwin)
all:
	$(CC) $(CFLAGS) -dynamiclib -DDSP_FORMAT=2 dsp_*.c -o libavdspruntimeint.dylib
	$(CC) $(CFLAGS) -dynamiclib -DDSP_FORMAT=3 dsp_*.c -o libavdspruntimefloat.dylib
	$(CC) $(CFLAGS) -dynamiclib -DDSP_FORMAT=4 dsp_*.c -o libavdspruntimedouble.dylib
	$(CC) $(CFLAGS) -dynamiclib -DDSP_FORMAT=5 dsp_*.c -o libavdspruntimefloatfloat.dylib
	$(CC) $(CFLAGS) -dynamiclib -DDSP_FORMAT=6 dsp_*.c -o libavdspruntimedoublefloat.dylib
	@rm -f -r *.dSYM

clean:
	@rm -f *.dylib 
	
else 
ifeq ($(uname_S), Windows)
#windows compilation here
    
else
#linux compilation here
all:
	$(CC) $(CFLAGS) -shared -DDSP_FORMAT=2 dsp_*.c -o libavdspruntimeint.so 
	$(CC) $(CFLAGS) -shared -DDSP_FORMAT=3 dsp_*.c -o libavdspruntimefloat.so
	$(CC) $(CFLAGS) -shared -DDSP_FORMAT=4 dsp_*.c -o libavdspruntimedouble.so
	$(CC) $(CFLAGS) -shared -DDSP_FORMAT=5 dsp_*.c -o libavdspruntimefloatfloat.so
	$(CC) $(CFLAGS) -shared -DDSP_FORMAT=6 dsp_*.c -o libavdspruntimedoublefloat.so

clean:
	@rm -f *.so

endif
endif
