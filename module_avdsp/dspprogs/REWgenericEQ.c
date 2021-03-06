#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dsp_encoder.h"

#define DACOUT(x) (x)
#define DACIN(x)  (8+(x))

static int encodeOneChannel(char *filename, int nc, int nv, float gain) {
  FILE *fd;
  char *line;
  size_t sz;

  fprintf(stderr,"core %d gain:%f in:%d out:%d\n", nc , gain, nc/nv,nc);

  fd=fopen(filename,"r");
  if(fd==NULL) {
	fprintf(stderr,"Could not open %s\n",filename);
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

  dsp_PARAM();
  int filter = dspBiquad_Sections_Flexible();

  while(!feof(fd)) {
	int nf;
	float Fc,G,Q;
	char *ppar;

  	free(line);
	line=NULL;sz=0;
	getline(&line,&sz,fd);

	if(sscanf(line,"Filter %d:",&nf)!=1) continue;
	if(strncmp(&(line[11]),"ON",2)) continue;

	Fc=0;G=1.0;Q=M_SQRT1_2;

	for(ppar=strtok(&(line[24])," \n");ppar;ppar=strtok(NULL," \n")) {
		if(strcmp(ppar,"Fc")==0) {
			ppar=strtok(NULL," \n");
			if(ppar == NULL) break;
			Fc=atof(ppar);
			continue;
		}
		if(strcmp(ppar,"Hz") == 0 ) continue;
		if(strcmp(ppar,"Q")==0) {
			ppar=strtok(NULL," \n");
			if(ppar == NULL) break;
			Q=atof(ppar);
			continue;
		}
		if(strcmp(ppar,"Gain")==0) {
			ppar=strtok(NULL," \n");
			if(ppar == NULL) break;
			G=powf(10.0,atof(ppar)/20.0);
			continue;
		}
		if(strcmp(ppar,"dB") == 0 ) continue;
	}

	if(Fc == 0.0) continue;

	if(strncmp(&(line[15]),"None",4)==0) {
		continue;
	}
	if(strncmp(&(line[15]),"PK",2)==0) {
		dsp_Filter2ndOrder(FPEAK,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"LP ",3)==0) {
		dsp_Filter2ndOrder(FLP2,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"HP ",3)==0) {
		dsp_Filter2ndOrder(FHP2,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"LP1",3)==0) {
		dsp_Filter1stOrder(FLP1,Fc,G);
		continue;
	}
	if(strncmp(&(line[15]),"HP1",3)==0) {
		dsp_Filter1stOrder(FHP1,Fc,G);
		continue;
	}
	if(strncmp(&(line[15]),"LPQ",3)==0) {
		dsp_Filter2ndOrder(FLP2,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"HPQ",3)==0) {
		dsp_Filter2ndOrder(FHP2,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"LS  ",4)==0) {
		dsp_Filter2ndOrder(FLS2,Fc,2.0/3.0,G);
		continue;
	}
	if(strncmp(&(line[15]),"HS  ",4)==0) {
		dsp_Filter2ndOrder(FHS2,Fc,2.0/3.0,G);
		continue;
	}
	if(strncmp(&(line[15]),"LSQ",3)==0) {
		dsp_Filter2ndOrder(FLS2,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"HSQ",3)==0) {
		dsp_Filter2ndOrder(FHS2,Fc,Q,G);
		continue;
	}
	if(strncmp(&(line[15]),"LS 6",4)==0) {
		dsp_Filter1stOrder(FLS1, Fc, G);
		continue;
	}
	if(strncmp(&(line[15]),"HS 6",4)==0) {
		dsp_Filter1stOrder(FHS1, Fc, G);
		continue;
	}
	if(strncmp(&(line[15]),"LS 12",5)==0) {
		fprintf(stderr,"Filter %d : not implemented\n",nf);
		continue;
	}
	if(strncmp(&(line[15]),"HS 12",5)==0) {
		fprintf(stderr,"Filter %d : not implemented\n",nf);
		continue;
	}
	if(strncmp(&(line[15]),"NO",2)==0) {
		dsp_Filter2ndOrder(FNOTCH,Fc,30.0,G);
		continue;
	}
	if(strncmp(&(line[15]),"AP",2)==0) {
		dsp_Filter2ndOrder(FAP2,Fc,Q,G);
		continue;
	}

	fprintf(stderr,"Filter %d : Unknown filter type %s\n",nf,&(line[15]));
  }


  dsp_CORE(); 
  if(nc == 0) dsp_TPDF_CALC(0);

  dsp_LOAD_GAIN_Fixed(DACIN(nc/nv),gain);

   dsp_BIQUADS(filter); 

   dsp_SAT0DB_TPDF(); 

   dsp_STORE(DACOUT(nc));

   fclose(fd);
   return 0;

}

static void usage(void) {
	fprintf(stderr,"REWgenericEQ -w n [-g gaindb ] eqfile1 [ [-g gaindb ] eqfile2 ... ]\n");
}

int dspProg(int argc,char **argv){
  int nc;
  int nv=1;
  float gain[8];

  if(argc==0) { usage(); return 1; }

  if(strcmp(argv[0],"-w") == 0 ) {
	if(argc<3)  { usage();  return 1; }
	nv=atoi(argv[1]);
	argc-=2;argv+=2;		
  }

  for(nc=0;nc<8;nc++)
	gain[nc]=1.0;

  nc=0;
  while(argc && nc<8) {

  	if(strcmp(argv[0],"-g") == 0 ) {
		if(argc<3)  { usage();  return 1; }
		gain[nc]=powf(10.0,atof(argv[1])/20.0);
		argc-=2;argv+=2;		
	}

  	if(argc==0) { usage(); return 1; }

	encodeOneChannel(argv[0],nc,nv,gain[nc]);
	nc++;
	argc--;argv++;		
  }

  if(nc==0) { usage(); return 1; }
 
  return dsp_END_OF_CODE();
}

