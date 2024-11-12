/*
 * dsp_fpmath.h
 *
 *  Version: May 1st 2020
 *      Author: Fabriceo
 */

// some basic maths in fixed point with flexible mantissa

// compute unsigned 32bit x 32 bits = 64 bits and returns only 32bits msb
// USED in DSP_DELAY and DSP_DELAY_DP
static inline unsigned int dspmulu32_32_32(unsigned int a, unsigned int b){
#ifdef DSP_XS1  // specific for xmos  architecture
    int z = 0;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(a),"=r"(b):"r"(a),"r"(b),"r"(z),"r"(z));
    return a;
#else
    unsigned long long res = (unsigned long long)a * b;
    return res >> 32;
#endif
}

// used in DSP_RMS and DSP_DISTRIB to scale the sample with a 31 bit factor
static inline int dspmuls32_32_32(int a, int b){
#ifdef DSP_XS1  // specific for xmos architecture
    #ifdef DSP_XS2A
        int ah; unsigned al; // dual issue mode only available for XS2
        asm("{ldc %0,0; ldc %1,0 }":"=r"(ah),"=r"(al) );
    #else
        int ah=0; unsigned al=0;
    #endif
        asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
        return ah;
#else
    long long res = (long long)a * b;
    return res >> 32;
#endif
}

// used in DSP_RMS, DSP_SQRTX and DSP_CLIP
static inline unsigned long long dspmulu64_32_32(unsigned int a, unsigned int b){
#ifdef DSP_XS1  // specific for xmos  architecture
    int z = 0;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(a),"=r"(b):"r"(a),"r"(b),"r"(z),"r"(z));
    return ((unsigned long long)a<<32) | b;
#else
    return (unsigned long long)a * b;;
#endif
}


// used in  DSP_LOAD_MUX, DSP_RMS, DSP_DCBLOCK, DSP_DITHER_NS2
static inline void dspmacs64_32_32(long long *alu, int a, int b){
#ifdef DSP_XS1  // specific for xmos  architecture
    int ah = (*alu)>>32;
    unsigned al = *alu;
    asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
    *alu = ((long long)ah << 32) | al;
#else
    long long res = (long long)a * b;
    *alu += res;
#endif
}

//used in DSP_LOAD_GAIN, DSP_DATA_TABLE, DSP_DIRAC
static inline void dspmacs64_32_32_0(long long *alu, int a, int b){
#ifdef DSP_XS1  // specific for xmos architecture
    int ah;// = (*alu)>>32;
    unsigned al;// = *alu;
#ifdef DSP_XS2A
    asm("{ldc %0,0; ldc %1,0 }":"=r"(ah),"=r"(al) );
#else
    ah=0;al=0;
#endif
    asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
    *alu = ((long long)ah << 32) | al;
#else
    *alu = (long long)a * b;
#endif
}

// used in DSP_SAT0DB, DSP_SAT0DB_GAIN
// double precision will be saturated then converted in s.31
static inline void dspSaturate64_031(long long *a , int mant){
#ifdef DSP_XS2A // instructions below are only available on XS2A or XS3A architecture
    unsigned al = *a;
    int ah = *a >> 32;
    asm("lsats %0,%1,%2":"+r"(ah),"+r"(al):"r"(mant));
    asm("lextract %0,%1,%2,%3,32":"=r"(al):"r"(ah),"r"(al),"r"(mant));
    *a = al;    // will sign extend
#else
    long long satpos = 1ULL << (mant+31);
    long long satneg = -satpos;
    if ((*a) >= satpos ) *a = 0x000000007FFFFFFF; else
    if ((*a) < satneg)   *a = 0xFFFFFFFF80000000;
    else (*a) >>= mant;
#endif
}


// used in DSP_SAT0DB, DSP_SAT0DB_GAIN
// double precision will be saturated then converted in s.31
static inline int dspSaturate64_031_test(long long *a , int mant){
#ifdef DSP_XS2A // instructions below are only available on XS2A or XS3A architecture
    unsigned al = *a;
    int ah = *a >> 32;
    asm("lsats %0,%1,%2":"+r"(ah),"+r"(al):"r"(mant));
    asm("lextract %0,%1,%2,%3,32":"=r"(al):"r"(ah),"r"(al),"r"(mant));
    *a = al;    // will sign extend
    if  ((al == 0x7FFFFFFF) || (al==0x80000000)) return 1; else return 0;
#else
    long long satpos = 1ULL << (mant+31);
    long long satneg = -satpos;
    if ((*a) >= satpos ) { *a = 0x000000007FFFFFFF; return 1; }
    else
    if ((*a) < satneg)   { *a = 0xFFFFFFFF80000000; return 1; }
    else { (*a) >>= mant; return 0; }
#endif
}

// used in DSP_BIQUAD, DSP_DCBLOCK, DSP_DITHER_NS2, DSP_SINE to fasten sample extraction
static inline int dspShiftInt(long long a, int mant){
#ifdef DSP_XS2A // instructions below is only available on XS2A architecture
    int ah = a>>32; unsigned int al = a;
    asm("lextract %0,%1,%2,%3,32":"=r"(al):"r"(ah),"r"(al),"r"(mant));
    return al;
#else
    return a >> mant;
#endif
}

// used in DSP_BIQUAD, DSP_DCBLOCK, DSP_DITHER_NS2, DSP_SINE to fasten sample extraction
static inline long long dspShiftInt64(long long a, int mant){
#ifdef DSP_XS2A // instructions below is only available on XS2A architecture
    int ah = a>>32; unsigned int al = a;
    asm("lextract %0,%0,%1,%2,32 ; ashr %0,%0,%2":"+r"(ah),"+r"(al):"r"(mant));
    return ((long long)ah<<32)+al;
#else
    return a >> mant;
#endif
}

