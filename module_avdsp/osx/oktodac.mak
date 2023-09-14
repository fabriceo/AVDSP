#make -f oktodac.mak all 

all:	dac8prodsp.h dacfabriceo.bin dacfabriceo_oppo.bin dacdiy1.bin

rebuild:	clean dac8prodsp.h dacfabriceo.bin dacfabriceo_oppo.bin dacdiy1.bin

clean:
	rm -f dac8prodsp.h dacfabriceo.bin dacfabriceo_oppo.bin dacdiy1.bin

dac8prodsp.h:	oktodac.dylib dspcreate
	./dspcreate -dspprog oktodac.dylib -hexfile dac8prodsp.h -dspformat 2  -dac8prodsp -dither 24
	
dacfabriceo.bin:	oktodac_fabriceo.dylib dspcreate
	./dspcreate -dspprog oktodac_fabriceo.dylib -binfile dacfabriceo.bin -dspformat 2  -fsmax 96000 -fx 800 -dither 23 -microslow 740 -gcomp 0.35
	
dacfabriceo_oppo.bin:	oktodac_fabriceo.dylib dspcreate
	./dspcreate -dspprog oktodac_fabriceo.dylib -binfile dacfabriceo_oppo.bin -dspformat 2  -fsmax 96000 -fx 800 -dither 23 -microslow 740 -gcomp 0.35 -oppo
	
dacdiy1.bin:	oktodac_diy.dylib dspcreate
	./dspcreate -dspprog oktodac_diy.dylib -binfile dacdiy1.bin -dspformat 2  -fsmax 192000 -diy1 -dither 24
