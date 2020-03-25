/*
 * dsp_inlinestd.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */

#ifdef DSP_MANT_FLEXIBLE
#define DSP_MANT_FLEX (dspMantissa) // defined in dsp_runtime.h
#else
#define DSP_MANT_FLEX (DSP_MANT)    // defined in header.h
#endif

#define DSP_QNM_INV ((double)1.0/(1ULL<<DSP_MANT_FLEX))
#define DSP_QNM_ONE (1ULL<<DSP_MANT_FLEX)

// compiler will hopefully compute this as static value where feasible
static inline long long dspQ31(dspALU_t f){
    if (f >=  1.0) return DSP_Q31_ONE; else
    if (f <= -1.0) return -1;
    f *= (DSP_2P31F + 0.5);
    return f; // implicit type cast and conversion to long long integer
}

static inline long long dspQNM_(dspALU_t f, int mant){
    int integ = 64-mant-1;
    long long max = (1ULL<<integ);
    double maxf = max;
    if (f >=   maxf)  return (1ULL<<mant); else
    if (f <= (-maxf)) return -1;
    double mul = (1ULL<<mant);
    mul += 0.5; // rounding
    mul *= f;
    return mul; // implicit type cast and conversion to long long integer
}

static inline long long dspQNM(dspALU_t f){
    return dspQNM_(f, DSP_MANT_FLEX);
}



// some basic maths in fixed point with flexible mantissa

// compute unsigned 32bit x 32 bits = 64 bits and returns only 32bits msb
// USED in DSP_DELAY and DSP_DELAY_DP
static inline unsigned int dspmulu32_32_32(unsigned int a, unsigned int b){
#ifndef DSP_ARCH  // default treatment
    unsigned long long res = (unsigned long long)a * b;
    return res >> 32;
#elif DSP_XS2A  // specific for xmos xs2 architecture
    int z = 0;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(a),"=r"(b):"r"(a),"r"(b),"r"(z),"r"(z));
    return a;
#endif
}

// used in DSP_RMS to scale the sample with a 31 bit factor
static inline int dspmuls32_32_32(int a, int b){
#ifndef DSP_ARCH  // default treatment
    long long res = (long long)a * b;
    return res >> 32;
#elif DSP_XS2A  // specific for xmos xs2 architecture
    int ah;
    unsigned al;
    asm("{ldc %0,0; ldc %1,0 }":"=r"(ah),"=r"(al) );
    asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
    return ah;
#endif
}

// used in DSP_RMS
static inline unsigned long long dspmulu64_32_32(unsigned int a, unsigned int b){
#ifndef DSP_ARCH  // default treatment
    return (unsigned long long)a * b;;
#elif DSP_XS2A  // specific for xmos xs2 architecture
    int z = 0;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(a),"=r"(b):"r"(a),"r"(b),"r"(z),"r"(z));
    return ((unsigned long long)a<<32) | b;
#endif
}


// used in  DSP_LOAD_MUX, DSP_RMS
static inline void dspmacs64_32_32(long long *alu, int a, int b){
#ifndef DSP_ARCH  // default treatment
    long long res = (long long)a * b;
    *alu += res;
#elif DSP_XS2A  // specific for xmos xs2 architecture
    int ah = (*alu)>>32;
    unsigned al = *alu;
    asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
    *alu = ((long long)ah << 32) | al;
#endif
}

//used in DSP_LOAD_GAIN, DSP_DATA_TABLE and dsp_RMS
static inline void dspmacs64_32_32_0(long long *alu, int a, int b){
#ifndef DSP_ARCH  // default treatment
    *alu = (long long)a * b;
#elif DSP_XS2A  // specific for xmos xs2 architecture
    int ah;// = (*alu)>>32;
    unsigned al;// = *alu;
    asm("{ldc %0,0; ldc %1,0 }":"=r"(ah),"=r"(al) );
    asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
    *alu = ((long long)ah << 32) | al;
#endif
}

// used in DSP_SAT0DB, DSP_SAT0DB_TPDF, DSP_SAT0DB_GAIN, DSP_SAT0DB_TPDF_GAIN
// double precision will be saturated then converted in 0.31
static inline void dspSaturate64_031(long long *a){
#ifndef DSP_ARCH
    long long satpos = 1ULL << (DSP_MANT_FLEX+31);
    long long satneg = -satpos;
    if ((*a) >= satpos ) *a = DSP_Q31_ONE; else
    if ((*a) <= satneg)  *a = -1;
    else (*a) >>= DSP_MANT_FLEX;
#elif DSP_XS2A
    unsigned int al = *a;
    signed   int ah = *a >> 32;
    asm("lsats %0,%1,%2":"+r"(ah),"+r"(al):"r"(DSP_MANT_FLEX));
    asm("lextract %0,%1,%2,%3,32":"=r"(al):"r"(ah),"r"(al),"r"(DSP_MANT_FLEX));
    *a = al;
#endif
}

static inline void dspSaturateFloat(dspALU_t *ALU){
    if ((*ALU) > 1.0)  *ALU =  1.0; else
    if ((*ALU) < -1.0) *ALU = -1.0;
}

// basically used to remove 28 bits of precision
//used in DSP_SAT0DB_GAIN, DSP_SAT0DB_TPDF_GAIN
static inline void dspShiftMant(long long *a){
    (*a) >>= DSP_MANT_FLEX;
}

// used in DSP_BIQUAD to fasten sample extraction
static inline int dspShiftInt(long long a, int mant){
#ifndef DSP_ARCH
    return a >> mant;
#elif DSP_XS2A
    int ah = a>>32; unsigned int al = a;
    asm("lextract %0,%1,%2,%3,32":"=r"(al):"r"(ah),"r"(al),"r"(mant));
    return al;
#endif
}

// dither generation functions 

typedef struct dspTpdf_s {
 dspAligned64_t notMask64;              // mask in s4.59 format to be applied with a "and" to get the truncated sample
 dspAligned64_t round64;                // equivalent to 0.5 at the right bit position in s4.59 format
 dspALU_t value;                        // raw value of the tpdf generator 32 bits -1..+1
 dspALU_t scaled;                       // tpdf scaled value to be added to the sample, before truncation
 int factor;                            // scaling factor to reach the right dither bit, either float32 or int32
 int shift;                             // number of shift occurence to perform on tpdf value to get the tpdf scaled value
 int dither;                            // value of the expected dither
 float factorFloat;                     // needed to scale the value according to the dither's bit
 unsigned int notMask32;                // tbd
 float roundFloat;                      // correspond to factor float / 2
 unsigned int randomSeed;               // random number changed at every sample, initialised by runtimeInit
#ifndef DSP_ARCH
 unsigned int s32[4];			        // xoshiro128p prng internal state
#endif

 //unsigned int notMaskInt32;             // needed for storing dithered sample at final stage (float version)
 //int shiftRigth;                        // number of shift right occurence to get proper float number
} dspTpdf_t;


#ifndef DSP_ARCH

typedef unsigned int uint32_t;
static inline uint32_t rotl(const uint32_t x, unsigned int k) {
    return (x << k) | (x >> (32 - k));
}

static inline void dspTpdfRandomInit(dspTpdf_t * dspTpdf, unsigned int seed){
    dspTpdf->randomSeed = seed;          // required for WHITE
    dspTpdf->factor     = 0;
    dspTpdf->notMask64  = -1;
    dspTpdf->notMask32  = -1;          // default value

    uint32_t *s32=dspTpdf->s32;
    s32[0]=(seed|1);
    s32[1]=rotl((seed|8),7);
    s32[2]=rotl((seed|16),11);
    s32[3]=rotl((seed|24),17);
}

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

static inline dspALU_t dspTpdfRandomCalc(dspTpdf_t * dspTpdf){ // return an integer or a float
    int rnd1 = xoshiro128p(dspTpdf->s32);
    int rnd2 = xoshiro128p(dspTpdf->s32);
    dspTpdf->randomSeed = rnd2;  // used by WHITE()
    int rnd = ( rnd1 >> 1 ) + ( rnd2 >> 1 );  // tpdf distribution

#if (DSP_FORMAT >= DSP_FORMAT_FLOAT)
    dspTpdf->value  = rnd;   // convert int to float
    dspTpdf->value *= DSP_2P31F_INV;   // get back to -1..+1
    dspTpdf->scaled = dspTpdf->value * dspTpdf->factorFloat;    // factor is pre-computed by encoder as (1.0 / 2^(N-1) )
#else
    // compute a scaled value according to DSP_MANT
    // using a shift factor previously computed by the encoder for faster runtime
    dspTpdf->value  = rnd;
    long long tpdf = rnd;
    if (dspTpdf->shift >=0 )
         tpdf <<= dspTpdf->shift;
    else tpdf >>= (-dspTpdf->shift);
    //tpdf += dspTpdf.round64;    // not sure the rounding should be done like this...
    dspTpdf->scaled = tpdf;
#endif
    return dspTpdf->scaled;
}

#elif DSP_XS2A

static inline void dspTpdfRandomInit(dspTpdf_t * dspTpdf,unsigned int seed) {
    dspTpdf->randomSeed = seed;
    dspTpdf->factor = 0;
}


static inline dspALU_t dspTpdfRandomCalc(){
    unsigned  rnd = dspTpdf->randomSeed;

    // sugested in xmos application note here : https://xcore.github.io/doc_tips_and_tricks/pseudo-random-numbers.html
    const unsigned poly = 0xEDB88320;
    asm ("crc32 %0,%1,%2":"+r"(rnd):"r"(-1),"r"(poly));
    unsigned  random = rnd;
    asm ("crc32 %0,%1,%2":"+r"(rnd):"r"(-1),"r"(poly));
    dspTpdf->randomSeed = rnd;
    // better to use unsigned on XS2A due to instruction set for shr able to run on dual lane vs ashr single lane
    dspTpdf->value = ( rnd >> 1 ) - ( random >> 1 ); // same as (rnd>>1)+(random>>1) on signed int (verified :)

    int ah; unsigned al; int val = dspTpdf->value;
    long long * adr = &dspTpdf->round64;
    asm("ldd %0,%1,%2[0]":"=r"(ah),"=r"(al):"r"(adr));    // load round
    if (al & ((1<<(DSP_MANT_FLEX+2))-1)) {
        asm("maccs %0,%1,%2,%3":"+r"(al),"+r"(ah):"r"(val),"r"(dspTpdf->factor));
        asm("std %0,%1,%2[1]"::"r"(0),"r"(al),"r"(adr));        // store scaled value just after round, 64 bits
        return al;
    } else {
        asm("maccs %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(val),"r"(dspTpdf->factor));
        asm("std %0,%1,%2[1]"::"r"(ah),"r"(al),"r"(adr));       // store scaled value just after round, 64 bits
        return ((long long)ah<<32) | al;
    }
    // this function returns dspTpdf.scaled
}
#endif


#if DSP_ALU_FLOAT32
static inline float dspInt2Float(int x, int mant){
    return (float)x * ((float)1.0/ (float)(1ULL << mant)); }
#elif  DSP_ALU_FLOAT64
static inline double dspInt2Float(int x, int mant){
    return (double)x * ((float)1.0/ (float)(1ULL << mant)); }
#endif

// return a float from the given float ptr, or convert as float if the ptr point on a QNM int32
#define DSP_PTR_TO_FLOAT(x) ( dspMantissa ? dspInt2Float( *((int*)x),DSP_MANT_FLEX) : (*x) )

