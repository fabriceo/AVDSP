/*
 * dsp_runtime.h
 *
 *  Created on: 9 janv. 2020
 *      Author: Fabrice
 */


#ifndef DSP_RUNTIME_H_
#define DSP_RUNTIME_H_

#include "dsp_header.h"

typedef struct dspLoHi_s    { unsigned lo; int hi; } dspLoHi_t;
typedef struct dspMantExp_s { int mant; int exp;   } dspMantExp_t;
typedef union  dspALU64_u   { long long i;
                              double f;
                              dspLoHi_t lh;
                              dspMantExp_t me; } dspALU64_t;
typedef union dspALU32_u    { int i; float f;  } dspALU32_t;

//types definition for suporting all DSP_FORMAT
typedef short       dspSample_st;
typedef int         dspALU_it;
typedef int         dspALU_SP_it;
typedef short       dspParam_st;

typedef int         dspSample_it;
typedef long long   dspALU_llt;
typedef int         dspParam_it;

typedef float       dspALU_ft;
typedef float       dspALU_SP_ft;
typedef float       dspParam_ft;

typedef double      dspALU_dt;

#ifndef DSP_FORMAT
#define DSP_FORMAT 2
//#warning "default DSP_FORMAT 2 (INT64)"
#endif
#if (DSP_FORMAT == 1)     // sample is int16, alu is int32, param int16. NOT fully Implemented in this version
#error "INT32 runtime not yet compatible"

    typedef short       dspSample_t;
    typedef int         dspALU_t;
    typedef dspALU_t    dspALU_SP_t;
    typedef short       dspParam_t;
    #define DSP_ALU_INT         1
    #define DSP_ALU_64B         0
    #define DSP_ALU_FLOAT       0
    #define DSP_ALU_FLOAT       0
    #define DSP_SAMPLE_INT      1
    #define DSP_SAMPLE_FLOAT    0
    #define DSP_RUNTIME_FORMAT(name) name ## _1
    #ifndef DSP_MANT_FLEXIBLE
        #if (DSP_MANT > 15)
        #error "DSP_MANT max = 15bits"
        #endif
        #if (DSP_MANT <8)
        #error "DSP_MANT min 8bits"
        #endif
    #endif
    static const unsigned dspFormat = 0;    //bit 0=32/64, bit 2=int/float, bit1=sample int/float

#elif (DSP_FORMAT == 2)         // sample is int32, alu is int64, param int32

    typedef int         dspSample_t;
    typedef long long   dspALU_t;
    typedef int         dspALU_SP_t;
    typedef int         dspParam_t;
    #define DSP_ALU_INT         1
    #define DSP_ALU_64B         1
    #define DSP_ALU_FLOAT       0
    #define DSP_SAMPLE_INT      1
    #define DSP_SAMPLE_FLOAT    0
    #define DSP_RUNTIME_FORMAT(name) name ## _2
    #ifndef DSP_MANT_FLEXIBLE
        #if (DSP_MANT <8)
        #error "DSP_MANT min 8bits"
        #endif
        #if (DSP_MANT >30)
        #error "DSP_MANT max 30bits"
        #endif
    #endif
    static const unsigned dspFormat = 1;

#elif (DSP_FORMAT == 3)             // sample is int32, alu is float32, param float32

    typedef int         dspSample_t;
    typedef float       dspALU_t;
    typedef dspALU_t    dspALU_SP_t;
    typedef float       dspParam_t;
    #define DSP_ALU_INT         0
    #define DSP_ALU_64B         0
    #define DSP_ALU_FLOAT       1
    #define DSP_SAMPLE_INT      1
    #define DSP_SAMPLE_FLOAT    0
    #define DSP_RUNTIME_FORMAT(name) name ## _3
    static const unsigned dspFormat = 4;

#elif (DSP_FORMAT == 4)             // sample int32, alu is double float, param float32

    typedef int         dspSample_t;
    typedef double      dspALU_t;
    typedef float       dspALU_SP_t;
    typedef float       dspParam_t;
    #define DSP_ALU_INT         0
    #define DSP_ALU_64B         1
    #define DSP_ALU_FLOAT       1
    #define DSP_SAMPLE_INT      1
    #define DSP_SAMPLE_FLOAT    0
    #define DSP_RUNTIME_FORMAT(name) name ## _4
    static const unsigned dspFormat = 5;

#elif (DSP_FORMAT == 5)               // sample float, alu float32, param float32

    typedef float       dspSample_t;
    typedef float       dspALU_t;
    typedef float       dspALU_SP_t;
    typedef float       dspParam_t;
    #define DSP_ALU_INT         0
    #define DSP_ALU_64B         0
    #define DSP_ALU_FLOAT       1
    #define DSP_SAMPLE_INT      0
    #define DSP_SAMPLE_FLOAT    1
    #define DSP_RUNTIME_FORMAT(name) name ## _5
    static const unsigned dspFormat = 6;

#elif (DSP_FORMAT == 6)               // sample float, alu double float, param float32

    typedef float       dspSample_t;
    typedef double      dspALU_t;
    typedef float       dspALU_SP_t;
    typedef float       dspParam_t;
    #define DSP_ALU_INT         0
    #define DSP_ALU_FLOAT       1
    #define DSP_ALU_64B         1
    #define DSP_SAMPLE_INT      0
    #define DSP_SAMPLE_FLOAT    1
    #define DSP_RUNTIME_FORMAT(name) name ## _6
    static const unsigned dspFormat = 7;

#elif (DSP_FORMAT == 7)         // sample is int32, param int32, alu is 32bits mantissa and 32bits exponent
                                //EXPERIMENTAL not implemented yet
    typedef dspALU64_t   dspALU_u;
    typedef int          dspSample_t;
    typedef dspMantExp_t dspALU_t;
    typedef int          dspALU_SP_t;
    typedef int          dspParam_t;
    #define DSP_ALU_INT         1
    #define DSP_ALU_64B         1
    #define DSP_ALU_FLOAT       1
    #define DSP_SAMPLE_INT      1
    #define DSP_SAMPLE_FLOAT    0
    #define DSP_RUNTIME_FORMAT(name) name ## _7
    #undef  DSP_MANT
    #define DSP_MANT 31
    #ifdef DSP_MANT_FLEXIBLE
        #error "MANTISSA forced to 31 in this mode"
    #endif
    static const unsigned dspFormat = 3;

#else
#error "DSP_FORMAT undefined or not supported"
#endif

#define dspFormatIsInt          ((dspFormat & 4)==0)
#define dspFormatIsInt64        ((dspFormat & 7)==1)
#define dspFormatIsFloat        ((dspFormat & 4)==4)
#define dspFormatIs32bits       ((dspFormat & 1)==0)
#define dspFormatIs64bits       ((dspFormat & 1)==1)
#define dspFormatSampleFloat    ((dspFormat & 6)==6)

#if DSP_ALU_INT
#if defined(  DSP_MANT_FLEXIBLE ) && (DSP_MANT_FLEXIBLE > 0)
#define DSP_MANT_FLEX (dspMantissa) // initialized in dspRuntimeInit() {dsp_runtime.c}
#else
#define DSP_MANT_FLEX (DSP_MANT)    // defined in dsp_header.h
#endif
#endif


#if DSP_ALU_FLOAT
#include <math.h>   // importing function sqrt
#endif


#if defined(__XS3A__)
#define DSP_XS3A 3
#define DSP_XS2A 2
#define DSP_XS1  1
#define DSP_ARCH DSP_XS3A
#elif defined(__XS2A__)
#define DSP_XS2A 2
#define DSP_XS1  1
#define DSP_ARCH DSP_XS2A
#elif defined(__XS1__)
#define DSP_XS1 1
#define DSP_ARCH DSP_XS1
#elif 0
// other architecture defines here
#define DSP_MYARCH 4
#define DSP_ARCH DSP_MYARCH
#endif


// special syntax for XMOS XCore compiler...
#if  defined(__XC__)
#define XCunsafe unsafe
#else
#define XCunsafe
#endif


//prototypes, compatible with XC specific compiler
opcode_t * XCunsafe dspFindCore(opcode_t * XCunsafe ptr, const int numCore);
opcode_t * XCunsafe dspFindCoreBegin(opcode_t * XCunsafe ptr);
int dspRuntimeReset(const int fs, int random, int defaultDither);
int dspRuntimeInit(opcode_t * XCunsafe codePtr, int maxSize, const int fs, int random, int defaultDither, unsigned cores);
int DSP_RUNTIME_FORMAT(dspRuntime)(opcode_t * XCunsafe ptr, dspSample_t * XCunsafe sampPtr, dspALU_t * XCunsafe ALUptr);
#ifdef DSP_XS2A //specific runtime for XCORE written in assembly
int dspRuntimeXS2(opcode_t * XCunsafe ptr, dspSample_t * XCunsafe sampPtr);
#endif


extern int dspMantissa;                        // reflects DSP_MANT or dspHeaderPtr->format
extern int dspDelayLineFactor;                 // used to compute size of delay line
extern int dspBiquadFreqOffset;                // used in biquad routine to compute coeficient adress
extern int dspBiquadFreqSkip;                  // used in biquad routine to compute coeficient adress at fs
extern int * dspRuntimeDataPtr;                // contains a pointer on the data area (after program)
extern int dspSaturationNumber;                //count the number of stauration and volume reduction by -1 or -6db for XS2
extern dspParam_t dspSaturationGain;           //represent the reduction gain applied due to saturation
extern dspParam_t dspSaturationVolume;         //represent volume master reduced by saturationgain
extern unsigned dspSaturationCoefUnsigned;
extern unsigned dspCoresToBeUsed;              // store cores condition given by dspRuntimeInit
extern opcode_t * dspCore2codePtr;             // point on core 2 if core 2 exist otherwise end of dsp program

//optional
#define dspDecibelTableSize (128)
#if defined(dspDecibelTableSize) && (dspDecibelTableSize>0)
extern dspParam_t dspDecibelTable[dspDecibelTableSize];
#endif



#endif /* DSP_RUNTIME_H_ */
