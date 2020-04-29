/*
 * dsp_coder.h
 *
 *  Created on: 2 janv. 2020
 *      Author: fabriceo
 */

#ifndef DSP_HEADER_H_
#define DSP_HEADER_H_

#define DSP_FORMAT_INT32        (1)
#define DSP_FORMAT_INT64        (2)
#define DSP_FORMAT_FLOAT        (3)
#define DSP_FORMAT_DOUBLE       (4)
#define DSP_FORMAT_FLOAT_FLOAT  (5)
#define DSP_FORMAT_DOUBLE_FLOAT (6)

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

// this define the precision for the fixed point maths when opcodes are encoded with DSP_FORMAT_INxx
// suggested format is 8.24 for parameters, gain and filters coeficients.
// for INT32, DSP_MANT is maximum 15 by design
// minimum is 8 by design
#ifndef DSP_MANT
#define DSP_MANT 28
#endif

// this defines the precision and format for the biquad coefficient only. suggested same as DSP_MANT but not mandatory
// remark for XS2 architecture, this value must be modified also in the biquad assembly file
#ifndef DSP_MANTBQ
#define DSP_MANTBQ 28
#endif

// special macro for s.31 format including saturation and special treatment for +1.0 recognition (nomally +1.0 is not possible!)
#define DSP_Q31_MAX     (0x000000007fffffffULL)
#define DSP_Q31_MIN     (0xFFFFFFFF80000000ULL)
#define DSP_2P31F       (2147483648.0)
#define DSP_2P31F_INV   (1.0/DSP_2P31F)
#define DSP_F31(x)      ( (x == DSP_Q31_MAX) ? 1.0 : (double)(x)/DSP_2P31F )
#define DSP_Q31(f)      ( (f >= 1.0 ) ? DSP_Q31_MAX : (f <= -1.0) ? DSP_Q31_MIN : ( (long long)( (f) * (DSP_2P31F + 0.5) ) ) )

// list of all DSP supported opcode as of this version
enum dspOpcodesEnum {
    DSP_END_OF_CODE,    // this opcode value is 0 and its length is 0 as a convention
    DSP_HEADER,         // contain summary information about the program.
    DSP_NOP,            // sometime used to align opcode adress start to a 8byte cell
    DSP_CORE,           // used to separate each dsp code trunck and distribute opcodes on multiple tasks.
    DSP_PARAM,          // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
    DSP_PARAM_NUM,      // same as PARAM but the data area is indexed and each param_num section can be accessed separately
    DSP_SERIAL,         // if not equal to product serial number, then DSP will reduce its output by 24db !

/* math engine */
    DSP_TPDF_CALC,      // generate radom number (white & triangular) and prepare for dithering bit Nth
    DSP_TPDF,           // prepare for dithering at bit N as parameter
    DSP_WHITE,          // load the random int32 number that was generated for the tpdf
    DSP_CLRXY,          // clear both ALU register
    DSP_SWAPXY,         // exchange ALU with second one "Y".
    DSP_COPYXY,         // save ALU X in a second "Y" register.
    DSP_COPYYX,         // copy ALU Y to ALU X

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
    DSP_SQRTX,          // perfomr X = sqrt(x) where x is int64 or float
    DSP_SHIFT,          // perform shift left or right if param is negative
    DSP_VALUE,          // load an imediate value qnm or float
    DSP_VALUE_INT,      // load an imediate int32 value without conversion
    DSP_MUL_VALUE,      // perform X = X * V where V is provided as a parameter qnm/float
    DSP_MUL_VALUE_INT,  // perform X = X * V where V is provided as a pure int32 value
    DSP_DIV_VALUE,      // perform X = X / V where V is provided as a parameter qnm/float
    DSP_DIV_VALUE_INT,  // perform X = X / V where V is provided as a pure int32 value
    DSP_AND_VALUE_INT,  // perform X = X & V (bitwise and)where V is provided as a pure int32 value

/* IO engine */
    DSP_LOAD,           // load a sample from the sample array location Z into the ALU "X" without conversion in s.31 format
                        // eg physical ADC input number = position in the sample array
    DSP_LOAD_GAIN,      // load a sample from the sample array location Z into the ALU "X" and apply a QNM gain. result is s4.59

    DSP_LOAD_MUX,       // combine many inputs samples into a value, same as summing many DSP_LOAD_GAIN. result is s4.59

    DSP_STORE,          // store the LSB of ALU "X" into the sample aray location Z without conversion. s.31 expected in ALU

    DSP_LOAD_STORE,     // move many samples from location X to Y without conversion (int32 or float) for N entries
                        // source in the sample array
                        // dest in the sample array

    DSP_LOAD_MEM,       // load a memory location 64bits into the ALU "X" without any conversion. ALU X saved in ALU Y
    DSP_STORE_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)

/* gains */
    DSP_GAIN,           // apply a fixed gain (eg 4.28) on the ALU , if a ALU was a sample s.31 then it becomes s5.59

    DSP_SAT0DB,         // verify boundaries -1/+1. input as s4.59, output as 33.31 (s.31 in lsb only)
    DSP_SAT0DB_TPDF,    // same + add the tpdf calculated and preformated
    DSP_SAT0DB_GAIN,    // apply a gain and then check boundaries
    DSP_SAT0DB_TPDF_GAIN,// apply a gain and the tpdf, then check boundaries

/* delays */
    DSP_DELAY_1,        // equivalent to a delay of 1 sample (Z-1)

    DSP_DELAY,          // execute a delay line (32 bits only). to be used just before a DSP_STORE for example.

    DSP_DELAY_DP,       // same as DSP_DELAY but 64bits (twice data required)

/* table of data */
    DSP_DATA_TABLE,      // extract one sample of a data block

/* filters */
    DSP_BIQUADS,        // execute N biquad. ALU is expected s4.59 and will be return as s4.59

    DSP_FIR,             // execute a fir filter with many possible impulse depending on frequency


/* workin progress only */
    DSP_RMS,            // compute sum of square during a given period then compute moving overage with sqrt (64bits->32bits)
    DSP_DCBLOCK,
    DSP_DITHER,         // add dithering on bit x
    DSP_DITHER_NS2,
    DSP_DISTRIB,        // for fun, use a dsp_WHITE before it
    DSP_DIRAC,          // generate a single sample pulse at a given frequency. pulse depends on provided float number
    DSP_SQUAREWAVE,
    DSP_CLIP,           // check wether a new sample is reaching the thresold given. ALU Y is FS square wave, ALU X is 1 sample pulse

    DSP_MAX_OPCODE      // latest opcode, supported by this runtime version. this will be compared in the runtime init
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


//search a literal frequency in the list of possible supported frequencies
// complier will replace this by a const table which was not possible to do inside a .h file :)
static inline int dspConvertFrequencyToIndex(int freq){
    switch (freq) {
    case  8000  : return F8000; break;
    case 16000  : return F16000; break;
    case 24000  : return F24000; break;
    case 32000  : return F32000; break;
    case 44100  : return F44100; break;
    case 48000  : return F48000; break;
    case 88200  : return F88200; break;
    case 96000  : return F96000; break;
    case 176400 : return F176400; break;
    case 192000 : return F192000; break;
    case 352800 : return F352800; break;
    case 384000 : return F384000; break;
    case 705600 : return F705600; break;
    case 768000 : return F768000; break;
    default :     return FMAXpos; break;
    }
}
static inline int dspConvertFrequencyFromIndex(enum dspFreqs freqIndex){
    switch (freqIndex) {
    case  F8000  : return 8000; break;
    case F16000  : return 16000; break;
    case F24000  : return 24000; break;
    case F32000  : return 32000; break;
    case F44100  : return 44100; break;
    case F48000  : return 48000; break;
    case F88200  : return 88200; break;
    case F96000  : return 96000; break;
    case F176400 : return 176400; break;
    case F192000 : return 192000; break;
    case F352800 : return 352800; break;
    case F384000 : return 384000; break;
    case F705600 : return 705600; break;
    case F768000 : return 768000; break;
    default :      return 768000; break;
    }
}

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
typedef struct dspHeader_s {    // 11 words
/* 0 */     opcode_t head;
/* 1 */     int   totalLength;  // the total length of the dsp program (in 32 bits words), rounded to upper 8bytes
/* 2 */     int   dataSize;     // required data space for executing the dsp program (in 32 bits words)
/* 3 */     unsigned checkSum;  // basic calculated value representing the sum of all opcodes used in the program
/* 4 */     int   numCores;     // number of cores/tasks declared in the dsp program
/* 5 */     int   version;      // version of the encoder used MAJOR, MINOR,BUGFIX
/* 6 */     unsigned short   format;       // contains DSP_MANT used by encoder or 0 for float encoding
/*   */     unsigned short   maxOpcode;    // last op code number used in this program (to check compatibility with runtime)
/* 7 */     int   freqMin;      // minimum frequency possible for this program, in raw format eg 44100
/* 8 */     int   freqMax;      // maximum frequency possible for this program, in raw format eg 192000
/* 9 */     unsigned usedInputs;   //bit mapping of all used inputs
/* 10 */    unsigned usedOutputs;  //bit mapping of all used outputs
        } dspHeader_t;

//prototypes
#include <stdio.h>
static inline void dspCalcSumCore(opcode_t * ptr, unsigned int * sum, int * numCore, unsigned int maxcode){
    *sum = 0;
    *numCore = 0;
    unsigned int p = 0;
    while(1){
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        if (skip == 0) {
            if (*numCore == 0) *numCore = 1;
            break;   // end of program encountered
        }
        if (code == DSP_CORE) (*numCore)++;
        *sum += ptr->u32;
        p += skip;
        if (p > maxcode) { printf("BUGG : p = %d, *p=0x%X\n",p,ptr->u32); break;}  // issue
        ptr += skip;
    } // while(1)
}

static inline long long dspQNMmax(){ return DSP_Q31_MAX; }
static inline long long dspQNMmin(){ return DSP_Q31_MIN; }

// convert a float number to a fixed point integer with a mantissa of "mant" bit
// eg : if mant = 28, the value 0.5 will be coded as 0x08000000
static inline int dspQNM(float f, int mant){
    int integ = 32-mant-1;
    int max = (1<<integ);
    float maxf = max;
    if (f >=   maxf)  return dspQNMmax();
    if (f < (-maxf))  return dspQNMmin();
    unsigned mul = 1<<mant;
    f *= mul;
    return f;
}


#endif /* DSP_HEADER_H_ */
