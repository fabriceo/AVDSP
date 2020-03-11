#make -f dsprunosx.mak all

INCLUDES = -I../runtime -I../encoder
CFLAGS = -DOSX -Ofast  -fPIC  -Wall $(INCLUDES)
SOURCES = dsprunosx.c ../runtime/dsp_*.c ../encoder/dsp_fileaccess.c
CC = gcc -g

all:
	$(CC) $(CFLAGS) $(SOURCES) -DDSP_FORMAT=2 -o dsprunint
	$(CC) $(CFLAGS) $(SOURCES) -DDSP_FORMAT=3 -o dsprunfloat
	$(CC) $(CFLAGS) $(SOURCES) -DDSP_FORMAT=4 -o dsprundouble
	$(CC) $(CFLAGS) $(SOURCES) -DDSP_FORMAT=5 -o dsprunfloatfloat
	$(CC) $(CFLAGS) $(SOURCES) -DDSP_FORMAT=6 -o dsprundoublefloat
	

