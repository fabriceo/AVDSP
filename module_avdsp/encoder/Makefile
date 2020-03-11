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
CFLAGS += -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)
CC = gcc -g

ifeq ($(uname_S), Darwin)
all:
	$(CC) $(CFLAGS) -dynamiclib  -o libavdspencoder.dylib dsp_*.c
	
clean:
	@rm -f *.dylib 

else
ifeq ($(uname_S), Windows)
    
else
all:
	$(CC) $(CFLAGS) -shared -o libavdspencoder.so dsp_*.c 
clean:
	@rm -f *.so 
endif
endif

