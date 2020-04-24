#make -f oktodac.mak all
all:
	./dspcreate -dspprog oktodac.dylib -hexfile dspdac8pro.h -dspformat 2  -dac8pro
	./dspcreate -dspprog oktodac.dylib -hexfile dspdacstereo.h -dspformat 2  -dacstereo
	./dspcreate -dspprog oktodac.dylib -binfile dacfabriceo.bin -dspformat 2  -fsmax 96000 -dacfabriceo -fx 800 -dither 23 -microslow 740 -gcomp 0.35 
	./dspcreate -dspprog oktodac.dylib -binfile dacstereodsp4.bin -dspformat 2  -dacstereodsp4
