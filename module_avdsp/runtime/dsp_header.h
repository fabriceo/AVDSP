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

extern int dspPrintfVal;

#include <stdio.h>
#define dspprintf(...)  do { if (dspPrintfVal>=1) printf(__VA_ARGS__); } while(0)   // we do the normal printf
#define dspprintf1(...) do { if (dspPrintfVal>=1) printf(__VA_ARGS__); } while(0)   // we do the normal printf
#define dspprintf2(...) do { if (dspPrintfVal>=2) printf(__VA_ARGS__); } while(0)
#define dspprintf3(...) do { if (dspPrintfVal>=3) printf(__VA_ARGS__); } while(0)
//low level debugging only
#define dspprintf4(...) do { if (dspPrintfVal>=4) printf(__VA_ARGS__); } while(0)

// special syntax for XMOS XCore compiler...
#if  defined(__XC__)
#ifndef XCunsafe
#define XCunsafe unsafe
#endif
#else
#ifndef XCunsafe
#define XCunsafe
#endif
#endif


// list of all DSP supported opcode as of this version. The last opcode is marked in the header
enum dspOpcodesEnum {
    DSP_END_OF_CODE = 0,// this opcode value is 0 and its length is 0 as a convention
    DSP_HEADER,         // contain summary information about the program.
    DSP_PARAM,          // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
    DSP_PARAM_NUM,      // same as PARAM but the data area is indexed and each param_num section can be accessed separately
    DSP_NOP,            // sometime used to align opcode adress start to a 8byte cell
    DSP_CORE,           // used to separate each dsp code trunck and distribute opcodes on multiple tasks.
    DSP_SECTION,        //conditional section.

/* IO engine */
    DSP_LOAD = 7,           //load a sample from the sample array location Z into the ALU "X" without conversion in s.31 format
    DSP_STORE,          // store the LSB of ALU "X" into the sample aray location Z without conversion. sat0db expected upfront
    DSP_LOAD_STORE,     // move many samples from location X to Y without conversion (int32 or float) for N entries
    DSP_STORE_TPDF,     // apply a gain and store result in an output
    DSP_STORE_GAIN,     // apply a gain and store result in an output
    DSP_LOAD_GAIN,      // load a sample from the sample array location Z into the ALU "X" and apply a Qnm gain. result is double precision
    DSP_LOAD_MUX,       // combine many inputs samples into a value, same as summing many DSP_LOAD_GAIN. result is double precision
    DSP_MIXER,          // load all inputs with their respective gain, couples stores below opcode

    DSP_LOAD_X_MEM,       // load a memory location 64bits into the ALU "X" without any conversion.
    DSP_STORE_X_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)
    DSP_LOAD_Y_MEM,       // load a memory location 64bits into the ALU "X" without any conversion.
    DSP_STORE_Y_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)

    DSP_LOAD_MEM_DATA,  // load the double precision value stored in data space at an absolute adress

/* math engine */

    DSP_CLRXY = 20,          //clear both ALU register
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
    DSP_VALUEX,         // load an imediate value qnm or float
    DSP_VALUEY,         // load an imediate int32 value without conversion


/* gains */
    DSP_GAIN = 39,      // apply a fixed gain (qnm or float) on the ALU , if a ALU was a sample s.31 then it becomes s4.59
    DSP_CLIP,           // check wether the sample is reaching the thresold given and maximize/minize it accordingly.

    DSP_SAT0DB,         // verify boundaries -1/+1. and set a flag in case of saturation
    DSP_SAT0DB_VOL,     // apply volume and then verify boundaries -1/+1.
    DSP_STORE_VOL,      // store the accumulator in the target location and apply digital volume
    DSP_SAT0DB_GAIN,    // apply a gain and then check boundaries as above
    DSP_STORE_VOL_SAT,  // store the accumulator in the target location and apply digital volume and eventually apply saturation gain and detect extra saturation
    DSP_SERIAL,         // if not equal to product serial number, then DSP will reduce its output by 24db !

/* delays */
    DSP_DELAY_1 = 47,   // equivalent to a delay of 1 sample (Z-1), used to synchronize multi-core output.
    DSP_DELAY,          // 48 execute a delay line (32 bits ONLY). to be used just before a DSP_STORE for example.
    DSP_DELAY_DP,       // 49 same as DSP_DELAY but 64bits (twice data space required ofcourse)


/* filters */
    DSP_BIQUADS,        // 50 execute N biquad cell. ALU is expected s4.59 and will be return as s4.59
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
    DSP_FIR,            // execute a fir filter with many possible impulse depending on frequency
    DSP_WFIR,           // execute a warped fir filter with many possible impulse depending on frequency

    DSP_DELAY_FB_MIX = 66,
    DSP_INTEGRATOR,
    DSP_CICUS,
    DSP_CICN,
    DSP_EXPMA,
    DSP_THDCOMP,
    DSP_BIQUADS_FS,     //accept cascaded biquads parameters. single raw of coefficicients expected to be computed at each fs change
    DSP_BIQUADS_FS_FAST,    //same, using VPU capabilities, increasing THD+N at lowfrequency and high sampling rate
    DSP_BIQUADS_FS_FAST8,   //same using specific VPU capability to handle 8 biquad section as fast as possible
    DSP_SEND,           //send data to other tiles
    DSP_RECEIVE,        //receive data from other tiles
    DSP_DRC_ENV_PEAK,   //compute peak enveloppe, given an attack+release alpha coeficient
    DSP_DRC_ENV_RMS,    //compute rms enveloppe, , given an attack+release alpha coeficient
    DSP_DRC_LIM_PEAK,   //gain limiter based on peak enveloppe
    DSP_DRC_LIM_RMS,    //gain limiter based on rms enveloppe
    DSP_DRC_LIM_PEAK_CLIP,  //gain limiter based on peak enveloppe, and hard clipping anyway
    DSP_DRC_COMPRESSOR, // compressor based on rms enveloppe, with threshold, gain and slope
    DSP_DRC_EXPANDER,   // expander based on rms enveloppe, with threshold, gain and slope
    DSP_DRC_NOISE_GATE, //remove low level signal based on rms enveloppe, with threshold, gain

    DSP_MEMCLR = 85,         //these functions extends the possibilities already offered with Y accumulator
    DSP_SWAPMEM,
    DSP_ADDMEM,
    DSP_MEMADD,
    DSP_SUBMEM,
    DSP_MEMSUB,
    DSP_MULMEM,
    DSP_DIVMEM,
    DSP_AVGMEM,
    DSP_MEMAVG,
    DSP_MEMNEG,
    DSP_SAVEMEM,
    DSP_LOADMEM,
    DSP_MEMVALUE,
    DSP_MIXERMEM,
    DSP_MEMGAIN,
    DSP_MEMINPUT,

    DSP_CORE_EXTERN = 102,       //102  same as DSP_CORE but to declare external core like spdif task
    DSP_FULL_LOAD,
    DSP_PRIO_ON,
    DSP_PRIO_OFF,
    DSP_TRANSFER2,
    DSP_TRANSFER8,

    DSP_FLOAD = 108,         //108   same as DSP_LOAD but converted to float ieee754
    DSP_FSTORE,
    DSP_FVALUEX,
    DSP_FGAIN,
    DSP_FBIQUADS,       //112
    // new opcodes should come here below
DSP_MAX_OPCODE,      // latest opcode, supported by this runtime version. this will be compared with header at runtimeinit
/*

    DSP_OP113,
    DSP_OP114,
    DSP_OP115,
    DSP_OP116,
    DSP_OP117,
    DSP_OP118,
    DSP_OP119,
    DSP_OP120,
    DSP_OP121,
    DSP_OP122,
    DSP_OP123,
    DSP_OP124,
    DSP_OP125,
    DSP_OP126,
    DSP_OP127, */
    DSP_LAST_OPCODE
};

extern const char * XCunsafe dspOpcodeText[DSP_LAST_OPCODE];  //defined in dsp_header.c

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

typedef opcode_t * opcodePtr_t;

//used at the very begining of the tile dsp program to store basic information
typedef struct dspHeader_s {    //
/* 0 */     opcode_t head;      // marker
/* 1 */     unsigned totalLength;  // the total length of the dsp program (in 32 bits words), rounded to upper 16bytes boundary
/* 2 */     unsigned dataSize;     // maximum required data space for executing the dsp program (in 32 bits words)
/* 3 */     unsigned checkSum;  // basic calculated value representing the sum of all opcodes used in the program for this header only
/* 4 */     unsigned numCores;     // number of cores/tasks declared in the dsp program (excluding external cores)
/* 5 */     unsigned version;      // version of the encoder used MAJOR, MINOR,BUGFIX
/* 6 */     unsigned short format;     // contains DSP_MANT used by encoder or 0 for float encoding
/*   */     unsigned short maxOpcode;  // highest op code number used in this program (to check compatibility with runtime)
/* 7 */     unsigned freqMin;      // minimum frequency possible for this program
/* 8 */     unsigned freqMax;      // maximum frequency possible for this program
/* 9-10 */  unsigned long long usedInputs;    // bit mapping of all used inputs  (max 64 in this version)
/* 11-12 */ unsigned long long usedOutputs;   // bit mapping of all used outputs (max 64 in this version)
/* 13 */    unsigned mantissa2;    //for integer runtime, this value (if not 0) provides the expected size of fractional part of accumulator
/* 14 */    unsigned serialHash;    // hash code to enable 0dbFS output (otherwise -24db)
/* 15 */    unsigned tileNum;       //number of the tile (0..7) only 8 supported here
/* 16 */    unsigned clockcpu;      //clock in MHZ
/* 17 */    unsigned clock176k;     //when spdif mode and 176k
/* 18 */    unsigned clock192k;     //when spdif mode and 192k
/* 19 */    unsigned coreaesprio;
} dspHeader_t;

typedef struct dspSymbol_s {
    unsigned address;
    char type;
    char tileNum;
    char tileUsed;
    char length;
#ifdef __XC__
    char * unsafe name;
#else
    char * name;
#endif
    char name_[1];  //asciiz extended by malloc. mandatory to keep at the end of the structure!
} dspSymbol_t;


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

// this defines the precision for the accumulator when runtime is using DSP_FORMAT_INTxx
// suggested format is 8.56 for parameters, gain and filters coeficients.
// for INT32, DSP_MANT2 is maximum 31 by design
// minimum is 16 by design
// remark for XS2/XS3 architecture, this value MUST be modified also in the assembly file as it is not passed as parameter
#ifndef DSP_MANT2
#define DSP_MANT2 56
#endif

// convert a float/double number to a fixed point integer with a mantissa of "m" bit
// eg : if mant = 28, the value 0.5 will be coded as 0x08000000 = 2^27
// and the maximum number for a 32bits container will be 7.99999999 (2^3-epsilon) as bit 31 is used for sign
// using double as input in order to have 52bits precision compared to float which is 24bits mantissa only
// should works for any target container between 8bits and 64 bits. QM32 and QM64 are predefined for 32/64bits targets

#define DSP_MAXPOS(b) ( ((b)>=64) ?   9223372036854775807LL      : ((1UL << (b-1))-1) )
#define DSP_MINNEG(b) ( ((b)>=64) ? (-9223372036854775807LL-1LL) :  (1UL << (b-1))    )
#define DSP_QMSCALE(x,m,b) ( ((b)>=33) ? (long long)((double)(x)*(1LL<<(m))) : (int)((double)(x)*(1LL<<(m))) )
#define DSP_QMBMIN(x,m,b) ( ( (-(x)) >  ( 1ULL << ( (b)-(m)-1) ) ) ? DSP_MINNEG(b) : DSP_QMSCALE(x,m,b) )
#define DSP_QMBMAX(x,m,b) ( (   (x)  >= ( 1ULL << ( (b)-(m)-1) ) ) ? DSP_MAXPOS(b) : DSP_QMBMIN(x,m,b)  )
#define DSP_QMB(x,m,b) ( (1/(1-( ((m)>=(b))||((b)>64)||((m)<1) ) ) ) ? DSP_QMBMAX(x,m,b) : 0 )

#define DSP_QNM(x,n,m) DSP_QMB(x,m,n+m) //convert to m bit mantissa and n bit integer part including sign bit
#define DSP_QM32(x,m)  DSP_QMB(x,m,32)  //convert to 32bits int with mantissa "m"
#define DSP_QM64(x,m)  DSP_QMB(x,m,64)  //convert to 64bit long long with mantissa "m"

//same as functions
extern long long dspQNM(double x, int n, int m);
extern long long dspQM64(double x, int m);
extern int dspQM32(double x, int m);

extern void dspCalcSumCore(opcode_t * XCunsafe ptr, unsigned int * XCunsafe sum, int * XCunsafe numCore, unsigned int maxcode);
#endif /* DSP_HEADER_H_ */
