/*
 * dsp_inlinestd.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */


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
    asm("maccu %0,%1,%2,%3":"+r"(ah),"+r"(al):"r"(a),"r"(b));
    *alu = ((long long)ah << 32) | al;
#endif
}

// used in DSP_SAT0DB, DSP_SAT0DB_TPDF, DSP_SAT0DB_GAIN, DSP_SAT0DB_TPDF_GAIN
// double precision will be saturated then converted in 0.31
static inline void dspSaturate64_031(long long *a){
#ifndef DSP_ARCH
    long long satpos = 1ULL << (DSP_MANT+31);
    long long satneg = -satpos;
    if ((*a) >= satpos ) *a = DSP_Q31( 1.0); else
    if ((*a) <= satneg)  *a = DSP_Q31(-1.0);
    else (*a) >>= DSP_MANT;
#elif DSP_XS2A
    unsigned int al = *a;
    signed   int ah = *a >> 32;
    asm("lsats %0,%1,%2":"+r"(ah),"+r"(al):"r"(DSP_MANT));
    asm("lextract %0,%1,%2,%3,32":"=r"(al):"r"(ah),"r"(al),"r"(DSP_MANT));
    *a = al;
#endif
}

void dspSaturateFloat(dspALU_t *ALU){
    if ((*ALU) > 1.0)  *ALU =  1.0; else
    if ((*ALU) < -1.0) *ALU = -1.0;
}

// basically used to remove 28 bits of precision
//used in DSP_SAT0DB_GAIN, DSP_SAT0DB_TPDF_GAIN
static inline void dspShiftMant(long long *a){
    (*a) >>= DSP_MANT;
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


static const unsigned short crc16Table[256]=
{
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

typedef unsigned int uint32_t;

static inline uint32_t rotl( uint32_t x, unsigned int k) {
    return (x << k) | (x >> (32 - k));
}

uint32_t xoshiro128p(void) {
    static uint32_t s32[4] = { 1, 2, 3, 4 };

    s32[2] ^= s32[0];
    uint32_t result = s32[0] + s32[3];
    s32[3] ^= s32[1];
    uint32_t t = s32[1] << 9;
    s32[1] ^= s32[2];
    s32[0] ^= s32[3];
    s32[2] ^= t;

    s32[3] = rotl(s32[3], 11);

    return result;
}

// global variable. no need for volatile :
// can be accessed in read mode by any task.
// writen only by one core at the begining of dspruntime

struct dspTpdf_s {
 dspAligned64_t notMask;                // mask to be applied with a "and" to get the truncated sample
 dspAligned64_t round;                  // equivalent to 0.5, scaled at the right bit position
 dspAligned64_t value;                  // resulting value to be added to the sample, before truncation
 int factor;                            // scaling factor to reach the right bit
 int random;                            // 1 - z-1, violet noise
 unsigned int randomSeed;               // random number changed at every sample, initialise by runtimeInit
} dspTpdf;

static inline void dspTpdfRandomCalc(){
    unsigned int random = dspTpdf.randomSeed;
    unsigned int rnd;
#ifndef DSP_ARCH
    rnd = xoshiro128p();
    //rnd =  (random << 8) ^ crc16Table[(random >> 8) & 0xFF ]; // very basic randomizer ...
#elif DSP_XS2A
    rnd = random;
    asm ("crc32 %0,%1,%2":"+r"(rnd):"r"(-1),"r"(0xEB31D82E));
#endif
    dspTpdf.randomSeed = rnd;    // new value
    dspTpdf.random = rnd - random;
    dspTpdf.value = dspTpdf.round;
    dspmacs64_32_32( &dspTpdf.value , dspTpdf.random , dspTpdf.factor );
}

