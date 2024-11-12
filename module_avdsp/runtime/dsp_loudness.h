#ifndef DSP_LOUDNESS_H
#define DSP_LOUDNESS_H

#define dspLoudnessMantissa (29)    // 29 bits mantissa gives 2 bit for potential overflow = 4 = 12db
#define dspLoudnessMaxInt ((1 << dspLoudnessMantissa)-1)
#define dspLoudnessMinInt (- dspLoudnessMaxInt)
#define dspLoudnessQfactor ((float)(1ULL << dspLoudnessMantissa))
#define dspLoudnessVolumeMode 1 // 0 = 100%digital, 1 = digital between 0 and volref if loudness is active

extern int __attribute__((aligned(8)))  loudnesscoef[6*2];
extern int __attribute__((aligned(8)))  loudnessleft[8];
extern int __attribute__((aligned(8)))  loudnessright[8];
extern int __attribute__((aligned(8)))  harm[4];

#if defined(__XC__)
extern int * unsafe loudnesscoefptr;
#else
extern int * loudnesscoefptr;
#endif

#if defined(__XS1__) || defined(__XS2A__)

static inline int dspLoudnessCalcSample(int sample, int state[]) {
    // two first order biquad
    int h, l, res, a1, b0, b1, A1, B0, B1, Xn1, Yn, Yn1, Xn = sample;

    asm("ldd %0,%1,%2[2]":"=r"(h),  "=r"(l): "r"(state));
    asm("ldd %0,%1,%2[0]":"=r"(Yn1),"=r"(Xn1):"r"(state));
    asm("ldd %0,%1,%2[0]":"=r"(b1), "=r"(b0): "r"(loudnesscoefptr));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Xn), "r"(b0),"0"(h),"1"(l));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Xn1),"r"(b1),"0"(h),"1"(l));
    asm("ldd %0,%1,%2[1]":"=r"(B0),"=r"(a1):"r"(loudnesscoefptr));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Yn1),"r"(a1),"0"(h),"1"(l));
    asm("ldd %0,%1,%2[2]":"=r"(A1),"=r"(B1):"r"(loudnesscoefptr));
    // save 64bit result as remainder for next cycle
    asm("std %0,%1,%2[2]"::"r"(h), "r"(l),"r"(state));
    asm("lextract %0,%1,%2,%3,32":"=r"(Yn):"r"(h),"r"(l),"r"(dspLoudnessMantissa));
    asm("std %0,%1,%2[0]"::"r"(Yn), "r"(Xn),"r"(state));

    asm("ldd %0,%1,%2[3]":"=r"(h),"=r"(l):"r"(state));   // load previous 64bits result
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Yn),"r"(B0),"0"(h),"1"(l));
    asm("ldd %0,%1,%2[1]":"=r"(Yn1),"=r"(Xn1):"r"(state));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Xn1),"r"(B1),"0"(h),"1"(l));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Yn1),"r"(A1),"0"(h),"1"(l));
    asm("lsats %0,%1,%2":"=r"(h),"=r"(l):"r"(dspLoudnessMantissa),"0"(h),"1"(l));
    // save 64bit result as remainder
    asm("std %0,%1,%2[3]"::"r"(h), "r"(l),"r"(state));
    asm("lextract %0,%1,%2,%3,32":"=r"(res):"r"(h),"r"(l),"r"(dspLoudnessMantissa));
    asm("std %0,%1,%2[1]"::"r"(res), "r"(Yn),"r"(state));

return res;
}

#else

#if defined(__XC__)

static inline int dspLoudnessCalcSample(int sample, int state[]) {
    dspLoudnessCalcSampleRaw(sample, &state[0]);
}
#else

static inline int dspLoudnessCalcSample(int sample, int * state) {
    dspLoudnessCalcSampleRaw(sample, &state[0]);
}
#endif

#endif

extern void dspLoudnessSetFreq(float freqLo, float freqHi);
extern void dspLoudnessSetBoost(float gain);
extern void dspLoudnessSetVolumeRef(float ref);
extern void dspLoudnessSetRange(float range);
extern void dspLoudnessSetFS(int fs);
extern void dspLoudnessCalc(float volume);
extern void dspLoudnessCalcTarget(float volume);
// not needed as it is called already by dspLoudnessSetFS() and dspLoudnessCalc()
extern void dspLoudnessCaclBiquads();


#endif
