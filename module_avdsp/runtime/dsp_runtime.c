/*
 * dsp_runtime.c
 *
 *  Version: May 1st 2020
 *      Author: fabriceo
 */

//#define DSP_MANT_FLEXIBLE 1 // this force the runtime to accept programs encoded with any value for DSP_MANT (slower execution)
//#define DSP_SINGLE_CORE 1   // to be used when the target architecture support only one core. then all cores are just chained
#define DSP_IEEE754_OPTIMISE 63   // 63 is the default value if not defined here, check yourself against your F.P. CPU

#include "dsp_runtime.h"        // enum dsp codes, typedefs and QNM definition
#include "dsp_ieee754.h"        // some optimized function for IEEE float/double number as static inline
#include "dsp_fpmath.h"         // some fixed point maths as static inline
#include "dsp_tpdf.h"           // functions related to randomizer, tpdf, truncation as static inline
#include "dsp_biquadSTD.h"      // biquad related functions
#include "dsp_firSTD.h"         // fir functions prototypes

//prototypes
// search for a core number and return pointer on begining of core code
opcode_t * dspFindCore(opcode_t * ptr, const int numCore);
// point further in the dsp core code, after PARAM sections
opcode_t * dspFindCoreBegin(opcode_t * ptr);

int dspRuntimeInit( opcode_t * codePtr,     // pointer on the dspprogram
                    int maxSize,            // size of the opcode table available in memory including data area
                    const int fs,           // sampling frequency to be used
                    int random,             // initial value for random generator
                    int defaultDither);     // dither value used for dsp_TPDF(0)

int DSP_RUNTIME_FORMAT(dspRuntime)( opcode_t * ptr,         // pointer on the coree to be executed
                                    int * rundataPtr,       // pointer on the data area (end of code)
                                    dspSample_t * sampPtr); // pointer on the working table where the IO samples are available

//all variable below are globals for every dsp core and initialized by the call to dspRuntimeInit
dspHeader_t * dspHeaderPtr;             // points on opcode header see dsp_header.h for structure.
int dspBiquadFreqSkip;                  // note : used in biquad xs2 assembly routine as external
int dspMantissa;                        // reflects DSP_MANT or dspHeaderPtr->format


// search a core occurence in the opcode table
opcode_t * dspFindCore(opcode_t * codePtr, const int numCore){  // search core and return begining of core code

    if (codePtr->op.opcode != DSP_HEADER) return 0;
    opcode_t * ptr = codePtr;
    int num = 0;
    while (1) {
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        if (skip == 0) {// = END_OF_CORE
            if (num == 0) return codePtr;   // if no dsp_CORE, then return begining of code
            else return 0;}
        if (code == DSP_CORE) {
            num++;
            if (num == numCore) return ptr;
        }
        ptr += skip;
    } // while(1)
}

// skip begining of code which is not needed at each cycle
opcode_t * dspFindCoreBegin(opcode_t * ptr){
if ((ptr != 0) && (ptr->op.opcode == DSP_CORE))
    while (1) { // skip garabage at begining of core code
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        if (skip == 0) return ptr; // = END_OF_CODE
        if ( (code == DSP_CORE) ||
             (code == DSP_NOP) ||
             (code == DSP_PARAM) ||
             (code == DSP_PARAM_NUM) )  // skip any data if at begining of code
            ptr += skip;
         else
             break;    // most likely a valid code now on
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

static int dspSamplingFreq;
static int dspMinSamplingFreq;
static int dspMaxSamplingFreq;
static int dspNumSamplingFreq;
static int dspSamplingFreqIndex;
static int dspDelayLineFactor;
static int dspRmsFactorFS;
static int dspBiquadFreqOffset;

static void dspChangeFormat(opcode_t * ptr, int newFormat);

// to be ran after dspRuntimeInit() at EACH sampling frequency change.

int dspRuntimeReset(const int fs,
                    int random,                     // for tpdf / dithering
                    int defaultDither) {            // dither value used for dsp_TPDF(0)
        int freqIndex = dspConvertFrequencyToIndex(fs);
        if (freqIndex>=FMAXpos) {
            dspprintf("ERROR : sampling frequency not supported.\n"); return -1; }
        int min = dspHeaderPtr->freqMin;
        int max = dspHeaderPtr->freqMax;
        if ((freqIndex < min) || (freqIndex > max)) {
             dspprintf("ERROR : sampling freq not compatible with encoded dsp program.\n"); return -2; }

        dspSamplingFreq     = freqIndex;
        dspMinSamplingFreq  = min;
        dspMaxSamplingFreq  = max;
        dspSamplingFreqIndex= freqIndex - min;  // relative index of the sampling freq vs the encoded min freq
        dspNumSamplingFreq  = max - min +1;
        dspBiquadFreqSkip   = 2+6*dspNumSamplingFreq;   // 3 words for filter user params (type+freq, Q, gain) + 5 coef alligned per biquad
        dspBiquadFreqOffset = 5+6*dspSamplingFreqIndex; // skip also the 1+1+3 first words
        dspDelayLineFactor  = dspTableDelayFactor[freqIndex];
        dspRmsFactorFS      = dspTableRmsFactor[freqIndex];
        // now clear the data area, just after the program area
        int length = dspHeaderPtr->totalLength;    // lenght of the program
        int size   = dspHeaderPtr->dataSize;       // size of the data needed
        int * intPtr = (int*)dspHeaderPtr;
        intPtr += length;  // point on data space
        for (int i = 0; i < size; i++) *(intPtr+i) = 0;

        dspTpdfInit(random,defaultDither);
	return 0;
}

// make the basic sanity check, predefine some FS related variable
// DOES CLEAR THE DATA AREA from end of program to max_code_size
// to be ran once before dspRuntime().
int dspRuntimeInit( opcode_t * codePtr,             // pointer on the dspprogram
                    int maxSize,                    // size of the opcode table available in memory including data area
                    const int fs,                   // sampling frequency currently in use
                    int random,                     // for initialization of tpdf / dithering
                    int defaultDither) {            // dither value used in case of dsp_TPDF(0)

    dspHeaderPtr = (dspHeader_t*)codePtr;
    opcode_t* cptr = codePtr;
    int code = cptr->op.opcode;
    if (code == DSP_HEADER) {
        unsigned sum ;
        int numCores ;
	int res;

    int length = dspHeaderPtr->totalLength;    // lenght of the program
    int size   = dspHeaderPtr->dataSize;       // size of the data needed
    if ((size+length) > maxSize){
        dspprintf("ERROR : total size (program+data = %d) is over the allowed size (%d).\n",length+size, maxSize); return -6; }

        dspCalcSumCore(codePtr, &sum, &numCores, length);
        if (numCores < 1) {
            dspprintf("ERROR : no cores defined in the program.\n"); return -3; }
        if (sum != dspHeaderPtr->checkSum) {
            dspprintf("ERROR : checksum problem with the program.\n"); return -4; }

        if (dspHeaderPtr->maxOpcode >= DSP_MAX_OPCODE) {
            dspprintf("ERROR : some recent opcodes in this dsp program are not supported by this runtime version.\n"); return -5; }


        dspMantissa = DSP_MANT; // possibility to pass this as a parameter in a later version
#if   DSP_ALU_INT
        if (dspHeaderPtr->format != DSP_MANT_FLEX)
            dspChangeFormat(codePtr, DSP_MANT_FLEX);
#elif DSP_ALU_FLOAT
        if (dspHeaderPtr->format != 0)  // encoded 0 means float
            dspChangeFormat(codePtr, 0);
#endif

	if(fs) {
		res=dspRuntimeReset(fs,random,defaultDither);
		if(res) return res;
	}
        return length;  // ok
    } else {
        dspprintf("ERROR : no dsp header in this program.\n"); return -1; }
}


static void dspChangeThisData(dspALU32_t * p, int old, int new){
    if (old)    // old is interger
        if (new) {// new is integer
            int delta = new-old;
            if (delta>0) p->i <<= delta;
            if (delta<0) p->i >>= -delta;
        } else // new is float
            p->f = (float)(p->i) / (float)(1 << old);
    else // old is float
        if (new) // new is integer
            p->i = dspQNM(p->f, new);
}

// to be launched just after runtime, to potentially convert the encoded format.
// typically needed when encoder generate float and runtime is INT64. versatile
static void dspChangeFormat(opcode_t * ptr, int newFormat){
    dspHeader_t * headerPtr = (dspHeader_t *)ptr;
    int oldFormat = headerPtr->format;
    if (oldFormat == newFormat) return; // sanity check
    while(1){
        int code = ptr->op.opcode;
        unsigned int skip = ptr->op.skip;
        if (skip == 0) break;       // end of program encountered
        dspALU32_t * cptr = (dspALU32_t*)(ptr+1); // point on the first parameter
        switch(code){

        case DSP_DIRAC:             // dataptr then imediate value
        case DSP_SQUAREWAVE:
             cptr++;
        case DSP_MUL_VALUE:         // immediate value
        case DSP_DIV_VALUE:
        case DSP_DATA_TABLE:
        case DSP_CLIP: {
            dspChangeThisData(cptr, oldFormat, newFormat);
            break;}

        case DSP_LOAD_GAIN:         // index then gainptr
             cptr++;                // skip index
        case DSP_GAIN:              // gainptr (indirect value)
        case DSP_SAT0DB_GAIN:
        case DSP_SAT0DB_TPDF_GAIN:{
            cptr = (dspALU32_t*)(ptr+cptr->i);   // compute gainptr
            dspChangeThisData(cptr, oldFormat, newFormat);
            break;}

        case DSP_LOAD_MUX:{ // tableptr
            cptr = (dspALU32_t*)(ptr+cptr->i);   //point on table
            short num = cptr->i;                // number of records
            cptr++;   // point on couples
            for (int i=0;i<num;i++) {
                cptr++; // skip index
                dspChangeThisData(cptr++, oldFormat, newFormat); }
            break;}

        case DSP_BIQUADS: {// dataspace pointer then tableptr
            cptr++; // skip pointer on data space
            cptr = (dspALU32_t*)(ptr+cptr->i);   //point on coef table
            short numSection = cptr->i;   // number of section (lsb)
            cptr+=3;    // point on 1st section
            for (int i=0; i< numSection; i++) {
                cptr+=2;    // skip Q and Gain
                for (int j=0; j< dspNumSamplingFreq; j++) {
                    for (int k=0; k<5; k++)
                        dspChangeThisData(cptr++, oldFormat, newFormat);
                    cptr++; // round up to 6th position
                }
            }
        } break;

        case DSP_FIR: {// TODO
        } break;

        case DSP_DITHER_NS2: {// data space pointer then table ptr
            cptr++; // skip pointer on data space
            cptr = (dspALU32_t*)(ptr+cptr->i); //point on coef table
            for (int i=0; i< dspNumSamplingFreq; i++) {
                dspChangeThisData(cptr++, oldFormat, newFormat);
                dspChangeThisData(cptr++, oldFormat, newFormat);
                dspChangeThisData(cptr++, oldFormat, newFormat);
            }
        } break;

        case DSP_DCBLOCK: { // dataspace pointer then imediate table of pole
            cptr++; // skip pointer on data space
            for (int i=0; i< dspNumSamplingFreq; i++)
                dspChangeThisData(cptr++, oldFormat, newFormat);
        } break;

        } // switch

        ptr += skip;
    } // while(1)
    headerPtr->format = newFormat;
}

// dsp interpreter implementation
int DSP_RUNTIME_FORMAT(dspRuntime)( opcode_t * ptr,         // pointer on the coree to be executed
                                    int * rundataPtr,       // pointer on the data area (end of code)
                                    dspSample_t * sampPtr) {// pointer on the working table where the IO samples are available

    tpdf_t tpdfLocal;   // local data for tpdf / dither
    tpdf_t * tpdfPtr = &dspTpdfGlobal;  // by default point on global dataset
    dspALU_t ALU2 = 0;
    dspALU_t ALU  = 0;

    while (1) { // main loop

        int * cptr = (int*)ptr;
        int opcode = ptr->op.opcode;
        unsigned int skip   = ptr->op.skip;
        dspprintf2("[%2d] <+%3d> : ", opcode, skip);
        cptr++; // will point on the first potential parameter just after the opcode

        switch (opcode) { // efficiently managed with a jump table

        case DSP_END_OF_CODE: {
            dspprintf2("END");
            return 0; }

        case DSP_CORE: {
            dspprintf2("CORE");
#if defined( DSP_SINGLE_CORE ) && ( DSP_SINGLE_CORE == 1 )
            break; }    // this will continue the dsp execution over this opcode
#else
            return 0; }
#endif

        case DSP_NOP: { // might be generated by the encoder to force allignement on 8 bytes
            dspprintf2("NOP");
            break; }

        case DSP_SWAPXY: {
            dspprintf2("SWAPXY");
            dspALU_t tmp = ALU;
            ALU = ALU2;
            ALU2 = tmp;
            break;}

        case DSP_COPYXY: {
            dspprintf2("COPYXY");
            ALU2 = ALU;
            break;}

        case DSP_COPYYX: {
            dspprintf2("COPYYX");
            ALU = ALU2;
            break;}

        case DSP_CLRXY: { // tested
            dspprintf2("CLRXY");
            ALU2 = 0;
            ALU = 0;
            break;}

        case DSP_ADDXY: {
            dspprintf2("ADDXY");
            ALU += ALU2;
            break;}

        case DSP_ADDYX: {
            dspprintf2("ADDYX");
            ALU2 += ALU;
            break;}

        case DSP_SUBXY: {
            dspprintf2("SUBXY");
            ALU -= ALU2;
            break;}

        case DSP_SUBYX: {
            dspprintf2("SUBYX");
            ALU2 -= ALU;
            break;}

        case DSP_NEGX: {
            dspprintf2("NEGX");
            ALU = -ALU;
            break;}

        case DSP_NEGY: {
            dspprintf2("NEGY");
            ALU2 = -ALU2;
            break;}

        case DSP_SHIFT: {
            dspprintf2("SHIFT");
            int shift = *cptr;      // get parameter
            #if DSP_ALU_INT
                if (shift >= 0)
                     ALU <<= (shift>=100? DSP_MANT_FLEX : shift);
                else
                    ALU >>= (shift<=-100? DSP_MANT_FLEX : -shift);
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    dspShiftDouble( &ALU, shift);
                #else
                    dspShiftFloat(  &ALU, shift);
                #endif
            #endif
            break; }


        case DSP_MULXY: {
            dspprintf2("MULXY");
            ALU *= ALU2;
            break;}


        case DSP_DIVXY: {   // TODO
            dspprintf2("DIVXY");
            ALU /= ALU2;
            break;}


        case DSP_DIVYX: {   // TODO
            dspprintf2("DIVYX");
            ALU2 /= ALU;
            break;}


        case DSP_AVGXY: {   // TODO
            dspprintf2("AVGXY");
            ALU = ALU/2 + ALU2/2;
            break;}


        case DSP_AVGYX: {   // TODO
            dspprintf2("AVGYX");
            ALU2 = ALU/2 + ALU2/2;
            break;}


        case DSP_SQRTX: {  // TODO
            dspprintf2("SQRTX");
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


        case DSP_SAT0DB: {
            dspprintf2("SAT0DB");
            #if DSP_ALU_INT
                dspSaturate64_031( &ALU, DSP_MANT_FLEX );
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    dspSaturateDouble0db( &ALU );
                #else
                    dspSaturateFloat0db( &ALU );
                #endif
            #endif
            break;}


        case DSP_SAT0DB_TPDF: {
            dspprintf2("SAT0DB_TPDF");
            #if DSP_ALU_INT
                dspTpdfApply( &ALU , tpdfPtr);
                dspSaturate64_031( &ALU , DSP_MANT_FLEX);   // now s31
            #elif DSP_ALU_FLOAT
                dspTpdfApply( &ALU , tpdfPtr);
                #if DSP_ALU_64B
                    dspSaturateDouble0db( &ALU );
                #else
                    dspSaturateFloat0db( &ALU );
                #endif
            #endif
            break;}


        case DSP_SAT0DB_GAIN: {
            dspprintf2("SAT0DB_GAIN");
            dspParam_t * gainPtr = (dspParam_t*)(ptr+*cptr);
            #if DSP_ALU_INT
                ALU >>= DSP_MANT_FLEX;   // reduce precision by removing DSP_MANT bits to give free size for coming multiply
                ALU *= (*gainPtr);
                dspSaturate64_031( &ALU ,DSP_MANT_FLEX);
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


        case DSP_SAT0DB_TPDF_GAIN: {
            dspprintf2("SAT0DB_TPDF_GAIN");
            dspParam_t * gainPtr = (dspParam_t*)(ptr+*cptr);
            #if DSP_ALU_INT
                ALU >>= DSP_MANT_FLEX;
                ALU *= (*gainPtr);
                dspTpdfApply( &ALU , tpdfPtr);
                dspSaturate64_031( &ALU , DSP_MANT_FLEX);
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    dspALU_SP_t tmp = ALU;
                    ALU = dspMulFloatDouble( tmp, *gainPtr );
                    dspTpdfApply( &ALU , tpdfPtr);
                    dspSaturateDouble0db( &ALU );
                #else
                    ALU = dspMulFloatFloat( ALU, *gainPtr );
                    dspTpdfApply( &ALU , tpdfPtr);
                    dspSaturateFloat0db( &ALU );
                #endif
            #endif
            break; }


        case DSP_TPDF_CALC: {
            dspprintf2("TPDF_CALC");
            if (dspTpdfPrepare(&dspTpdfGlobal,&dspTpdfGlobal,*cptr)) {
                ALU = dspTpdfCalc();
                int offset = *(cptr+1);
                dspALU_t * dataPtr = (dspALU_t *)(rundataPtr+offset);
                *dataPtr = ALU;
            } else ALU = 0;
            break;}

        case DSP_TPDF: {
            dspprintf2("TPDF");
            if (! dspTpdfPrepare(tpdfPtr, &tpdfLocal,*cptr)) tpdfPtr = &tpdfLocal;   // move tpdf pointer to local otherwise keep asis (maybe global or local already)
            #if DSP_ALU_INT
                ALU = dspTpdfValue;
            #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                    ALU = dspIntToDoubleScaled(dspTpdfValue,31);
                #else
                    ALU = dspIntToFloatScaled(dspTpdfValue,31);
                #endif
            #endif
                int offset = *(cptr+1);
                dspALU_t * dataPtr = (dspALU_t *)(rundataPtr+offset);
                *dataPtr = ALU;
            break;}


        case DSP_LOAD: {    // load a RAW sample direcly in the ALU
            ALU2 = ALU;     // save ALU in Y in case someone want to use it later
            int index = *cptr;
            dspprintf2("LOAD input[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT
                ALU = *samplePtr;
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    #if DSP_ALU_64B
                        ALU = dspIntToDoubleScaled(*samplePtr,31);
                    #else
                        ALU = dspIntToFloatScaled(*samplePtr,31);
                    #endif
                #else
                        ALU = *samplePtr;
                #endif
            #endif
            break; }


        case DSP_LOAD_GAIN: {
            ALU2 = ALU;     // save ALU in Y in case someone want to use it later
            int index = *cptr++;
            dspSample_t * samplePtr = sampPtr+index;
            dspParam_t * gainPtr = (dspParam_t*)(ptr+*cptr);
            dspprintf2("LOAD input[%d] with gain",index);
            #if DSP_ALU_INT
                dspmacs64_32_32_0( &ALU, *samplePtr, (*gainPtr));
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    dspALU_SP_t tmp = dspIntToFloatScaled(*samplePtr,31);
                    #if DSP_ALU_64B
                        ALU = dspMulFloatDouble( tmp, *gainPtr );
                    #else
                        ALU = dspMulFloatFloat( tmp, *gainPtr );
                    #endif
                #else
                        ALU = *samplePtr;
                        ALU *= (*gainPtr);
                #endif
            #endif
            break; }


        case DSP_STORE: {   // store the ALU
            int index = *cptr;
            dspprintf2("STORE output[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            dspSample_t sample;
            #if DSP_ALU_INT             // expect value to be already satirated and in s31 format!
                sample = ALU;
                sample &= tpdfPtr->mask;   // remove lowest bits according to dither
                *samplePtr = sample;
            #elif DSP_ALU_FLOAT
                #if DSP_SAMPLE_INT
                    #if DSP_ALU_64B
                        sample =  dsps31Double0DB(ALU);
                    #else
                        sample =  dsps31Float0DB(ALU);
                    #endif
                    sample &= tpdfPtr->mask;
                    *samplePtr = sample;
                #else
                    sample = ALU;    // no conversion needed, no mask
                    *samplePtr = sample;
                #endif
            #endif
            break; }


        case DSP_GAIN:{
            dspprintf2("GAIN");
            dspParam_t * gainPtr = (dspParam_t*)(ptr+*cptr);
            ALU *= *gainPtr;
            break;}


        case DSP_VALUE:{
            dspprintf2("VALUE");
            dspParam_t * valuePtr = (dspParam_t*)(ptr+*cptr);
            ALU2 = ALU;
            ALU = (*valuePtr);  // for DSP_INT, scalling has to be done by user with a SHIFT(100)
        break;}


        case DSP_VALUE_INT:{
            dspprintf2("VALUE");
            ALU2 = ALU;
        #if DSP_ALU_INT
            dspParam_t * valuePtr = (dspParam_t*)(ptr+*cptr);
            ALU = (*valuePtr);
        #elif DSP_ALU_FLOAT
            int * valuePtr = (int*)(ptr+*cptr);
            ALU = (*valuePtr);
        #endif
        break;}


        case DSP_WHITE: {
             dspprintf2("WHITE");
             #if DSP_ALU_INT
                 ALU = dspTpdfRandom;
             #elif DSP_ALU_FLOAT
                #if DSP_ALU_64B
                 ALU = dspIntToDoubleScaled(dspTpdfRandom,31);
                #else
                 ALU = dspIntToFloatScaled(dspTpdfRandom,31);
                #endif
             #endif
             break;}


        case DSP_MUL_VALUE:{
            dspprintf2("MUL_VALUE");
            dspParam_t * valuePtr = (dspParam_t*)cptr;          // value stored as a fix number just below opcode
            ALU *= (*valuePtr);
            break;}


        case DSP_DIV_VALUE:{
            dspprintf2("DIV_VALUE");
            dspParam_t * valuePtr = (dspParam_t*)cptr;
            ALU /= (*valuePtr);
            break;}


        case DSP_MUL_VALUE_INT:{
            dspprintf2("MUL_VALUE");
            #if DSP_ALU_INT
            dspParam_t * valuePtr = (dspParam_t*)cptr;
            ALU *= (*valuePtr);
            #elif DSP_ALU_FLOAT
            int * valuePtr = (int*)cptr;
            ALU *= (*valuePtr);
            #endif
            break;}


        case DSP_DIV_VALUE_INT:{
            dspprintf2("DIV_VALUE_INT");
            #if DSP_ALU_INT
            dspParam_t * valuePtr = (dspParam_t*)cptr;
            ALU /= (*valuePtr);
            #elif DSP_ALU_FLOAT
            int * valuePtr = (int*)cptr;
            ALU /= (*valuePtr);
            #endif
            break;}


        case DSP_AND_VALUE_INT:{
            dspprintf2("AND_VALUE_INT");
            #if DSP_ALU_INT
            dspParam_t * valuePtr = (dspParam_t*)cptr;
            long long val = *valuePtr; // sign extend voluntary
            ALU &= val;
            #endif
            break;}


        case DSP_DELAY_1:{
            ALU2 = ALU;
            int offset = *cptr; //point on the area where we have the sample storage (2 words size)
            int * dataPtr = rundataPtr+offset;
            dspprintf2("DELAY_1");
            dspALU_t * memPtr = (dspALU_t*)dataPtr;   // formal type casting to enable double word instructions
            dspALU_t tmp = *memPtr;
            *memPtr = ALU;
            ALU = tmp;
            break;}


        case DSP_LOAD_STORE: {
            unsigned int max = skip-1;   // length of following data in number of words
            dspprintf2("LOAD_STORE (%d)",max);
            while (max) {
                int index = *cptr++;
                dspSample_t value = *(sampPtr+index);
                index   = *cptr++;
                *(sampPtr+index) = value;
                max -= 2; }
            break; }


        case DSP_LOAD_MEM: {
            ALU2 = ALU;
            int offset = *cptr;   //point on the area where we have the memory location in 32 or 64 bits
            int * offsetPtr = (int*)(ptr+offset);
            dspprintf2("LOAD_MEM");
            dspALU_t * memPtr = (dspALU_t*)offsetPtr;
            ALU = *memPtr;
            break; }


        case DSP_STORE_MEM:{
            int offset = *cptr;    //point on the area where we have the memory location in 32 or 64 bits
            int * offsetPtr = (int*)(ptr+offset);
            dspprintf2("STORE_MEM");
            dspALU_t * memPtr = (dspALU_t*)offsetPtr;
            *memPtr = ALU;
            break; }


        case DSP_DELAY: {
            dspprintf2("DELAY");
            unsigned  maxSize = *cptr++;                // maximum size in number of samples, OR delay in uSec
            int offset  = *cptr++;                      // adress of data table for storage
            int * dataPtr = rundataPtr+offset;          // now points on the delay line data, starting with current index
            unsigned nSamples;
            offset = *cptr;                             // get the offset where we can find the delay in microsseconds
            if (offset == 0) {                          // 0 means it is a fixed delay already defined in maxSize variable
                nSamples = dspmulu32_32_32(maxSize, dspDelayLineFactor);    // microSec was stored in maxSize by design for DELAY_Fixed
            } else {
                int * microSecPtr = (int*)(ptr+offset);   // where is stored the delay
                unsigned short microSec = *microSecPtr;   // read microsec (16bits=short)
                nSamples = dspmulu32_32_32(microSec, dspDelayLineFactor);
                if (nSamples > maxSize) nSamples = maxSize; // security check as the host aplication may have put an higher data...
            }
            if (nSamples) { // sanity check in case host application put delay=0 to bypass it
                int index = *dataPtr++;                 // the dynamic index is stored as the first word in the delayline table
                dspALU_SP_t * linePtr = (dspALU_SP_t*)(dataPtr+index);
                dspALU_SP_t value = *linePtr;
                *linePtr = ALU;
                ALU = value;
                index++;
                if (index >= nSamples) index = 0;
                *(--dataPtr) = index;
            }
            break;}


        // exact same code as above appart size of value exchange. Should be rationalized in a later version
        case DSP_DELAY_DP: {
            dspprintf2("DELAY_DP");
            unsigned maxSize = *cptr++;               // maximum size in number of samples, OR delay in uSec
            int offset  = *cptr++;                    // adress of data table for storage
            int * dataPtr = rundataPtr+offset;        // now points on the delay line data, starting with current index
            unsigned nSamples;
            offset = *cptr;                           // get the offset where we can find the delay in microsseconds
            if (offset == 0) {
                nSamples = dspmulu32_32_32(maxSize, dspDelayLineFactor);    // microSec was stored in maxSize by design for DELA_Fixed
            } else {
                int * microSecPtr = (int*)ptr+offset;
                unsigned short microSec = *microSecPtr;                // read microsec
                nSamples = dspmulu32_32_32(microSec, dspDelayLineFactor);
                if (nSamples > maxSize) nSamples = maxSize;
            }
            if (nSamples) {
                int index = *dataPtr++;
                dspALU_t * linePtr = (dspALU_t *)dataPtr;   // garanteed to be alligned 8
                linePtr += index;   //(this multiply Index by 2 if dspALU_T is 64bits)
                dspALU_t value = *linePtr;
                *linePtr = ALU;
                ALU = value;
                index++;
                if (index >= nSamples) index = 0;
                *(--dataPtr) = index;
            }
            break;}


        case DSP_BIQUADS: {
            dspprintf2("BIQUAD\n");
            #if DSP_ALU_INT
            asm("#biquadentry:");
            dspSample_t sample = dspShiftInt( ALU, DSP_MANTBQ );    //remove the size of a biquad coef, as the result will be scaled accordingly
            #endif
            dspALU_SP_t * dataPtr = (dspALU_SP_t*)(rundataPtr + *cptr++);
            int * numPtr = (int*)(ptr + *cptr);    // point on the number of sections
            dspParam_t * coefPtr = (dspParam_t*)(numPtr+dspBiquadFreqOffset); //point on the right coefficient according to fs
            int num = *numPtr++;    // number of sections in 16 lsb, biquad routine should keep only 16bits lsb
            if (*numPtr) {  // verify if biquad is in bypass mode or no
            #if DSP_ALU_INT
                // ALU is expected to contain the sample in double precision (typically after DSP_LOAD_GAIN)
                #ifdef DSP_XS2A
                    ALU = dsp_biquads_xs2( sample , coefPtr, dataPtr, num); //DSP_MANTBQ and dspBiquadFreqSkip are defined in assembly file
                #else
                    ALU = dsp_calc_biquads_int( sample, coefPtr, dataPtr, num, DSP_MANTBQ, dspBiquadFreqSkip);
                #endif
            #elif DSP_ALU_FLOAT
                ALU = dsp_calc_biquads_float(ALU, coefPtr, dataPtr, num, dspBiquadFreqSkip);
            #endif
            }
            break; }


        case DSP_PARAM: {
            dspprintf2("PARAM");
            // the data below will be skipped automatically
            break; }

        case DSP_PARAM_NUM: {
            dspprintf2("PARAM_NUM ...");
            // the data below will be skipped automatically
            break; }


        case DSP_SERIAL:{    //
            //int serial= *cptr++;  // supposed to be the serial number of the product, or a specif magic code TBD
            //dspprintf2("SERIAL %d",serial);
            //int check = *cptr++;
            // serial check to do here
            break; }


        case DSP_LOAD_MUX:{
            dspprintf2("LOAD_MUX");
            int offset = *cptr;
            int * tablePtr = (int*)(ptr+offset);
            short max = *tablePtr++ ;   //number of sections
            ALU = 0;
            while (max)  {
                int index = *tablePtr++;
                dspSample_t sample = *(sampPtr+index);
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
            offset = *(cptr+1);
            dspALU_t * dataPtr = (dspALU_t *)(rundataPtr+offset);
            *dataPtr = ALU;
            break; }


        case DSP_DATA_TABLE: {
            dspprintf2("DATA_TABLE");
            dspParam_t * gainPtr = (dspParam_t*)cptr++;  // this is 4.28 format (or float)
            int div =  *cptr++;         // frequency divider
            int size=  *cptr++;         // table size
            int index = *cptr++;        // offset in dataspace where the index is stored
            int *indexPtr = rundataPtr+index;
            int offset= *cptr;          // offset on where the data are stores
            dspSample_t * dataPtr = (dspSample_t*)(ptr+offset);
            index = *indexPtr;
            dspSample_t data = *(dataPtr+index);
            index += div;               // in order to divide the frequency, we skip n value
            if (index >= size) index -= size;
            *indexPtr = index;
            #if DSP_ALU_INT
                dspmacs64_32_32_0( &ALU, data, (*gainPtr));
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
            dspprintf2("FIR \n");
            int freq = dspSamplingFreqIndex;    // this is a delta compared to the minimum supported freq
            int * tablePtr = (int*)cptr+freq;   // point on the offset to be used for the current frequency
            //dspprintf3("tableptr[%d] = 0x%X\n",freq,(int)tablePtr);
            cptr += numFreq;                    // skip all the table of pointers
            int offset = *(tablePtr++);         // get the offset where we have the size of impulse and coef for the current frequency
            if (offset) {
                tablePtr = (int*)ptr+offset;    // now points on the impulse associated to current frequency
                //dspprintf3("fir ImpulsePtr = 0x%X\n",(int)tablePtr);
                int length = *(tablePtr++);     // length of the impulse
                offset = *cptr;                 // offset where are the data for states filter
                dspALU_SP_t * dataPtr = (dspALU_SP_t*)(rundataPtr+offset);
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
            dspprintf2("DSP_RMS");
            asm("#dsprms:");
            //parameter list below opcode_t :
            // (0) data pointer
            // (1) delay
            int offset     = *cptr++;                   // where is the data space for rms calculation
            unsigned delay = *cptr++;                   // size of delay line, followed by couples : counter - factor
            unsigned * dataPtr  = (unsigned*)(rundataPtr+offset);
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
            int * tablePtr = (int*)(cptr+freq);         // point on the offset to be used for the current frequency
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
                    unsigned index = *(dataPtr+1);          // retreive current position in the delay line
                    dspALU_t *delayPtr   = sumSquarePtr+2;  // start of the delay line
                    dspALU_t value = *(delayPtr+index);     // get oldest value
                    *(delayPtr+index) = ALU;                // overwrite oldest and store newest
                    ALU -= value;
                    ALU += *averagePtr;                 // use it in the result
                    index++;                            // prepare for next position in the delay line
                    if (index >= delay) index = 0;
                    *(dataPtr+1) = index; }             // memorize futur position in delay line
                *averagePtr = ALU;                      // memorize new averaged value
                *dataPtr = 0;                           // reset counter of sum.square
                *sumSquarePtr = 0;                      // reset sum.square
                ALU = *(dataPtr+2);                     // load latest sqrt value

            } else {
                *sumSquarePtr = ALU;                    // memorize sumsquare
                *dataPtr = counter;                     // and new incremented counter

                #if DSP_ALU_INT
                if (counter == 1){                      // very first time enterring here, maybe a sumaverage has been computed in last cycle
                    *(dataPtr+4) = 1<<30;               // memorize sqrt iteration bit for next cycle
                    *(dataPtr+3) = 0;                   // reset temporary sqrt value
                    ALU = *(dataPtr+2);                 // retreive previous sqrt calculation, if already done ofcourse
                } else {
                    // compute 64bit sqrt=>32 bits, doing one check at every fs cycle, assuming maxCounter always > 64
                    unsigned bit = *(dataPtr+4);            // read sqrt iteration bit (from 0x800000 to 0)
                    if (bit) {
                        unsigned temp = *(dataPtr+3) | bit; // last calculation result
                        dspALU_t value = dspmulu64_32_32( temp , temp );
                        if (*averagePtr >= value) { *(dataPtr+3) = temp; }
                        bit >>= 1;                          // next iteration
                        *(dataPtr+4) = bit;                 // memorize sqrt iteration bit for next cycle
                        ALU = *(dataPtr+2);                 // read previous sqrt value
                    } else {
                        ALU = *(dataPtr+3);                 // get last calculation result
                        *(dataPtr+2) = ALU;                 // set sqrt value with last calculation
                    }
                }
                #else // float
                    ALU = sqrt(*averagePtr);                // computed at each cycle... a bit too much : TODO
                #endif
            }
            break; }


        case DSP_DCBLOCK: {
            //inspired from here : http://dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping/
            int offset = *cptr++;                   // where is the data space for state data calculation
            dspALU_t * accPtr = (dspALU_t *)(rundataPtr+offset);
            dspALU_SP_t * dataPtr  = (dspALU_SP_t*)(accPtr+1);
            int freq = dspSamplingFreqIndex;        // 1 pole per freq
            dspParam_t * tablePtr = (dspParam_t*)(cptr+freq);   // point on the offset to be used for the current frequency
            dspParam_t pole = *tablePtr;                 // pole of the integrator part

            dspALU_SP_t Xn;
            #if DSP_ALU_INT
            Xn = dspShiftInt( ALU, DSP_MANT_FLEX);
            #elif DSP_ALU_FLOAT
            Xn = ALU;
            #endif
            dspALU_SP_t prevX = *(dataPtr+0);
            *(dataPtr+0) = Xn;
            Xn -= prevX;
            ALU = *accPtr;  // retreive ALU from previous cycle
            #if DSP_ALU_INT
            // ALU expected to contain s.31 sample (typically after LOAD or before STORE)
                dspALU_SP_t prevY = *(dataPtr+1);
                dspmacs64_32_32( &ALU, Xn, 1<<DSP_MANT_FLEX );     // add (Xn-X[n-1]) scaled 28bit up
                dspmacs64_32_32( &ALU, prevY, pole);    // add Y[n-1] * pole (pole is negative and small like -0.001)
                *accPtr = ALU;  // store ALU for re-integration at the next cycle
                *(dataPtr+1) = dspShiftInt( ALU, DSP_MANT_FLEX);    // reduce precision to store Yn
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
                int offset = *cptr++;                   // where is the data space for state data calculation
                dspALU_t * errorPtr  = (dspALU_t*)(rundataPtr+offset);
                dspALU_t temp0 = *(errorPtr+0);
                dspALU_t temp1 = *(errorPtr+1);
                dspALU_t temp2 = *(errorPtr+2);
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
                *(errorPtr+1) = temp0;
                *(errorPtr+2) = temp1;
                dspALU_t sample = ALU;
                dspTpdfApply( &ALU , tpdfPtr);
                dspTpdfTruncate( &ALU , tpdfPtr);    // remove lowest bits according to dither format
                *(errorPtr+0) = sample - ALU;       // resulting total error
        break; }

        case DSP_DITHER_NS2: {
            asm("#ditherns2:");
                int offset = *cptr++;                       // where is the data space for state data calculation
                dspALU_SP_t * errorPtr  = (dspALU_SP_t*)(rundataPtr+offset);
                offset = *cptr;
                int freq = dspSamplingFreqIndex * 3;
                dspParam_t * tablePtr = (dspParam_t*)(ptr+offset+freq);   // point on the offset to be used for the current frequency
                dspParam_t coef0 = (*tablePtr++);                   // eg 1.0
                dspParam_t coef1 = (*tablePtr++);                   // eg -0.5
                dspParam_t coef2 = (*tablePtr);                     // eg 0.5
                dspALU_SP_t err0 = *(errorPtr+0);
                dspALU_SP_t err1 = *(errorPtr+1);
                dspALU_SP_t err2 = *(errorPtr+2);
                #if DSP_ALU_INT
                    dspmacs64_32_32(&ALU, err0, coef0);
                    dspmacs64_32_32(&ALU, err1, coef1);
                    dspmacs64_32_32(&ALU, err2, coef2);
                #elif DSP_ALU_FLOAT
                    dspMaccFloatFloat( &ALU , err0, coef0 );
                    dspMaccFloatFloat( &ALU , err1, coef1 );
                    dspMaccFloatFloat( &ALU , err2, coef2 );
                #endif
                *(errorPtr+1) = err0;
                *(errorPtr+2) = err1;
                dspALU_t sample = ALU;
                dspTpdfApply( &ALU , tpdfPtr);
                dspTpdfTruncate( &ALU , tpdfPtr);
                sample -= ALU;                // compute error
                #if DSP_ALU_INT
                    *(errorPtr+0) = dspShiftInt(sample, DSP_MANT_FLEX);
                #elif DSP_ALU_FLOAT
                    *(errorPtr+0) = (dspALU_SP_t)sample;
                #endif
        break; }



        case DSP_DISTRIB:{
            int IO = *cptr++;       // get output number (like for STORE)
            dspSample_t * samplePtr = sampPtr+IO;
            int size = *cptr++;     // get size of the array, this factor is also used to scale the ALU
            int offset = *cptr;
            int * dataPtr = rundataPtr+offset;   // where we have a data space for us
            int index = *dataPtr++; // get position in the table for outputing the value as if it was a clean sample.
            dspALU_SP_t sample = ALU;
            int middle = size/2;
            if (sample) {
            #if DSP_ALU_INT
                int pos = dspmuls32_32_32(sample, size); // our sample is now between say -256..+255 if size = 512
            #elif DSP_ALU_FLOAT
                int pos = sample * middle;   // ALU expected to be -1..+1
            #endif
                pos += middle;            // our array is 0..511 so centering the value
                if ((pos>=0) && (pos<size)) (*(dataPtr+pos))++;   // one more sample counted
            }
            int value = *(dataPtr+index); // retreive occurence for displaying as a curve with REW Scope function
            if (value == 0) {   // special case to avoid one unique sample drop to 0
                if (index) value = *(dataPtr+index-1);
                else value = *(dataPtr+1);
            }
            index++;
            if (index >= size) index = 0;
            *--dataPtr = index;
            // store raw value
            #if DSP_ALU_INT
                *samplePtr = value;
            #elif DSP_SAMPLE_INT
                *samplePtr = value;
            #elif DSP_ALU_FLOAT
                *samplePtr = dspIntToFloatScaled(value,31);
            #endif
        break;}


        case DSP_DIRAC:{
            int offset = *cptr++;
            int * dataPtr = rundataPtr+offset;   // space for the counter
            int counter = *dataPtr;
            dspParam_t * gainPtr = (dspParam_t*)cptr++;
            int freq = dspSamplingFreqIndex;
            int * tablePtr = (int*)(cptr+freq);
            int maxCount = *tablePtr;
            if (counter == 0)
            #if DSP_ALU_INT
                dspmacs64_32_32_0(&ALU, dspQNMmax(), (*gainPtr));      // pulse in ALU
            #elif DSP_ALU_FLOAT
                ALU = (*gainPtr);
            #endif
            counter++;
            if (counter>=maxCount) counter = 0;
            *dataPtr = counter;

        break;}


        case DSP_SQUAREWAVE:{
            int offset = *cptr++;
            int * dataPtr = rundataPtr+offset;   // space for the counter
            int counter = *dataPtr;
            dspParam_t * gainPtr = (dspParam_t*)cptr++;
            int freq = dspSamplingFreqIndex;
            int * tablePtr = (int*)(cptr+freq);
            int maxCount = *tablePtr;
            if (counter <= (maxCount/2))
            #if DSP_ALU_INT
                dspmacs64_32_32_0(&ALU, (int)DSP_Q31(0.5), (*gainPtr));       // square wave +0.5
            else
                dspmacs64_32_32_0(&ALU, (int)DSP_Q31(-0.5), (*gainPtr));       // square wave -0.5
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
            dspParam_t * valuePtr = (dspParam_t *)cptr; // value expected to be positive only
            #if DSP_ALU_INT
                dspALU_t thresold = dspmulu64_32_32(1<<31,(*valuePtr));
            #elif DSP_ALU_FLOAT
                dspALU_t thresold = (*valuePtr);
            #endif
            if (ALU > thresold)     ALU =  thresold;
            else
            if (ALU < (-thresold))  ALU = -thresold;
        break; }

        // after release 1.0
        case DSP_LOAD_MEM_DATA:{
            int offset = *cptr;
            dspALU_t * dataPtr = (dspALU_t *)(rundataPtr+offset);   // where we have a data space for us
            ALU = *dataPtr;
        break;}

        } // switch (opcode)

        ptr += skip;    // move on to next opcode
        dspprintf2("\n");
        } //while 1

} // while(1)

