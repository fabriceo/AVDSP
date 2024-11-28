/*
 * dsp_runtime.c
 *
 *  Version: May 1st 2020
 *      Author: fabriceo
 */

#if !defined(DSP_PRINTF)&&defined(PRINTF) && defined(XSCOPE)  //specific to XCore envirronement
//#define DSP_PRINTF 2
#endif

//#define DSP_MANT_FLEXIBLE 1 // this force the runtime to accept programs encoded with any value for DSP_MANT (slower execution)
//#define DSP_SINGLE_CORE 1   // to be used when the target architecture support only one core. then all cores are just chained
#define DSP_IEEE754_OPTIMISE 63   // 63 is the default value if not defined here, check yourself against your F.P. CPU

#include "dsp_runtime.h"        // enum dsp codes, typedefs and QNM definition
#include "dsp_ieee754.h"        // some optimized function for IEEE float/double number as static inline
#include "dsp_fpmath.h"         // some fixed point maths as static inline
#include "dsp_tpdf.h"           // functions related to randomizer, tpdf, truncation as static inline
#include "dsp_biquadSTD.h"      // biquad related functions
#include "dsp_firSTD.h"         // fir functions prototypes
#include <math.h>               // for pow() decibel conversion
//prototypes
// search for a core number and return pointer on begining of core code
opcode_t * dspFindCore(opcode_t * ptr, const int numCore);
// point further in the dsp core code, after PARAM sections
opcode_t * dspFindCoreBegin(opcode_t * ptr);

int dspRuntimeInit( opcode_t * codePtr,     // pointer on the dspprogram
                    int maxSize,            // size of the opcode table available in memory including data area
                    const int fs,           // sampling frequency to be used
                    int random,             // initial value for random generator
                    int defaultDither,      // dither value used for dsp_TPDF(0)
                    unsigned cores);        // binary representation of compatible cores

int DSP_RUNTIME_FORMAT(dspRuntime)( opcode_t * ptr,         // pointer on the coree to be executed
                                    dspSample_t * sampPtr,  // pointer on the working table where the IO samples are available
                                    dspALU_t * ALUptr);     // pointer on ALU and ALU2 when used in single opcode mode otherwie NULL



// search a core occurence in the opcode table
opcode_t * dspFindCore(opcode_t * codePtr, const int numCore){  // search core and return begining of core code

    if (codePtr->op.opcode != DSP_HEADER) return 0;
    opcode_t * ptr = codePtr;
    int num = 0;
    int coreseen=0;
    while (1) {
        enum dspOpcodesEnum code = ptr->op.opcode;
        int skip = ptr->op.skip;
        if (skip == 0) {// = END_OF_CORE
            if ((num == 0) && (numCore == 1) )
                 return ptr; // CHANGED codePtr;    // if no dsp_CORE code for core1, then return begining of code
            else return 0;  //core not found
        }
        if (code == DSP_CORE) {
            coreseen=1;
            unsigned * p = (unsigned *)ptr;
            unsigned any1 = p[5];
            unsigned only0 = p[6];
            //printf("core=%d\n",num); printf("any1=%x, only0=%x\n",any1,only0);
            unsigned a = (dspCoresToBeUsed & any1);
            unsigned b = (dspCoresToBeUsed & only0);
            //printf("a   =%x, b    =%x, res=%d\n",a,b,a && (b==0));

            if (a && (b==0)) {
                num++;
                if (num == 2) dspCore2codePtr = ptr;
                if (num == numCore) return ptr;
            }
        }
        if ( (coreseen==0) &&   //any starting opcode will generate an acceptable core
            (code != DSP_HEADER) &&
            (code != DSP_NOP) &&
            (code != DSP_PARAM) &&
            (code != DSP_PARAM_NUM) ) {
                num = 1;
                if (num == numCore) return ptr;
        }
        ptr += skip;
    } // while(1)
}

// skip begining of code which is not needed at each cycle
opcode_t * dspFindCoreBegin(opcode_t * ptr){
    int first = 1;
    while (ptr) { // skip garabage at begining of core code
        enum dspOpcodesEnum code = ptr->op.opcode;
        int skip = ptr->op.skip;
        if (skip == 0) return ptr; // = END_OF_CODE detected
        if ( ((code == DSP_CORE) && first) ||
             (code == DSP_NOP) ||
             (code == DSP_PARAM) ||
             (code == DSP_PARAM_NUM) )  // skip any data if at begining of code
            ptr += skip;
         else
             break;    // most likely a valid code now on
        first=0;
    }
    return ptr;

}


// table of magic number to be multiplied by a number of microseconds to get a number of sample (x2^32)
#define dspDelayFactor 4294.967296  // 2^32/10^6
static const unsigned int dspTableDelayFactor[FMAXpos] = {
        dspDelayFactor*8000.0,  dspDelayFactor*16000.0,
        dspDelayFactor*24000.0, dspDelayFactor*32000.0,
        dspDelayFactor*44100.0, dspDelayFactor*48000.0,
        dspDelayFactor*88200.0, dspDelayFactor*96000.0,
        dspDelayFactor*176400.0,dspDelayFactor*192000.0,
        dspDelayFactor*352800.0,dspDelayFactor*384000.0,
        dspDelayFactor*705600.0,dspDelayFactor*768000.0
};

#if 0
#define dspRmsFactor 1000.0
static const unsigned int dspTableRmsFactor[FMAXpos] = {
        dspRmsFactor/8000.0,   dspRmsFactor/16000.0,
        dspRmsFactor/24000.0,  dspRmsFactor/32000.0,
        dspRmsFactor/44100.0,  dspRmsFactor/48000.0,
        dspRmsFactor/88200.0,  dspRmsFactor/96000.0,
        dspRmsFactor/176400.0, dspRmsFactor/192000.0,
        dspRmsFactor/352800.0, dspRmsFactor/384000.0,
        dspRmsFactor/705600.0, dspRmsFactor/768000.0
};
static int dspRmsFactorFS;
#endif

//all variable below are globals for every dsp core and initialized by the call to dspRuntimeInit or reset
dspHeader_t * dspHeaderPtr;             // points on opcode header see dsp_header.h for structure.
int dspMantissa;                        // reflects DSP_MANT or dspHeaderPtr->format
int dspDelayLineFactor;                 // used to compute size of delay line
int dspBiquadFreqOffset;                // used in biquad routine to compute coeficient adress
int dspBiquadFreqSkip;                  // used in biquad routine to compute coeficient adress at fs
int * dspRuntimeDataPtr;                // contains a pointer on the data area (after program)
unsigned dspCoresToBeUsed;              // store condition given by dspRuntimeInit
opcode_t * dspCore2codePtr;             // point on core 2 if core 2 exist

int dspVolumeMaster;                    //signed coefficient (always positive) representing volume reduction in dB
//used to manage saturation situation
int dspSaturationFlag;                  //set to one by any saturate function when the value is outside +/-1
int dspSaturationNumber;                //count the number of stauration and volume reduction by -1db
dspParam_t dspSaturationGain;           //represent the reduction gain applied due to saturation
dspParam_t dspSaturationVolume;         //represent the new volumemaster integrating saturation gain reduction
float dspSaturationCoef = 0.89125094; // -1db
unsigned dspSaturationCoefUnsigned = 3827893631; //-1db/2^32

#if defined(dspDecibelTableSize) && (dspDecibelTableSize>0)

dspParam_t   dspDecibelTable[dspDecibelTableSize];

//create a table with decibel coefficient, from 0 to -127
//initialized once by dspRuntimeInit
static void dspCreateDecibelTable(){
    if (dspDecibelTable[0] == 0) {
        for (int i =0; i < dspDecibelTableSize; i++) {
            double db = - i ;
            db /= 20.0;
            db = pow (10.0, db);
        #if DSP_ALU_INT
            dspDecibelTable[i] = DSP_QM32(db,31);
        #elif
            dspDecibelTable[i] = db;
        #endif
    }
        dspDecibelTable[dspDecibelTableSize-1] = 0; // mute
    }
}
#endif

static int dspSamplingFreq;
static int dspMinSamplingFreq;
static int dspMaxSamplingFreq;
static int dspNumSamplingFreq;

int dspSamplingFreqIndex;   //global


static void dspChangeFormat(opcode_t * ptr, int newFormat);

static void dspCalcTpdfMasks();

//clear the data area. called by dspRuntimeReset at each fs change
static void dspRuntimeResetData(){
    int length = dspHeaderPtr->totalLength;    // lenght of the program
    int size   = dspHeaderPtr->dataSize;       // size of the data needed
    int * intPtr = (int*)dspHeaderPtr;
    intPtr += length;  // point on data space
    for (int i = 0; i < size; i++) intPtr[i] = 0;
    dspSaturationFlag = 0;
    dspVolumeMaster = 0;
    //table containing up to 32 volumes
    for (int i=0; i<32; i++) dspRuntimeDataPtr[i] = 0; // 0db by default
}

// to be ran after dspRuntimeInit() at EACH sampling frequency change.
int dspRuntimeReset(const int fs,
                    int random,                     // for tpdf / dithering
                    int defaultDither) {            // default dither value used for dsp_TPDF(0)

    int freqIndex = dspConvertFrequencyToIndex(fs);
    if (freqIndex>=FMAXpos) {
        dspprintf("ERROR : sampling frequency not supported.\n"); return -1; }
    if ((freqIndex < dspMinSamplingFreq) || (freqIndex > dspMaxSamplingFreq)) {
         dspprintf("ERROR : sampling freq not compatible with encoded dsp program.\n"); return -2; }
    dspSamplingFreq     = freqIndex;
    dspSamplingFreqIndex= freqIndex - dspMinSamplingFreq;  // relative index of the sampling freq vs the encoded min freq
    dspBiquadFreqOffset = 5+6*dspSamplingFreqIndex; // skip also the 1+1+3 first words
    dspDelayLineFactor  = dspTableDelayFactor[freqIndex];
    //dspRmsFactorFS      = dspTableRmsFactor[freqIndex];

    dspRuntimeResetData();                          // now clear the data area, just after the program area
    dspSaturationNumber = 0;
    dspSaturationGain   = 0;                        //zero means no gain attenuation expected
    dspSaturationVolume = 0;
    dspTpdfInit(random,defaultDither);
	return 0;
}

// make the basic sanity check, predefine some FS related variable
// DOES CLEAR THE DATA AREA from end of program to max_code_size
// to be ran once before dspRuntime().
int dspRuntimeInit( opcode_t * codePtr,             // pointer on the begining of dspprogram
                    int maxSize,                    // size of the opcode table available in memory including data area
                    const int fs,                   // sampling frequency currently in use
                    int random,                     // for initialization of tpdf / dithering
                    int defaultDither,              // dither value to be used in case of dsp_TPDF(0)
                    unsigned cores) {               // binary representation to select compatible cores

#if defined(dspDecibelTableSize) && (dspDecibelTableSize>0)
    dspCreateDecibelTable();
#endif
    dspHeaderPtr = (dspHeader_t*)codePtr;           //setup global variable
    opcode_t* cptr = codePtr;
    enum dspOpcodesEnum code = cptr->op.opcode;
    if (code == DSP_HEADER) {                       //check presence of the header info and analyse it
        unsigned sum ;
        int numCores ;
        int res;

        int length = dspHeaderPtr->totalLength;    // lenght of the program
        int size   = dspHeaderPtr->dataSize;       // size of the data needed
        if ((size+length) > maxSize){
            dspprintf("ERROR : total size (program+data = %d) is over the allowed size (%d).\n",length+size, maxSize); return -6; }
        dspRuntimeDataPtr = &codePtr[length].i32;
        dspCoresToBeUsed = cores;                   //initialize global cores selector
        dspCore2codePtr = &codePtr[length];
        dspCalcSumCore(codePtr, &sum, &numCores, length);
        if (sum != dspHeaderPtr->checkSum) {
            dspprintf("ERROR : checksum problem with the program.\n"); return -4; }
        if (numCores < 1) {
            dspprintf("ERROR : no cores defined in the program.\n"); return -3; }
        if (dspHeaderPtr->maxOpcode >= DSP_MAX_OPCODE) {
            dspprintf("ERROR : some recent opcodes in this dsp program are not supported by this runtime version.\n"); return -5; }

        int min = dspHeaderPtr->freqMin;
        int max = dspHeaderPtr->freqMax;
        dspMinSamplingFreq  = min;
        dspMaxSamplingFreq  = max;
        dspNumSamplingFreq  = max - min +1;
        dspBiquadFreqSkip   = 2+6*dspNumSamplingFreq;   // 3 words for filter user params (type+freq, Q, gain) + 5 coef alligned per biquad
    #if   DSP_ALU_INT
            dspMantissa = DSP_MANT; // possibility to pass this as a parameter in a later version
            if (dspHeaderPtr->format != DSP_MANT_FLEX)
                dspChangeFormat(codePtr, DSP_MANT_FLEX);
    #elif DSP_ALU_FLOAT
            if (dspHeaderPtr->format != 0)  // encoded 0 means float otherwise number of bit of mantissa
                dspChangeFormat(codePtr, 0);
    #endif
        dspCalcTpdfMasks();                 //compute some masks based on dither to speedup runtime
        if(fs) {
            res=dspRuntimeReset(fs,random,defaultDither);
            if(res) return res;
        }
        return length;  // ok
    } else {
        dspprintf("ERROR : no dsp header in this program.\n");
        return -1; }
}

//convert a pointed numerical value (float or integer) to target type and mantissa
static void dspChangeThisData(dspALU32_t * p, int old, int new){
    if (old > 0)        // old is interger
        if (new > 0) {  // new is integer
            int delta = new - old;  //change mantissa if old and new are different
            if (delta > 0) p->i <<=  delta;
            if (delta < 0) p->i >>= -delta;
        } else  // new is float
            p->f = (float)(p->i) / (float)(1UL << old);   //convert integer to float
    else                // old is float
        if (new > 0)    // new is integer
            p->i = DSP_QM32(p->f, new);     //convert float to integer
}


// to be launched just after runtimeInit, to potentially convert the encoded format.
// typically needed when encoder generate float and runtime is INT64. versatile
static void dspChangeFormat(opcode_t * ptr, int newFormat){
    dspHeader_t * headerPtr = (dspHeader_t *)ptr;
    int oldFormat = headerPtr->format;
    //printf("Change format from %d to %d\n",oldFormat,newFormat);
    if (oldFormat == newFormat) return; // sanity check if nothing to change
    while(1){   //go through the whole program
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        //printf("\n%s",dspOpcodeText[code]);
        if (skip == 0) break;       // end of program encountered
        dspALU32_t * cptr = (dspALU32_t*)(ptr+1); // point on the first parameter after opcode
        switch(code){

        case DSP_DIRAC:             // dataptr then imediate value
        case DSP_SQUAREWAVE:
             cptr++;                // skip offset to data space
        case DSP_DATA_TABLE:
        case DSP_CLIP: {
            //change imediate value
            dspChangeThisData(cptr, oldFormat, newFormat);
            break;}

        case DSP_LOAD_GAIN:         // index then gainptr
            cptr++;                 // skip sample index
        case DSP_GAIN: {             // gainptr (indirect value)
            cptr = (dspALU32_t*)(ptr+cptr->i);   // compute gainptr
            //change gain
            dspChangeThisData(cptr, oldFormat, newFormat);
            break;}

        case DSP_LOAD_MUX:{         // tableptr
            cptr = (dspALU32_t*)(ptr+cptr->i);   //point on table
            short num = cptr->i;                 // number of records
            cptr++;   // point on couples
            for (int i=0;i<num;i++) {
                cptr++; // skip index
                dspChangeThisData(cptr++, oldFormat, newFormat); }
            break;}

        case DSP_BIQUADS: { // dataspace pointer then tableptr
            cptr++;         // skip pointer on data space
            cptr = (dspALU32_t*)(ptr+cptr->i);  //point on coef table
            short numSection = cptr->i;         // number of section (lsb)
            cptr+=3;        // point on 1st section
            for (int i=0; i< numSection; i++) {
                cptr+=2;    // skip Q and Gain
                for (int j=0; j< dspNumSamplingFreq; j++) {
                    for (int k=0; k<5; k++)
                        dspChangeThisData(cptr++, oldFormat, newFormat);
                    if (newFormat) cptr->i = newFormat;  //coef[5] now stores the MANTBQ
                    else cptr->f = 1.0;
                    cptr++; // round up to 6th position
                }
            }
        } break;

        case DSP_FIR: {// TODO
        } break;

        case DSP_DITHER_NS2: {  // data space pointer then table ptr
            cptr++; // skip pointer on data space
            cptr = (dspALU32_t*)(ptr+cptr->i); //point on coef table in dsp_param section
            //modify each pole triplet
            for (int i=0; i< dspNumSamplingFreq; i++) {
                dspChangeThisData(cptr++, oldFormat, newFormat);
                dspChangeThisData(cptr++, oldFormat, newFormat);
                dspChangeThisData(cptr++, oldFormat, newFormat);
            }
        } break;

        case DSP_DCBLOCK: { // dataspace pointer then imediate table of pole
            cptr++; // skip pointer on data space
            //modify each value of "pole"
            for (int i=0; i< dspNumSamplingFreq; i++)
                dspChangeThisData(cptr++, oldFormat, newFormat);
        } break;

        case DSP_SINE: {
            cptr++;     // skip pointer on data space
            dspChangeThisData(cptr++, oldFormat, newFormat);
             //modify each value of "epsilon"
            for (int i=0; i< dspNumSamplingFreq; i++)
                dspChangeThisData(cptr++, oldFormat, newFormat);
        } break;

        case DSP_MIXER: {
            int n = (skip-1)>>2;
            for (int i=0; i<n; i++) {
                cptr++; //skip input index
                dspChangeThisData(cptr++, oldFormat, newFormat);
            }
        } break;
        case DSP_DELAY_FB_MIX: {
            if ( ((oldFormat==0) && (newFormat!=0)) || ((oldFormat!=0) && (newFormat==0)) ){
                cptr+=3;
                for (int i=0; i<3; i++)
                    dspChangeThisData(cptr++, oldFormat ?31:0, newFormat?31:0);
            }
        } break;
        case DSP_CICUS: {
            if ( ((oldFormat==0) && (newFormat!=0)) || ((oldFormat!=0) && (newFormat==0)) ){
                cptr+=2;    //skip time and dataptr
                for (int i=0; i< dspNumSamplingFreq; i++)
                    dspChangeThisData(cptr++, oldFormat?31:0, newFormat?31:0);
            }
        } break;
        case DSP_CICN: {
            if ( ((oldFormat==0) && (newFormat!=0)) || ((oldFormat!=0) && (newFormat==0)) ){
                cptr+=2;    //skip time and dataptr
                //only 1 coef q31 to change
                dspChangeThisData(cptr++, oldFormat?32:0, newFormat?32:0);
            }
        } break;

        } // switch

        ptr += skip;
    } // while(1)
    headerPtr->format = newFormat;
}

//compute some constant to speedup execution of applying tpdf
void dspCalcTpdfMasks(){
    opcode_t * ptr = (opcode_t*)dspHeaderPtr;
    while(1){   // go through the whole program
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        if (skip == 0) break;       // end of program encountered
        switch(code){
        case DSP_TPDF:
        case DSP_TPDF_CALC: {
            dspTpdfPrepare(&dspTpdfGlobal,&dspTpdfGlobal, ptr[1].i32 );
            ptr[2].i32 = dspTpdfGlobal.shift;
            ptr[3].i32 = dspTpdfGlobal.factor;
            ptr[4].i32 = dspTpdfGlobal.mask64 & 0xFFFFFFFF;
            ptr[5].i32 = dspTpdfGlobal.mask64 >> 32;
            break; }
        }
        ptr += skip;
    }
}

//clear the state variable of any opcode requiring state variable
void dspResetFiltersStateData(){
    opcode_t * ptr = (opcode_t*)dspHeaderPtr;
    while(1){
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        if (skip == 0) break;       // end of program encountered
        switch(code){
        case DSP_BIQUADS: {
            int * p = dspRuntimeDataPtr+ptr[1].i32;
            int * q = &ptr[ ptr[2].i32 ].i32;
            short section = *q;
            while(section--) { for (int i=0; i<6; i++) { *p = 0; p++; } }
            break; }
        case DSP_DCBLOCK : {
            int * p = dspRuntimeDataPtr+ptr[1].i32;
            p[0] = 0;
            break; }
        case DSP_DITHER:  //fall through
        case DSP_DITHER_NS2: {
            int * p = dspRuntimeDataPtr+ptr[1].i32;
            p[0] = 0; p[1] = 0; p[2] = 0;
            break; }
        case DSP_INTEGRATOR: {
            int * p = dspRuntimeDataPtr+ptr[1].i32;
            p[0] = 0;   //clear sum
            break;}
        }
        ptr += skip;
    }
}


//used to detect a stauration and then to compute the reduction gain.
int dspCheckSaturationFlag(){
    if (dspSaturationFlag) {
        dspSaturationFlag = 0;
        if (dspFormatIsInt) {
            unsigned gain = dspSaturationGain;
            if (gain) {
                unsigned long long acc = gain;
                acc *= dspSaturationCoefUnsigned;
                acc >>= 32;
                dspSaturationGain = acc;
                acc *= dspVolumeMaster;
                acc >>= 32;
                dspSaturationVolume = acc;
            } else {
                dspSaturationGain = dspSaturationCoefUnsigned;
                unsigned long long acc = dspSaturationCoefUnsigned;
                acc *= dspVolumeMaster;
                acc >>= 32;
                dspSaturationVolume = acc;
                dspSaturationNumber = 0;
            }
        } else { //dspformatfloat
            dspParam_ft gain = dspSaturationGain;
            if (gain) dspSaturationGain *= dspSaturationCoef;
            else {
                dspSaturationGain = dspSaturationCoef;
                dspSaturationNumber = 0;
            }
            dspSaturationVolume = dspSaturationGain * dspVolumeMaster;
        }
        dspSaturationNumber++;
        //printf("SATURATION detected %d, reducing by -1db\n",dspSaturationNumber);
        //dspResetFiltersStateData();   //optional
        return 1;
    } else {
        //TODO potentially update dspSaturationVolume if any change in dspVolumeMaster
        return 0;
    }
}

// dsp interpreter full implementation
int DSP_RUNTIME_FORMAT(dspRuntime)( opcode_t * ptr,         // pointer on the coree to be executed
                                    dspSample_t * sampPtr,  // pointer on the working table where the IO samples are available
                                    dspALU_t * ALUptr)      // pointer on ALU and ALU2 when used in single opcode mode
{
    tpdf_t   tpdfLocal;   // local data for tpdf / dither
    tpdf_t * tpdfPtr = &dspTpdfGlobal;  // by default point on global dataset

    dspALU_t ALU, ALU2;

    if (ptr < dspCore2codePtr) dspCheckSaturationFlag();    //executed by core1 only

    if (ALUptr) { ALU = ALUptr[0]; ALU2 = ALUptr[1]; }  //single opcode mode

    while (1) { // main loop

        enum dspOpcodesEnum opcode = ptr->op.opcode;

        switch (opcode) { // efficiently managed with a jump table

        case DSP_HEADER: {
            break; }

        case DSP_MAX_OPCODE: {
            return 1; }

        case DSP_END_OF_CODE: {
            // TODO : change to execute this at begining of 1st core only
            return 0;
            break; }

        case DSP_CORE: {
#if defined( DSP_SINGLE_CORE ) && ( DSP_SINGLE_CORE == 1 )
            break; }    // this will continue the dsp execution over this opcode
#else
            return 0; } //end of task
#endif

        case DSP_NOP: { // might be generated by the encoder to force allignement on 8 bytes
            break; }

        case DSP_SECTION: { //conditional section
            unsigned any1 = ptr[2].u32;
            unsigned only0 = ptr[3].u32;
            unsigned a = dspCoresToBeUsed & any1;
            unsigned b = dspCoresToBeUsed & only0;
            if ((a==0) || b) ptr = &ptr[1]; //jump and skip the whole section
            break; }

        case DSP_SWAPXY: { asm volatile ("#DSP_SWAPXY:");
            dspALU_t tmp = ALU;
            ALU = ALU2;
            ALU2 = tmp;
            break;}

        case DSP_COPYXY: {
            ALU2 = ALU;
            break;}

        case DSP_COPYYX: {
            ALU = ALU2;
            break;}

        case DSP_CLRXY: {
            ALU2 = 0;
            ALU = 0;
            break;}

        case DSP_ADDXY: { asm volatile("#DSP_ADDXY:");
            ALU += ALU2;
            break;}

        case DSP_ADDYX: { asm volatile("#DSP_ADDYX:");
            ALU2 += ALU;
            break;}

        case DSP_SUBXY: { asm volatile("#DSP_SUBXY:");
            ALU -= ALU2;
            break;}

        case DSP_SUBYX: { asm volatile("#DSP_SUBYX:");
            ALU2 -= ALU;
            break;}

        case DSP_NEGX: { asm volatile("#DSP_NEGX:");
            ALU = -ALU;
            break;}

        case DSP_NEGY: { asm volatile("#DSP_NEGY:");
            ALU2 = -ALU2;
            break;}

        case DSP_SHIFT: { asm volatile("#DSP_SHIFT:");
            int shift = ptr[1].i32;      // get parameter
            #if DSP_ALU_INT
                if (shift >= 0)
                    ALU <<= shift;
                else
                    ALU >>= -shift;
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    dspShiftDouble( &ALU, shift);
                #else
                    dspShiftFloat(  &ALU, shift);
                #endif
            #endif
            break; }


        case DSP_MULXY: { asm volatile("#DSP_MULXY:");
            ALU *= ALU2;
            //TODO SHIFT LEFT NEEDED to remove (32-DSP_MANT*2)
            break;}

        case DSP_MULYX: { asm volatile("#DSP_MULYX:");
            ALU2 *= ALU;
            //TODO SHIFT LEFT NEEDED
            break;}


        case DSP_DIVXY: {   asm volatile("#DSP_DIVXY:");
            ALU /= ALU2;
            break;}


        case DSP_DIVYX: {   asm volatile("#DSP_DIVYX:");
            ALU2 /= ALU;
            break;}


        case DSP_AVGXY: {   asm volatile("#DSP_AVGXY:");
            ALU /= 2;
            ALU += ALU2/2;
            break;}


        case DSP_AVGYX: {   asm volatile("#DSP_AVGYX:");
            ALU2 /= 2;
            ALU2 += ALU/2;
            break;}


        case DSP_SQRTX: {  // TODO
            #if DSP_ALU_INT
                unsigned int res = 0;
                if (ALU >> 32) {    // potentially 64 bits used
                    unsigned int bit = 1<<30;
                    while (bit){
                        unsigned int temp = res | bit;
                        dspALU_t value = dspmulu64_32_32(temp,temp);
                        if ( ALU >= value ) res = temp;
                        bit >>= 1; }
                } else { // only 32 bits
                    unsigned int bit = 1<<15;
                    while (bit){
                        unsigned int temp = res | bit;
                        temp *= temp;
                        if ( ALU >= temp ) res = temp;
                        bit >>= 1; }
                }
                ALU = res;
            #elif DSP_ALU_FLOAT
                ALU = sqrt(ALU);    // from math.h
            #endif
            break;}

        case DSP_SAT0DB_VOL: // TODO
        case DSP_SAT0DB: {
            asm volatile ("#sat0b:");
            #if DSP_ALU_INT
                ALU = dspShiftInt(ALU, 30);
                ALU *= dspSaturationGain;
                if (dspSaturate64_031_test( &ALU, DSP_MANT_FLEX )) dspSaturationFlag = 1;
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    dspSaturateDouble0db( &ALU );
                #else
                    dspSaturateFloat0db( &ALU );
                #endif
            #endif
            break;}


        case DSP_SAT0DB_GAIN: {
            dspParam_t * gainPtr = (dspParam_t*)(&ptr[ptr[2].u32]);
            #if DSP_ALU_INT
                ALU >>= DSP_MANT_FLEX;   // reduce precision by removing DSP_MANT bits to give free size for coming multiply
                ALU *= (*gainPtr);
                ALU = dspShiftInt(ALU, 30);
                ALU *= dspSaturationGain;
                if (dspSaturate64_031_test( &ALU, DSP_MANT_FLEX )) dspSaturationFlag = 1;
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    dspALU_SP_t tmp = ALU;
                    ALU = dspMulFloatDouble( tmp, *gainPtr );
                    dspSaturateDouble0db( &ALU );
                #else
                    ALU = dspMulFloatFloat( ALU, *gainPtr );
                    dspSaturateFloat0db( &ALU );
                #endif
            #endif
            break;}


        case DSP_TPDF_CALC: {
            //TODO change behavious as per recent changes for XS2
            dspTpdfPrepare(&dspTpdfGlobal,&dspTpdfGlobal,ptr[1].i32);
            break;}

        case DSP_TPDF: {
            //TODO change behavious as per recent changes for XS2
            // move tpdf pointer to local otherwise keep asis (maybe global or local already)
            if (! dspTpdfPrepare(tpdfPtr, &tpdfLocal,ptr[1].i32)) tpdfPtr = &tpdfLocal;
            break;}


        case DSP_LOAD: { asm volatile ("#DSP_LOAD:");   // load a RAW sample direcly in the ALU
            int index = ptr[1].i32;
            #if DSP_ALU_INT
                ALU = dspShiftInt64((long long)sampPtr[index] << 32, 32-DSP_MANT_FLEX );
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    #if DSP_ALU_64B
                        ALU = dspIntToDoubleScaled(sampPtr[index],31);
                    #else
                        ALU = dspIntToFloatScaled(sampPtr[index],31);
                    #endif
                #else
                        ALU = sampPtr[index];
                #endif
            #endif
            break; }


        case DSP_LOAD_GAIN: {
            int index = ptr[1].i32;
            //gain is indirect value
            dspParam_t * gainPtr = (dspParam_t*)(ptr+ptr[2].u32);
            #if DSP_ALU_INT
                dspmacs64_32_32_0( &ALU, sampPtr[index], *gainPtr );
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    dspALU_SP_t tmp = dspIntToFloatScaled(sampPtr[index],31);
                    #if DSP_ALU_64B
                        ALU = dspMulFloatDouble( tmp, *gainPtr );
                    #else
                        ALU = dspMulFloatFloat( tmp, *gainPtr );
                    #endif
                #else
                        ALU = sampPtr[index];
                        ALU *= *gainPtr;
                #endif
            #endif
            break; }

        case DSP_STORE_VOL: //TODO !!!! apply digital volume Master
        case DSP_STORE_VOL_SAT: //TODO !!!! apply digital volume Master
        case DSP_STORE: {   // store the ALU
            int index = ptr[1].i32;
            dspSample_t sample;
            #if DSP_ALU_INT             // expect value to be already saturated and in s31 format!
                sample = ALU;
                sample &= tpdfPtr->mask;   // remove lowest bits according to dither
                sampPtr[index] = sample;
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    #if DSP_ALU_64B
                        sample =  dsps31Double0DB(ALU);
                    #else
                        sample =  dsps31Float0DB(ALU);
                    #endif
                    sample &= tpdfPtr->mask;
                    sampPtr[index] = sample;
                #else
                    sample = ALU;    // no conversion needed, but then no mask
                    sampPtr[index] = sample;
                #endif
            #endif
            break; }

        case DSP_STORE_TPDF: {   // store the ALU
            int index = ptr[1].i32;
            //int tpdfofset = ptr[2].i32;
            dspSample_t sample;
            #if DSP_ALU_INT
                sample = ALU;
                sample &= tpdfPtr->mask;   // remove lowest bits according to dither
                sampPtr[index] = sample;
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    #if DSP_ALU_64B
                        sample =  dsps31Double0DB(ALU);
                    #else
                        sample =  dsps31Float0DB(ALU);
                    #endif
                    sample &= tpdfPtr->mask;
                    sampPtr[index] = sample;
                #else
                    sample = ALU;    // no conversion needed, but then no mask
                    sampPtr[index] = sample;
                #endif
            #endif
            break; }

        case DSP_STORE_GAIN:{   //TODO !
            break; }

        case DSP_GAIN:{
            //always indirect value
            dspParam_t * gainPtr = (dspParam_t*)(&ptr[ptr[1].u32]);
            ALU *= *gainPtr;
            break;}


        case DSP_VALUEX:{
            //TODO RECONSIDERE SHIFTING
            //always indirect value
            dspParam_t * valuePtr = (dspParam_t*)(&ptr[ptr[1].u32]);
            #if DSP_ALU_INT
            dspParam_t value = *valuePtr;
            ALU = ((dspALU_t)value <<32);
            #elif DSP_ALU_FLOAT
            ALU = (*valuePtr);
            #endif
        break;}

        case DSP_VALUEY:{
            //always indirect value
            dspParam_t * valuePtr = (dspParam_t*)(&ptr[ptr[1].u32]);
            #if DSP_ALU_INT
            dspParam_t value = *valuePtr;
            ALU2 = ((dspALU_t)value <<32);
            #elif DSP_ALU_FLOAT
            ALU2 = (*valuePtr);
            #endif
        break;}


        case DSP_WHITE: {
             #if DSP_ALU_INT
                 ALU = dspTpdfRandom;
                 //TODO SHIFTS
             #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                 ALU = dspIntToDoubleScaled(dspTpdfRandom,31);
                #else
                 ALU = dspIntToFloatScaled(dspTpdfRandom,31);
                #endif
             #endif
             break;}


        case DSP_DELAY_1:{
            int offset = ptr[1].i32; //point on the area where we have the sample storage (2 words size)
            int * dataPtr = dspRuntimeDataPtr+offset;
            dspALU_t * memPtr = (dspALU_t*)dataPtr;   // formal type casting to enable double word instructions
            dspALU_t tmp = *memPtr;
            *memPtr = ALU;
            ALU = tmp;
            break;}


        case DSP_LOAD_STORE: { asm volatile( "#DSP_LOAD_STORE:");
        int max = ptr->op.skip;
            while(--max) {
                sampPtr[ptr[max].i32] = sampPtr[ptr[max-1].i32];
                max--;
            }
            break; }


        case DSP_LOAD_X_MEM: { asm volatile( "#DSP_LOAD_X_MEM:");
            int offset = ptr[1].i32;   //point on the area where we have the memory location in 32 or 64 bits
            dspALU_t * memPtr = (dspALU_t*)&ptr[offset];
            ALU = *memPtr;
            break; }


        case DSP_STORE_X_MEM:{ asm volatile( "#DSP_STORE_X_MEM:");
            int offset = ptr[1].i32;    //point on the area where we have the memory location in 32 or 64 bits
            dspALU_t * memPtr = (dspALU_t*)&ptr[offset];
            *memPtr = ALU;
            break; }

        case DSP_LOAD_Y_MEM: { asm volatile( "#DSP_LOAD_Y_MEM:");
            int offset = ptr[1].i32;   //point on the area where we have the memory location in 32 or 64 bits
            dspALU_t * memPtr = (dspALU_t*)&ptr[offset];
            ALU2 = *memPtr;
            break; }


        case DSP_STORE_Y_MEM:{ asm volatile( "#DSP_STORE_Y_MEM:");
            int offset = ptr[1].i32;    //point on the area where we have the memory location in 32 or 64 bits
            dspALU_t * memPtr = (dspALU_t*)&ptr[offset];
            *memPtr = ALU2;
            break; }

        case DSP_DELAY_FB_MIX: //NOT IMPLEMENTED : default to standard DSP_DELAY
        case DSP_DELAY: //in next version, SHOULD STORE nSamples in *dataPtr once calculated at first cycle
        case DSP_DELAY_DP:  {
            unsigned  maxSize = ptr[1].u32;                // maximum size in number of samples, OR delay in uSec
            int offset  = ptr[2].i32;                      // adress of data table for storage
            int * dataPtr = dspRuntimeDataPtr+offset;          // now points on the delay line data, starting with current index
            unsigned nSamples;
            offset = ptr[3].i32;                             // get the offset where we can find the delay in microsseconds
            if (offset == 0) {                          // 0 means it is a fixed delay already defined in maxSize variable
                nSamples = dspmulu32_32_32(maxSize, dspDelayLineFactor);    // microSec was stored in maxSize by design for DELAY_Fixed
            } else {
                int * microSecPtr = (int*)(ptr+offset);   // where is stored the delay
                int microSec = *microSecPtr;   // read microsec
                nSamples = dspmulu32_32_32(microSec, dspDelayLineFactor);
                if (nSamples > maxSize) nSamples = maxSize; // security check as the host aplication may have put an higher data...
            }
            if (nSamples) { // sanity check in case host application put delay=0 to bypass it
                int index = *dataPtr++;                 // the dynamic index is stored as the first word in the delayline table
                                                        //now dataPtr points at the begining of the sample delay line
                if(ptr->op.opcode == DSP_DELAY) {       //simple words
                    dspALU_SP_t * linePtr = (dspALU_SP_t*)dataPtr;
                    dspALU_SP_t value = linePtr[index];
                    linePtr[index] = ALU;
                    ALU = value;
                } else {                                        //double words version
                    dspALU_t * linePtr = (dspALU_t*)dataPtr;   // garanteed to be alligned 8
                    dspALU_t value = linePtr[index];
                    linePtr[index] = ALU;
                    ALU = value;
                }
                index++;
                if (index >= nSamples) index = 0;
                *(--dataPtr) = index;
            }
            break;}


        case DSP_BIQUADS: {
            #if DSP_ALU_INT
            dspSample_t sample = dspShiftInt( ALU, DSP_MANTBQ );    //remove the size of a biquad coef, as the result will be scaled accordingly
            #endif
            dspALU_SP_t * dataPtr = (dspALU_SP_t*)(dspRuntimeDataPtr + ptr[1].u32);
            int * numPtr = &ptr[ptr[2].u32].i32;    // point on the number of sections
            dspParam_t * coefPtr = (dspParam_t*)(numPtr+dspBiquadFreqOffset); //point on the right coefficient according to fs
            int num = *numPtr++;    // number of sections in 16 lsb, biquad routine should keep only 16bits lsb
            if (*numPtr) {  // verify if biquad is in bypass mode or no
            #if DSP_ALU_INT
                // ALU is expected to contain the sample in double precision (typically after DSP_LOAD_GAIN)
                #ifdef DSP_XS2A
                    //DSP_MANTBQ and dspBiquadFreqSkip are defined in assembly file directly to minimize parameters...
                    ALU = dsp_biquads_xs2( sample , coefPtr, dataPtr, num);
                #else
                    ALU = dsp_calc_biquads_int( sample, coefPtr, dataPtr, num, DSP_MANTBQ, dspBiquadFreqSkip);
                #endif
            #elif DSP_ALU_FLOAT
                ALU = dsp_calc_biquads_float(ALU, coefPtr, dataPtr, num, dspBiquadFreqSkip);
            #endif
            }
            break; }


        case DSP_PARAM: {
            // the data below will be skipped automatically
            break; }

        case DSP_PARAM_NUM: {
            // the data below will be skipped automatically
            break; }

        case DSP_SERIAL:{    //
            int serial= ptr[1].i32;  // supposed to be the serial number of the product, or a specif magic code TBD
            if (serial) serial ++;
            //serial check to be done here do here
            break; }


        case DSP_LOAD_MUX:{
            int offset = ptr[1].i32;
            int * tablePtr = (int*)(ptr+offset);
            short max = *tablePtr++ ;   //number of sections
            ALU = 0;
            while (max)  {
                int index = *tablePtr++;
                dspSample_t sample = sampPtr[index];
                dspParam_t * gainPtr = (dspParam_t*)tablePtr++;
                #if DSP_ALU_INT
                    dspmacs64_32_32( &ALU, sample, (*gainPtr) );
                #elif DSP_ALU_FLOAT
                    #if DSP_SAMPLE_INT
                        dspALU_SP_t tmp = dspIntToFloatScaled(sample,31);
                        dspMaccFloatFloat( &ALU , tmp, *gainPtr);
                    #else
                        dspMaccFloatFloat( &ALU , sample, *gainPtr);
                    #endif
                #endif
                max--;
            }
            // from release >1.0 the calculated value is also stored in data space for potential reuse
            dspALU_t * dataPtr = (dspALU_t *)(dspRuntimeDataPtr+ptr[2].u32);
            *dataPtr = ALU;
            break; }


        case DSP_DATA_TABLE: {
            dspParam_t * gainPtr = (dspParam_t*)&ptr[1];  // this is 4.28 format (or float)
            int div =  ptr[2].i32;         // frequency divider
            int size=  ptr[3].i32;         // table size
            int index = ptr[4].i32;        // offset in dataspace where the index is stored
            int * indexPtr = dspRuntimeDataPtr+index;
            int offset= ptr[5].i32;          // offset on where the data are stores
            dspSample_t * dataPtr = (dspSample_t*)(ptr+offset);
            index = *indexPtr;
            dspSample_t data = dataPtr[index];
            index += div;               // in order to divide the frequency, we skip n value
            if (index >= size) index -= size;
            *indexPtr = index;
            #if DSP_ALU_INT
                dspmacs64_32_32_0( &ALU, data, *gainPtr);
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    ALU = dspMulFloatDouble( data, *gainPtr );
                #else
                    ALU = dspMulFloatFloat(  data, *gainPtr );
                #endif
            #endif
            break; }


        // WORK IN PROGRESS

        case DSP_FIR: { // quite opaque of course ..
            int numFreq = dspNumSamplingFreq;   // the range of frequencies in the header defines the size of the table pointing on impulses
            int freq = dspSamplingFreqIndex;    // this is a delta compared to the minimum supported freq
            int * tablePtr = &ptr[1+freq].i32;   // point on the offset to be used for the current frequency
            //dspprintf3("tableptr[%d] = 0x%X\n",freq,(int)tablePtr);
            int offset = *(tablePtr++);         // get the offset where we have the size of impulse and coef for the current frequency
            if (offset) {
                tablePtr = (int*)ptr+offset;    // now points on the impulse associated to current frequency
                //dspprintf3("fir ImpulsePtr = 0x%X\n",(int)tablePtr);
                int length = *(tablePtr++);     // length of the impulse
                offset = ptr[numFreq].i32;      // offset where are the data for states filter
                dspALU_SP_t * dataPtr = (dspALU_SP_t*)(dspRuntimeDataPtr+offset);
                //dspprintf3("state data @0x%X, length %d\n",(int)dataPtr,length);
                int delay = length >> 16;
                if (delay) {    // simple delay line
                    int index = *dataPtr++; // read position in the delay line
                    dspALU_SP_t * linePtr = (dspALU_SP_t*)dataPtr+index;
                    dspALU_SP_t value = *linePtr;
                    #if DSP_ALU_INT
                        *linePtr = dspShiftInt( ALU, DSP_MANT_FLEX );    //remove the size of a biquad coef, as the result will be scaled accordingly
                    #else
                        *linePtr = ALU;
                    #endif
                    ALU = value;
                    index++;
                    if (index >= delay) index = 0;
                    *(--dataPtr) = index;
                } else {
                    if (length > 0) {
                        dspParam_t * coefPtr = (dspParam_t*)tablePtr;
                    #if DSP_ALU_INT
                        dspSample_t sample = dspShiftInt( ALU, DSP_MANTBQ );    //remove the size of a biquad coef, as the result will be scaled accordingly
                        ALU = dsp_calc_fir_int(sample, coefPtr, dataPtr, length);
                    #elif DSP_ALU_FLOAT
                        ALU = dsp_calc_fir_float(ALU, coefPtr, dataPtr, length);
                    #endif
                    }
                }
            }
            break; }


        case DSP_RMS: {
            //parameter list below opcode_t :
            // (0) data pointer
            // (1) delay
            int offset     = ptr[1].i32;                   // where is the data space for rms calculation
            unsigned delay = ptr[2].i32;                   // size of delay line, followed by couples : counter - factor
            unsigned * dataPtr  = (unsigned*)(dspRuntimeDataPtr+offset);
            // structure of the data space preallocated by encoder
            // 1 counter    (0)
            // 1 index      (1)
            // 1 lastsqrt   (2)
            // 1 sqrtwip    (3)
            // 1 sqrtbit    (4)
            // 2 sumsquare  (5)
            // 2 movingavg  (+1)
            // 2xN delayLine(+2)
            unsigned counter = (*dataPtr);              // retreive counter actual value and increment it
            counter++;

            int freq = dspSamplingFreqIndex * 2;        // 2words per frequences : maxCounter & factor
            int * tablePtr = &ptr[3+freq].i32;          // point on the offset to be used for the current frequency
            unsigned maxCounter = *tablePtr++;          // get the max number of counts before entering the delay line
            int factor = *tablePtr;                     // get the magic factor to apply to each sample, to avoid 64bits sat
            dspALU_t *sumSquarePtr = (dspALU_t*)(dataPtr+5);   // current 64bits sum.square
            #if DSP_ALU_INT
                if (factor >0) {                        // trick to avoid duplicating code, and inexpensive in execution time
                    dspSample_t sample = dspmuls32_32_32(ALU,factor);   // scale sample (ALU) according factor from table depending on fs
                    ALU = *sumSquarePtr;                    // get previous sum.square
                    dspmacs64_32_32(&ALU, sample, sample);  // ALU += sample^2
                } else {
                    dspSample_t samplex = dspmuls32_32_32(ALU,factor);   // scale sample (ALU) according factor from table depending on fs
                    dspSample_t sampley = dspmuls32_32_32(ALU2,factor);   // scale sample (ALU) according factor from table depending on fs
                    ALU = *sumSquarePtr;                    // get previous sum.square
                    dspmacs64_32_32(&ALU, samplex, sampley);  // ALU += samplex * sampley
                }
            #else
                if (factor > 0)
                     ALU *= ALU;                        // square current sample
                else ALU *= ALU2;                       // multiply X and Y
                ALU += *sumSquarePtr;                   // add previous sum.square
            #endif
            dspALU_t *averagePtr = sumSquarePtr+1;      // position of the averaged value of sum.square
            if (counter >= maxCounter) {
                if (delay) {
                    unsigned index = dataPtr[1];            // retreive current position in the delay line
                    dspALU_t *delayPtr   = sumSquarePtr+2;  // start of the delay line
                    dspALU_t value = delayPtr[index];       // get oldest value
                    delayPtr[index] = ALU;                  // overwrite oldest and store newest
                    ALU -= value;
                    ALU += *averagePtr;                 // use it in the result
                    index++;                            // prepare for next position in the delay line
                    if (index >= delay) index = 0;
                    dataPtr[1] = index; }               // memorize futur position in delay line
                *averagePtr = ALU;                      // memorize new averaged value
                dataPtr[0] = 0;                         // reset counter of sum.square
                *sumSquarePtr = 0;                      // reset sum.square
                ALU = dataPtr[2];                       // load latest sqrt value

            } else {
                *sumSquarePtr = ALU;                    // memorize sumsquare
                *dataPtr = counter;                     // and new incremented counter

                #if DSP_ALU_INT
                if (counter == 1){                      // very first time enterring here, maybe a sumaverage has been computed in last cycle
                    dataPtr[4] = 1<<30;                 // memorize sqrt iteration bit for next cycle
                    dataPtr[3] = 0;                     // reset temporary sqrt value
                    ALU = dataPtr[2];                   // retreive previous sqrt calculation, if already done ofcourse
                } else {
                    // compute 64bit sqrt=>32 bits, doing one check at every fs cycle, assuming maxCounter always > 64
                    unsigned bit = dataPtr[4];          // read sqrt iteration bit (from 0x800000 to 0)
                    if (bit) {
                        unsigned temp = dataPtr[3] | bit; // last calculation result
                        dspALU_t value = dspmulu64_32_32( temp , temp );
                        if (*averagePtr >= value) { dataPtr[3] = temp; }
                        bit >>= 1;                          // next iteration
                        dataPtr[4] = bit;                   // memorize sqrt iteration bit for next cycle
                        ALU = dataPtr[2];                   // read previous sqrt value
                    } else {
                        ALU = dataPtr[3];                   // get last calculation result
                        dataPtr[2] = ALU;                   // set sqrt value with last calculation
                    }
                }
                #else // float
                    ALU = sqrt(*averagePtr);                // computed at each cycle... a bit too much : TODO
                #endif
            }
            break; }


        case DSP_DCBLOCK: {
            //inspired from here : http://dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping/
            int offset = ptr[1].i32;                   // where is the data space for state data calculation
            dspALU_t * accPtr = (dspALU_t *)(dspRuntimeDataPtr+offset);
            dspALU_SP_t * dataPtr  = (dspALU_SP_t*)(accPtr+1);
            int freq = dspSamplingFreqIndex;        // 1 pole per freq
            dspParam_t * tablePtr = (dspParam_t*)&ptr[2+freq];   // point on the offset to be used for the current frequency
            dspParam_t pole = *tablePtr;                 // pole of the integrator part

            dspALU_SP_t Xn;
            #if DSP_ALU_INT
            Xn = dspShiftInt( ALU, DSP_MANT_FLEX);
            #elif DSP_ALU_FLOAT
            Xn = ALU;
            #endif
            dspALU_SP_t prevX = dataPtr[0];
            dataPtr[0] = Xn;
            Xn -= prevX;
            ALU = *accPtr;  // retreive ALU from previous cycle
            #if DSP_ALU_INT
            // ALU expected to contain s.31 sample (typically after LOAD or before STORE)
                dspALU_SP_t prevY = dataPtr[1];
                dspmacs64_32_32( &ALU, Xn, 1<<DSP_MANT_FLEX );     // add (Xn-X[n-1]) scaled 28bit up
                dspmacs64_32_32( &ALU, prevY, pole);    // add Y[n-1] * pole (pole is negative and small like -0.001)
                *accPtr = ALU;  // store ALU for re-integration at the next cycle
                dataPtr[1] = dspShiftInt( ALU, DSP_MANT_FLEX);    // reduce precision to store Yn
            #elif DSP_ALU_FLOAT
                dspALU_SP_t prevY = ALU;
                ALU += Xn;
                dspMaccFloatFloat( &ALU , prevY, pole );
                *accPtr = ALU;  // store ALU for re-integration at the next cycle
            #endif
        break; }

        /* inspired form MPD dithering code
        inline T PcmDither::Dither(T sample) noexcept
        {       constexpr T round = 1 << (scale_bits - 1);
                constexpr T mask = (1 << scale_bits) - 1;
                sample += error[0] - error[1] + error[2];
                error[2] = error[1];
                error[1] = error[0] / 2;
                T output = sample + round;
                const T rnd = pcm_prng(random);
                output += (rnd & mask) - (random & mask);
                random = rnd;
                output &= ~mask;
                error[0] = sample - output;
                return output >> scale_bits; }
        */
        case DSP_DITHER: {  // using MPD algorythm with Hz = 1.0(z-2), -0.5(z-1), 0.5z +1
                int offset = ptr[1].i32;                   // where is the data space for state data calculation
                dspALU_t * errorPtr  = (dspALU_t*)(dspRuntimeDataPtr+offset);
                dspALU_t temp0 = errorPtr[0];
                dspALU_t temp1 = errorPtr[1];
                dspALU_t temp2 = errorPtr[2];
                ALU += temp0;
                #if DSP_ALU_INT
                    temp0 >>= 1;
                #elif DSP_ALU_FLOAT
                    #if DSP_ALU_64B
                        dspShiftDouble( &temp0, -1);
                    #else
                        dspShiftFloat(  &temp0, -1);
                    #endif
                #endif
                ALU -= temp1;
                ALU += temp2;
                errorPtr[1] = temp0;
                errorPtr[2] = temp1;
                dspALU_t sample = ALU;
                dspTpdfApply( &ALU , tpdfPtr);
                dspTpdfTruncate( &ALU , tpdfPtr);    // remove lowest bits according to dither format
                errorPtr[0] = sample - ALU;       // resulting total error
        break; }

        case DSP_DITHER_NS2: {
                int offset = ptr[1].i32;                       // where is the data space for state data calculation
                dspALU_SP_t * errorPtr  = (dspALU_SP_t*)(dspRuntimeDataPtr+offset);
                offset = ptr[2].i32;
                int freq = dspSamplingFreqIndex * 3;
                dspParam_t * tablePtr = (dspParam_t*)&ptr[offset+freq];   // point on the offset to be used for the current frequency
                const dspParam_t coef0 = tablePtr[0];                   // eg  1.0
                const dspParam_t coef1 = tablePtr[1];                   // eg -0.5
                const dspParam_t coef2 = tablePtr[2];                   // eg  0.5
                const dspALU_SP_t err0 = errorPtr[0];
                const dspALU_SP_t err1 = errorPtr[1];
                const dspALU_SP_t err2 = errorPtr[2];
                #if DSP_ALU_INT
                    dspmacs64_32_32( &ALU, err0, coef0);
                    dspmacs64_32_32( &ALU, err1, coef1);
                    dspmacs64_32_32( &ALU, err2, coef2);
                #elif DSP_ALU_FLOAT
                    dspMaccFloatFloat( &ALU , err0, coef0 );
                    dspMaccFloatFloat( &ALU , err1, coef1 );
                    dspMaccFloatFloat( &ALU , err2, coef2 );
                #endif
                errorPtr[1] = err0;
                errorPtr[2] = err1;
                dspALU_t sample = ALU;
                dspTpdfApply( &ALU , tpdfPtr);
                dspTpdfTruncate( &ALU , tpdfPtr);
                sample -= ALU;                // compute error
                #if DSP_ALU_INT
                    errorPtr[0] = dspShiftInt(sample, DSP_MANT_FLEX);
                #elif DSP_ALU_FLOAT
                    errorPtr[0] = (dspALU_SP_t)sample;  //simply convert to simple 32 bits precisions
                #endif
        break; }



        case DSP_DISTRIB:{
            int IO = ptr[1].i32;          // get output number (like for STORE)
            dspSample_t * samplePtr = sampPtr+IO;
            int size = ptr[2].i32;       // get size of the array, this factor is also used to scale the ALU
            int offset = ptr[3].i32;     //offset of the storage table
            int * dataPtr = dspRuntimeDataPtr+offset;   // where we have the storage space
            int index = *dataPtr++; // get position in the table for outputing the value as if it was a clean sample.
            int middle = size >> 1; //midle of the table
            dspALU_SP_t sample = ALU;   //convert as simple precision
            if (sample) {
            #if DSP_ALU_INT
                int pos = dspmuls32_32_32(sample, size); // our sample is now between say -256..+255 if size = 512
            #elif DSP_ALU_FLOAT
                int pos = sample * middle;   // sample expected to be -1..+1 so pos expeted to be say -256..+256
            #endif
                pos += middle;            // our array is 0..511 so centering the value
                if ((pos>=0) && (pos<size))     //check boundaries
                    dataPtr[pos]++;   // one more sample counted
            }
            int value = dataPtr[index]; // retreive occurence for displaying as a curve with REW Scope function
            if (value == 0) {   // special case to avoid one unique sample drop to 0
                if (index) value = dataPtr[index-1];  // get value of previous storage bin
                else value = dataPtr[1];
            }
            index++;
            if (index >= size) index = 0;   //rollover check
            *--dataPtr = index; //store index in the first location of our dataspace
            // store raw value
            #if DSP_ALU_INT
                *samplePtr = value;     //store value in the output number given
            #elif DSP_SAMPLE_INT
                *samplePtr = value;
            #elif DSP_ALU_FLOAT
                *samplePtr = dspIntToFloatScaled(value,31);
            #endif
        break;}


        case DSP_DIRAC:{
            int offset = ptr[1].i32;
            int * dataPtr = dspRuntimeDataPtr+offset;   // space for the counter
            int counter = *dataPtr;
            dspParam_t * gainPtr = (dspParam_t*)&ptr[2]; //expected to be positive only
            int freq = dspSamplingFreqIndex;
            int maxCount = ptr[3+freq].i32;
            if (counter == 0)
            #if DSP_ALU_INT
                ALU = (dspALU_t)(*gainPtr) << 31;      // pulse in ALU
            #elif DSP_ALU_FLOAT
                ALU = (*gainPtr);
            #endif
            counter++;
            if (counter>=maxCount) counter = 0;
            *dataPtr = counter;

        break;}


        case DSP_SQUAREWAVE:{
            int offset = ptr[1].i32;
            int * dataPtr = dspRuntimeDataPtr+offset;   // space for the counter
            int counter = *dataPtr;
            dspParam_t * gainPtr = (dspParam_t*)&ptr[2];
            int freq = dspSamplingFreqIndex;
            int maxCount = ptr[3+freq].i32;
            if (counter <= (maxCount/2))
            #if DSP_ALU_INT
                dspmacs64_32_32_0(&ALU, DSP_QNM(0.5,1,31), (*gainPtr));       // square wave +0.5
            else
                dspmacs64_32_32_0(&ALU, DSP_QNM(-0.5,1,31), (*gainPtr));       // square wave -0.5
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    ALU = dspMulFloatDouble( +0.5, *gainPtr);
                else
                    ALU = dspMulFloatDouble( -0.5, *gainPtr);
                #else
                    ALU = dspMulFloatFloat(  +0.5, *gainPtr);
                else
                    ALU = dspMulFloatFloat(  -0.5, *gainPtr);
                #endif
            #endif
            counter++;
            if (counter >= maxCount) counter=0;
            *dataPtr = counter;
        break;}


        case DSP_CLIP:{
            dspParam_t * valuePtr = (dspParam_t *)&ptr[1]; // value expected to be positive only
            #if DSP_ALU_INT
                dspALU_t thresold = (dspALU_t)(*valuePtr) << 31;
            #elif DSP_ALU_FLOAT
                dspALU_t thresold = (*valuePtr);
            #endif
            if (ALU > thresold)     ALU =  thresold;
            else
            if (ALU < (-thresold))  ALU = -thresold;
        break; }

        // after release 1.0
        case DSP_LOAD_MEM_DATA:{ asm volatile( "#DSP_LOAD_MEM_DATA:");
            int offset = ptr[1].i32;
            dspALU_t * memPtr = (dspALU_t*)(dspRuntimeDataPtr+offset);
            ALU = *memPtr;
        break;}


        case DSP_SINE:{ //perfect -140db THD by using 64bit accumulator (integer64 or double float).
            int offset = ptr[1].i32;
            dspALU_t * aluPtr = (dspALU_t*)(dspRuntimeDataPtr+offset);   // space for the 2 state variable xn yn potentially 64 bitsdataPtr;
            dspParam_t * gainPtr = (dspParam_t*)&ptr[2]; //amplitude of sine, expected to be positive only
            dspParam_t * epsilonPtr = (dspParam_t*)&ptr[3+dspSamplingFreqIndex];
            dspParam_t epsilon = *epsilonPtr;
            ALU = aluPtr[1];      //yn = sinus
            #if DSP_ALU_INT     //35 cycles on XS2
            // ALU format is 64 bits where sample is s.31 and scalling is applied
            // as 4.28 (depending on DSP_MANT_FLEX). result is 4.60 or s3.60
                if (ALU == 0)
                    ALU2 = (dspALU_t)(*gainPtr) << 31;    //force xn = max value
                else ALU2 = aluPtr[0]; //xn
                dspALU_SP_t yn = dspShiftInt( ALU, DSP_MANT_FLEX );  //get a 32 bit version of yn
                dspmacs64_32_32( &ALU2, -epsilon, yn );    //compute xn+1 = xn - epsilon * yn
                dspALU_SP_t xn = dspShiftInt( ALU2, DSP_MANT_FLEX );  //get a 32 bit version of previous result (xn+1)
                dspmacs64_32_32( &ALU, epsilon, xn );    //compute yn+1 = yn + epsilon * xn+1
            #elif DSP_ALU_FLOAT
                ALU2 = (ALU == 0.0) ? *gainPtr : aluPtr[0];    //force xn to "1" if yn=0
                #if DSP_ALU_64B
                    //this provides exact same quality as INT64
                    ALU2 += (-epsilon * ALU);     //xn+1 = xn - epsilon * yn
                    ALU  += ( epsilon * ALU2);    //yn+1 = yn + epsilon * xn+1
                #else
                    //this provides good thd quality but jitter (below 140db...).
                    dspMaccFloatFloat( &ALU2,-epsilon, ALU);
                    dspMaccFloatFloat( &ALU,  epsilon, ALU2);
                #endif
            #endif
             aluPtr[0] = ALU2;   //xn = cosine
             aluPtr[1] = ALU;    //yn = sine
        break;}


        case DSP_MIXER: { asm volatile ("#DSP_MIXER:");
            dspParam_t gain;
            dspSample_t sample;
            ALU = 0;
            int index = ptr->op.skip;
            while (--index){
                gain = (dspParam_t)ptr[index].i32;
                sample = sampPtr[ptr[--index].i32];
                #if DSP_ALU_INT
                     dspmacs64_32_32( &ALU, sample, gain );
                 #elif DSP_ALU_FLOAT
                     #if DSP_SAMPLE_INT
                         dspALU_SP_t tmp = dspIntToFloatScaled(sample,31);
                         #if DSP_ALU_64B
                             ALU += dspMulFloatDouble( tmp, gain );
                         #else
                             ALU += dspMulFloatFloat( tmp, gain );
                         #endif
                     #else
                          ALU += sample * gain;
                     #endif
                 #endif
            }

            break; }

        case DSP_INTEGRATOR : break;    //TODO
        case DSP_CICUS : break;    //TODO
        case DSP_CICN : break;    //TODO

        } // end of switch (opcode)

        if (ALUptr) { //when using in single opcode mode
            ALUptr[0] = ALU; ALUptr[1] = ALU2;
            return 0; }

        ptr = &ptr[ptr->op.skip];    // move on to next opcode
        dspprintf2("\n");
        } //while 1

} // while(1)
