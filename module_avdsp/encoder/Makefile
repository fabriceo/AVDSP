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

#not satisfied with dsp_*c to be changed in a later version

INCLUDES = -I../encoder -I../runtime
CFLAGS += -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)


ifeq ($(uname_S), Darwin)
all:
	$(CC) $(CFLAGS) -dynamiclib  -o libavdspencoder.dylib dsp_*.c
	@rm -rf *.dSYM
	
clean:
	@rm -rf *.dylib 

else 
ifeq ($(uname_S), Windows)

#windows compile here with MinGW
#nothing done here, see windows folder
	
else
all:
	$(CC) $(CFLAGS) -shared -o libavdspencoder.so dsp_*.c 
	
clean:
	@rm -f *.so 
endif
endif
