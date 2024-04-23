/*
 * dsp_header.h
 *
 *  Version: May 1st 2020
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
#define dspprintf1(...) { printf(__VA_ARGS__); }   // we do the normal printf
#else
#define dspprintf(...)  { } // we do nothing
#define dspprintf1(...) { }
#endif
#if defined(DSP_PRINTF) && ( DSP_PRINTF >=2 )
#define dspprintf2(...) { printf(__VA_ARGS__); }
#else
#define dspprintf2(...) { }
#endif
#if defined(DSP_PRINTF) && ( DSP_PRINTF >=3 )
#define dspprintf3(...) { printf(__VA_ARGS__); }
#else
#define dspprintf3(...) { }
#endif


// list of all DSP supported opcode as of this version. The last opcode is marked in the header
enum dspOpcodesEnum {
    DSP_END_OF_CODE,    // 0 this opcode value is 0 and its length is 0 as a convention
    DSP_HEADER,         // contain summary information about the program.
    DSP_PARAM,          // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
    DSP_PARAM_NUM,      // same as PARAM but the data area is indexed and each param_num section can be accessed separately
    DSP_NOP,            // sometime used to align opcode adress start to a 8byte cell
    DSP_CORE,           // used to separate each dsp code trunck and distribute opcodes on multiple tasks.
    DSP_SECTION,        //conditional section.

/* IO engine */
    DSP_LOAD,           // 6 load a sample from the sample array location Z into the ALU "X" without conversion in s.31 format
    DSP_STORE,          // store the LSB of ALU "X" into the sample aray location Z without conversion. sat0db expected upfront
    DSP_LOAD_STORE,     // move many samples from location X to Y without conversion (int32 or float) for N entries

    DSP_STORE_TPDF,     //apply a gain and store result in an output
    DSP_STORE_GAIN,     //apply a gain and store result in an output
    DSP_LOAD_GAIN,      // load a sample from the sample array location Z into the ALU "X" and apply a Qnm gain. result is double precision
    DSP_LOAD_MUX,       // combine many inputs samples into a value, same as summing many DSP_LOAD_GAIN. result is double precision
    DSP_MIXER,          // load all inputs with their respective gain, couples stores below opcode

    DSP_LOAD_X_MEM,       // load a memory location 64bits into the ALU "X" without any conversion.
    DSP_STORE_X_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)
    DSP_LOAD_Y_MEM,       // load a memory location 64bits into the ALU "X" without any conversion.
    DSP_STORE_Y_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)

    DSP_LOAD_MEM_DATA,  // load the double precision value stored in data space at an absolute adress

/* math engine */

    DSP_CLRXY,          // 15 clear both ALU register
    DSP_SWAPXY,         // exchange ALU X with second one "Y".
    DSP_COPYXY,         // copy ALU X in a second "Y" register.
    DSP_COPYYX,         // copy ALU Y to ALU X
    DSP_ADDXY,          // perform X = X + Y, 64 bits
    DSP_ADDYX,          // perform Y = X + Y
    DSP_SUBXY,          // perform X = X - Y
    DSP_SUBYX,          // perform Y = Y - X
    DSP_MULXY,          // perform X = X * Y
    DSP_MULYX,          // perform X = X * Y
    DSP_DIVXY,          // perform X = X / Y
    DSP_DIVYX,          // perform Y = Y / X
    DSP_AVGXY,          // perform X = X/2 + Y/2;
    DSP_AVGYX,          // perform Y = X/2 + Y/2;
    DSP_NEGX,           // perform X = -X
    DSP_NEGY,           // perform Y = -Y
    DSP_SHIFT,          // perform shift left or right if param is negative
    DSP_VALUEX,          // load an imediate value qnm or float
    DSP_VALUEY,      // load an imediate int32 value without conversion


/* gains */
    DSP_GAIN,           // 38 apply a fixed gain (qnm or float) on the ALU , if a ALU was a sample s.31 then it becomes s4.59
    DSP_CLIP,           // check wether the sample is reaching the thresold given and maximize/minize it accordingly.

    DSP_SAT0DB,         // verify boundaries -1/+1.
    DSP_SAT0DB_VOL,     // apply volume and then verify boundaries -1/+1.
    DSP_SAT0DB_TPDF,    // same + add the tpdf calculated and preformated
    DSP_SAT0DB_GAIN,    // apply a gain and then check boundaries as above
    DSP_SAT0DB_TPDF_GAIN,// apply a gain and the tpdf, then check boundaries as above
    DSP_SERIAL,         // if not equal to product serial number, then DSP will reduce its output by 24db !

/* delays */
    DSP_DELAY_1,        // 45 equivalent to a delay of 1 sample (Z-1), used to synchronize multi-core output.
    DSP_DELAY,          // 46 execute a delay line (32 bits ONLY). to be used just before a DSP_STORE for example.
    DSP_DELAY_DP,       // 47 same as DSP_DELAY but 64bits (twice data space required ofcourse)


/* filters */
    DSP_BIQUADS,        // execute N biquad cell. ALU is expected s4.59 and will be return as s4.59
    DSP_DCBLOCK,        // remove any DC offset. equivalent to first order high pass with noise reinjection

/* specials */
    DSP_DATA_TABLE,      // extract one sample of a data block. typically used for wave generation
    DSP_TPDF_CALC,      // generate radom number (white & triangular) and prepare for dithering bit Nth
    DSP_TPDF,           // prepare for dithering at bit N as parameter
    DSP_WHITE,          // load the random int32 number that was generated for the tpdf
    DSP_DITHER,         // add dithering on bit x and noise shapping
    DSP_DITHER_NS2,     // same with custom noise shapping factors
    DSP_DISTRIB,        // experimental : distribute the value of ALU towards a table[N] and provide table[i] as outcome. Used to show the noise distibution on a scope view
    DSP_DIRAC,          // generate a single sample pulse at a given frequency. frequency and pulse amplitude depends on provided float number
    DSP_SQUAREWAVE,     // generate a square waved (zero symetrical) at a given frequency and amplitude
    DSP_SINE,           // generate a sine wave (zero symetrical) at a given frequency using modified coupled form oscillator.
    DSP_SQRTX,          // perfomr X = sqrt(x) where x is int64 or float
    DSP_RMS,            // compute sum of square during a given period then compute moving overage with sqrt (64bits->32bits)
    DSP_FIR,             // execute a fir filter with many possible impulse depending on frequency, EXPERIMENTAL

    // new code after release 1.0

    DSP_MAX_OPCODE      // latest opcode, supported by this runtime version. this will be compared during runtimeinit
};

extern const char * dspOpcodeText[DSP_MAX_OPCODE];  //defined in dsp_header.c

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
    default :     break; }
    return FMAXpos;
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
    case F768000 : break;
    default :      break; }
    return 768000;
}

#define DSP_DEFAULT_MIN_FREQ (F44100)
#define DSP_DEFAULT_MAX_FREQ (F192000)

typedef float  dspGainParam_t;          // all gain type of variable considered as float
typedef double dspFilterParam_t;        // can be changed for float but then biqad coef will loose some precision when encoded in 4.28

typedef long long __attribute__((aligned(8))) dspAligned64_t;

typedef union opcode_u {
    struct opcode_s {   // both coded in a single word
        short skip;
        short opcode; } op;
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
/* 0 */     opcode_t head;      // marker
/* 1 */     int   totalLength;  // the total length of the dsp program (in 32 bits words), rounded to upper 8bytes
/* 2 */     int   dataSize;     // required data space for executing the dsp program (in 32 bits words)
/* 3 */     unsigned checkSum;  // basic calculated value representing the sum of all opcodes used in the program
/* 4 */     int   numCores;     // number of cores/tasks declared in the dsp program
/* 5 */     int   version;      // version of the encoder used MAJOR, MINOR,BUGFIX
/* 6 */     unsigned short   format;       // contains DSP_MANT used by encoder or 0 for float encoding (recomended)
/*   */     unsigned short   maxOpcode;    // last op code number used in this program (to check compatibility with runtime)
/* 7 */     int   freqMin;          // minimum frequency possible for this program, in raw format eg 44100
/* 8 */     int   freqMax;          // maximum frequency possible for this program, in raw format eg 192000
/* 9 */     unsigned usedInputs;    // bit mapping of all used inputs  (max 32 in this version)
/* 10 */    unsigned usedOutputs;   // bit mapping of all used outputs (max 32 in this version)
/* 11 */    unsigned serialHash;    // hash code to enable 0dbFS output (otherwise -24db)
        } dspHeader_t;


//this function is declared inline and not stored in dsp_header.c
//just for compatibility with XMOS XC compiler (due to unsafe pointers)...
//it is used only once in both dsp_runtime.c and dsp_encoder.c
static inline void dspCalcSumCore(opcode_t * ptr, unsigned int * sum, int * numCore, unsigned int maxcode){
    *sum = 0;
    *numCore = 0;
    unsigned int p = 0;
    while(1){
        enum dspOpcodesEnum code = ptr->op.opcode;
        int skip = ptr->op.skip;
        if (skip == 0) {
            if (*numCore == 0) *numCore = 1;
            break;   // end of program encountered
        }
        if (code == DSP_CORE) (*numCore)++;
        if ( ( *numCore == 0 ) &&   //any first opcode will generate a core
                (code != DSP_HEADER) &&
                (code != DSP_NOP) &&
                (code != DSP_PARAM) &&
                (code != DSP_PARAM_NUM) ) *numCore = 1;
        *sum += ptr->u32;
        p += skip;
        if (p > maxcode) { dspprintf("BUGG in memory : p = %d, *p=0x%X\n",p,ptr->u32); break;}  // fatal issue
        ptr += skip;
    } // while(1)
}


// this define the precision for the fixed point maths when runtime is using DSP_FORMAT_INTxx
// suggested format is 4.28 for parameters, gain and filters coeficients.
// for INT32, DSP_MANT is maximum 15 by design
// minimum is 8 by design
// remark for XS2/XS3 architecture, this value MUST be modified also in the assembly file as it is not passed as parameter
#ifndef DSP_MANT
#define DSP_MANT 28
#endif

// this defines the precision and format for the biquad coefficient ONLY.
// suggested same as DSP_MANT but not mandatory, absolute maximum is 30, to allow coeff between -2..+1.999
// remark for XS2/XS3 architecture this value is forced same as DSP_MANT
#ifndef DSP_MANTBQ
#define DSP_MANTBQ 28
#endif


// convert a float/double number to a fixed point integer with a mantissa of "m" bit
// eg : if mant = 28, the value 0.5 will be coded as 0x08000000 = 2^27
// and the maximum number for a 32bits container will be 7.99999999 (2^3-epsilon) as bit 31 is used for sign
// using double as input in order to have 52bits precision compared to float which is 24bits mantissa only
// should works for any target container between 8bits and 64 bits. QM32 and QM64 are predefined for 32/64bits targets

#define DSP_MAXPOS(b) ( ((b)>=64) ?   9223372036854775807LL      : ((1UL << (b-1))-1) )
#define DSP_MINNEG(b) ( ((b)>=64) ? (-9223372036854775807LL-1LL) :  (1UL << (b-1))    )
#define DSP_QMSCALE(x,m,b) ( ((b)>=33) ? (long long)((double)(x)*(1LL<<(m))) : (int)((double)(x)*(1L<<(m))) )
#define DSP_QMBMIN(x,m,b) ( ( (-(x)) >  ( 1ULL << ( (b)-(m)-1) ) ) ? DSP_MINNEG(b) : DSP_QMSCALE(x,m,b) )
#define DSP_QMBMAX(x,m,b) ( (   (x)  >= ( 1ULL << ( (b)-(m)-1) ) ) ? DSP_MAXPOS(b) : DSP_QMBMIN(x,m,b)  )
#define DSP_QMB(x,m,b) ( ( ((m)>=(b))||((b)>64)||((m)<1) ) ? (1/((m)-(m))) : DSP_QMBMAX(x,m,b) )

#define DSP_QNM(x,n,m) DSP_QMB(x,m,n+m) //convert to m bit mantissa and n bit integer part including sign bit
#define DSP_QM32(x,m)  DSP_QMB(x,m,32)  //convert to 32bits int with mantissa "m"
#define DSP_QM64(x,m)  DSP_QMB(x,m,64)  //convert to 64bit long long with mantissa "m"

//same as functions
extern long long dspQNM(double x, int n, int m);
extern long long dspQM64(double x, int m);
extern int dspQM32(double x, int m);

#endif /* DSP_HEADER_H_ */
