/*
 * sigmadspfilter.c
 *
 *  Created on: 14 may 2020
 *      Author: fabriceo
 *
 */

#include <stdio.h>

#include "sigmadsppresets.h" // this also include the sigmastudio generic file and filters


// default template for no filter
const dspFilter_t dspFilterNone = { FNONE, 0,0,0, 0.0, 0.0, 0.0 };

const dspBiquadCoefs_t dspBiquadNone = { 1.0, 0.0, 0.0, 0.0, 0.0 };

// calculate full preset checksum
int dspPresetChecksumCompute(dspPreset_t * p){
    int sum = 0;
    int * pint = (int*)&p->fb;
    for (int i=0; i< (sizeof(p->fb)/sizeof(int)); i++) sum += *pint++;
    if (sum == 0) sum = -1;
    return sum;
}

void dspPresetChecksumUpdate(dspPreset_t * p){
    p->checksum = dspPresetChecksumCompute(p);
}

int dspPresetChecksumVerify(dspPreset_t * p){
    int sum = dspPresetChecksumCompute(p);
    return (sum == p->checksum);
}

static void dspPresetResetAllFilterBank(dspFilterBlock_t * fb, int chanMax) {
    for (int i=0; i<chanMax; i++) {
        fb->numberOfFilters = filterBankSize[i];
        for (int j=0; j < DSP_INPUTS; j++) fb->mixer[j] = (i==j) ? 1.0 : 0.0;
        for (int j=0; j < FILTER_BANK_SIZE; j++)  fb->filter[j] = dspFilterNone;
        fb->gain = 1.0;
        fb->mute = 0;
        fb->invert = 0;
        fb->delayMicroSec = 0.0;
        fb++; }
}

void dspPresetReset(dspPreset_t * p, int numPreset){
    p->presetNumber      = numPreset;
    p->filterBankSize    = FILTER_BANK_SIZE;
    p->numberOfInputs    = DSP_INPUTS;
    p->numberOfOutputs   = DSP_OUTPUTS;
    dspPresetResetAllFilterBank( &p->fb[0],  DSP_IO_TOTAL);
    dspPresetChecksumUpdate(p);
}

void dspPresetSetFilter(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter){
    if (channel < DSP_IO_TOTAL)
        if (numFilter < p->fb[channel].numberOfFilters)
            p->fb[channel].filter[numFilter] = filter;
}


int dspFilterNeedQ(enum filterTypes ftype){
    if (ftype < FLP1) return 0;
    const char dspFilterQrequired[] = { // only for filters starting at LP1
            0,1,0,1,0,1,1,1,
            0,1,0,1,1,1  };
    return dspFilterQrequired[ ftype - FLP1 ];
}

int dspFilterNeedGain(enum filterTypes ftype){
    if (ftype < FBP0DB) return 0;
    return 1;
}


// go through the list of filters and ensure that any FNONE is at the end of the list
void dspPresetMoveNoneFilters(dspPreset_t * p, int ch){
    int numFilters = p->fb[ch].numberOfFilters;
    dspFilter_t * f = &p->fb[ch].filter[0];
    for (int i=0; i < (numFilters-1); i++)
        if (f[i].ftype == FNONE) { // park it at the end and shift the table to the left
            for (int j=i; j < (numFilters-1); j++) f[j] = f[j+1];
            f[numFilters-1] = dspFilterNone; }
}
// cleanup the filter bank, by reordering filters by growing frequency
void dspPresetSortFilters(dspPreset_t * p, int ch){
    dspPresetMoveNoneFilters(p, ch);
    dspFilter_t * f = &p->fb[ch].filter[0];
    int numFilters = p->fb[ch].numberOfFilters;
    int en_desordre = 1;
    for (int i = 0; i < numFilters && en_desordre; ++i) {
        en_desordre = 0;
        for (int j = 1; j < p->filterBankSize - i; ++j) {
            if (f[j-1].freq > f[j].freq) {
                dspFilter_t temp = f[j-1];
                f[j-1] = f[j];
                f[j] = temp;
                en_desordre = 1; }
        }
    }
}

int dspPresetCalcCellsUsed(dspPreset_t * p, int ch) {
    int numCells = 0;
    for (int i=0; i < p->fb[ch].numberOfFilters; i++)
        numCells += dspFilterCells[ p->fb[ch].filter[i].ftype ];
    return numCells;
}

// generate a table of index pointing on the filter used according to the filter required cells
// eg. if filter[0].ftype = LPBU2 and filter[1].ftype = LPBE3, table = { 0+1,1+1,1+1, 0,0,0,0,0,0,0... }
int dspPresetCalcTableCellsUsed(dspPreset_t * p, int ch, char * table) {
    dspPresetMoveNoneFilters(p, ch);
    int numCells;
    int pos = 0;
    for (int i=0; i < p->filterBankSize; i++) {
        numCells = dspFilterCells[ p->fb[ch].filter[i].ftype ];
        if ((pos+numCells) <= p->filterBankSize)
            for (int j=0; j<numCells; j++)  table[pos+j] = i;
        pos += numCells;
    }
    for (int i=pos; i< p->filterBankSize; i++) table[i] = p->filterBankSize; // indicate the space is free
    return pos;
}

// to be called for each channels (inputs then outputs) pointing on an array for getting data to download to sigmadsp
int dspPresetConvert(dspPreset_t * p, int channel, int * dest, dspFloat_t fs, dspFloat_t gainMul, dspFloat_t delayAdd) {
    int * initialDest = dest;
    dspFilterBlock_t * fb = &p->fb[channel];
    dspSamplingFreq = fs;

    // calculate mixer addres by checking if the channel is in the input space or output space
    int mixerAddr;
    if (channel < DSP_INPUTS) mixerAddr = mixerInAddr + channel * DSP_INPUTS;
    else mixerAddr = mixerOutAddr + (channel- DSP_INPUTS) * DSP_INPUTS;

    // generate information to download mixer value for each
    *dest++ = mixerAddr;
    *dest++ = DSP_INPUTS; // number of value for the mixer
    for (int i=0; i<DSP_INPUTS; i++) *dest++ = dspQ8_24(fb->mixer[i]);

    // generate information for the gain
    int gainAddr;
    if (channel < DSP_INPUTS) gainAddr = gainInAddr + channel;
    else gainAddr = gainOutAddr + channel- DSP_INPUTS;
    *dest++ = gainAddr;
    *dest++ = 1;    // 1 word is following
    if (fb->mute) gainMul = 0.0;
    if (fb->invert) gainMul = -gainMul;
    *dest++ = dspQ8_24(fb->gain * gainMul);

    //generate information for the delay
    *dest++ = delayOutAddr + channel;
    *dest++ = 1;    // 1 word is following
    dspFloat_t delay = fb->delayMicroSec + delayAdd;
    *dest++ = fs / 1000000.0 * delay;   // this gives a number of samples at "fs"

    //generate filters
    *dest++ = filterBankAddr[channel];
    *dest++ = fb->numberOfFilters * 5;
    int numCells = 0;
    dspFloat_t * coefs;
    for (int i=0; i < fb->numberOfFilters; i++) {
        tempBiquadIndex = 0;
        dspFilter_t * f = &fb->filter[i];
        int cells = 0;
        if ((f->ftype != FNONE) && (f->bypass == 0)) {
            cells = dsp_filter(f->ftype, f->freq, f->Q, f->gain);
            coefs = &tempBiquad[0].b0;

            if ((cells + numCells) <= fb->numberOfFilters) {
                if (f->invert) {
                    *coefs = - *coefs; coefs++;
                    *coefs = - *coefs; coefs++;
                    *coefs = - *coefs; coefs -= 2; }
                for (int j=0; j < cells; j++)
                    for (int k=0; k < 5; k++) *dest++ = dspQ8_24(*coefs++);
                numCells += cells; }
        }
    }
    if (numCells < fb->numberOfFilters) {
        tempBiquad[0] = dspBiquadNone;
        for (int i=numCells; i< fb->numberOfFilters; i++) {
            coefs = &tempBiquad[0].b0;
            for (int k=0; k < 5; k++) *dest++ = dspQ8_24(*coefs++); }
    }
    *dest++ = 0; // end of buffer
    return (int)(dest - initialDest);
}


