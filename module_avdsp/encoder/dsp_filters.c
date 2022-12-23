/*
 * dsp_coder.c
 *
 *  Created on: 1 janv. 2020
 *      Author: fabriceo
 *      this program will create a list of opcodes to be executed lated by the dsp engine
 */

#include "dsp_filters.h"      // enum dsp codes, typedefs and QNM definition

extern //prototype
void    compute_coefs_spec_order_tbw (float *coef_arr, int nbr_coefs, float transition);

extern int dspMinSamplingFreq;  // from encoder.c, initialised by EncoderInit
extern int dspMaxSamplingFreq;

// calc the biquad coefficient for a given filter type. dspFilterParam_t is double (see dsp_header.h)
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
    dspFilterParam_t tw2, a0, alpha;
    tw2 = tan(M_PI * freq / fs);
    *a2 = 0.0;
    *b2 = 0.0;
    //stability condition : abs(a1)<1
    switch (type) {
    case FLP1: {
        alpha = 1.0 + tw2;
        *a1 = (1.0-tw2) / alpha;
        *b0 = tw2 / alpha * gain;
        *b1 = *b0;
    break; }
    case FHP1: {
        alpha = 1.0 + tw2;
        *a1 = ( (1.0-tw2) / alpha );
        *b0 =  1.0 / alpha * gain;
        *b1 = -1.0 / alpha * gain;
    break; }
    case FHS1: {
        dspFilterParam_t A = sqrt(gain);
        a0 = A*tw2+1.0;
        *a1 = -( A * tw2 - 1.0 ) / a0;
        *b0 =  ( A * tw2 + gain ) / a0;
        *b1 =  ( A * tw2 - gain ) / a0;
        break; }
    case FLS1: {
        dspFilterParam_t A = sqrt(gain);
        a0 = tw2 + A;
        *a1 = -( tw2 - A ) / a0;
        *b0 =  ( gain * tw2 + A) / a0;
        *b1 =  ( gain * tw2 - A) / a0;
        break; }
    case FAP1: {
        alpha = (tw2 - 1.0) / (tw2 + 1.0) ;
        *a1 = -alpha;
        *b0 = alpha * gain;
        *b1 = gain ;
        break;
    }
    } //switch

}

/*
 *
 * g = tan(pi*(Fc/Fs));
 * b0 = (g-1)/(g+1);
    b1 = 1;
    b2 = 0;
    a1 = b0;
 */

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
    dspFilterParam_t a0, w0, cw0, sw0, tw2, alpha;
    w0 = M_PI * 2.0 * freq / fs;
    cw0 = cos(w0);
    sw0 = sin(w0);
    tw2 = tan(w0/2.0);
    if (Q != 0.0) alpha = sw0 / 2.0 / Q; else alpha = 1;
    a0 = (1.0 + alpha);
    *a1 = -(-2.0 * cw0) / a0;       // sign is changed to accomodate convention
    *a2 = -(1.0 - alpha ) / a0;     // and coeficients are normalized vs a0
    // stability condition : abs(a2)<1 &&  abs(a1)<(1+a2)
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


// from dsp_encoder.c
extern int  addBiquadCoeficients(dspFilterParam_t b0,dspFilterParam_t b1,dspFilterParam_t b2,dspFilterParam_t a1,dspFilterParam_t a2);
extern int  addFilterParams(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain);
extern void sectionBiquadCoeficientsBegin();
extern void sectionBiquadCoeficientsEnd();

int dsp_Filter2ndOrder(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain){
    int coefPtr = 0;
    dspFilterParam_t a1, a2, b0, b1, b2;
    sectionBiquadCoeficientsBegin();
    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        dspFilterParam_t fs = dspConvertFrequencyFromIndex(f);
        dspFilter2ndOrder(type, fs, freq, Q, gain, &b0, &b1, &b2, &a1, &a2);

        if (coefPtr==0) {
            coefPtr =  addFilterParams(type, freq, Q, gain);
            dspprintf2("FILTER f = %f, Q = %f, G = %f\n", freq, Q, gain);
            dspprintf3(" b0 = %f, ",b0);
            dspprintf3(" b1 = %f,",b1);
            dspprintf3(" b2 = %f,",b2);
            dspprintf3(" a1 = %f, ",a1);
            dspprintf3(" a2 = %f\n",a2); }
        addBiquadCoeficients(b0, b1, b2, a1, a2);
    }
    sectionBiquadCoeficientsEnd();
    return coefPtr;
}

int dsp_Filter1stOrder(int type, dspFilterParam_t freq, dspGainParam_t gain){
    int coefPtr = 0;
    dspFilterParam_t a1, a2, b0, b1, b2;
    sectionBiquadCoeficientsBegin();
    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        dspFilterParam_t fs = dspConvertFrequencyFromIndex(f);
        dspFilter1stOrder(type, fs, freq, gain, &b0, &b1, &b2, &a1, &a2);

        if (coefPtr==0) {
            coefPtr =  addFilterParams(type, freq, 0.0, gain);
            dspprintf2("FILTER f = %f, G = %f\n", freq, gain);
            dspprintf3(" b0 = %f, ",b0);
            dspprintf3(" b1 = %f,",b1);
            dspprintf3(" b2 = %f,",b2);
            dspprintf3(" a1 = %f, ",a1);
            dspprintf3(" a2 = %f\n",a2); }
        addBiquadCoeficients(b0, b1, b2, a1, a2);
    }
    sectionBiquadCoeficientsEnd();
    return coefPtr;
}

int dsp_Hilbert(int stages, dspFilterParam_t transition, dspGainParam_t phase){
    int coefPtr = 0;
    float coefs[20];// max 10 stages allowed => 20 coefs
    for (int i=0; i< stages; i++) {
        int d = i*2 + ((phase == 0.0) ? 1 : 0);
        sectionBiquadCoeficientsBegin();
        for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
            dspFilterParam_t fs = dspConvertFrequencyFromIndex(f);
            compute_coefs_spec_order_tbw( &coefs[0], stages*2, transition / fs );
            if (coefPtr == 0)
                dspprintf3("HILBERT stage = %d, transition = %f, fs=%f\n", stages,transition, fs);
            if (f == dspMinSamplingFreq) {
                coefPtr =  addFilterParams(FHILB, 1000, transition, 1.0 );
                dspprintf3("c%d = %f\n",d, coefs[d]);
            }                   // xn       xn-1   xn-2    yn-1   yn-2
            addBiquadCoeficients( coefs[d], 0.0,   -1.0,   0.0,   coefs[d] );
        } // for f
        sectionBiquadCoeficientsEnd();
    } // for i
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
    dsp_Filter1stOrder(FLP1, freq * 1.03054454544, 1.0);
    return tmp;
}

int dsp_LP_BES3_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with high pass
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.32267579991, 0.691046625825 , 1.0);
    dsp_Filter1stOrder(FLP1, freq * 1.44761713315,  1.0);
    return tmp;
}

int dsp_HP_BES3(dspFilterParam_t freq) {   // high pass cutoff freq is in phase with low pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.941600026533 , 0.691046625825 , 1.0);
    dsp_Filter1stOrder(FHP1, freq / 1.03054454544,    1.0);
    return tmp;
}

int dsp_HP_BES3_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.32267579991 , 0.691046625825 , 1.0);
    dsp_Filter1stOrder(FHP1, freq / 1.44761713315,   1.0);
    return tmp;
}

int  dsp_LP_BUT3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    dsp_Filter1stOrder(FLP1, freq,  1.0);
    return tmp;
}

int dsp_HP_BUT3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    dsp_Filter1stOrder(FHP1, freq,  1.0);
    return tmp;
}

int dsp_LP_LR3(dspFilterParam_t freq) { // -6db cutoff at fc ?
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0);
    dsp_Filter1stOrder(FLP1, freq,  1.0);
    return tmp;
}

int dsp_HP_LR3(dspFilterParam_t freq) { // -6db cutoff ? 180 deg phase shift vs low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0);
    dsp_Filter1stOrder(FHP1, freq,  1.0);
    return tmp;
}



int dsp_LP_BES4(dspFilterParam_t freq) {   // low pass cutoff freq is IN PHASE with high pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.944449808226 , 0.521934581669 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.05881751607  , 0.805538281842 , 1.0);
    return tmp;
}

int dsp_LP_BES4_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.43017155999  , 0.521934581669 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.60335751622  , 0.805538281842 , 1.0);
    return tmp;
}
int dsp_HP_BES4(dspFilterParam_t freq) {       // high pass cutoff freq is in phase with low pass cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.944449808226 , 0.521934581669 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.05881751607  , 0.805538281842 , 1.0);
    return tmp;
}

int dsp_HP_BES4_3DB(dspFilterParam_t freq) {       // cutoff is at -3db, but not in phase with low pass
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.43017155999  , 0.521934581669 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.60335751622  , 0.805538281842 , 1.0);
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
    dsp_Filter2ndOrder(FLP2, freq * 0.928156550439 , 0.510317824749 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.977488555538 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.10221694805  , 1.02331395383 , 1.0);
    return tmp;
}

int dsp_LP_BES6_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.60391912877 , 0.510317824749 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.68916826762 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.9047076123  , 1.02331395383  , 1.0);
    return tmp;
}

int dsp_HP_BES6(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.928156550439 , 0.510317824749 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 0.977488555538 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.10221694805  , 1.02331395383  , 1.0);
    return tmp;
}

int dsp_HP_BES6_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.60391912877 , 0.510317824749 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.68916826762 , 0.611194546878 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.9047076123  , 1.02331395383  , 1.0);
    return tmp;
}

int dsp_LP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.51763809 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2  , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 1.9318517  , 1.0);
    return tmp;
}

int dsp_HP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.51763809 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2  , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 1.9318517  , 1.0);
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
    dsp_Filter2ndOrder(FLP2, freq * 0.920583104484 , 0.505991069397 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 0.948341760923 , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.01102810214  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.13294518316  , 1.22566942541 , 1.0);
    return tmp;
}

int dsp_LP_BES8_3(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.77846591177  , 0.505991069397 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.8320926012   , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 1.95319575902  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq * 2.18872623053  , 1.22566942541  , 1.0);
    return tmp;
}

int dsp_HP_BES8(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.920583104484 , 0.505991069397 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 0.948341760923 , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.01102810214  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.13294518316  , 1.22566942541  , 1.0);
    return tmp;
}

int dsp_HP_BES8_3DB(dspFilterParam_t freq) {
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.77846591177  , 0.505991069397 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.8320926012   , 0.559609164796 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 1.95319575902  , 0.710852074442 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq / 2.18872623053  , 1.22566942541  , 1.0);
    return tmp;
}

int dsp_LP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.50979558 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 0.60134489 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 0.89997622 , 1.0);
    dsp_Filter2ndOrder(FLP2, freq, 2.5629154  , 1.0);
    return tmp;
}

int dsp_HP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    int tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.50979558 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 0.60134489 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 0.89997622 , 1.0);
    dsp_Filter2ndOrder(FHP2, freq, 2.5629154  , 1.0);
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
        dspprintf("NOT SUPPORTED (type = %d)\n",type);

    }
    return tmp;
}


