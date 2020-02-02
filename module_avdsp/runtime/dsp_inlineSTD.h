/*
 * dsp_inlinestd.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */
#ifndef __XC__

// some maths in fixed point


// used in DSP_MULXY
static inline void dspmul64_64QNM(long long * a,long long *b, int mant){ // example mant = 56 for 8.56 original format
    // a = a * b -> 8.56 * 8.56 = 16.112 converted back into 8.56
    // simplified version
    *a >>= (mant/2);
    *b >>= (mant/2);
    *a *= *b;

}

// used in DSP_GAIN, DSP_GAIN0DB and DATA_TABLE
static inline long long dspmul64_32QNM(long long a, int b, int mant){

    // simplified version
    a >>= mant;    // reduce precision of ALU to fit the multiplication impact
    a *= b;
    return a;

}

//used in DSP_GAIN, DSP_GAIN0DB and dsp_MUX0DB and DSP_MACC
static inline long long dspmul031_32QNM(int a, int b, const int mant, const int mantdp){
    int shift = mantdp-(mant+31);   // compiler will treat all this as const
    long long res;
    res = (long long)a * b;
    if (shift > 0) res <<= shift;
    else
        if (shift < 0) res >>= (-shift);
    return res;

}


// used in DSP_DIVXY
static inline void dspdiv64_64QNM( long long * a, long long * b, int mantdp){
    // simplified approach by reducing precision of b
    long long tmp = (*b >> (mantdp/2)); // convert  to 4.28
    *a /= tmp;                // 8.56 / 4.28 => 4.28
    *a <<= (mantdp/2);              // convert back to 8.56
}

// used in DSP_SQRT
static inline void dspsqrt64QNM( long long *a, int mantdp){
    // TODO ...
}

// used in DSP_FIR to be confirmed
static inline int dspconv64_32QNM(long long a, int mant){  // 8.56 will be converted in 4.28 if mant = 28
    return a >> mant;
}

// used in DSP_FIR to be confirmed
static inline long long dsp32QNM_to64(int a, int mant){  // 4.28 will be converted to 8.56 if mant = 28
    return (long long)a << mant;
}

// used in DSP_SAT0DB, DSP_STORE_DP, DSP_DEALY and DSP_BIQUAD
static inline int dsp64QNM_to031(long long a, int mantdp){  // 8.56 will be converted in 0.31 if mant = 56
    long long satpos = ((long long)1<<mantdp)-1;
    long long satneg = -satpos-1;
    if (a > satpos ) a = 0x7FFFFFFF;
    else
        if (a < satneg) a = -1;
        else a >>= (mantdp-32+1);
    return a;
}

// used in DSP_LOAD_DP
static inline long long dsp031_to64QNM(int a, int mantdp){  // 0.31 will be converted to 8.56 if mant = 56
    return (long long)a << (mantdp-32+1);
}

// used in DSP_BIQUAD
static inline int dsp64QNM_to031check(long long a, int mantdp){
    if (a>>32) return a >> (mantdp-32+1);
    else return a;
}

// compute 32bit x 32 bits = 64 bits and returns only 32bits msb (not used)
static inline int dspmul32_32_32(int a, int b){
    long long res = ((long long) a)*b;
    return res >> 32;
}

// compute 32bit x 32 bits = 64 bits and returns only 32bits msb
// USED in DSP_DELAY and DSP_DELAY_DP
static inline unsigned int dspmulu32_32_32(unsigned int a, unsigned int b){
    long long res = ((long long) a)*b;
    return res >> 32;
}

// not used
static inline int dspconvfloat_QNM( float x, int q )
{
    if( x < 0 ) return (((double)(1<<q)) * x - 0.5);
    else if( x > 0 ) return (((double)((1<<q)-1)) * x + 0.5);
    return 0;
}


#endif // __XC__
