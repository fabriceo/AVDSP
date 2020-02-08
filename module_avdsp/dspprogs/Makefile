
INCLUDES = -I../encoder -I../runtime
CFLAGS = -DDSP_PRINTF=3 -Ofast -fPIC -Wall $(INCLUDES)

#SLIBS = oktoprog.so oktoprog2.so crossover2x2lfe.so
SLIBS = crossover2x2lfe.so

%.so:	%.o 
	$(CC) -shared $^ -o $*.so  

all:	$(SLIBS)

clean:
	@rm -f $(SLIBS) 

