/*
 * dsp_runtime.h
 *
 *  Created on: 9 janv. 2020
 *      Author: Fabrice
 */


#ifndef DSP_RUNTIME_H_
#define DSP_RUNTIME_H_

#include "dsp_header.h"

#if defined(  DSP_MANT_FLEXIBLE ) && (DSP_MANT_FLEXIBLE > 0)
#define DSP_MANT_FLEX (dspMantissa) // defined in dsp_runtime.h
#else
#define DSP_MANT_FLEX (DSP_MANT)    // defined in header.h
#endif

typedef union dspALU32_u { int i; float f; } dspALU32_t;
typedef struct dsplh_s   { unsigned lo; int hi; } dsplh_t;
typedef union dspALU64_u { long long i; double f; dsplh_t lh; } dspALU64_t;


#if (DSP_FORMAT == 1)     // 32bits integer model
#error "INT runtime not yet compatible"

    typedef dspALU32_t  dspALU_u;
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

#elif (DSP_FORMAT == 2)

    typedef dspALU64_t  dspALU_u;
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

#elif (DSP_FORMAT == 3)             // sample is int32, alu is float32, param float32

    typedef dspALU32_t  dspALU_u;
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

#elif (DSP_FORMAT == 4)             // sample int32, alu is double float, param float32

    typedef dspALU64_t  dspALU_u;
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

#elif (DSP_FORMAT == 5)               // sample float, alu float32, param float32

    typedef dspALU32_t  dspALU_u;
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

#elif (DSP_FORMAT == 6)               // sample float, alu double float, param float32

    typedef dspALU64_t  dspALU_u;
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

#else
#error "DSP_FORMAT undefined or not supported"
#endif

#if DSP_ALU_FLOAT
#include <math.h>   // importing function sqrt
#endif

#if defined(__XS2A__)
#define DSP_XS2A 2
#define DSP_XS1  1
#define DSP_ARCH DSP_XS2A
#elif defined(__XS1__)
#define DSP_XS1 1
#define DSP_ARCH DSP_XS1
#elif 0
// other architecture defines here
#define DSP_MYARCH 3
#define DSP_ARCH DSP_MYARCH
#endif


// special syntax for XCore compiler...
#if  defined(__XC__)
#define XCunsafe unsafe
#else
#define XCunsafe
#endif


//prototypes
opcode_t * XCunsafe dspFindCore(opcode_t * XCunsafe ptr, const int numCore);
opcode_t * XCunsafe dspFindCoreBegin(opcode_t * XCunsafe ptr);
int dspRuntimeReset(const int fs, int random, int defaultDither);
int dspRuntimeInit(opcode_t * XCunsafe codePtr, int maxSize, const int fs, int random, int defaultDither);
int DSP_RUNTIME_FORMAT(dspRuntime)(opcode_t * XCunsafe ptr, int * XCunsafe rundataPtr, dspSample_t * XCunsafe sampPtr);

#endif /* DSP_RUNTIME_H_ */
