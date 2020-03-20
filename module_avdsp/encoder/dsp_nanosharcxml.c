#include <string.h>
#include <stdlib.h>
#include <stdio.h>


static FILE* minidspOutput = NULL;
static FILE* minidspInput  = NULL;

static char * minidspInfoFileName = "nanosharcinfo.h";

static char minidspLine[512];

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

static int parseFilter(char * function, int *x, int*y, float *freq, float *Q, float *boost, int*bypass, int *ftype){

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
                *freq = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(minidspLine, "<q>" ))) {
                s1 += 3;
                *Q = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(minidspLine, "<boost>" ))) {
                s1 += 7;
                *boost = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(minidspLine, "<type>" ))) {
                s1 += 6;
                s2 = strstr(s1,"</type>");
                *s2 = (char)0;
                *ftype = searchFilter(s1);
                continue; }
            if ((s1 = strstr(minidspLine, "<bypass>" ))) {
                s1 += 8;
                *bypass = strtol(s1, &s3, 10);
                continue;}
            if ((s1 = strstr(minidspLine, "</filter>" ))) return 1;
        }
    }
    return 0;
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
    float freq; float Q; float boost; int bypass;
    int ftype;

    if (strstr(minidspLine, status)) {
        if (parseSimpleValue(dgain, &x, &y, &val)){
            printf("dgain_status %2d-%2d = %f\n",x,y,val);  // val=1=muted, val=2=ok
        }
    } else {
        if (parseSimpleValue(dgain, &x, &y, &val)){
            printf("dgain %2d-%2d = %f\n",x,y,val);
        }
        if (parseSimpleValue(mixer, &x, &y, &val)){
            printf("mixer %2d-%2d = %f\n",x,y,val);
        }
        if (parseSimpleValue(delay, &x, &y, &val)){
            printf("delay %2d-%2d = %f\n",x,y,val);
        }
        if (parseSimpleValue(polarity, &x, &y, &val)){
            printf("polarity %2d-%2d = %f\n",x,y,val);
        }
        if (parseFilter(PEQ,&x,&y, &freq, &Q, &boost, &bypass, &ftype)) {
            printf("PEQ %2d-%2d f=%5.0f, Q=%8f, g=%5f, byp=%d, type=%d\n",x,y,freq, Q, boost, bypass, ftype);
        }
        if (parseFilter(BPF,&x,&y, &freq, &Q, &boost, &bypass, &ftype)) {
            printf("BPF %2d-%2d f=%5.0f, Q=%8f, g=%5f, byp=%d, type=%d\n",x,y,freq, Q, boost, bypass, ftype);
        }
    }

return 0;
}

int minidspCreateParameters(char * xmlName){
printf("converting nanosharc xml file\n");
char * xmlfirstline = "<setting version=";

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
    fprintf(minidspOutput,"//converted information from %s.\n",xmlName);
    if (fgets(minidspLine, 512, minidspInput)) {
        if (strstr( minidspLine, xmlfirstline)) {
            while (fgets(minidspLine, 512, minidspInput)) {
                if (minidspParseLine()) break;
            }
        }
    }
    fclose(minidspOutput);
    fclose(minidspInput);
    return 0;
}
