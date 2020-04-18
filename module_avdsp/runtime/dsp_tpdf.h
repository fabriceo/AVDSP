// dither generation functions


// global variables related to TPDF
int dspTpdfValue;               // global value of the tpdf update by dspTpdfCalc()
int dspTpdfRandom;              // global white
int dspTpdfDefaultDither;       // default value passed by runtime init

typedef struct tpdf_s {
    int dither;         // position of the expeted dithering between 8..32 max
    unsigned mask;      // 32bit mask to remove bits before storing in DAC
    long long mask64;   // same scaled by DSP_MANT
    int shift;          // number of shift left / right for adjusting the tpdf value to the fixed point ALU
    dspALU_SP_t factor; // magic number to multiply tpdfvalue and get a 64bit msb at the right place (for xs2 optimization only)
} tpdf_t;

tpdf_t dspTpdfGlobal = { .dither=0, .mask=-1, .mask64=-1 };   // global instance of the above structure, used by default


#ifndef DSP_ARCH

typedef unsigned int uint32_t;
static inline uint32_t rotl(const uint32_t x, unsigned int k) {
    return (x << k) | (x >> (32 - k));
}

unsigned int dspTpdfs32[4];     // xoshiro128p prng internal state

static inline uint32_t xoshiro128p(uint32_t *s32) {
    //uint32_t *s32=dspTpdf.s32;
    const uint32_t result = s32[0] + s32[3];

    const uint32_t t = s32[1] << 9;

    s32[2] ^= s32[0];
    s32[3] ^= s32[1];
    s32[1] ^= s32[2];
    s32[0] ^= s32[3];
    s32[2] ^= t;

    s32[3] = rotl(s32[3], 11);

    return result;
}
#endif



static inline void dspTpdfInit(int seed, int defaultDither){
    dspTpdfRandom = seed;
    dspTpdfValue = 0;
    dspTpdfDefaultDither = defaultDither;
    dspTpdfGlobal.dither = 0;
    dspTpdfGlobal.mask   = -1;
    dspTpdfGlobal.mask64 = -1;

#ifndef DSP_ARCH
    uint32_t *s32 = dspTpdfs32;
    s32[0]=(seed|1);
    s32[1]=rotl((seed|8),7);
    s32[2]=rotl((seed|16),11);
    s32[3]=rotl((seed|24),17);
#endif
}

// prepare the tpdf structure according to requested dithering. check request with exisitng global value before creating new data
static inline int dspTpdfPrepare(tpdf_t * current, tpdf_t * local, int dith){
    if (dith == 0) dith = dspTpdfDefaultDither;
    if (dith != current->dither){
        local->dither = dith;
        local->mask = (-1 << (32-dith));  // 32bits version
#if DSP_ALU_INT
        local->mask64 = 0xFFFFFFFF00000000 | local->mask;
        local->mask64 <<= DSP_MANT_FLEX;
        int bits = DSP_MANT_FLEX - dith + 1;    // number of shifts required : +5 for 24bit and dsp_mant = 28
#ifdef DSP_XS1
        // preparing a factor to be used with the fast "maccs" instruction instead of using long long shift left
        if (bits >= 0) {
            local->factor = (1<<bits)-1;        // we can use a 32x32=64 mul for getting the tpdf scaled directly from the multiplication
            local->shift = 0;                   // no shift right needed in this case
        } else {
            local->shift = -bits;               // number of shift right to apply to tpdfvalue
            local->factor = 1; }                // no multiplication required then, but still a long add will be performed efficiently
        //printf("dither = %d, mask = 0x%X, bits = %d, shift = %d, factor = 0x%X\n",dith,local->mask,bits,local->shift,local->factor);
#else
        local->shift = bits;                    // simple shifting is used for "standard" architecture.
#endif
#endif
        return 0;
    } else
        return 1;   // means there was not change to local ptr
}



static inline dspALU_t dspTpdfCalc(){
#ifdef DSP_XS1
    unsigned  rnd1 = dspTpdfRandom;
    // sugested in xmos application note, here : https://xcore.github.io/doc_tips_and_tricks/pseudo-random-numbers.html
    const unsigned poly = 0xEDB88320;
    asm ("crc32 %0,%1,%2":"+r"(rnd1):"r"(-1),"r"(poly));
    unsigned  rnd2 = rnd1;
    asm ("crc32 %0,%1,%2":"+r"(rnd2):"r"(-1),"r"(poly));
    dspTpdfRandom = rnd2;
    // better to use unsigned on XS2A due to instruction set for shr able to run on dual lane vs ashr single lane
    int rnd = ( rnd1 >> 1 ) - ( rnd2 >> 1 ); // same as (rnd1>>1)+(rnd2>>1) on signed int (verified :)
#else
    int rnd1 = xoshiro128p(dspTpdfs32);
    int rnd2 = xoshiro128p(dspTpdfs32);
    dspTpdfRandom = rnd2;  // used by WHITE()
    int rnd = ( rnd1 >> 1 ) + ( rnd2 >> 1 );  // tpdf distribution
#endif
    dspTpdfValue = rnd;
#if DSP_ALU_INT
    return rnd;
#elif DSP_ALU_FLOAT
    #if DSP_ALU_64B
        return dspIntToDoubleScaled(rnd,31);
    #else
        return dspIntToFloatScaled(rnd,31);
    #endif
#endif
}


static inline void dspTpdfApply(dspALU_t *alu, tpdf_t * p){
#if DSP_ALU_INT
#ifdef DSP_XS1
    int tpdf = dspTpdfValue >> p->shift;    // preconditioning
    dspmacs64_32_32(alu, tpdf, p->factor);  // this cost less than differentiating both cases
#else
    // compute a scaled value according to DSP_MANT
    // using a shift and factor previously computed by the encoder for faster runtime
    long long tpdf = dspTpdfValue;
    if (p->shift >=0 )
         tpdf <<= p->shift;
    else tpdf >>= (-p->shift);
    *alu += tpdf;
#endif
#elif DSP_ALU_FLOAT
    #if DSP_ALU_64B
        *alu += dspIntToDoubleScaled(dspTpdfValue, 31 + p->dither -1);
    #else
        *alu += dspIntToFloatScaled(dspTpdfValue,  31 + p->dither -1);
    #endif
#endif
}


static inline void dspTpdfTruncate(dspALU_t *alu , tpdf_t * p){
#if DSP_ALU_INT
    *alu &= p->mask64;
#elif DSP_ALU_FLOAT
    #if DSP_ALU_64B
        dspTruncateDouble0DB(alu, p->dither);
    #else
        dspTruncateFloat0DB(alu, p->dither);
    #endif
#endif
}

