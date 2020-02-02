/*
 * dsp_coder.c
 *
 *  Created on: 1 janv. 2020
 *      Author: fabriceo
 *      this program will create a list of opcodes to be executed lated by the dsp engine
 */

#include "dsp_filters.h"      // enum dsp codes, typedefs and QNM definition

extern int dspMinSamplingFreq;  // from encoder.c
extern int dspMaxSamplingFreq;

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

    dspFilterParam_t a0, w0, cw0, sw0, alpha;
    w0 = M_PI * 2.0 * freq / fs;
    cw0 = cos(w0);
    sw0 = sin(w0);
    alpha = sw0 / 2.0 / Q;
    a0 = (1.0 + alpha);
    *a1 = -(-2.0 * cw0) / a0;
    *a2 = -(1.0 - alpha ) / a0;
    switch (type) {
    case FLP2: { // b0,b1,b2 are very small and this LP2 filter requires double precision biquad
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
    case FPEAK: {
        dspFilterParam_t A = sqrt(gain);
        a0 = 1.0 + alpha / A;
        *a2 = -(1.0 - alpha / A ) / a0;
        *b0 = (1.0 + alpha * A) / a0;
        *b1 = -*a1;
        *b2 = (1.0 - alpha * A) / a0;
        break; }
    case FLS2: {
        dspFilterParam_t A = sqrt(gain);
        dspFilterParam_t sqA = sqrt( A );
        a0 = ( A + 1.0) + ( A - 1.0 ) * cw0 + 2.0 * sqA * alpha;
        *a1 = -(-2.0 *( (A-1.0) + (A+1.0) * cw0 ) ) / a0;
        *a2 = -( (A + 1.0) + (A - 1.0) * cw0 - 2.0 * sqA * alpha ) / a0;
        *b0 = ( A * ( ( A + 1.0) - ( A - 1.0) * cw0 + 2.0 * sqA * alpha ) ) / a0 * gain;
        *b1 = ( 2.0 * A * ( ( A - 1.0 ) - ( A + 1.0 ) * cw0 ) ) / a0 * gain;
        *b2 = ( A * ( ( A + 1.0 ) - ( A - 1.0 ) * cw0 - 2.0 * sqA * alpha )) / a0 * gain;
        break; }
    case FHS2: {
        dspFilterParam_t A = sqrt(gain);
        dspFilterParam_t sqA = sqrt( A );
        a0 = ( A + 1.0) - ( A - 1.0 ) * cw0 + 2.0 * sqA * alpha;
        *a1 = -(2.0 *( (A-1.0) - (A+1.0) * cw0 ) ) / a0;
        *a2 = -( (A + 1.0) - (A - 1.0) * cw0 - 2.0 * sqA * alpha ) / a0;
        *b0 = ( A * ( ( A + 1.0) + ( A - 1.0) * cw0 + 2.0 * sqA * alpha ) ) / a0 * gain;
        *b1 = ( -2.0 * A * ( ( A - 1.0 ) + ( A + 1.0 ) * cw0 ) ) / a0 * gain;
        *b2 = ( A * ( ( A + 1.0 ) + ( A - 1.0 ) * cw0 - 2.0 * sqA * alpha )) / a0 * gain;
        break; }
    } //switch

}


// from des_encode.c
extern int  addBiquadCoeficients(dspFilterParam_t b0,dspFilterParam_t b1,dspFilterParam_t b2,dspFilterParam_t a1,dspFilterParam_t a2);
extern int addFilterParams(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain);
extern void sectionBiquadCoeficientsBegin();
extern void sectionBiquadCoeficientsEnd();


static const int dspTableFreq[FMAXpos] = {
        8000, 16000,
        24000, 32000,
        44100, 48000,
        88200, 96000,
        176400,192000,
        352800,384000,
        705600, 768000 };


int dsp_Filter2ndOrder(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain){
    int coefPtr = 0;
    dspFilterParam_t a1, a2, b0, b1, b2;
    sectionBiquadCoeficientsBegin();
    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        dspFilterParam_t fs = dspTableFreq[f];
        dspFilter2ndOrder(type, fs, freq, Q, gain, &b0, &b1, &b2, &a1, &a2);

        if (f == F44100) {
            dspprintf2("FILTER f = %f, Q = %f, G = %f\n", freq, Q, gain);
            dspprintf3(" b0 = %f, ",b0);
            dspprintf3(" b1 = %f,",b1);
            dspprintf3(" b2 = %f,",b2);
            dspprintf3(" a1 = %f, ",a1);
            dspprintf3(" a2 = %f\n",a2); }

        int tmp = addFilterParams(type, freq, Q, gain);
                  addBiquadCoeficients(b0, b1, b2, a1, a2);
        if (coefPtr==0) coefPtr = tmp;  // memorize first opcode adress
    }
    sectionBiquadCoeficientsEnd();
    return coefPtr;
}


int dsp_LP_BES2(dspFilterParam_t freq) {   // low pass cutoff freq is in phase with high pass cutoff
    return dsp_Filter2ndOrder(FLP2, freq, 0.57735026919 , 1.0); }

int dsp_LP_BES2_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with high pass
    return dsp_LP_BES2(freq * 1.27201964951);
}

int dsp_HP_BES2(dspFilterParam_t freq) {   // high pass cutoff freq is in phase with low pass cutoff
    return dsp_Filter2ndOrder(FHP2, freq, 0.57735026919 , 1.0); }

int dsp_HP_BES2_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    return dsp_HP_BES2(freq * 1.27201964951);  // DIVIDE ???
}

int  dsp_LP_BUT2(dspFilterParam_t freq) {
    return dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0); }

int dsp_HP_BUT2(dspFilterParam_t freq) {
    return dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0); }

int dsp_LP_LR2(dspFilterParam_t freq) { // -6db cutoff at fc ?
    return dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0); }

int dsp_HP_LR2(dspFilterParam_t freq) { // -6db cutoff ? 180 deg phase shift vs low pass
    return dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0); }

int dsp_LP_BES4(dspFilterParam_t freq) {   // low pass cutoff freq is IN PHASE with high pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.05881751607  , 0.805538281842 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.944449808226 , 0.521934581669 , 1.0);
    return tmp;
}

int dsp_LP_BES4_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.60335751622  , 0.805538281842 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.43017155999 , 0.521934581669 , 1.0);
    return tmp;
}
int dsp_HP_BES4(dspFilterParam_t freq) {       // high pass cutoff freq is in phase with low pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq * 1.05881751607  , 0.805538281842 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 0.944449808226 , 0.521934581669 , 1.0);
    return tmp;
}

int dsp_HP_BES4_3DB(dspFilterParam_t freq) {       // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq * 1.60335751622  , 0.805538281842 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.43017155999 , 0.521934581669 , 1.0);
    return tmp;
}

int dsp_LP_BUT4(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.54119610 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 1.3065630 , 1.0);
    return tmp;
}

int dsp_HP_BUT4(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.54119610 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 1.3065630 , 1.0);
    return tmp;
}

int dsp_LP_LR4(dspFilterParam_t freq) {   // -6db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    return tmp;
}

int dsp_HP_LR4(dspFilterParam_t freq) {   // -6db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    return tmp;
}

int dsp_LP_BES6(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.10221694805  , 1.02331395383 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.977488555538 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.928156550439 , 0.510317824749 , 1.0);
    return tmp;
}

int dsp_LP_BES6_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.9047076123  , 1.02331395383 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.68916826762 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.60391912877 , 0.510317824749 , 1.0);
    return tmp;
}

int dsp_HP_BES6(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq * 1.10221694805  , 1.02331395383 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 0.977488555538 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 0.928156550439 , 0.510317824749 , 1.0);
    return tmp;
}

int dsp_HP_BES6_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq * 1.9047076123  , 1.02331395383 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.68916826762 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.60391912877 , 0.510317824749 , 1.0);
    return tmp;
}

int dsp_LP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.51763809 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 1.9318517 , 1.0);
    return tmp;
}

int dsp_HP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.51763809 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 1.9318517 , 1.0);
    return tmp;
}

int dsp_LP_LR6(dspFilterParam_t freq) {   // TODO  Q ?
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    return tmp;
}

int dsp_HP_LR6(dspFilterParam_t freq) {   // ?
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    return tmp;
}

int dsp_LP_BES8(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.13294518316  , 1.22566942541 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.01102810214  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.948341760923 , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.920583104484 , 0.5 , 1.0);
    return tmp;
}

int dsp_LP_BES8_3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 2.18872623053  , 1.22566942541 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.95319575902  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.8320926012 , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.77846591177 , 0.5 , 1.0);
    return tmp;
}

int dsp_HP_BES8(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq * 1.13294518316  , 1.22566942541 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.01102810214  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 0.948341760923 , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 0.920583104484 , 0.505991069397 , 1.0);
    return tmp;
}

int dsp_HP_BES8_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq * 2.18872623053  , 1.22566942541 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.95319575902  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.8320926012 , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq * 1.77846591177 , 0.505991069397 , 1.0);
    return tmp;
}

int dsp_LP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.50979558 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 0.60134489 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 0.89997622 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 2.5629154 , 1.0);
    return tmp;
}

int dsp_HP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.50979558 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 0.60134489 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 0.89997622 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 2.5629154 , 1.0);
    return tmp;
}

int dsp_LP_LR8(dspFilterParam_t freq) {   // -6db cutoff ?
    int tmp =
    dsp_LP_BUT4(freq);
    dsp_LP_BUT4(freq);
    return tmp;
}

int dsp_HP_LR8(dspFilterParam_t freq) {   // -6db cutoff ?
    int tmp =
    dsp_HP_BUT4(freq);
    dsp_HP_BUT4(freq);
    return tmp;
}


/*
futur Header :
- filterType        // int 32   : msb = nombre de descripteurs de filtres restant
- freq              // int 32
- q (if relevant)   // 8.24 or float : float or int coded in msb as exponent cant be x tbd
- gain              // 8.24 or flot
*/

// WORK IN PROGRESS to have a single function to create filter parameter and store a filter summary for dynamic changes
int dsp_filter(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain) {
    int tmp=0;
    switch (type) {
    case BELP2 : tmp = dsp_LP_BES2(freq); break;
    case BELP4 : tmp = dsp_LP_BES4(freq); break;
    case BELP6 : tmp = dsp_LP_BES6(freq); break;
    case BELP8 : tmp = dsp_LP_BES8(freq); break;
    case BEHP2 : tmp = dsp_HP_BES2(freq); break;
    case BEHP4 : tmp = dsp_HP_BES4(freq); break;
    case BEHP6 : tmp = dsp_HP_BES6(freq); break;
    case BEHP8 : tmp = dsp_HP_BES8(freq); break;
    case BULP2 : tmp = dsp_LP_BUT2(freq); break;
    case BULP4 : tmp = dsp_LP_BUT4(freq); break;
    case BULP6 : tmp = dsp_LP_BUT6(freq); break;
    case BULP8 : tmp = dsp_LP_BUT8(freq); break;
    case BUHP2 : tmp = dsp_HP_BUT2(freq); break;
    case BUHP4 : tmp = dsp_HP_BUT4(freq); break;
    case BUHP6 : tmp = dsp_HP_BUT6(freq); break;
    case BUHP8 : tmp = dsp_HP_BUT8(freq); break;
    case LRLP2 : tmp = dsp_LP_LR2(freq); break;
    case LRLP4 : tmp = dsp_LP_LR4(freq); break;
    case LRLP6 : tmp = dsp_LP_LR6(freq); break;
    case LRLP8 : tmp = dsp_LP_LR8(freq); break;
    case LRHP2 : tmp = dsp_HP_LR2(freq); break;
    case LRHP4 : tmp = dsp_HP_LR4(freq); break;
    case LRHP6 : tmp = dsp_HP_LR6(freq); break;
    case LRHP8 : tmp = dsp_HP_LR8(freq); break;
    case FLP2  : tmp = dsp_Filter2ndOrder(FLP2,freq,Q,gain); break;
    case FHP2  : tmp = dsp_Filter2ndOrder(FHP2,freq,Q,gain); break;
    case FLS2  : tmp = dsp_Filter2ndOrder(FLS2,freq,Q,gain); break;
    case FHS2  : tmp = dsp_Filter2ndOrder(FHS2,freq,Q,gain); break;
    case FAP2  : tmp = dsp_Filter2ndOrder(FAP2,freq,Q,gain); break;
    case FPEAK : tmp = dsp_Filter2ndOrder(FPEAK,freq,Q,gain); break;
    case FNOTCH: tmp = dsp_Filter2ndOrder(FNOTCH,freq,Q,gain); break;
//first order
    case FLP1:
    case FHP1:
    case FLS1:
    case FHS1:
    case FAP1:
    default:
        dspprintf("NOT SUPPORTED (type = %d)\n",type);

    }
    return tmp;
}


