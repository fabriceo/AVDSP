#make -f oktodac.mak all
all:
	./dspcreate -dspprog oktodac.dylib -hexfile dac8prodsp.h -dspformat 2  -dac8prodsp -dither 24
	./dspcreate -dspprog oktodac_fabriceo.dylib -binfile dacfabriceo.bin -dspformat 2  -fsmax 96000 -fx 800 -dither 23 -microslow 740 -gcomp 0.35
	./dspcreate -dspprog oktodac_diy.dylib -binfile dacdiy1.bin -dspformat 2  -fsmax 192000 -diy1 -dither 24
	