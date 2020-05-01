#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define numberChannels       (16+1)   // max possible number of input and output channels
#define filterBankSize       (16+1)   // max possible filters per channels

typedef struct dspChannel_s {
    int   muted;
    int   inverted;
    float gain;
    float delay;
    float inputMix[numberChannels];
}dspChannel_t;

typedef struct dspFilter_s {
    int   type;
    float freq;
    float Q;
    float boost;
    int   bypass;
} dspFilter_t;


dspChannel_t dspChannel0 = { 0, 0, 0.0, 0.0, { 0.0 } };

dspChannel_t dspChannels[numberChannels];

const dspFilter_t dspFilter0 = { 1, 0, 1000.0, 1.0, 0.0 };

dspFilter_t dspFilters[numberChannels][filterBankSize];

static FILE* minidspOutput = NULL;
static FILE* minidspInput  = NULL;

static char * minidspInfoFileName = "nanosharcinfo.h";

static char minidspLine[512];

static int numberChannelsMax = 0;
static int filterBankSizeMax = 0;

int checkChannels(int x, int y, int z){
    if ( (x >= numberChannels ) || (y >= z) ) {
        printf("out of range x=%d, y=%d ",x,y);
        return 0;
    } else {
        if (x>numberChannelsMax) numberChannelsMax = x;
        if (y > filterBankSizeMax) filterBankSizeMax = y;
        return 1;
    }
}

static int parseSimpleValue(char * function, int *x, int*y, float*val){
    char * s1;
    char * s2;
    char * s3;

    if ((s1 = strstr(minidspLine, function))){
        int len = strlen(function);
        s1 += len;
        *x = strtol(s1, &s2, 10);
        s2++;   // skip the "_" underscore caracter
        *y = strtol(s2, &s3, 10);
        // read the next line
        if (fgets(minidspLine, 512, minidspInput)){
            if ((s1 = strstr(minidspLine, "<dec>"))) {
                s1 += 5; // skip <dec>
                *val = strtof(s1, &s3); }
            return 1;
        }
    }
    return 0;
}

const char * filterNames[] = {
   "PK",
   "APF",
   "SH",
   "SL",
   "BWLPF_1",
   "BWLPF_2",
   "BWLPF_3",
   "BWLPF_4",
   "BWLPF_5",
   "BWLPF_6",
   "BWLPF_7",
   "BWLPF_8",
   "BWHPF_1",
   "BWHPF_2",
   "BWHPF_3",
   "BWHPF_4",
   "BWHPF_5",
   "BWHPF_6",
   "BWHPF_7",
   "BWHPF_8",
   "LRLPF_2",
   "LRLPF_4",
   "LRLPF_8",
   "LRHPF_2",
   "LRHPF_4",
   "LRHPF_8",
   "BSLPF"
};

static int searchFilter(char *s){
    int n = sizeof(filterNames)/sizeof(filterNames[0]);
    for (int i=0; i<n; i++)
        if (strcmp(s,filterNames[i])==0) return i;

    return -1;
}

static int parseFilter(char * function, int *x, int*y, dspFilter_t* filter){

    char * s1;
    char * s2;
    char * s3;

    if ((s1 = strstr(minidspLine, function))){
        s1 += strlen(function);
        *x = strtol(s1, &s2, 10);
        s2++; // skip "_"
        *y = strtol(s2, &s3, 10);
        while (fgets(minidspLine, 512, minidspInput)) {
            if ((s1 = strstr(minidspLine, "<freq>"))) {
                s1 += 6;
                filter->freq = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(minidspLine, "<q>" ))) {
                s1 += 3;
                filter->Q = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(minidspLine, "<boost>" ))) {
                s1 += 7;
                filter->boost = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(minidspLine, "<type>" ))) {
                s1 += 6;
                s2 = strstr(s1,"</type>");
                *s2 = (char)0;
                filter->type = searchFilter(s1);
                continue; }
            if ((s1 = strstr(minidspLine, "<bypass>" ))) {
                s1 += 8;
                filter->bypass = strtol(s1, &s3, 10);
                continue;}
            if ((s1 = strstr(minidspLine, "</filter>" ))) return 1;
        }
    }
    return 0;
}

void filterSort() {
  dspFilter_t filt0,filt1;

  for( int n = 0; n < numberChannels; n++ ) {

      for( int i = 0; i < filterBankSize; i++ ) {
          for( int j = 0; j < ( filterBankSize - 1 - i) ; j++) {
              filt0 = dspFilters[n][ j ];
              filt1 = dspFilters[n][ j + 1 ];
              float freq0, freq1;
              freq0 = filt0.freq;
              freq1 = filt1.freq;

              if( freq0 > freq1 ) {
                  dspFilters[n][ j ] = filt1;
                  dspFilters[n][ j + 1 ] = filt0;
              }//end if
          }//end for j
      }//end for i
  }
}

static int minidspParseLine(){
    char * dgain        = "<item name=\"DGain_";
    char * status       = "_status";
    char * mixer        = "<item name=\"MixerNxMSmoothed1_";
    char * delay        = "<item name=\"Delay_";
    char * polarity     = "<item name=\"polarity_in_1_";
    char * PEQ          = "<filter name=\"PEQ_";
    char * BPF          = "<filter name=\"BPF_";

    int x; int y; float val;

    dspFilter_t filter;

    if (strstr(minidspLine, status)) {
        if (parseSimpleValue(dgain, &x, &y, &val)){
            if ( checkChannels(x,y,1) ) dspChannels[x].muted = 2.0-val; // val=1=muted, val=2=ok
            //printf("dgain_status %2d-%2d = %f\n",x,y,val);
        }
    } else {
        if (parseSimpleValue(dgain, &x, &y, &val)){
            if ( checkChannels(x,y,1) ) dspChannels[x].gain = val;
            //printf("dgain %2d-%2d = %f\n",x,y,val);
        }
        if (parseSimpleValue(mixer, &x, &y, &val)){
            if ( checkChannels(x,y,numberChannels) ) dspChannels[x].inputMix[y] = val;
            //printf("mixer %2d-%2d = %f\n",x,y,val);
        }
        if (parseSimpleValue(delay, &x, &y, &val)){
            if ( checkChannels(x,y,1) ) dspChannels[x].delay = val;
            //printf("delay %2d-%2d = %f\n",x,y,val);
        }
        if (parseSimpleValue(polarity, &x, &y, &val)){
            if ( checkChannels(x,y,1) ) dspChannels[x].inverted = val;
            //printf("polarity %2d-%2d = %f\n",x,y,val);
        }
        if (parseFilter(PEQ, &x, &y, &filter) || parseFilter(BPF,&x,&y, &filter) ) {
            if ( checkChannels(x,y,filterBankSize ) ) dspFilters[x][y] = filter;
            //printf("filter %2d-%2d f=%5.0f, Q=%8f, g=%5f, byp=%d, type=%s\n",x,y,filter.freq, filter.Q, filter.boost, filter.bypass, filterNames[filter.type]);
        }
    }

return 0;
}

#define fout stderr // minidspOutput

void generateChan(dspChannel_t chan){
        fprintf(fout,"{ .muted=%d, .inverted=%d, .gain=%f, .delay=%f,\n",chan.muted, chan.inverted, chan.gain, chan.delay);
        fprintf(fout,"  .inputMix = { ");
        for (int i=0;i<numberChannelsMax;i++)
            fprintf(fout,"%f%c ",chan.inputMix[i],i==(numberChannelsMax-1)? 32:44);
        fprintf(fout,"} }");
}

void generateChannels(){
    for (int n = 0; n<numberChannelsMax; n++) {
        fprintf(fout,"const dspChannel_t chan%d = ",n);
        generateChan(dspChannels[n]);
        fprintf(fout,";\n");
    }
    fprintf(fout,"\nconst dspChannel_t dspChannels[%d] = {\n",numberChannelsMax);
    for (int n = 0; n<numberChannelsMax; n++) {
        generateChan(dspChannels[n]);
        if (n==(numberChannelsMax-1)) fprintf(fout," };\n\n");
        else fprintf(fout,",\n");
    }
}

void generateFilt(dspFilter_t filt){
    fprintf(fout,"{ .type=%2d, .freq=%f, .Q=%f, .boost=%f, .bypass=%d }",filt.type, filt.freq, filt.Q,filt.boost,filt.bypass);
}
void generateFilters(){
    fprintf(fout,"\nconst dspFilter_t dspFilters[%d][%d] = \n",numberChannelsMax,filterBankSizeMax);
    for (int x=0; x<numberChannelsMax; x++) {
        if (x == 0) fprintf(fout,"{ "); else fprintf(fout,"  ");
        for (int y=0; y<filterBankSizeMax; y++) {
            if (y == 0) fprintf(fout,"{ "); else fprintf(fout,"    ");
            generateFilt(dspFilters[x][y]);
            if (y!=(filterBankSizeMax-1)) fprintf(fout,",");
            else {
                fprintf(fout," }");
                if (x!=(numberChannelsMax-1)) fprintf(fout,",");
            }
            fprintf(fout," // %s\n",filterNames[dspFilters[x][y].type]);
        }
        if (x!=(numberChannelsMax-1)) fprintf(fout,"\n");
    }
    fprintf(fout,"};\n");
}

void generateFunctions(){
    fprintf(fout,"... W.I.P....\n");
}

int minidspCreateParameters(char * xmlName){
printf("converting nanosharc xml file\n");
char * xmlfirstline = "<setting version=";

    for (int x=0; x<numberChannels; x++) dspChannel0.inputMix[x] = 0.0;
    for (int x=0; x<numberChannels; x++) {
        dspChannels[x] = dspChannel0;
        for (int y=0; y<filterBankSize; y++)
            dspFilters[x][y] = dspFilter0;
    }
    minidspInput = fopen( xmlName, "r" );
    if (minidspInput == NULL) {
        fprintf(stderr,"Error: Failed to open minidsp xml file.\n");
        return -1;
    }
    if(fseek( minidspInput, 0, SEEK_END ) ) {
      fprintf(stderr,"Error: Failed to discover minidsp xml file size.\n");
      fclose(minidspInput);
      return -1;
    }

    int minidspFileSize = ftell( minidspInput );

    if(fseek( minidspInput, 0, SEEK_SET ) ) {
      fprintf(stderr,"Error: Failed to input file pointer for minidsp xml file %d.\n",minidspFileSize);
     return -1;
    }

    minidspOutput = fopen( minidspInfoFileName, "w" );
    if( minidspOutput == NULL ) {
      fprintf(stderr,"Error: Failed to create %s.\n",minidspInfoFileName);
      fclose(minidspInput);
      return -1;
    }
    fprintf(fout,"//converted information from %s.\n\n",xmlName);
    if (fgets(minidspLine, 512, minidspInput)) {
        if (strstr( minidspLine, xmlfirstline)) {
            while (fgets(minidspLine, 512, minidspInput)) {
                if (minidspParseLine()) break;
            }
            fprintf(fout,"\ntypedef struct dspChannel_s {\n");
            fprintf(fout,"    int   muted;\n");
            fprintf(fout,"    int   inverted;\n");
            fprintf(fout,"    float gain;\n");
            fprintf(fout,"    float delay;\n");
            fprintf(fout,"    float inputMix[numberChannels];\n");
            fprintf(fout,"}dspChannel_t;\n");
            fprintf(fout,"\n");
            fprintf(fout,"typedef struct dspFilter_s {\n");
            fprintf(fout,"    int   type;\n");
            fprintf(fout,"    float freq;\n");
            fprintf(fout,"   float Q;\n");
            fprintf(fout,"    float boost;\n");
            fprintf(fout,"    int   bypass;\n");
            fprintf(fout,"} dspFilter_t;\n\n\n");
            fprintf(fout,"#define numberChannels (%d)\n",numberChannelsMax);
            fprintf(fout,"#define filterBankSize (%d)\n\n",filterBankSizeMax);

            generateChannels();
            generateFilters();

            generateFunctions();
        }
    }
    fclose(minidspOutput);
    fclose(minidspInput);
    return 0;
}
