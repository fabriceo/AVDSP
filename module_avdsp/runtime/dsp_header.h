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
// code is optimized for int64 ALU and best instruction generation with parameters as 8.24 and ALU as 8.56
#ifndef DSP_MANT
#define DSP_MANT 24
#endif
#ifndef DSP_INT
#define DSP_INT (32-DSP_MANT)
#endif
#define DSP_INTDP (DSP_INT)     // suggestion to have same interger size for DP or Single precision to simplify msb extraction
#define DSP_MANTDP (64-DSP_INTDP)
#define DSP_MANTDP_INT32 (DSP_MANTDP-32)

#ifndef DSP_MANTBQ
#define DSP_MANTBQ 24           // suggested 8.24 format for biquad coef , could be 0.30 max if -2<coef <2
#endif
#ifndef DSP_INTBQ
#define DSP_INTBQ (32-DSP_MANTBQ)
#endif

// define the fixed point macro used for encoding the default 32 bits parameters (gain, mux...)
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
#define DSP_Q31(f) ((f >= 1.0 ) ? (int)0x7FFFFFFF : (f <= -1.0) ? (int)(-1) : (int)((signed long long)((f) * ((unsigned long long)1 << (31+20)) + (1<<19)) >> 20))


// list of all DSP supported opcode as of this version
enum dspOpcodesEnum {
    DSP_END_OF_CODE,    // this opcode value is 0 and its length is 0 as a convention
    DSP_HEADER,         // contain key information about the program. will be generated at begining of program
    DSP_NOP,            // just for fun
    DSP_CORE,           // used to separate each dsp code trunck and distribute opcodes on multiple tasks. at least one mandatory
    DSP_PARAM,          // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
                            // predefined values data : N words, can be changed remotely by adressing in this area with boudarie control;
    DSP_PARAM_NUM,      // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
                        // num of the table
                            // predefined values data : N words, can be changed remotely by adressing in this area with boudarie control;
    DSP_SERIAL,         // if not equal to product serial number, then DSP will reduce its output by 24db !
                        // serial number
                        // special checksum number formula tbd
/* math engine */
    DSP_CLRXY,          // clear both ALU register
    DSP_SWAPXY,         // exchange ALU with second one "Y". no additional param
    DSP_COPYXY,         // save ALU X in a second "Y" register. no additional param
    DSP_ADDXY,          // perform X = X + Y
    DSP_SUBXY,          // perform X = X + Y
    DSP_MULXY,          // perform X = X * Y
    DSP_DIVXY,          // perform X = X / Y
    DSP_SQRTX,          // perfomr X = sqrt(x)
    DSP_SAT0DB,         // maximize or minimize ALU (DP) to -1..1 and convert to 0.31
    DSP_TPDF,           // modify the ALU to add triangular random patern to the lsb bits provided in param
                        // number of lower bits to add
/* IO engine */
    DSP_LOAD,           // load a sample from the sample array location Z into the ALU "X" without conversion
                        // eg physical ADC input number = position in the sample array
    DSP_STORE,          // store the LSB of ALU "X" into the sample aray location Z without conversion
                        // eg physical DAC output number = position in the sample array
    DSP_STORE_TPDF,     // gives same result as combining DSP_PDF and DSP_STORE
                        // physical DAC output
                        // tpdf size in number of bits
    DSP_LOAD_DP,        // load a sample from the sample array location Z into the ALU "X" and convert it in DSP_QNMDP 64 bits format
                        // eg physical ADC input number = position in the sample array
    DSP_STORE_DP,       // store the ALU "X" into the sample aray location Z converting ALU to original signed 32 bits format
                        // eg physical DAC output number = position in the sample array
    DSP_LOAD_STORE,     // move many samples from location X to Y without conversion (int32 or float) for N entries
                        // source in the sample array
                        // dest in the sample array
    DSP_LOAD_MEM,       // load a memory location (32 or 64bits) into the ALU "X" without any conversion.
                        // offset in the opcode table
    DSP_STORE_MEM,      // store the ALU "X" into a memory location without conversion (raw 32 or 64bits)
                        // offset in the opcode table
/* gain and mux */
    DSP_GAIN,           // apply a fixed gain on the ALU considering the parameter as a signed QNM bit format (eg 4.28) or float
                        // offset adress in a data space where the gain is stored (default value at first runtime)
    DSP_GAIN0DB,        // apply a fixed gain on the ALU considering the parameter as a signed 0.31 format so in range -1..1
                        // offset adress in a data space where the gain is stored (default value at first runtime)
    DSP_MACC,           // for many samples located in the sample aray at X,Y,Z..., multiply & add sample(X..Z) by gain(X...Z)
                        // number of couples
                        // offset where the table is located in a data space
                            // input number in sample array
                            // gain value (1 word) in QN_M or float
    DSP_MUX0DB,         // combine many inputs samples into a value, same as DPS_MACC but ALU start at 0 and gain are -1..1

/* delays */
    DSP_DELAY_1,        // equivalent to a delay of 1 sample (Z-1)
                        // offset in the data area where the sample is stored (64 bits)
    DSP_DELAY,          // execute a fixed delay line (32 bits)
                        // offset in a dataspace where the delay line parameter are stored, or just below optionally
                        // offset in the running data space where the samples are stored, and the index on current sample in data[0]
    DSP_DELAY_DP,       // same as DSP_DELAY but 64bits (twice data required)

/* table of data */
    DSP_DATA_TABLE,      // extract one sample of a data block
                         // gain in 0.31 format to be applied to each sample
                         // frequency divider : if N then will skip N-1 sample at each cycle
                         // table size in words (used for index rollover)
                         // offset in the data space where the table is stored (potentially below)
                         // current index for access to the table (start at 0)
/* filters */
    DSP_BIQUADS,        // execute N biquad
                        // number of biquad sections .
                        // offset of the data space where the coef are stored, possibly stored just below
                        // offset of running state data (4 words per biquads for storing xn-1,xn-2,yn-1,yn-2)
                            // optional filter coef : 5 word per filter, always multiplied by number of frequency covered (eg 8)
    DSP_FIR,             // execute a fir filter with many possible impulse depending on frequency
                         // offset where the largest array for holding filter state data is located
                         // table of offset of each impulse per freq

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


typedef float  dspGainParam_t;
typedef double dspFilterParam_t;        // can be changed for float but then biqad coef will loose some precision when encoded in 4.28

typedef long long __attribute__((aligned(8))) dspAligned64_t;


typedef union opcode_u {
            struct opcode_s {
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


typedef struct dspHeader_s {    // 7 words length
            opcode_t head;
            int   totalLength;
            int   dataSize;
            int   checkSum;
            int   numCores;
            short version;
            short maxOpcode;
            short freqMin;
            short freqMax;
        } dspHeader_t;

//prototypes

static inline void dspCalcSumCore(opcode_t * ptr, unsigned int * sum, int * numCore){
    *sum = 0;
    *numCore = 0;
    while(1){
        short code = ptr->op.opcode;
        short skip = ptr->op.skip;
        if (skip == 0) break;   // end of program encountered
        if (code == DSP_CORE) (*numCore)++;
        *sum += ptr->u32;
        ptr += skip;
    } // while(1)
}


#endif /* DSP_CODER_H_ */
