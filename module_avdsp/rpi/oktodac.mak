#make -f oktodac.mak all
all:
	./dspcreate -dspprog oktodac.so -binfile dacfabriceo.bin -dspformat 2  -fsmax 96000 -dacfabriceo -fx 800 -dither 23 -microslow 740 -gcomp 0.35 
	./dspcreate -dspprog oktodac.so -binfile testrew.bin -dspformat 2  -fsmax 96000 -testrew 
