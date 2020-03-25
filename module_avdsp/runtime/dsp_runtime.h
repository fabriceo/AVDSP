/*
 * dsp_runtime.h
 *
 *  Created on: 9 janv. 2020
 *      Author: Fabrice
 */


#ifndef DSP_RUNTIME_H_
#define DSP_RUNTIME_H_

#include "dsp_qformat.h"    // used for macro with fixed point format like Q28(x) for 4.28 or Q(30)(x) for 2.30

#include "dsp_header.h"

#if (DSP_FORMAT == 1)     // 32bits integer model
#if (DSP_MANT > 15)
#error DSP_MANT max = 15bits
#endif
#if (DSP_MANT <8)
#error DSP_MANT min 8bits
#endif

#error "INT runtime not yet compatible"
    #define dspSample_t short       // samples always considered as int16 (use Q15(x) or F15(x) to convert from/to float)
    #define dspALU_t    int
    #define dspALU_SP_t dspALU_t    //single precision, here is same as normal precision
    #define dspParam_t  short       //int16
    #define DSP_ALU_INT 1
    #define DSP_ALU_INT32 1
    #define DSP_ALU_32B 1
    #define DSP_SAMPLE_INT 1
    #define DSP_SAMPLE_FLOAT 0
    #define DSP_RUNTIME_FORMAT(name) name ## _1

#elif (DSP_FORMAT == 2)

    #define dspSample_t int             // samples always considered as int32 (use Q31(x) or F31(x) to convert from/to float)
    #define dspALU_t signed long long   // dual precision
    #define dspALU_SP_t int             //single precision, for storage in delay line
    #define dspParam_t  int
    #define DSP_ALU_INT 1
    #define DSP_ALU_INT64 1
    #define DSP_ALU_64B 1
    #define DSP_SAMPLE_INT 1
    #define DSP_SAMPLE_FLOAT 0
    #define DSP_RUNTIME_FORMAT(name) name ## _2
#if (DSP_MANT <8)
#error "DSP_MANT min 8bits"
#endif
#if (DSP_MANT >30)
#error "DSP_MANT max 30bits"
#endif

#elif (DSP_FORMAT == 3)             // sample is int32, alu is float32, param float32

    #define dspSample_t int         // samples always considered as int32 (use Q31(x) or F31(x) to convert from/to float)
    #define dspALU_t    float
    #define dspALU_SP_t dspALU_t    //single precision, here is same as normal precision
    #define dspParam_t  float
    #define DSP_ALU_FLOAT 1
    #define DSP_ALU_FLOAT32 1
    #define DSP_ALU_FLOAT64 0
    #define DSP_ALU_32B 1
    #define DSP_SAMPLE_INT 1
    #define DSP_SAMPLE_FLOAT 0
    #define DSP_RUNTIME_FORMAT(name) name ## _3

#elif (DSP_FORMAT == 4)               // sample int32, alu is double float, param float32

    #define dspSample_t int         // samples always considered as int32 (use Q31(x) or F31(x) to convert from/to float)
    #define dspALU_t double
    #define dspALU_SP_t float       // single precision, for storage in standard delay line
    #define dspParam_t float
    #define DSP_ALU_FLOAT 1
    #define DSP_ALU_FLOAT32 0
    #define DSP_ALU_FLOAT64 1
    #define DSP_ALU_64B 1
    #define DSP_SAMPLE_INT 1
    #define DSP_SAMPLE_FLOAT 0
    #define DSP_RUNTIME_FORMAT(name) name ## _4

#elif (DSP_FORMAT == 5)               // sample float, alu float32, param float32

    #define dspSample_t float
    #define dspALU_t    float
    #define dspALU_SP_t float       // single precision, for storage in standard delay line
    #define dspParam_t  float
    #define DSP_ALU_FLOAT 1
    #define DSP_ALU_FLOAT32 1
    #define DSP_ALU_FLOAT64 0
    #define DSP_ALU_32B 1
    #define DSP_SAMPLE_INT 0
    #define DSP_SAMPLE_FLOAT 1
    #define DSP_RUNTIME_FORMAT(name) name ## _5

#elif (DSP_FORMAT == 6)               // sample float, alu double float, param float32

    #define dspSample_t float
    #define dspALU_t double
    #define dspALU_SP_t float       // single precision, for storage in standard delay line
    #define dspParam_t float
    #define DSP_ALU_FLOAT 1
    #define DSP_ALU_FLOAT32 0
    #define DSP_ALU_FLOAT64 1
    #define DSP_ALU_64B 1
    #define DSP_SAMPLE_INT 0
    #define DSP_SAMPLE_FLOAT 1
    #define DSP_RUNTIME_FORMAT(name) name ## _6

#endif

// special syntax for XCore compiler...
#if  defined(__XC__)
#define XCunsafe unsafe
#else
#define XCunsafe
#endif

#if defined(DSP_ARCH) && defined( DSP_XS2A )     // specific for xmos xs2 architecture
// fast biquad assembly routine inspired from https://github.com/xmos/lib_dsp/blob/master/lib_dsp/src/dsp_biquad.S
// value for DSP_MANTBQ is defined INSIDE the assembly file and must be updated according to DSP_MANT in dsp_header.h
extern long long dsp_biquads_xs2(dspSample_t xn, dspParam_t * coefPtr, dspSample_t * dataPtr, int num);
#endif


//prototypes
opcode_t * XCunsafe dspFindCore(opcode_t * XCunsafe ptr, const int numCore);
int dspRuntimeInit(opcode_t * XCunsafe codePtr, int maxSize, const int fs, int random);
int DSP_RUNTIME_FORMAT(dspRuntime)(opcode_t * XCunsafe ptr, int * XCunsafe rundataPtr, dspSample_t * XCunsafe sampPtr);

#endif /* DSP_RUNTIME_H_ */
