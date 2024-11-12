
#include <math.h>
#include <stdio.h>
#include "dsp_loudness.h"

int __attribute__((aligned(8)))  loudnesscoef[6*2] = { dspLoudnessQfactor, 0, 0, dspLoudnessQfactor, 0, 0, 0, 0, 0, 0, 0, 0 };
int __attribute__((aligned(8)))  loudnessleft[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int __attribute__((aligned(8)))  loudnessright[8]= { 0, 0, 0, 0, 0, 0, 0, 0 };
int __attribute__((aligned(8))) * loudnesscoefptr = &loudnesscoef[0];
//harmonics test
int __attribute__((aligned(8)))  harm[4] = { 1.0 * (1<<dspLoudnessMantissa), 0.01*(1ULL<<(dspLoudnessMantissa+2)), 0.001*(1ULL<<(dspLoudnessMantissa+4)), 0.0001*(1ULL<<34) };

float dspLoudnessVolume      =-99.9;
float dspLoudnessFreqLow     = 80.0;
float dspLoudnessGainMax     = 10.0;
float dspLoudnessGain        =  0.0;
float dspLoudnessFreqHigh    = 3500;
float dspLoudnessVolumeRef   =  0.0;
float dspLoudnessVolumeRange = 20.0;
float dspLoudnessFS          = 44100.0;


void dspLoudnessSetFreq(float freqLo, float freqHi) {
    dspLoudnessFreqLow    = freqLo;
    dspLoudnessFreqHigh   = freqHi;
}

void dspLoudnessSetBoost(float gain) {
    if (gain  >= dspLoudnessVolumeRange)
        gain  =  dspLoudnessVolumeRange;   // sanity check
    dspLoudnessGain    = gain;
    dspLoudnessGainMax = gain;
}

static void dspLoudnessCaclFilter(const int lo0_hi1, float freq, int * coef){
    float a1, b0, b1;
    if (dspLoudnessVolume < -99.9) {
        b0 = 0.0; b1 = 0.0; a1 = 0.0;
    } else  {
        float range = dspLoudnessVolumeRange;
        if ((dspLoudnessVolumeRef + range) >= 0.0) range = -dspLoudnessVolumeRef;
        float delta = dspLoudnessVolume - dspLoudnessVolumeRef;
        if ( (delta >= range)) {
            dspLoudnessGain  = 0.0;
        } else {
            if (delta <= 0.0 )  {
                dspLoudnessGain  = dspLoudnessGainMax;
                if (dspLoudnessGain > range) dspLoudnessGain = range;
            } else {
                float factor = (range - delta) / range;
                if (dspLoudnessGainMax < range) range = dspLoudnessGainMax;
                dspLoudnessGain  = range  * factor;   }
        }
        float gaindb;
        if ((dspLoudnessVolume + dspLoudnessGain) >= 0.0)
             gaindb = - dspLoudnessVolume;
        else gaindb = - dspLoudnessGain;
        if (lo0_hi1 == 1) gaindb = -gaindb;
        if ( (gaindb > 0.1) || (gaindb < -0.1) ) {
            float a0, w0, tw, A, gain;
            gain = pow( 10.0 , gaindb / 20.0);
            w0 = M_PI * 2.0 * freq / dspLoudnessFS;
            tw = tan( w0 / 2.0 );
            A = sqrt( gain );
/*       if (lo0_hi1 == 1) {
            // low shelf
            a0 =   A + tw;
            a1 = ( A - tw ) / a0;
            b0 = ( gain * tw + A ) / a0;
            b1 = ( gain * tw - A ) / a0;
          }  else { */
            // hishelf
            a0 =    A * tw + 1.0;
            a1 = -( A * tw - 1.0  ) / a0;
            b0 =  ( A * tw + gain ) / a0;
            b1 =  ( A * tw - gain ) / a0;
        } else {
            // passtrough
            gaindb = 0.0;
            a1 = 0.0;
            b0 = 1.0;
            b1 = 0.0; }
        // due to mantissa reintegration in biquad algorythm,  and its storage format
        // a1 is reduced by 1.0 , in order to remove 1 * Yn-1 term
        a1 -= 1.0;
        // apply digital volume on second biquad
        if (lo0_hi1 == 1) {
            float volume = dspLoudnessVolume;
            // check if volume control is mix analog/digital
            if (dspLoudnessVolumeMode == 1)
                if (volume < dspLoudnessVolumeRef)
                    volume = dspLoudnessVolumeRef;  // then max gain reduction is volref

            float factor = (volume + gaindb );
            if (factor >= 0.0) factor = 0.0;

            //printf("loudness volume = %f, limit = %f, gain = %f, factor = %f\n", dspLoudnessVolume, volume, -gaindb, volume+gaindb);

            factor = pow( 10.0 , factor / 20.0);
            b0 *= factor;
            b1 *= factor;
        }
    }
    coef[0] = b0 * dspLoudnessQfactor;
    coef[1] = b1 * dspLoudnessQfactor;
    coef[2] = a1 * dspLoudnessQfactor;
    //printf("b0 = %f, b1 = %f, a1 = %f = 0x%X\n", b0, b1, a1, coef[2]);
}

void dspLoudnessCaclBiquads(){
    if (loudnesscoefptr == &loudnesscoef[0]) {
        dspLoudnessCaclFilter(0, dspLoudnessFreqLow, &loudnesscoef[6]);
        dspLoudnessCaclFilter(1, dspLoudnessFreqHigh,&loudnesscoef[9]);
        loudnesscoefptr = &loudnesscoef[6];
    } else {
        dspLoudnessCaclFilter(0, dspLoudnessFreqLow, &loudnesscoef[0]);
        dspLoudnessCaclFilter(1, dspLoudnessFreqHigh,&loudnesscoef[3]);
        loudnesscoefptr = &loudnesscoef[0];
    }
}

void dspLoudnessSetFS(int fs) {
    dspLoudnessFS = fs;
    dspLoudnessCaclBiquads();
}

void dspLoudnessSetRange(float range) {
    if (range < 0.0) range = 0.0;
    if ((dspLoudnessVolumeRef + range) >= 0.0) range = - dspLoudnessVolumeRef;
    dspLoudnessVolumeRange = range;
}

void dspLoudnessSetVolumeRef(float ref) {
    if (ref >= 0.0) ref = 0.0;
    dspLoudnessVolumeRef = ref;
}

void dspLoudnessCalc(float volume) {
    if (volume >= 0.0)  volume = 0.0;
    if (volume != dspLoudnessVolume) {
        dspLoudnessVolume = volume;
        dspLoudnessCaclBiquads();
    }
}

void dspLoudnessCalcTarget(float volume) {
    if (volume >= 0.0)  volume = 0.0;
    if (volume < -99.9) dspLoudnessCalc( -100.0 );
    else {
        float hyst = 0.125;
        if (dspLoudnessVolume <= dspLoudnessVolumeRef) hyst = 1.0;
        if (volume >= (dspLoudnessVolume + hyst/2.0) ) {
        dspLoudnessCalc( dspLoudnessVolume + hyst);
        } else
            if (volume < (dspLoudnessVolume - hyst/2.0) ) {
            dspLoudnessCalc( dspLoudnessVolume - hyst);
        }
    }
}

// need verification
static inline void dspLoudnessSaturate(long long *a , const int mant){
    long long satpos = (1ULL << (mant*2)) - 1;
    long long satneg = -satpos;
    if ((*a) >= satpos ) *a = satpos;
    else
    if ((*a) < satneg)   *a = satneg;
    (*a) >>= mant;
}

// to be verified
int dspLoudnessCalcSampleRaw(int sample, int * state) {
    sample >>= (32-dspLoudnessMantissa);
    int b0 = loudnesscoef[0]; int b1 = loudnesscoef[1]; int a1 = loudnesscoef[2];
    int Xn1 = state[0];
    int Yn1 = state[1];
    long long res = ((long long)state[5] << 32) | state[4];
    res = (long long)sample * b0;
    res += (long long)Xn1 * b1;
    res += (long long)Yn1 * a1;
    int Xn = (res >> dspLoudnessMantissa);
    state[4] = res & 0xFFFFFFFF;
    state[5] = res >> 32;
    state[4] = res & 0xFFFFFFFF;
    state[0] = sample;
    state[1] = Xn;
    res += ((long long)state[7] << 32) | state[6];
    b0 = loudnesscoef[3]; b1 = loudnesscoef[4]; a1 = loudnesscoef[5];
    Xn1 = state[2];
    Yn1 = state[3];
    res += (long long)Xn  * b0;
    res += (long long)Xn1 * b1;
    res += (long long)Yn1 * a1;
    dspLoudnessSaturate(&res, dspLoudnessMantissa);
    state[6] = res & 0xFFFFFFFF;
    state[7] = res >> 32;
    res >>= dspLoudnessMantissa;
    state[2] = Xn;
    Xn = res & 0xFFFFFFFF;
    state[3] = Xn;
    return Xn;
}
