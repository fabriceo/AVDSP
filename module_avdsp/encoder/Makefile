ifeq ($(OS),Windows_NT)
    uname_S := Windows
else
    uname_S := $(shell uname -s)
endif

ifeq ($(uname_S), Windows)
    CFLAGS = -DWIN32
else ($(uname_S), Darwin)
    CFLAGS = -DOSX
else
	CFLAGS = -DLINUX
endif

#not satisfied with dsp_*c to be changed in a later version

INCLUDES = -I../encoder -I../runtime
CFLAGS += -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)

ifeq ($(uname_S), Darwin)
all:
	$(CC) $(CFLAGS) -dynamiclib  -o libavdspencoder.dylib dsp_*.c
	@rm -f -r *.dSYM
	
clean:
	@rm -f *.dylib 

else ($(uname_S), Windows)
#windows compile here    
else
all:
	$(CC) $(CFLAGS) -shared -o libavdspencoder.so dsp_*.c 
	
clean:
	@rm -f *.so 
endif

