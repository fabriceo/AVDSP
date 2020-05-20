
#include "sigmadspfilters.h" // this also include the sigmastudio generic file

#if 1   // autorizing the 2 below function in the source code, they need the 3 libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define textFileLineSize 512
static char textFileLine[textFileLineSize]; // buffer for reading 1 line of the text file

// save a table of preset (numberOfPresets>0)
void dspPresetWriteTextfile(dspPreset_t * p, int numberOfPresets, char * filename){
    FILE* fout = NULL;
    fout = fopen( filename, "w" );
    if( fout == NULL ) {
      fprintf(stderr,"Error: Failed to create %s.\n",filename);
      fclose(fout);
      return ; }
    fprintf(stderr,"saving %d presets\n",numberOfPresets);
    for (int num=0; num < numberOfPresets; num++) {
        dspPresetChecksumUpdate(p);
        if (p->presetNumber == 0) p->presetNumber = num+1;
        if (num == 0)
             fprintf(fout,"PRESET %d INPUTS %d, OUTPUTS %d,  FILTERBANKSIZEMAX %d\n",p->presetNumber, p->numberOfInputs, p->numberOfOutputs, p->filterBankSize);
        else fprintf(fout,"PRESET %d\n", p->presetNumber);
        DSP_FOR_ALL_CHANNELS(p,ch) {
            if (ch < p->numberOfInputs)
                 fprintf(fout,"INPUT  %d / %d\n",ch+1, p->numberOfInputs);
            else fprintf(fout,"OUTPUT %d / %d\n",ch+1 - p->numberOfInputs, p->numberOfOutputs);
            fprintf(fout,"MIXER  ");
            for (int i=0; i< p->numberOfInputs; i++)
                fprintf(fout,"%f ",p->fb[ch].mixer[i]);
            fprintf(fout,"\n");
            fprintf(fout,"GAIN   %f\n",p->fb[ch].gain);
            if (p->fb[ch].mute) fprintf(fout,"MUTED\n");
            if (p->fb[ch].invert) fprintf(fout,"INVERTED\n");
            fprintf(fout,"DELAY  %f\n",p->fb[ch].delayMicroSec);
            for (int i=0; i< p->fb[ch].numberOfFilters; i++)
                if ((i == 0)||(p->fb[ch].filter[i].ftype != FNONE)) {
                    if (p->fb[ch].filter[i].ftype == FNONE)
                        fprintf(fout,"FILTER NONE\n");
                    else
                        fprintf(fout,"FILTER %s %d %d %d %f %f %f\n",
                                dspFilterNames[ p->fb[ch].filter[i].ftype ],
                                p->fb[ch].filter[i].bypass,
                                p->fb[ch].filter[i].invert,
                                p->fb[ch].filter[i].locked,
                                p->fb[ch].filter[i].freq,
                                p->fb[ch].filter[i].Q,
                                p->fb[ch].filter[i].gain ); }
        }
        dspPresetChecksumUpdate(p);
        fprintf(fout,"CHECKSUM %d PRESET %d\n\n",p->checksum, p->presetNumber);
        p++;
    }
    fclose(fout);
}

FILE* dspInputFile = NULL;

int dspOpenInputFile(char *filename){
    dspInputFile = fopen( filename, "r" );
    if (dspInputFile == NULL) {
        fprintf(stderr,"Error: Failed to open file %s.\n",filename); return 0; }
    if(fseek( dspInputFile, 0, SEEK_END ) ) {
        fprintf(stderr,"Error: Failed to discover file size %s.\n", filename);
        fclose(dspInputFile);  return 0; }
    int fileSize = ftell( dspInputFile );
    if(fseek( dspInputFile, 0, SEEK_SET ) ) {
        fprintf(stderr,"Error: Failed to input file pointer %d.\n",fileSize);
        fclose(dspInputFile);  return 0; }
    return fileSize;
}

// read a text file containing preset, read a maximum quantity of "numberOfPresets"
// if "numberOfPresets" is negative (eg -3) then read only 1 preset and search for index number 3
int dspPresetReadTextfile(dspPreset_t * p, int numberOfPresets, char * filename){
    int check = 1;
    int numPreset = 0;
    int increment = 0;
    if (numberOfPresets < 0) {
        numPreset = -numberOfPresets;
        numberOfPresets = 1; }

    for (int i=0; i< numberOfPresets; i++)  dspPresetReset(&p[i], 0);

    char * PRESET = "PRESET ";
    char * INPUT  = "INPUT ";
    char * OUTPUT = "OUTPUT ";  // always after inputs
    char * MIXER  = "MIXER ";
    char * GAIN   = "GAIN ";
    char * DELAY  = "DELAY ";
    char * MUTED  = "MUTED";
    char * INVERTED = "INVERTED";
    char * FILTER = "FILTER ";
    char * NONE   = "NONE";
    char * CHECKSUM = "CHECKSUM ";  // indicate the end of a preset definition

    if (dspOpenInputFile(filename)) {
    fprintf(stderr,"reading preset file\n");
    int ch = 0; int filter = 0; char *s; char *err;
    // read one full line from the file and search for unic key words
    while (fgets(textFileLine, sizeof(textFileLine)-1, dspInputFile)) {
        if (p->presetNumber != 0)
            if (ch< DSP_IO_TOTAL)
                if (filter < filterBankSize[ch])
                    if ( ( s= strstr(textFileLine, FILTER) ) ) {
                        s+= 7; // skip word FILTER and space separator
                        //printf("filter\n");
                        if (strstr(textFileLine, NONE) )
                            p->fb[ch].filter[filter] = dspFilterNone;
                        else {
                            int ftype = dspFilterNameSearch(s);
                            if (ftype) {
                                p->fb[ch].filter[filter].ftype  = ftype;
                                s += 6; // skip filter name+space
                                p->fb[ch].filter[filter].bypass = strtol(s, &err,10); s=err;
                                p->fb[ch].filter[filter].invert = strtol(s, &err,10); s=err;
                                p->fb[ch].filter[filter].locked = strtol(s, &err,10); s=err;
                                p->fb[ch].filter[filter].freq   = strtof(s, &err); s=err;
                                if(dspFilterNeedQ(ftype)) {
                                    p->fb[ch].filter[filter].Q = strtof(s, &err); s=err; }
                                if (dspFilterNeedGain(ftype))
                                    p->fb[ch].filter[filter].gain   = strtof(s, &err);
                                filter++;  }
                        }
                        continue; }
        filter = 0;
        if ( ( s= strstr(textFileLine, PRESET) ) ) {
            s+= 6;
            if ((numPreset != 0) && (p->presetNumber == numPreset)) break;  // too much preset and we are done
            int preset = strtol(s, &err, 10);
            printf("preset %d\n",preset);
            increment++;
            if ((numPreset == 0)&&(increment > numberOfPresets)) break; // too much preset and we are done
            if ((numPreset == 0)&&(increment > 1)) p++;
            if ((numPreset == 0)||(numPreset == preset))
                 dspPresetReset(p, preset);
            ch = 0;
        } else
            if (p->presetNumber != 0) {

                if ( ( s= strstr(textFileLine, CHECKSUM) ) ) { s+= 8;
                    p->checksum = strtol(s, &err, 10);
                    printf("checksum %d\n",p->checksum);
                    if (dspPresetChecksumVerify(p) == 0) {
                            printf("checksum FALSE\n\n");
                            check = 0; p->checksum = 0; }
                    if ((numPreset != 0) && (p->presetNumber == numPreset)) break;  // preset requested is found
                    if ((numPreset == 0) && (increment >= numberOfPresets)) break;  // enough preset loaded
                } else
                if ( ( s= strstr(textFileLine, INPUT) ) ) { s+= 5;
                    ch = strtol(s, &err, 10) -1;
                    //printf("input %d\n",ch);
                } else
                if ( ( s= strstr(textFileLine, OUTPUT) ) ) { s+= 6; // output expected to always come after inputs
                    ch = strtol(s, &err, 10) - 1 + DSP_INPUTS;
                    //printf("output %d\n",ch);
               } else
                    if (ch< DSP_IO_TOTAL) {
                        if ( ( s= strstr(textFileLine, MIXER) ) ) { s+= 5;
                            //printf("mixer\n");
                            for (int i=0; i < DSP_INPUTS; i++) {
                                p->fb[ch].mixer[i] = strtof(s, &err); s=err;  }
                        } else
                        if ( ( s= strstr(textFileLine, GAIN) ) ) { s+= 4;
                            p->fb[ch].gain = strtof(s, &err);
                            //printf("gain %f\n",p->fb[ch].gain);
                        } else
                        if ( ( s= strstr(textFileLine, MUTED) ) ) {
                            p->fb[ch].mute = 1;
                        } else
                        if ( ( s= strstr(textFileLine, INVERTED) ) ) {
                            p->fb[ch].invert = 1;
                        } else
                        if ( ( s= strstr(textFileLine, DELAY) ) ) { s+= 5;
                            p->fb[ch].delayMicroSec = strtof(s, &err);
                            //printf("delay %f\n",p->fb[ch].delayMicroSec);
                            }
                   }
            }
    } // while
    fclose(dspInputFile);
    }

    return check;
}
#endif

static int miniParseSimpleValue(char * function, int *x, int*y, float *val){
    char * s1;
    char * s2;
    char * s3;

    if ((s1 = strstr(textFileLine, function))){
        int len = strlen(function);
        s1 += len;
        *x = strtol(s1, &s2, 10);   // get first value
        s2++;   // skip the "_" underscore caracter
        *y = strtol(s2, &s3, 10);   // get second one
        // read the next line, expected to be <dec>
        if (fgets(textFileLine, sizeof(textFileLine)-1, dspInputFile)){
            if ((s1 = strstr(textFileLine, "<dec>"))) {
                s1 += 5; // skip <dec>
                *val = strtof(s1, &s3); }
            return 1;
        }
    }
    return 0;
}

const char * miniFilterNames[] = {
   "PK", "APF", "SH", "SL",
   "BWLPF_1", "BWLPF_2", "BWLPF_3", "BWLPF_4", "BWLPF_5", "BWLPF_6", "BWLPF_7", "BWLPF_8",
   "BWHPF_1", "BWHPF_2", "BWHPF_3", "BWHPF_4", "BWHPF_5", "BWHPF_6", "BWHPF_7", "BWHPF_8",
   "LRLPF_2", "LRLPF_4", "LRLPF_8", "LRHPF_2", "LRHPF_4", "LRHPF_8",
   "BSLPF"
};

const enum filterTypes miniFilterMap[] = {
        FPEAK, FAP2, FHS2, FLS2,
        FNONE,LPBU2,LPBU3,LPBU4,FNONE,LPBU6,FNONE,LPBU8,
        FNONE,HPBU2,HPBU3,HPBU4,FNONE,HPBU6,FNONE,HPBU8,
        LPLR2,LPLR4,LPLR8,HPLR2,HPLR4,HPLR8,
        LPBE2
};

static enum filterTypes miniSearchFilter(char *s){
    int n = sizeof(miniFilterNames)/sizeof(miniFilterNames[0]);
    for (int i=0; i<n; i++)
        if (strcmp(s,miniFilterNames[i])==0) return miniFilterMap[i];

    return FNONE;
}

static int miniParseFilter(char * function, int *x, int*y, dspFilter_t* filter){

    char * s1;
    char * s2;
    char * s3;

    if ((s1 = strstr(textFileLine, function))){
        s1 += strlen(function);
        *x = strtol(s1, &s2, 10);
        s2++; // skip "_"
        *y = strtol(s2, &s3, 10);
        *filter = dspFilterNone;
        while (fgets(textFileLine, sizeof(textFileLine)-1, dspInputFile)) {
            if ((s1 = strstr(textFileLine, "<freq>"))) {
                s1 += 6;
                filter->freq = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(textFileLine, "<q>" ))) {
                s1 += 3;
                filter->Q = strtof(s1, &s3);
                continue; }
            if ((s1 = strstr(textFileLine, "<boost>" ))) {
                s1 += 7;
                dspFloat_t gain = strtof(s1, &s3); // value is in dB
                gain = pow(10.0, gain/20.0);
                filter->gain = gain;
                continue; }
            if ((s1 = strstr(textFileLine, "<type>" ))) {
                s1 += 6;
                s2 = strstr(s1,"</type>");
                *s2 = (char)0;
                filter->ftype = miniSearchFilter(s1);
                continue; }
            if ((s1 = strstr(textFileLine, "<bypass>" ))) {
                s1 += 8;
                filter->bypass = strtol(s1, &s3, 10);
                continue;}
            if ((s1 = strstr(textFileLine, "</filter>" ))) {
                if ((filter->ftype == FPEAK) && (filter->gain == 1.0) ) filter->ftype = FNONE;
                if (filter->ftype == FNONE) *filter = dspFilterNone;
                return 1; }
        }
    }
    return 0;
}


static int miniParseLine(dspPreset_t * p){
    char * dgain        = "<item name=\"DGain_";
    char * status       = "_status";
    char * mixer        = "<item name=\"MixerNxMSmoothed1_";
    char * delay        = "<item name=\"Delay_";
    char * polarity_in  = "<item name=\"polarity_in_1_";
    char * polarity_out = "<item name=\"polarity_out_1_";
    char * PEQ          = "<filter name=\"PEQ_";
    char * BPF          = "<filter name=\"BPF_";

    int x; int y; dspFloat_t val;
    dspFilter_t filter;

    if (strstr(textFileLine, status)) { //
        if (miniParseSimpleValue(dgain, &x, &y, &val)){ // DGain_1_0_status : x = 1..12, y=0
            if (x <= DSP_INPUTS)  p->fb[x-1].mute = (val == 1.0) ? 1 : 0;
            else
                if (x>=5) {
                    x -=5;
                    if (x < DSP_OUTPUTS) p->fb[DSP_OUTPUTS + x].mute = (val == 1.0) ? 1 : 0; } // val=1=muted, val=2=ok
        }
        if (miniParseSimpleValue(mixer, &x, &y, &val)){ // MixerNxMSmoothed1_0_1_status : x = input 0..3, y = output 0..7
            if ( (x < DSP_INPUTS) & (y < DSP_OUTPUTS) )
                p->fb[DSP_INPUTS + y].mixer[x] = (val == 1.0) ? 1.0 : 0.0;
            //printf("mixer %2d-%2d = %f\n",x,y,val);
        }
    } else {
        if (miniParseSimpleValue(dgain, &x, &y, &val)){ // DGain_1_0 : x = 1..12
            val = pow(10.0, val/20.0);
            if (x <= DSP_INPUTS) p->fb[x-1].gain = val;
            else if (x>=5) {
                    x -= 5;
                    if (x < DSP_OUTPUTS) p->fb[DSP_OUTPUTS + x].gain = val;  }
            //printf("dgain %2d-%2d = %f\n",x,y,val);
        }
        if (miniParseSimpleValue(mixer, &x, &y, &val)){ // MixerNxMSmoothed1_0_0 : x = input 0..3, y = output 0..7
            val = pow(10.0, val/20.0);
            if ( (x < DSP_INPUTS) & (y < DSP_OUTPUTS) )
                p->fb[DSP_INPUTS + y].mixer[x] *= val;
            //printf("mixer %2d-%2d = %f\n",x,y,val);
        }
        if (miniParseSimpleValue(delay, &x, &y, &val)){ // Delay_5_0 : x=5..12
            if (x>=5) {
                x -= 5;
                if ( x < DSP_OUTPUTS ) p->fb[DSP_OUTPUTS + x].delayMicroSec = val*10.0;}
            //printf("delay %2d-%2d = %f\n",x,y,val);
        }
        if (miniParseSimpleValue(polarity_in, &x, &y, &val)){ // polarity_in_1_0 : x = 0..3
            if ( x < DSP_INPUTS ) p->fb[x].invert = val;
            //printf("polarity %2d-%2d = %f\n",x,y,val);
        }
        if (miniParseSimpleValue(polarity_out, &x, &y, &val)){ // polarity_out_1_4 : x = 4..11
            if (x >= 4) {
                x -= 4;
                if (x < DSP_OUTPUTS) p->fb[DSP_OUTPUTS + x].invert = val; }
            //printf("polarity %2d-%2d = %f\n",x,y,val);
        }
        // x=1..12, y=1..10
        if (miniParseFilter(PEQ, &x, &y, &filter)) { // PEQ_1_1 : x=1..12, y=1..10
            if ((x <= DSP_INPUTS) && (y <= (filterBankSize[x-1]-2)))
                    p->fb[x-1].filter[y+1] = filter;    // start at 3rd filter bin, to leave space for crossover
            else if (x>=5) {
                x -= 5;
                if (( x < DSP_OUTPUTS ) && (y <= (filterBankSize[DSP_OUTPUTS + x]-2)))
                    p->fb[DSP_OUTPUTS+x].filter[y+1] = filter; }
            //printf("filter %2d-%2d f=%5.0f, Q=%8f, g=%5f, byp=%d, type=%s\n",x,y,filter.freq, filter.Q, filter.boost, filter.bypass, miniFilterNames[filter.type]);
        }
        if (miniParseFilter(BPF,&x,&y, &filter) ) { // x=1..12, y=1 or 5
            if (y == 5) y=2;
            if (y<=2) {
                 if (x>=5) {
                     x -= 5;
                     if ( x < DSP_OUTPUTS )
                         p->fb[DSP_OUTPUTS+x].filter[y-1] = filter; }
                //printf("filter %2d-%2d f=%5.0f, Q=%8f, g=%5f, byp=%d, type=%s\n",x,y,filter.freq, filter.Q, filter.boost, filter.bypass, miniFilterNames[filter.type]);
            }
        }
    }

return 0;
}

int miniParseFile(dspPreset_t * p, int preset, char * filename){
    int check = 0;
    char * xmlfirstline = "<setting version=";
    dspPresetReset(p, 0);
    if (dspOpenInputFile(filename)) {
        if (fgets(textFileLine, sizeof(textFileLine)-1, dspInputFile))
            if (strstr( textFileLine, xmlfirstline)) {
                check = 1;
                while (fgets(textFileLine, sizeof(textFileLine)-1, dspInputFile))
                    if (miniParseLine(p)) { check = 0; break; }
                if (check) {
                    dspPresetChecksumUpdate(p);
                    p->presetNumber = preset;}
            }
        fclose(dspInputFile);
    }
    return check;
}

//temporary buffer for storing data blocks for sigmadsp download
static sigmadspBuffer_t dspBuffer;

void dspWriteSPI(int addr, int num, int * ptr){
    // write "num" words (32bits) to an adress over SPI channel;
    for (int i=0; i<num; i++) {
        if ((i % 10)== 0) {
            if (i == 0)
                 printf("%5d [%2d] ",addr+i,num-i);
            else printf("%5d (%2d) ",addr+i,num-i); }
        printf("%8X ",*(ptr+i));
        if ((i % 10) == 9)printf("\n"); }
    if ((num % 10)!= 0)printf("\n");
}

// read the buffer and send the blocks of data to the DSP
void dspSendBufferParam(int * ptr){
    while (*ptr) {
        int addr = *ptr++;              // address in sigmadsp PARAM (DM0)
        int num  = *ptr++;              // number of 32 bits words to send
        dspWriteSPI(addr, num, ptr);    // call SPI routine
        ptr += num; }           // move to next block until NULL
}


// used to simplify the syntax.
#define leftin    0
#define rightin   1
#define out1  2
#define out2  3
#define out3  4

// create a preset in memory and then save it as a text file
void createPresets(dspPreset_t * p){
    //  { ftype, bypass, invert, unused, freq, Q, gain }
    dspFilter_t f1 = { FPEAK, 0,0,0, 125, 1.0, 2.0 };  // +6db at 125hz
    dspFilter_t f2 = { LPLR4, 0,0,0, 398 };            // low pass for out 1
    dspFilter_t f3 = { HPLR4, 0,0,0, 398 };            // high pass for out2
    dspFilter_t f4 = { LPLR4, 0,0,0, 2000 };           // low pass for out2
    dspFilter_t f5 = { FPEAK, 0,0,0, 1250, 3.0, 0.75 };  // -2.5dB at 1250hz with sharp Q=3
    dspFilter_t f6 = { HPLR4, 0,0,0, 2000 };           // high pass for out3

    for (int i=0; i<4; i++)
        dspPresetReset(&p[i], i+1);    // reset the preset and set them to number "i+1"

    dspPreset_t * preset = &p[0];      // pointer on the firt preset

    preset->fb[leftin].gain = 0.5;             // -6db by default as a preconditioning gain.
    preset->fb[leftin].delayMicroSec = 0.0;    // no need for delaying inputs here
    dspPresetSetFilter(preset, leftin,  0, f1);

    preset->fb[rightin] = preset->fb[leftin]; // same config as left, but this overwrite the mixer so:
    preset->fb[rightin].mixer[leftin]  = 0.0;  // correcting mixer
    preset->fb[rightin].mixer[rightin] = 1.0;  //

    dspPresetSetFilter(preset, out1, 0, f2); // bass
    dspPresetSetFilter(preset, out2, 0, f3); // medium
    dspPresetSetFilter(preset, out2, 1, f4);
    dspPresetSetFilter(preset, out2, 2, f5);
    dspPresetSetFilter(preset, out3, 0, f6); //treble
    preset->fb[out3].delayMicroSec = 0.1/340.0;  // delayed by 10cm for sound speed 340m/s

    p[1] = p[0];        // copy this preset in the next one
    preset = &p[1];
    preset->presetNumber = 2;               // because it was overwritten by the above copy
    // change LR4 by LR6, other mean of directly accessing the table
    preset->fb[out1].filter[0].ftype = LPLR6;
    preset->fb[out2].filter[0].ftype = HPLR6;

    p[2] = p[0];        // copy this preset in the next one
    preset = &p[2];
    preset->presetNumber = 3;               // because it was overwritten by the above copy
    // remove peq on inputs
    dspPresetSetFilter(preset, leftin,   0, dspFilterNone);
    dspPresetSetFilter(preset, rightin,  0, dspFilterNone);

    p[3] = p[1];        // copy this preset in the next one
    preset = &p[3];
    preset->presetNumber = 4;               // because it was overwritten by the above copy
    // remove peq on inputs
    dspPresetSetFilter(preset, leftin,   0, dspFilterNone);
    dspPresetSetFilter(preset, rightin,  0, dspFilterNone);
}

// storage for presets
#define maxPreset 10
dspPreset_t tablePreset[maxPreset];

//storage for frequency bins (geometrical)
#define numberOfFreq 41
dspBode_t bode[numberOfFreq];

int main(int argc, char **argv) {

    printf("SIGMADSP Library. %d Inputs x %d filters , %d Outputs x %d filters\n",
            DSP_INPUTS, filterBankSize[0], DSP_OUTPUTS, filterBankSize[DSP_INPUTS]);

    //miniParseFile(&tablePreset[0], 1,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting1.xml");
    //miniParseFile(&tablePreset[0],2,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting2.xml");
    //miniParseFile(&tablePreset[0],3,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting3.xml");
    //miniParseFile(&tablePreset[0],4,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting4.xml");

    //dspPresetWriteTextfile(&tablePreset[0], 1, "testnanosharc.dsp");

    createPresets(&tablePreset[0]); // create aa basic 3way crossover in 4 version
    // save the table of 4 presets
    dspPresetWriteTextfile(&tablePreset[0], 4, "testpreset.dsp");

    dspPreset_t *pPreset = &tablePreset[0];
    // load presets
    int ok = dspPresetReadTextfile(pPreset, 10, "testpreset.dsp");

    if (ok) {
        printf("ok, sigmadsp data dump:\n");
        DSP_FOR_ALL_CHANNELS(pPreset, ch) {
            dspPresetConvert(pPreset, ch , &dspBuffer[0], 192000.0, 1.0, 0.0 );
            dspSendBufferParam(&dspBuffer[0]);
        }
    }

    float min,max;
    dspBodeFreqInit(10.0, 100000.0, 1, &bode[0], numberOfFreq, 192000.0);
    printf("filter response input1:\n");
    dspBodeResponseInit(&bode[0], numberOfFreq, 1.0);
    dspBodeApplyFilterBank(&bode[0], numberOfFreq, &tablePreset[0].fb[0]);
    dspBodeMagnitudeDB(&bode[0], numberOfFreq, 1.0, &min, &max);
    dspBodePhase(&bode[0], numberOfFreq);
    printf("min = %fdB, max = %fdB\n",min,max);
    for (int i=0; i<numberOfFreq; i++)
        printf("%2d %5.0f %5.2fdB %4d°\n",i+1, bode[i].freq,bode[i].mag,(int)(bode[i].phase+0.5));

    printf("crossover response input1 + output1:\n");
    dspBodeResponseInit(&bode[0], numberOfFreq, 1.0);
    dspBodeApplyFilterBank(&bode[0], numberOfFreq, &tablePreset[0].fb[0]);  // apply input filters
    dspBodeApplyFilterBank(&bode[0], numberOfFreq, &tablePreset[0].fb[2]);  // apply output 1 filters
    dspBodeMagnitudeDB(&bode[0], numberOfFreq, 1.0, &min, &max);
    dspBodePhase(&bode[0], numberOfFreq);
    printf("min = %fdB, max = %fdB\n",min,max);
    for (int i=0; i<numberOfFreq; i++)
        printf("%2d %5.0f %5.2fdB %4d°\n",i+1, bode[i].freq,bode[i].mag,(int)(bode[i].phase+0.5));

return ok;
}