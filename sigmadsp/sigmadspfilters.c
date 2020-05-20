/*
 * sigmadspfilter.c
 *
 *  Created on: 14 may 2020
 *      Author: fabriceo
 *
 */

#include <stdio.h>

#include "sigmadspfilters.h" // this also include the sigmastudio generic file

// the below VALUE are expected to be present in the generic sigmastudio_PARAM.h file
static const dsp32_t mixerInAddr  = DSP_MIXER_IN_ADDR(0);
static const dsp32_t mixerOutAddr = DSP_MIXER_OUT_ADDR(0);
static const dsp32_t gainInAddr   = DSP_GAIN_IN_ADDR;
static const dsp32_t gainOutAddr  = DSP_GAIN_OUT_ADDR;
static const dsp32_t delayOutAddr    = DSP_DELAY_OUT_ADDR;

// inventory of the number of generic filter banks with name FILTERX when X is between 1 and 16 max in this version
static const dsp32_t filterBankAddr[16] = {
#if DEF_FILTER_ADDR(1)
        DSP_FILTER_ADDR(1),
#else
        0,
#endif
#if DEF_FILTER_ADDR(2)
        DSP_FILTER_ADDR(2),
#else
        0,
#endif
#if DEF_FILTER_ADDR(3)
        DSP_FILTER_ADDR(3),
#else
        0,
#endif
#if DEF_FILTER_ADDR(4)
        DSP_FILTER_ADDR(4),
#else
        0,
#endif
#if DEF_FILTER_ADDR(5)
        DSP_FILTER_ADDR(5),
#else
        0,
#endif
#if DEF_FILTER_ADDR(6)
        DSP_FILTER_ADDR(6),
#else
        0,
#endif
#if DEF_FILTER_ADDR(7)
        DSP_FILTER_ADDR(7),
#else
        0,
#endif
#if DEF_FILTER_ADDR(8)
        DSP_FILTER_ADDR(8),
#else
        0,
#endif
#if DEF_FILTER_ADDR(9)
        DSP_FILTER_ADDR(9),
#else
        0,
#endif
#if DEF_FILTER_ADDR(10)
        DSP_FILTER_ADDR(10),
#else
        0,
#endif
#if DEF_FILTER_ADDR(11)
        DSP_FILTER_ADDR(11),
#else
        0,
#endif
#if DEF_FILTER_ADDR(12)
        DSP_FILTER_ADDR(12),
#else
        0,
#endif
#if DEF_FILTER_ADDR(13)
        DSP_FILTER_ADDR(13),
#else
        0,
#endif
#if DEF_FILTER_ADDR(14)
        DSP_FILTER_ADDR(14),
#else
        0,
#endif
#if DEF_FILTER_ADDR(15)
        DSP_FILTER_ADDR(15),
#else
        0,
#endif
#if DEF_FILTER_ADDR(16)
        DSP_FILTER_ADDR(16)
#else
        0
#endif
};

const int filterBankSize[16] = {
#if DEF_FILTER_COUNT(1)
        DSP_FILTER_COUNT(1) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(2)
        DSP_FILTER_COUNT(2) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(3)
        DSP_FILTER_COUNT(3) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(4)
        DSP_FILTER_COUNT(4) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(5)
        DSP_FILTER_COUNT(5) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(6)
        DSP_FILTER_COUNT(6) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(7)
        DSP_FILTER_COUNT(7) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(8)
        DSP_FILTER_COUNT(8) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(9)
        DSP_FILTER_COUNT(9) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(10)
        DSP_FILTER_COUNT(10) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(11)
        DSP_FILTER_COUNT(11) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(12)
        DSP_FILTER_COUNT(12) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(13)
        DSP_FILTER_COUNT(13) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(14)
        DSP_FILTER_COUNT(14) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(15)
        DSP_FILTER_COUNT(15) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(16)
        DSP_FILTER_COUNT(16) / 5
#else
        0
#endif
};


// inventory of the RMS algorythms found in the generic sigmastudio file, if any!
#if DEF_RMS_ADDR(1)
const dsp32_t rmsAddr[8] = {
        DSP_RMS_ADDR(1),

#if DEF_RMS_ADDR(2)
        DSP_RMS_ADDR(2),
#else
        0,
#endif
#if DEF_RMS_ADDR(3)
        DSP_RMS_ADDR(3),
#else
        0,
#endif
#if DEF_RMS_ADDR(4)
        DSP_RMS_ADDR(4),
#else
        0,
#endif
#if DEF_RMS_ADDR(5)
        DSP_RMS_ADDR(5),
#else
        0,
#endif
#if DEF_RMS_ADDR(6)
        DSP_RMS_ADDR(6),
#else
        0,
#endif
#if DEF_RMS_ADDR(7)
        DSP_RMS_ADDR(7),
#else
        0,
#endif
#if DEF_RMS_ADDR(8)
        DSP_RMS_ADDR(8)
#else
        0
#endif
};
#endif

const char * dspFilterNames[] = { "NONE ",
  "LPBE1","LPBE2","LPBE3","LPBE4","LPBE5","LPBE6","LPBE7","LPBE8",
  "HPBE1","HPBE2","HPBE3","HPBE4","HPBE5","HPBE6","HPBE7","HPBE8",
  "LPBe1","LPBe2","LPBe3","LPBe4","LPBe5","LPBe6","LPBe7","LPBe8",
  "HPBe1","HPBe2","HPBe3","HPBe4","HPBe5","HPBe6","HPBe7","HPBe8",
  "LPBU1","LPBU2","LPBU3","LPBU4","LPBU5","LPBU6","LPBU7","LPBU8",
  "HPBU1","HPBU2","HPBU3","HPBU4","HPBU5","HPBU6","HPBU7","HPBU8",
  "LPLR1","LPLR2","LPLR3","LPLR4","LPLR5","LPLR6","LPLR7","LPLR8",
  "HPLR1","HPLR2","HPLR3","HPLR4","HPLR5","HPLR6","HPLR7","HPLR8",
  "LP1  ","LP2  ","HP1  ","HP2  ","AllP1","AllP2","BP0dB","BPQ  ",
  "LSh1 ","LSh2 ","HSh1 ","HSh2 ","PEQ  ","NOTCH", };

// number of biquad cells used by a filter
const char dspFilterCells[] = { 0,
        1,1,2,2,0,3,0,4,    // 0 means unsupported
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1 };


// default template for no filter
const dspFilter_t dspFilterNone = { FNONE, 0,0,0, 0.0, 0.0, 0.0 };

const dspBiquadCoefs_t dspBiquadNone = { 1.0, 0.0, 0.0, 0.0, 0.0 };

enum filterTypes dspFilterNameSearch(char *s){
    int n = sizeof(dspFilterNames)/sizeof(dspFilterNames[0]);
    for (int i=0; i<n; i++) {
        char * p = s;
        char * q = (char *)dspFilterNames[i];
        while (*p == *q) { p++; q++; }
        if (*q == 0) return i;
    }
    return FNONE;
}

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

#define tempBiquadMax 4
static dspBiquadCoefs_t tempBiquad[tempBiquadMax];         // temporary array of max 4 biquad cell
int tempBiquadIndex = 0;
static dspFloat_t dspSamplingFreq = 192000.0;

//prototype
int dsp_filter(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain);

// to be called for each channels (inputs then outputs) pointing on an array for getting data to download to sigmadsp
int dspPresetConvert(dspPreset_t * p, int channel, dsp32_t * dest, dspFloat_t fs, dspFloat_t gainMul, dspFloat_t delayAdd) {
    dsp32_t * initialDest = dest;
    dspFilterBlock_t * fb = &p->fb[channel];
    dspSamplingFreq = fs;

    // calculate mixer addres by checking if the channel is in the input space or output space
    dsp32_t mixerAddr;
    if (channel < DSP_INPUTS) mixerAddr = mixerInAddr + channel * DSP_INPUTS;
    else mixerAddr = mixerOutAddr + (channel- DSP_INPUTS) * DSP_INPUTS;

    // generate information to download mixer value for each
    *dest++ = mixerAddr;
    *dest++ = DSP_INPUTS; // number of value for the mixer
    for (int i=0; i<DSP_INPUTS; i++) *dest++ = dspQ8_24(fb->mixer[i]);

    // generate information for the gain
    dsp32_t gainAddr;
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



// calc the biquad coefficient for a given filter type.
void dspFilter1stOrder( int type,
        dspFilterParam_t fs,
        dspFilterParam_t freq,
        dspGainParam_t   gain,
        dspFilterParam_t * b0,
        dspFilterParam_t * b1,
        dspFilterParam_t * b2,
        dspFilterParam_t * a1,
        dspFilterParam_t * a2
 )
{
    if (type == FNONE) {
        *b0 = 1.0; *b1 = 0.0; *b2 = 0.0; *a1 = 0.0; *a2 = 0.0; return;
    }
    dspFilterParam_t a0, w0, tw2, alpha;
    w0 = M_PI * 2.0 * freq / fs;
    tw2 = tan(w0/2.0);
    a0 = gain;
    *a2 = 0;
    *b2 = 0;
    switch (type) {
    case FLP1: {
        alpha = 1.0 + tw2;
        *a1 = ((1.0-tw2)/alpha) / a0;
        *b0 = tw2/alpha / a0;
        *b1 = *b0;
    break; }
    case FHP1: {
        alpha = 1.0 + tw2;
        *a1 = ((1.0-tw2)/alpha) /a0;
        *b0 =  1.0/alpha / a0;
        *b1 = -1.0/alpha / a0;
    break; }
#if 0
    case FLP1: {
        alpha = (1.0-tw2)/(1.0+tw2);
        *b0 = (1.0-alpha)/2.0;
        *b1 = *b0;
        *a1 = alpha;
    break; }
    case FHP1: {
        alpha = (1.0-tw2)/(1.0+tw2);
        *b0 = 1.0/(1.0+tw2);
        *b1 = -*b0;
        *a1 = alpha;
    break; }
#endif
    case FHS1: {
        dspFilterParam_t A = sqrt(gain);
        a0 = A*tw2+1.0;
        *a1 = -(A*tw2-1.0)/a0;
        *b0 = (A*tw2+gain)/a0;
        *b1 = (A*tw2-gain)/a0;
        break; }
    case FLS1: {
        dspFilterParam_t A = sqrt(gain);
        a0 = tw2+A;
        *a1 = -(tw2-A)/a0;
        *b0 = (gain*tw2+A)/a0;
        *b1 = (gain*tw2-A)/a0;
        break; }
    case FAP1: {
        break;
    }
    } //switch

}

// calc the biquad coefficient for a given filter type
void dspFilter2ndOrder( int type,
        dspFilterParam_t fs,
        dspFilterParam_t freq,
        dspFilterParam_t Q,
        dspGainParam_t   gain,
        dspFilterParam_t * b0,
        dspFilterParam_t * b1,
        dspFilterParam_t * b2,
        dspFilterParam_t * a1,
        dspFilterParam_t * a2
 )
{
    if (type == FNONE) {
        *b0 = 1.0; *b1 = 0.0; *b2 = 0.0; *a1 = 0.0; *a2 = 0.0; return;
    }
    dspFilterParam_t a0, w0, cw0, sw0, tw2, alpha;
    w0 = M_PI * 2.0 * freq / fs;
    cw0 = cos(w0);
    sw0 = sin(w0);
    tw2 = tan(w0/2.0);
    if (Q != 0.0) alpha = sw0 / 2.0 / Q; else alpha = 1;
    a0 = (1.0 + alpha);
    *a1 = -(-2.0 * cw0) / a0;       // sign is changed to accomodate convention
    *a2 = -(1.0 - alpha ) / a0;     // and coeficients are normalized vs a0
    switch (type) {
    case FLP2: {
        *b1 = (1.0 - cw0) / a0 * gain;
        *b0 = *b1 / 2.0;
        *b2 = *b0;
        break; }
    case FHP2: {
        *b1 = -(1.0 + cw0) / a0 * gain;
        *b0 = - *b1 / 2.0;
        *b2 = *b0;
        break; }
    case FAP2: {
        *b0 = -*a2 * gain;
        *b1 = -*a1 * gain;
        *b2 =  gain;
        break; }
    case FNOTCH: {
        *b0 = 1.0 / a0 * gain;
        *b1 = -*a1 * gain;
        *b2 = *b0;
        break; }
    case FBPQ : { // peak gain = Q
        *b0= sw0/2.0 / a0;
        *b1 = 0;
        *b2 = -sw0/2.0 / a0;
        break; }
    case FBP0DB : { // 0DB peak gain
        *b0= alpha / a0;
        *b1 = 0;
        *b2 = -alpha / a0;
    break; }
    case FPEAK: {
        dspFilterParam_t A = sqrt(gain);
        a0 = 1.0 + alpha / A;
        *a1 = 2.0 * cw0 / a0;
        *a2 = -(1.0 - alpha / A ) / a0;
        *b0 = (1.0 + alpha * A) / a0;
        *b1 = -2.0 * cw0 / a0;
        *b2 = (1.0 - alpha * A) / a0;
        break; }
    case FLS2: {
        dspFilterParam_t A = sqrt(gain);
        dspFilterParam_t sqA = sqrt( A );
        a0 = ( A + 1.0) + ( A - 1.0 ) * cw0 + 2.0 * sqA * alpha;
        *a1 = -(-2.0 *( (A-1.0) + (A+1.0) * cw0 ) ) / a0;
        *a2 = -( (A + 1.0) + (A - 1.0) * cw0 - 2.0 * sqA * alpha ) / a0;
        *b0 = ( A * ( ( A + 1.0) - ( A - 1.0) * cw0 + 2.0 * sqA * alpha ) ) / a0;
        *b1 = ( 2.0 * A * ( ( A - 1.0 ) - ( A + 1.0 ) * cw0 ) ) / a0;
        *b2 = ( A * ( ( A + 1.0 ) - ( A - 1.0 ) * cw0 - 2.0 * sqA * alpha )) / a0;;
        break; }
    case FHS2: {
        dspFilterParam_t A = sqrt(gain);
        dspFilterParam_t sqA = sqrt( A );
        a0 = ( A + 1.0) - ( A - 1.0 ) * cw0 + 2.0 * sqA * alpha;
        *a1 = -(2.0 *( (A-1.0) - (A+1.0) * cw0 ) ) / a0;
        *a2 = -( (A + 1.0) - (A - 1.0) * cw0 - 2.0 * sqA * alpha ) / a0;
        *b0 = ( A * ( ( A + 1.0) + ( A - 1.0) * cw0 + 2.0 * sqA * alpha ) ) / a0;
        *b1 = ( -2.0 * A * ( ( A - 1.0 ) + ( A + 1.0 ) * cw0 ) ) / a0;
        *b2 = ( A * ( ( A + 1.0 ) + ( A - 1.0 ) * cw0 - 2.0 * sqA * alpha )) / a0;
        break; }
    } //switch
}




int dsp_Filter2ndOrder(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain){

    dspBiquadCoefs_t coefs;

    dspFilter2ndOrder(type, dspSamplingFreq, freq, Q, gain, &coefs.b0, &coefs.b1, &coefs.b2, &coefs.a1, &coefs.a2);

    if (tempBiquadIndex < tempBiquadMax)
        tempBiquad[tempBiquadIndex++] = coefs;

return 1;
}

int dsp_Filter1stOrder(int type, dspFilterParam_t freq, dspGainParam_t gain){

    dspBiquadCoefs_t coefs;

    dspFilter1stOrder(type, dspSamplingFreq, freq, gain, &coefs.b0, &coefs.b1, &coefs.b2, &coefs.a1, &coefs.a2);

    if (tempBiquadIndex < tempBiquadMax)
        tempBiquad[tempBiquadIndex++] = coefs;

    return 1;
}


int dsp_LP_BES2(dspFilterParam_t freq) {   // low pass cutoff freq is in phase with high pass cutoff
    return dsp_Filter2ndOrder(FLP2, freq, 0.57735026919 , 1.0); }

int dsp_LP_BES2_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with high pass
    return dsp_LP_BES2(freq * 1.27201964951);
}

int dsp_HP_BES2(dspFilterParam_t freq) {   // high pass cutoff freq is in phase with low pass cutoff
    return dsp_Filter2ndOrder(FHP2, freq, 0.57735026919 , 1.0); }

int dsp_HP_BES2_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    return dsp_HP_BES2(freq / 1.27201964951);
}

int  dsp_LP_BUT2(dspFilterParam_t freq) {
    return dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0); }

int dsp_HP_BUT2(dspFilterParam_t freq) {
    return dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
}

int dsp_LP_LR2(dspFilterParam_t freq) { // -6db cutoff at fc ?
    return dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0); }

int dsp_HP_LR2(dspFilterParam_t freq) { // -6db cutoff ? 180 deg phase shift vs low pass
    return dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0); }



int dsp_LP_BES3(dspFilterParam_t freq) {   // low pass cutoff freq is in phase with high pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.941600026533, 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq * 1.03054454544, 1.0);
    return tmp;
}

int dsp_LP_BES3_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with high pass
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.32267579991, 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq * 1.44761713315,  1.0);
    return tmp;
}

int dsp_HP_BES3(dspFilterParam_t freq) {   // high pass cutoff freq is in phase with low pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.941600026533 , 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq / 1.03054454544,    1.0);
    return tmp;
}

int dsp_HP_BES3_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.32267579991 , 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq / 1.44761713315,   1.0);
    return tmp;
}

int  dsp_LP_BUT3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq,  1.0);
    return tmp;
}

int dsp_HP_BUT3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq,  1.0);
    return tmp;
}

int dsp_LP_LR3(dspFilterParam_t freq) { // -6db cutoff at fc ?
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq,  1.0);
    return tmp;
}

int dsp_HP_LR3(dspFilterParam_t freq) { // -6db cutoff ? 180 deg phase shift vs low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq,  1.0);
    return tmp;
}



int dsp_LP_BES4(dspFilterParam_t freq) {   // low pass cutoff freq is IN PHASE with high pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.944449808226 , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.05881751607  , 0.805538281842 , 1.0);
    return tmp;
}

int dsp_LP_BES4_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.43017155999  , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.60335751622  , 0.805538281842 , 1.0);
    return tmp;
}
int dsp_HP_BES4(dspFilterParam_t freq) {       // high pass cutoff freq is in phase with low pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.944449808226 , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.05881751607  , 0.805538281842 , 1.0);
    return tmp;
}

int dsp_HP_BES4_3DB(dspFilterParam_t freq) {       // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.43017155999  , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.60335751622  , 0.805538281842 , 1.0);
    return tmp;
}

int dsp_LP_BUT4(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.54119610 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.3065630 , 1.0);
    return tmp;
}

int dsp_HP_BUT4(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.54119610 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.3065630 , 1.0);
    return tmp;
}

int dsp_LP_LR4(dspFilterParam_t freq) {   // -6db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    return tmp;
}

int dsp_HP_LR4(dspFilterParam_t freq) {   // -6db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    return tmp;
}

int dsp_LP_BES6(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.928156550439 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 0.977488555538 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.10221694805  , 1.02331395383 , 1.0);
    return tmp;
}

int dsp_LP_BES6_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.60391912877 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.68916826762 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.9047076123  , 1.02331395383  , 1.0);
    return tmp;
}

int dsp_HP_BES6(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.928156550439 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 0.977488555538 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.10221694805  , 1.02331395383  , 1.0);
    return tmp;
}

int dsp_HP_BES6_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.60391912877 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.68916826762 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.9047076123  , 1.02331395383  , 1.0);
    return tmp;
}

int dsp_LP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.51763809 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2  , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.9318517  , 1.0);
    return tmp;
}

int dsp_HP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.51763809 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2  , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.9318517  , 1.0);
    return tmp;
}

int dsp_LP_LR6(dspFilterParam_t freq) {   // TODO  Q ?
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    return tmp;
}

int dsp_HP_LR6(dspFilterParam_t freq) {   // ?
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    return tmp;
}

int dsp_LP_BES8(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.920583104484 , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 0.948341760923 , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.01102810214  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.13294518316  , 1.22566942541 , 1.0);
    return tmp;
}

int dsp_LP_BES8_3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.77846591177  , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.8320926012   , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.95319575902  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 2.18872623053  , 1.22566942541  , 1.0);
    return tmp;
}

int dsp_HP_BES8(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.920583104484 , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 0.948341760923 , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.01102810214  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.13294518316  , 1.22566942541  , 1.0);
    return tmp;
}

int dsp_HP_BES8_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.77846591177  , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.8320926012   , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.95319575902  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 2.18872623053  , 1.22566942541  , 1.0);
    return tmp;
}

int dsp_LP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.50979558 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 0.60134489 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 0.89997622 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 2.5629154  , 1.0);
    return tmp;
}

int dsp_HP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.50979558 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 0.60134489 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 0.89997622 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 2.5629154  , 1.0);
    return tmp;
}

int dsp_LP_LR8(dspFilterParam_t freq) {   // -6db cutoff ?
    int tmp =
    dsp_LP_BUT4(freq);
    tmp += dsp_LP_BUT4(freq);
    return tmp;
}

int dsp_HP_LR8(dspFilterParam_t freq) {   // -6db cutoff ?
    int tmp =
    dsp_HP_BUT4(freq);
    tmp += dsp_HP_BUT4(freq);
    return tmp;
}



int dsp_filter(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain) {
    int tmp=0;
    switch (type) {
    case FNONE:  tmp = dsp_Filter2ndOrder(FNONE,0,0,0); break;
    case LPBE2 : tmp = dsp_LP_BES2(freq); break;
    case LPBE3 : tmp = dsp_LP_BES3(freq); break;
    case LPBE4 : tmp = dsp_LP_BES4(freq); break;
    case LPBE6 : tmp = dsp_LP_BES6(freq); break;
    case LPBE8 : tmp = dsp_LP_BES8(freq); break;
    case HPBE2 : tmp = dsp_HP_BES2(freq); break;
    case HPBE3 : tmp = dsp_HP_BES3(freq); break;
    case HPBE4 : tmp = dsp_HP_BES4(freq); break;
    case HPBE6 : tmp = dsp_HP_BES6(freq); break;
    case HPBE8 : tmp = dsp_HP_BES8(freq); break;
    case LPBE3db2 : tmp = dsp_LP_BES2(freq); break;
    case LPBE3db3 : tmp = dsp_LP_BES3(freq); break;
    case LPBE3db4 : tmp = dsp_LP_BES4(freq); break;
    case LPBE3db6 : tmp = dsp_LP_BES6(freq); break;
    case LPBE3db8 : tmp = dsp_LP_BES8(freq); break;
    case HPBE3db2 : tmp = dsp_HP_BES2(freq); break;
    case HPBE3db3 : tmp = dsp_HP_BES3(freq); break;
    case HPBE3db4 : tmp = dsp_HP_BES4(freq); break;
    case HPBE3db6 : tmp = dsp_HP_BES6(freq); break;
    case HPBE3db8 : tmp = dsp_HP_BES8(freq); break;
    case LPBU2 : tmp = dsp_LP_BUT2(freq); break;
    case LPBU3 : tmp = dsp_LP_BUT3(freq); break;
    case LPBU4 : tmp = dsp_LP_BUT4(freq); break;
    case LPBU6 : tmp = dsp_LP_BUT6(freq); break;
    case LPBU8 : tmp = dsp_LP_BUT8(freq); break;
    case HPBU2 : tmp = dsp_HP_BUT2(freq); break;
    case HPBU3 : tmp = dsp_HP_BUT3(freq); break;
    case HPBU4 : tmp = dsp_HP_BUT4(freq); break;
    case HPBU6 : tmp = dsp_HP_BUT6(freq); break;
    case HPBU8 : tmp = dsp_HP_BUT8(freq); break;
    case LPLR2 : tmp = dsp_LP_LR2(freq); break;
    case LPLR3 : tmp = dsp_LP_LR3(freq); break;
    case LPLR4 : tmp = dsp_LP_LR4(freq); break;
    case LPLR6 : tmp = dsp_LP_LR6(freq); break;
    case LPLR8 : tmp = dsp_LP_LR8(freq); break;
    case HPLR2 : tmp = dsp_HP_LR2(freq); break;
    case HPLR3 : tmp = dsp_HP_LR3(freq); break;
    case HPLR4 : tmp = dsp_HP_LR4(freq); break;
    case HPLR6 : tmp = dsp_HP_LR6(freq); break;
    case HPLR8 : tmp = dsp_HP_LR8(freq); break;
    case FLP2  : tmp = dsp_Filter2ndOrder(FLP2,freq,Q,gain); break;
    case FHP2  : tmp = dsp_Filter2ndOrder(FHP2,freq,Q,gain); break;
    case FLS2  : tmp = dsp_Filter2ndOrder(FLS2,freq,Q,gain); break;
    case FHS2  : tmp = dsp_Filter2ndOrder(FHS2,freq,Q,gain); break;
    case FAP2  : tmp = dsp_Filter2ndOrder(FAP2,freq,Q,gain); break;
    case FPEAK : tmp = dsp_Filter2ndOrder(FPEAK,freq,Q,gain); break;
    case FNOTCH: tmp = dsp_Filter2ndOrder(FNOTCH,freq,Q,gain); break;
    case FBP0DB: tmp = dsp_Filter2ndOrder(FBP0DB,freq,Q,gain); break;
    case FBPQ:   tmp = dsp_Filter2ndOrder(FBPQ,freq,Q,gain); break;
//first order
    case FLP1: tmp = dsp_Filter1stOrder(FLP1,freq, gain); break;
    case FHP1: tmp = dsp_Filter1stOrder(FHP1,freq, gain); break;
    case FLS1: tmp = dsp_Filter1stOrder(FLS1,freq, gain); break;
    case FHS1: tmp = dsp_Filter1stOrder(FHS1,freq, gain); break;
    case FAP1: tmp = dsp_Filter1stOrder(FAP1,freq, gain); break;
    default:
        tmp = dsp_Filter1stOrder(FNONE, 1.0, 1.0); break;
        //dspprintf("NOT SUPPORTED (type = %d)\n",type);

    }
    return tmp;
}

// some function to deal with complex number

dspComplex_t cOne  = { 1.0, 0.0 };
dspComplex_t cZero = { 0.0, 0.0 };

static dspComplex_t cComp(dspFloat_t re, dspFloat_t im){
    dspComplex_t c = { re, im};
    return c;
}

static dspFloat_t cMag(dspComplex_t a) {
    return sqrt( a.re*a.re + a.im*a.im );
}

static dspComplex_t cConj(dspComplex_t a) {
    a.im = - a.im;
    return a;
}

static dspComplex_t cAdd(dspComplex_t a,dspComplex_t b){
    a.re += b.re;
    a.im += b.im;
    return a;
}

static dspComplex_t cMul(dspComplex_t a, dspComplex_t b){
    dspComplex_t c;
    c.re = a.re*b.re - a.im*b.im;
    c.im = a.im*b.re + a.re*b.im;
    return c;
}
static dspComplex_t cMulReal(dspComplex_t a, float real){
    a.re *= real;
    a.im *= real;
    return a;
}


static dspFloat_t cArg(dspComplex_t a) {
    return atan2(a.im, a.re);
}


static dspComplex_t cDiv(dspComplex_t a, dspComplex_t b){
    return cMulReal( cMul( a, cConj(b) ), 1.0/(b.re*b.re + b.im*b.im) );
}

static dspComplex_t cExp(dspComplex_t a){
    dspComplex_t c;
    dspFloat_t ex = exp(a.re);
    c.re = ex * cos(a.im);
    c.im = ex * sin(a.im);
    return c;
}


// calculate the multiplier for producing a geometrical list of frequencies between start-stop
float dspFreqMultiplier(dspFloat_t start, dspFloat_t stop, int N){
    return exp(log(stop/start)/(N-1));
}

// initialize the table containing the response for further printing as a bode plot
void dspBodeResponseInit(dspBode_t * t, int N, dspFloat_t gain){
    for (int i=0; i< N; i++) {
        t[i].H = cComp( gain, 0);
        t[i].mag = 0.0;
        t[i].phase = 0.0; }
}

// initialize the table of frequencies with the geometrical list between start and stop
void dspBodeFreqInit(dspFloat_t start, dspFloat_t stop, int round, dspBode_t * t, int N, dspFloat_t fs){
    dspFloat_t mult = dspFreqMultiplier(start, stop, N);
    //printf("multiplier = %f\n",mult);
    dspSamplingFreq = fs;
    for (int i=0; i< N; i++) {
        dspFloat_t ff = start;
        if (round) {
            ff = (start + round/2)/round;
            int f = ff;
            f *= round;
            if (f == 0) f = 1;
            ff = f;}
        t[i].freq = ff;
        dspFloat_t w = 2 * M_PI * ff / fs;
        t[i].Z = cExp( cComp( 0, w ) );
        start *= mult; }
}


// apply a biquad on the response table to further print as a bode plot
void dspBodeApplyBiquad(dspBode_t * t, int N, dspBiquadCoefs_t * bq){
    //printf("bq : b0 %f, b1 %f, b2 %f, a1 %f, a2 %f\n",bq->b0,bq->b1,bq->b2,bq->a1,bq->a2);
    for (int i=0; i < N; i++) {
        dspComplex_t Z = t[i].Z;
        dspComplex_t Z2 = cMul( Z, Z);
        dspComplex_t cNum = cAdd( cComp( bq->b0, 0.0), cAdd( cMulReal( Z, bq->b1 ), cMulReal( Z2, bq->b2 ) ) );
        // coef a1 & a2 are negated due to filter calculation which invert the sign
        dspComplex_t cDen = cAdd( cOne, cAdd( cMulReal( Z, - bq->a1 ), cMulReal( Z2, - bq->a2 ) ) );
        dspComplex_t Hz = cDiv( cNum, cDen);
        t[i].H = cMul( t[i].H, Hz ); }
}

// apply a filter made of multiple biquad on the response table
void dspBodeApplyFilter(dspBode_t * t, int N, dspFilter_t * f){
    if (f->ftype == FNONE) return;
    tempBiquadIndex = 0;
    int cells = dsp_filter(f->ftype, f->freq, f->Q, f->gain);
    for (int i=0; i < cells; i++) {
        dspBodeApplyBiquad(t, N, &tempBiquad[i]);}
}

// apply all the bank filter on the response table
void dspBodeApplyFilterBank(dspBode_t * t, int N, dspFilterBlock_t * fb){
    for (int i=0; i < fb->numberOfFilters; i++)
        dspBodeApplyFilter(t, N, &fb->filter[i]);
}

// compute the magnitude of the response in deciBell. return also min and max
void dspBodeMagnitudeDB(dspBode_t * t, int N, dspFloat_t gain, dspFloat_t *minDB, dspFloat_t *maxDB){
    *minDB = 200.0; *maxDB = -200.0;
    dspFloat_t max = (1ULL<<32); dspFloat_t min = 1.0/max;
    for (int i=0; i < N; i++) {
        dspFloat_t mag = cMag( t[i].H) * gain;
        if (mag > max) mag=max;
        if (mag < min) mag=min;
        mag = 20.0 * log10(mag);
        t[i].mag = mag;
        if (mag>*maxDB) *maxDB=mag;
        if (mag<*minDB) *minDB=mag; }
}

void dspBodePhase(dspBode_t * t, int N){
    for (int i=0; i < N; i++) {
        dspFloat_t phase = cArg( t[i].H ) * 180.0/M_PI;
        t[i].phase = phase; }
}
