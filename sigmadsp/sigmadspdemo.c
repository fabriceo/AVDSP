
#include "sigmadsppresets.h"
#include "sigmadspresponse.h"
#include "sigmadspfiles.h"

#include <stdio.h>

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
