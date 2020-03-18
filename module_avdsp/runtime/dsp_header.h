/*
 * dsp_coder.h
 *
 *  Created on: 2 janv. 2020
 *      Author: fabriceo
 */

#ifndef DSP_HEADER_H_
#define DSP_HEADER_H_

#include "dsp_qformat.h"    // used for macro with fixed point format like Q28(x) for 4.28 or Q(30)(x) for 2.30

#define DSP_FORMAT_INT32 1
#define DSP_FORMAT_INT64 2
#define DSP_FORMAT_FLOAT 3
#define DSP_FORMAT_DOUBLE 4
#define DSP_FORMAT_FLOAT_FLOAT 5
#define DSP_FORMAT_DOUBLE_FLOAT 6

#ifndef DSP_FORMAT  // used in the runtime only, dynamic change of key type for optimizing the runtime
#define DSP_FORMAT DSP_FORMAT_INT64  // samples int32, alu int 64 bits, param int32 (q4.28 as defined later below)
#endif

#ifdef __XS2A__     // specific for xmos xs2 architecture
#define DSP_XS2A 1
#define DSP_ARCH DSP_XS2A
#elif 0
// other architecture defines here
#define DSP_MYARCH 2
#define DSP_ARCH DSP_MYARCH
#endif


//#define DSP_PRINTF 3 // 1=basic info encoder sumary and runime context, 2 provides encoder details and runtime, 3 full debug info
#if defined(DSP_PRINTF) && ( DSP_PRINTF >=1 )
#include <stdio.h>
#define dspprintf(...)  { printf(__VA_ARGS__); }   // we do the normal printf
#define dspprintf1(...) { if (DSP_PRINTF>=1) printf(__VA_ARGS__); }
#define dspprintf2(...) { if (DSP_PRINTF>=2) printf(__VA_ARGS__); }
#define dspprintf3(...) { if (DSP_PRINTF>=3) printf(__VA_ARGS__); }
#else
#define dspprintf(...)  { } // we do nothing
#define dspprintf1(...) { }
#define dspprintf2(...) { }
#define dspprintf3(...) { }
#endif

// this define the precision for the fixed point maths.
// code is optimized for int64 ALU . suggested format is 4.28 for param, gain and coeficients and double (8.56) for ALU
#ifndef DSP_MANT
#define DSP_MANT 28
#endif
#ifndef DSP_INT
#define DSP_INT (32-DSP_MANT)
#endif
#define DSP_INTDP (DSP_INT*2)   // double precision is twice the above DSP_MANT
#define DSP_MANTDP (64-DSP_INTDP)
#define DSP_MANTDP_INT32 (DSP_MANTDP-32)

// this defines the precision and format for the biquad coefficient only. suggested same as DSP_MANT but not mandatory
#ifndef DSP_MANTBQ
#define DSP_MANTBQ 28
#endif
#ifndef DSP_INTBQ
#define DSP_INTBQ (32-DSP_MANTBQ)
#endif

// define the fixed point macro used for encoding the default 32 bits parameters (gain, mux...) coded with DSP_MANT
// Q(x)(y) is imported from dsp_qformat.h
#ifndef DSP_QNM
#define DSP_QNM(f) Q(DSP_MANT)(f)
#endif
// define the fixed point macro used for encoding the default int biquad coeficients
#ifndef DSP_QNMBQ
#define DSP_QNMBQ(f) Q(DSP_MANTBQ)(f)
#endif
// special macro for 0.31 format including saturation and special treatment for +1.0 / -1.0 recognition (nomally +1.0 is not possible!)
#define DSP_F31(x) ((x == 0x7FFFFFFF) ? 1.0 : (x == 0xFFFFFFFF) ? -1.0 : (double)(x)/(double)(2147483648.0))
#define DSP_Q31(f) ((f >= 1.0 ) ? (int)0x7FFFFFFF : (f <= -1.0) ? (int)(0x80000001) : (int)((signed long long)((f) * (1ULL << (31+20)) + (1<<19)) >> 20))


// list of all DSP supported opcode as of this version
enum dspOpcodesEnum {
    DSP_END_OF_CODE,    // this opcode value is 0 and its length is 0 as a convention
    DSP_HEADER,         // contain key information about the program.
    DSP_NOP,            // just for fun
    DSP_CORE,           // used to separate each dsp code trunck and distribute opcodes on multiple tasks.
    DSP_PARAM,          // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
    DSP_PARAM_NUM,      // same as PARAM but the data area is indexed and can be accessed separately
    DSP_SERIAL,         // if not equal to product serial number, then DSP will reduce its output by 24db !

/* math engine */
    DSP_TPDF,           // create a white random TPDF value for dithering, either -1/0/+1 at each call.
    DSP_WHITE,          // load the random number that was generated for the tpdf-seed
    DSP_CLRXY,          // clear both ALU register
    DSP_SWAPXY,         // exchange ALU with second one "Y". no additional param
    DSP_COPYXY,         // save ALU X in a second "Y" register. no additional param

    DSP_ADDXY,          // perform X = X + Y, 64 bits
    DSP_ADDYX,          // perform Y = X + Y
    DSP_SUBXY,          // perform X = X - Y
    DSP_SUBYX,          // perform Y = Y - X
    DSP_MULXY,          // perform X = X * Y
    DSP_DIVXY,          // perform X = X / Y
    DSP_DIVYX,          // perform Y = Y / X
    DSP_AVGXY,          // perform X = X/2 + Y/2;
    DSP_AVGYX,          // perform Y = X/2 + Y/2;
    DSP_NEGX,           // perform X = -X
    DSP_NEGY,           // perform Y = -Y
    DSP_SQRTX,          // perfomr X = sqrt(x) where x is int64
    DSP_VALUE,          // load an imediate value (int32 or 4.28)
    DSP_SHIFT_VALINT,   // perform shift left or right if param is negative
    DSP_MUL_VALUE,      // perform X = X * V where V is provided as a parameter (int32 or 4.28)
    DSP_DIV_VALUE,      // perform X = X / V where V is provided as a parameter (int32 or 4.28)

/* IO engine */
    DSP_LOAD,           // load a sample from the sample array location Z into the ALU "X" without conversion in 0.31 format
                        // eg physical ADC input number = position in the sample array
    DSP_LOAD_GAIN,      // load a sample from the sample array location Z into the ALU "X" and apply a QNM gain. result is 5.59

    DSP_LOAD_MUX,       // combine many inputs samples into a value, same as summing many DSP_LOAD_GAIN. result is 5.59

    DSP_STORE,          // store the LSB of ALU "X" into the sample aray location Z without conversion. 0.31 expected in input

    DSP_LOAD_STORE,     // move many samples from location X to Y without conversion (int32 or float) for N entries
                        // source in the sample array
                        // dest in the sample array

    DSP_LOAD_MEM,       // load a memory location 64bits into the ALU "X" without any conversion. ALU saved in ALU2
    DSP_STORE_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)

/* gains */
    DSP_GAIN,           // apply a fixed gain (eg 4.28) on the ALU result is 8.56

    DSP_SAT0DB,         // verify boundaries -1/+1. input as 8.56, output as 33.31 (0.31 in lsb only)
    DSP_SAT0DB_TPDF,    // same + add the tpdf calculated
    DSP_SAT0DB_GAIN,    // apply a gain and then check boundaries
    DSP_SAT0DB_TPDF_GAIN,// apply a gain and the tpdf, then check boundaries

/* delays */
    DSP_DELAY_1,        // equivalent to a delay of 1 sample (Z-1)

    DSP_DELAY,          // execute a delay line (32 bits only). to be used just before a DSP_STORE for example.

    DSP_DELAY_DP,       // same as DSP_DELAY but 64bits (twice data required)

/* table of data */
    DSP_DATA_TABLE,      // extract one sample of a data block

/* filters */
    DSP_BIQUADS,        // execute N biquad. ALU is expected 8.56 and will be converted 4.28. result is 8.56

    DSP_FIR,             // execute a fir filter with many possible impulse depending on frequency


/* workin progress only */
    DSP_RMS,            // compute sum of square during a given period then compute moving overage but no sqrt
    DSP_CIC_D,          // decimator CIC filter delay D as parameter
    DSP_CIC_I,          // interpolator CIC filter delay D as parameter
    DSP_NOISE_SHAPE,    // add dithering, calculate error, execute filter to shape error, prepare for injection of error

    DSP_MAX_OPCODE      // latest opcode, supported by this runtime version
};


enum dspFreqs {
    F8000,   F16000,
    F24000,  F32000,
    F44100,  F48000,
    F88200,  F96000,
    F176400, F192000,
    F352800, F384000,
    F705600, F768000,
    FMAXpos
};


#define DSP_DEFAULT_MIN_FREQ (F44100)
#define DSP_DEFAULT_MAX_FREQ (F192000)


typedef float  dspGainParam_t;          // all gain type of variable considered as float
typedef double dspFilterParam_t;        // can be changed for float but then biqad coef will loose some precision when encoded in 4.28

typedef long long __attribute__((aligned(8))) dspAligned64_t;


typedef union opcode_u {
            struct opcode_s {   // both coded in a single word. to be changed in a later version for separate words
                unsigned short skip;
                unsigned short opcode; } op;
            struct short_s {
                short low;
                short high; } s16;
            unsigned u32;
            int      i32;
            float    f32;
            int      i[1];
            unsigned u[1];
        } opcode_t;


//used at the very begining of the dsp program to store basic information
typedef struct dspHeader_s {    // 13 words
            opcode_t head;
            int   totalLength;  // the total length of the dsp program (in 32 bits words), rounded to upper 8bytes
            int   dataSize;     // required data space for executing the dsp program (in 32 bits words)
            unsigned checkSum;  // basic calculated value representing the sum of all opcodes used in the program
            int   numCores;     // number of cores/tasks declared in the dsp program
            int   version;      // version of the encoder used
            int   maxOpcode;    // last op code number used in this program (to check compatibility with runtime)
            int   freqMin;      // minimum frequency possible for this program, in raw format eg 44100
            int   freqMax;      // maximum frequency possible for this program, in raw format eg 192000
            unsigned usedInputs;   //bit mapping of all used inputs
            unsigned usedOutputs;  //bit mapping of all used outputs
        } dspHeader_t;

//prototypes

static inline void dspCalcSumCore(opcode_t * ptr, unsigned int * sum, int * numCore){
    *sum = 0;
    *numCore = 0;
    while(1){
        int code = ptr->op.opcode;
        int skip = ptr->op.skip;
        if (skip == 0) break;   // end of program encountered
        if (code == DSP_CORE) (*numCore)++;
        *sum += ptr->u32;
        ptr += skip;
    } // while(1)
    if (*numCore == 0) *numCore = 1;
}


#endif /* DSP_HEADER_H_ */
