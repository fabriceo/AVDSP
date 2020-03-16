#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dsp_encoder.h"

int dspProg(int argc,char **argv){

FILE *fd;
char *line;
size_t sz;

  if(argc==0) {
	fprintf(stderr,"Need REW filters file name\n");
	return 1;
  }
  fd=fopen(argv[0],"r");
  if(fd==NULL) {
	fprintf(stderr,"Could not open %s\n",argv[0]);
	return 1;
  }

  // test first line for format
  line=NULL;sz=0;
  getline(&line,&sz,fd);
  if(strcmp(line,"Filter Settings file\n")) {
	fprintf(stderr,"Wrong file type\n");
	free(line);
	return 1;
  }

  while(!feof(fd)) {
  	free(line);
	line=NULL;sz=0;
	getline(&line,&sz,fd);
	if (strcmp(line,"Equaliser: Generic\n")==0) break; 
  }
  if(feof(fd)) {
	fprintf(stderr,"Wrong equaliser type. Need : Generic\n");
	free(line);
	return 1;
  }

  dsp_CORE(); 
  dsp_TPDF(24); 

  dsp_LOAD(0); 
  dsp_GAIN_Fixed(1.0);

  while(!feof(fd)) {
	int nf,len;
	float Fc,G,Q;

  	free(line);
	line=NULL;sz=0;
	getline(&line,&sz,fd);

	if(sscanf(line,"Filter %d:",&nf)!=1) continue;
	if(strncmp(&(line[11]),"ON",2)) continue;

	len=strlen(line);
	if(len<35) continue;	

	line[34]=0;
	Fc=atof((&line[27]));

	G=0;Q=M_SQRT1_2;

	if(len>53 && line[39]=='G') {
		line[50]=0;
		G=powf(10.0,atof((&line[44]))/20.0);
		
		if(len>62 && line[55]=='Q') {
			line[62]=0;
			Q=atof((&line[57]));
		}
	}
	if(len>46 && line[39]=='Q') {
		line[47]=0;
		Q=atof((&line[41]));
	}


    	dsp_PARAM();

	if(strncmp(&(line[15]),"PK",2)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FPEAK,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LP ",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FLP2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HP ",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FHP2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LP1",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter1stOrder(FLP1,Fc,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HP1",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter1stOrder(FHP1,Fc,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LPQ",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FLP2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HPQ",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FHP2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LS  ",4)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FLS2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HS  ",4)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FHS2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LSQ",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FLS2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HSQ",3)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FHS2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LS 6",4)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter1stOrder(FLS1,Fc,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HS 6",4)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter1stOrder(FHS1,Fc,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"LS 12",5)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FLS2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"HS 12",5)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FHS2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}
	if(strncmp(&(line[15]),"NO",2)==0) {
		fprintf(stderr,"Filter %d : NO not implemented\n",nf);
		continue;
	}
	if(strncmp(&(line[15]),"AP",2)==0) {
 		int filter = dspBiquad_Sections(1);
		dsp_Filter2ndOrder(FAP2,Fc,Q,G);
		dsp_BIQUADS(filter); 
		continue;
	}

	fprintf(stderr,"Filter %d : Unknown filter type %s\n",nf,&(line[15]));
  }

   fclose(fd);

   dsp_SAT0DB_TPDF(); 
   dsp_STORE(1);

   return dsp_END_OF_CODE();

}

