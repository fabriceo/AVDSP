#make -f oktodac.mak all 

PROGS = 	dac8prodsp.h \
			dacfabriceo.bin \
			dacfabriceo_oppo.bin \
			dacfabriceo_LXmini_LR2.bin \
			dacfabriceo_LXmini_LV8.bin \
			dacdiy1.bin \
			dsptest1.bin \
			crossoverLV6.bin

rebuild:	clean all

clean:
	rm -f 	$(PROGS)
	
all:		$(PROGS)

dac8prodsp.h:	oktodac.dylib dspcreate
	./dspcreate -dspprog oktodac.dylib -hexfile dac8prodsp.h -dspformat 2  -dac8prodsp -dither 24
	
dacfabriceo.bin:	oktodac_fabriceo.dylib dspcreate
	./dspcreate -dspprog oktodac_fabriceo.dylib -binfile dacfabriceo.bin -dspformat 2  -fsmax 96000 -fx 800 -microslow 740 -gcomp -0.35 #-centerhilbert
	
dacfabriceo_oppo.bin:	oktodac_fabriceo.dylib dspcreate
	./dspcreate -dspprog oktodac_fabriceo.dylib -binfile dacfabriceo_oppo.bin -dspformat 2  -fsmax 96000 -fx 800 -microslow 740 -gcomp -0.35 -oppo
	
dacfabriceo_LXmini_LR2.bin:	oktodac_LX.dylib dspcreate
	./dspcreate -dspprog oktodac_LX.dylib -binfile dacfabriceo_LXmini_LR2.bin -dspformat 2 -fsmax 96000 -lxmini -dither 23
	
dacfabriceo_LXmini_LV8.bin:	oktodac_LX.dylib dspcreate
	./dspcreate -dspprog oktodac_LX.dylib -binfile dacfabriceo_LXmini_LV8.bin -dspformat 2 -fsmax 96000 -lxmini -lv8  -dither 23
	
dacdiy1.bin:	oktodac_diy.dylib dspcreate
	./dspcreate -dspprog oktodac_diy.dylib -binfile dacdiy1.bin -dspformat 2  -fsmax 192000 -prog 1 -dither 24

dsptest1.bin:	testfunction.dylib dspcreate
	./dspcreate -dspprog testfunction.dylib -binfile dsptest1.bin -dspformat 3 -fsmax 96000 -test1  -dither 26

crossoverLV6.bin:	crossoverLV6.dylib dspcreate
	./dspcreate -dspprog crossoverLV6.dylib -binfile crossoverLV6.bin -dspformat 2 -fsmax 96000 -fx 800
	