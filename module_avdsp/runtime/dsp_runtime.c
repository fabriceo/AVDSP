/*
 * dsp_coder.c
 *
 *  Created on: 1 janv. 2020
 *      Author: fabriceo
 *      this program will create a list of opcodes to be executed lated by the dsp engine
 */

#include "dsp_runtime.h"      // enum dsp codes, typedefs and QNM definition
#include "dsp_inlineSTD.h"
#include "dsp_biquadSTD.h"
#include "dsp_firSTD.h"
#include "math.h"   // for importing sqrt()

//#define DSP_SINGLE_CORE 1 // set this define if only 1 core available on the target arch

int dspSamplingFreq;
int dspMinSamplingFreq = DSP_DEFAULT_MIN_FREQ;
int dspMaxSamplingFreq = DSP_DEFAULT_MAX_FREQ;
int dspNumSamplingFreq;
int dspSamplingFreqIndex;
int dspBiquadFreqSkip;
static int dspDelayLineFactor;
static int dspRmsFactorFS;
static int dspBiquadFreqOffset;


opcode_t * dspFindCore(opcode_t * ptr, const int numCore){  // search core and return begining of core code
    int num = 0;
    while (1) {
        int code = ptr->op.opcode;
        int skip = ptr->op.skip;
        //printf("1ptr @0x%X = %d-%d\n",(int)ptr,code, skip);
        if (skip == 0) return 0; // = END_OF_CORE
        if (code == DSP_CORE) {
            num++;
            if (num == numCore) {
                ptr++;
                break; } }
        ptr += skip;
    } // while(1)
    while (1) { // skip garabage at begining of core code
        int code = ptr->op.opcode;
        int skip = ptr->op.skip;
        //printf("2ptr @0x%X = %d-%d\n",(int)ptr,code, skip);
        if (skip == 0) break; // = END_OF_CODE
        if ( (code == DSP_NOP) ||
             (code == DSP_PARAM) ||
             (code == DSP_PARAM_NUM) )  // skip any data if at begining of code
            ptr += skip;
         else
             break;    // seems the code is now a valid code to process
    }
    return ptr;
}



static const int dspTableFreq[FMAXpos] = {
        8000, 16000,
        24000, 32000,
        44100, 48000,
        88200, 96000,
        176400,192000,
        352800,384000,
        705600, 768000 };

// table of magic number to be multiplied by a number of microseconds to get a number of sample (x2^32)
#define dspDelayFactor 4294.967296  // 2^32/10^6
const unsigned int dspTableDelayFactor[FMAXpos] = {
        dspDelayFactor*8000, dspDelayFactor*16000,
        dspDelayFactor*24000, dspDelayFactor*32000,
        dspDelayFactor*44100, dspDelayFactor*48000,
        dspDelayFactor*88200, dspDelayFactor*96000,
        dspDelayFactor*176400,dspDelayFactor*192000,
        dspDelayFactor*352800,dspDelayFactor*384000,
        dspDelayFactor*705600, dspDelayFactor*768000
};

#define dspRmsFactor 1000.0  // 2^32/10^6 TODO
const unsigned int dspTableRmsFactor[FMAXpos] = {
        dspRmsFactor/8000.0, dspRmsFactor/16000.0,
        dspRmsFactor/24000.0, dspRmsFactor/32000.0,
        dspRmsFactor/44100.0, dspRmsFactor/48000.0,
        dspRmsFactor/88200.0, dspRmsFactor/96000.0,
        dspRmsFactor/176400.0,dspRmsFactor/192000.0,
        dspRmsFactor/352800.0,dspRmsFactor/384000.0,
        dspRmsFactor/705600.0, dspRmsFactor/768000.0
};


static inline int dspFindFrequencyIndex(int freq){
    for (int i=0; i<FMAXpos; i++)
        if (freq == dspTableFreq[i]) return i;
    return -1;
}


// make the basic sanity check, predefined some FS related variable
// DOES CLEAR THE DATA AREA from end of program to max_code_size
// to be run once before dspRuntime() and at EACH sampling frequency change.
int dspRuntimeInit( opcode_t * codePtr,             // pointer on the dspprogram
                    int maxSize,                    // size of the opcode table available in memory including data area
                    const int fs,                   // sampling frequency currently in use
                    int random) {                   // for tpdf / dithering
    opcode_t* cptr = codePtr;
    dspHeader_t * headerPtr = (dspHeader_t*)codePtr;
    int code = cptr->op.opcode;
    if (code == DSP_HEADER) {
        int freqIndex = dspFindFrequencyIndex(fs);
        if (freqIndex<0) {
            dspprintf("ERROR : sampling frequency not supported.\n");
            return -1; }
        int min = headerPtr->freqMin;
        int max = headerPtr->freqMax;
        if ((freqIndex < min) || (freqIndex > max)) {
             dspprintf("ERROR : supported sampling freq not compatible with dsp program.\n");
             return -2; }
        //printf("INIT ** frequencies : min %d, max %d, num %d, index %d\n",min, max, max-min+1,freqIndex - min);
        dspSamplingFreq = freqIndex;
        dspMinSamplingFreq = min;
        dspMaxSamplingFreq = max;
        dspSamplingFreqIndex = freqIndex - min;
        dspNumSamplingFreq = max - min +1;
        dspBiquadFreqSkip = 2+6*dspNumSamplingFreq;   // 3 words for filter user params (type+freq, Q, gain) + 5 coef alligned per biquad
        dspBiquadFreqOffset = 5+6*dspSamplingFreqIndex; // skip also the 1+1+3 first words
        dspDelayLineFactor = dspTableDelayFactor[freqIndex];
        dspRmsFactorFS = dspTableRmsFactor[freqIndex];

        unsigned sum ;
        int numCores ;
        dspCalcSumCore(codePtr, &sum, &numCores);
        if (numCores < 1) {
            dspprintf("ERROR : no cores defined in the program.\n");
            return -3; }
        if (sum != headerPtr->checkSum) {
            dspprintf("ERROR : checksum problem with the program.\n");
            return -4; }
        if (headerPtr->maxOpcode >= DSP_MAX_OPCODE) {
            dspprintf("ERROR : some opcodes in the program are not supported in this runtime version.\n");
            return -5; }
        int length = headerPtr->totalLength;    // lenght of the program
        int size = headerPtr->dataSize;         // size of the data needed
        if ((size+length) > maxSize){
            dspprintf("ERROR : total size (program+data = %d) is over the allowed size (%d).\n",length+size, maxSize);
            return -6; }
        // now clear the data area, just after the program area
        int * intPtr = (int*)codePtr + length;  // point on data space
        for (int i = 0; i < size; i++) *(intPtr+i) = 0;
        dspTpdfRandomInit(random);
        return length;  // ok
    } else {
        dspprintf("ERROR : no header in dsp program.\n");
        return -1; }
}

// write a buffer in a param space TODO
int dspDataWrite(int offset, int length, int * source){
return 0;
}



// dsp interpreter implementation
int DSP_RUNTIME_FORMAT(dspRuntime)( opcode_t * ptr,         // pointer on the coree to be executed
                                    int * rundataPtr,       // pointer on the data area (end of code)
                                    dspSample_t * sampPtr) {// pointer on the working table where the IO samples are available
    dspALU_t ALU2 = 0;
    dspALU_t ALU = 0;
    while (1) {
        int * cptr = (int*)ptr;
        int opcode = ptr->op.opcode;
        int skip = ptr->op.skip;
        dspprintf2("[%2d] <+%3d> : ", opcode, skip);
        cptr++; // will point on the first potential parametr

        switch (opcode) { // efficiently managed with a jump table

        case DSP_END_OF_CODE: {
            dspprintf2("END");
            return 0; }

        case DSP_CORE: {
            dspprintf2("CORE");
#if defined( DSP_SINGLE_CORE ) && ( DSP_SINGLE_CORE == 1 )
            break;
#else
            return 0;
#endif
            }

        case DSP_NOP: {
            dspprintf2("NOP");
            break; }

        case DSP_SWAPXY: {
            dspprintf2("SWAPXY");
            dspALU_t tmp = ALU;
            ALU = ALU2;
            ALU2 = tmp;
            break;}

        case DSP_COPYXY: {
            dspprintf2("COPYY");
            ALU2 = ALU;
            break;}

        case DSP_COPYYX: {
            dspprintf2("COPYY");
            ALU2 = ALU;
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
            int shift = *cptr;
            #if DSP_ALU_INT64
                if (shift >= 0)
                     ALU <<= shift;
                else ALU >>= -shift;
            #else // float
                long long mul = 1;
                if (shift >= 0) {
                    mul <<= shift;
                    ALU *= mul;
                } else {
                    mul <<= -shift;
                    ALU /= mul; }
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
            #if DSP_ALU_INT64
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
            #else   // float
                ALU = sqrt(ALU);
            #endif
            break;}


        case DSP_SAT0DB: {
            dspprintf2("SAT0DB");
            #if DSP_ALU_INT64
                dspSaturate64_031( &ALU );
            #else // ALU float
                dspSaturateFloat( &ALU );
            #endif
            break;}


        case DSP_SAT0DB_TPDF: {
            dspprintf2("SATURATE_TPDF");
            #if DSP_ALU_INT64
                ALU += dspTpdf.scaled;
                ALU &= dspTpdf.notMask;
                dspSaturate64_031( &ALU );
            #else // ALU float
                ALU += DSP_F31(dspTpdf.valueInt32);
                dspSaturateFloat( &ALU );
            #endif
            break;}


        case DSP_SAT0DB_GAIN: {
            dspprintf2("SAT0DB_GAIN");
            dspParam_t gain = *((dspParam_t*)ptr+*cptr);
            #if DSP_ALU_INT64
                dspShiftMant( &ALU );   // reduce precision by removing 28 bits to make size for coming multiply
                ALU *= gain;
                dspSaturate64_031( &ALU );
            #else // ALU float
                ALU *= gain;
                dspSaturateFloat( &ALU );
            #endif
            break;}


        case DSP_SAT0DB_TPDF_GAIN: {
            dspprintf2("SAT0DB_TPDF_GAIN");
            dspParam_t gain = *((dspParam_t*)ptr+*cptr);
            #if DSP_ALU_INT64
                dspShiftMant( &ALU );
                ALU *= gain;
                ALU += dspTpdf.scaled;
                ALU &= dspTpdf.notMask;
                dspSaturate64_031( &ALU );
            #else // DSP_SAMPLE_FLOAT
                ALU *= gain;
                ALU += DSP_F31(dspTpdf.valueInt32);
                dspSaturateFloat( &ALU );
            #endif
            break; }


        case DSP_TPDF: {
            dspprintf2("TPDF");
            asm("#dsptpdf:");
            if (dspTpdf.factor==0) {        // set to 0 by dspRuntimeInit
                asm("#dsptpdf0:");          // 12 instructions on XMOS XS2A
                dspTpdf.factor = *((int*)(ptr+1));
                dspTpdf.notMask = *((long long *)(ptr+2));
                dspTpdf.round   = *((long long *)(ptr+4));  // this is alligned 8 by encoder
                dspTpdf.shift  = *((int*)(ptr+6));
                break; }
            #if DSP_ALU_INT64
                ALU = dspTpdfRandomCalc();  // 19 instructions on XMOS XS2A
            #else // ALU is float
                ALU = DSP_F31(dspTpdfRandomCalc());
            #endif
            break;}


        case DSP_LOAD: {    // load a RAW sample direcly in the ALU as a 8.56 value
            ALU2 = ALU;     // save ALU in Y in case someone want to use it later
            int index = *cptr;
            dspprintf2("LOAD input[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64
                ALU = *samplePtr;
            #elif DSP_SAMPLE_INT
                ALU = DSP_F31(*samplePtr);      // convert sample to a float number
            #else // sample is float
                ALU = *samplePtr;               // no conversion required
            #endif
            break; }


        case DSP_LOAD_GAIN: {
            ALU2 = ALU;     // save ALU in Y in case someone want to use it later
            int index = *cptr++;
            dspSample_t * samplePtr = sampPtr+index;
            dspParam_t gain = *((dspParam_t*)(ptr+*cptr));
            dspprintf2("LOAD input[%d] with gain",index);
            #if DSP_ALU_INT64
                dspmacs64_32_32_0( &ALU, *samplePtr, gain);
            #elif DSP_SAMPLE_INT
                ALU = DSP_F31(*samplePtr) * gain;    // convert sample to a float number and apply gain
            #else // sample is float
                ALU = *samplePtr * gain;           // no conversion required
            #endif
            break; }


        case DSP_STORE: {   // store the ALU, expecting it to be only 31bit saturated -1/+1
            int index = *cptr;
            dspprintf2("STORE output[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64
                *samplePtr = ALU;   // expecting ALU LSB to contain the sample 0.31 already saturaed
            #elif DSP_SAMPLE_INT
                *samplePtr = DSP_Q31(ALU);      // convert to int32
            #else // DSP_SAMPLE_FLOAT
                *samplePtr = ALU;               // no conversion needed
            #endif
            break; }


        case DSP_GAIN:{
            dspParam_t gain = *((dspParam_t*)ptr+*cptr);
            dspprintf2("GAIN");
            ALU *= gain;    // direct gain without precision reduction.
            break;}


        case DSP_VALUE:{
            dspprintf2("VALUE");
            dspParam_t value = *((dspParam_t*)(ptr+*cptr));
            ALU2 = ALU;
            ALU = value;
            break;}


        case DSP_WHITE: {
             dspprintf2("WHITE");
             #if DSP_ALU_INT64
                 ALU = dspTpdf.randomSeed;
             #else
                 ALU = DSP_F31(dspTpdf.randomSeed);       // convert 32bit value to a float number between -1..+1
             #endif
             break;}


          case DSP_MUL_VALUE:{
            dspprintf2("MUL_VALUE");
            dspParam_t value = *((dspParam_t*)(ptr+*cptr));
            ALU *= value;
            break;}


        case DSP_DIV_VALUE:{
            dspprintf2("DIV_VALUE");
            dspParam_t value = *((dspParam_t*)(ptr+*cptr));
            ALU /= value;
            break;}


        case DSP_DELAY_1:{
            int offset = *cptr; //point on the area where we have the sample storage (2 words size)
            int * dataPtr = rundataPtr+offset;
            dspprintf2("DELAY_1");
            dspALU_t * memPtr = (dspALU_t*)dataPtr;   // formal type casting to enable double word instructions
            dspALU_t tmp = *memPtr;
            *memPtr = ALU;
            ALU = tmp;
            break;}


        case DSP_LOAD_STORE: {
            int max = skip-1;   // length of following data in words
            dspprintf2("LOAD_STORE (%d)",max);
            while (max) {
                int index = *cptr++;
                dspSample_t value = *(sampPtr+index);
                index   = *cptr++;
                *(sampPtr+index) = value;
                max -= 2;
            }
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
                dspALU_t * linePtr = (dspALU_t*)(dataPtr+index);
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
            #if DSP_ALU_INT64
            asm("#biquadentry:");
            int sample = dspShiftInt( ALU, DSP_MANTBQ );    //remove the size of the biquad, as the result will be scaled accordingly
            #endif
            dspSample_t * dataPtr = (dspSample_t*)(rundataPtr + *cptr++);
            int * numPtr = (int*)(ptr + *cptr);    // point on the number of sections
            dspParam_t * coefPtr = (dspParam_t*)(numPtr+dspBiquadFreqOffset); //point on the right coefficient according to fs
            int num = *numPtr++;    // number of sections in 16 lsb, biquad routine should keep only 16bits lsb
            if (*numPtr) {  // verify if biquad is in bypass mode or no
            #if DSP_ALU_INT64
                // ALU is expected to contain the sample in 8.56 (typically after DSP_LOAD_GAIN)
                #ifndef DSP_ARCH
                ALU = dsp_calc_biquads_int( sample, coefPtr, dataPtr, num, DSP_MANTBQ, dspBiquadFreqSkip);
                #elif DSP_XS2A
                ALU = dsp_biquads_xs2( sample , coefPtr, dataPtr, num);
                #endif
            #else // DSP_ALU_FLOAT
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
                dspParam_t gain   = *((dspParam_t*)tablePtr++);
                #if DSP_ALU_INT64
                    dspmacs64_32_32( &ALU, sample, gain);
                #elif DSP_SAMPLE_INT
                    ALU += DSP_F31(sample) * gain;    // convert sample to a float number and apply gain
                #else // sample is float
                    ALU += sample * gain;           // no conversion required
                #endif
                max--;
            }
            break; }


        case DSP_DATA_TABLE: {
            dspprintf2("DATA_TABLE");
            dspParam_t gain = *cptr++;  // this is 4.28 format (or float)
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
            #if DSP_ALU_INT64
                dspmacs64_32_32_0( &ALU, data, gain);
            #elif DSP_SAMPLE_INT
                ALU = DSP_F31(data) * gain;  // convert sample to a float number and apply gain
            #else // sample is float
                ALU = data * gain;           // no conversion required
            #endif
            break; }


        // WORK IN PROGRESS

        case DSP_FIR: { // quite opaque of course ..
            int numFreq = dspNumSamplingFreq;   // the range of frequencies define the size of the table pointing on impulses
            dspprintf2("FIR \n");
            int freq = dspSamplingFreqIndex;   // this is a delta compared to the minimum supported freq
            int * tablePtr = (int*)cptr+freq;    // point on the offset to be used for the current frequency
            //dspprintf3("tableptr[%d] = 0x%X\n",freq,(int)tablePtr);
            cptr += numFreq;            // skip all the table of ofsset
            int offset = *(tablePtr++);         // get the offset where we have the size of impulse and coef for the current frequency
            if (offset) {
                tablePtr = (int*)ptr+offset;   // now points on the impulse associated to current frequency
                //dspprintf3("fir ImpulsePtr = 0x%X\n",(int)tablePtr);
                int length = *(tablePtr++);
                offset = *cptr;        // offset where are the data for states filter
                dspSample_t * dataPtr = (dspSample_t*)(rundataPtr+offset);
                //dspprintf3("state data @0x%X, length %d\n",(int)dataPtr,length);
                int delay = length >> 16;
                if (delay) {    // simple delay line
                    int index = *dataPtr++; // read position in the dalay line
                    dspALU_SP_t * linePtr = (dspALU_SP_t*)dataPtr+index;
                    dspALU_SP_t value = *linePtr;
                    *linePtr = ALU;
                    ALU = value;
                    index++;
                    if (index >= delay) index = 0;
                    *(--dataPtr) = index;
                } else {
                    if (length > 0) {
                    #if DSP_ALU_INT64
                        dspParam_t * coefPtr = (dspParam_t*)tablePtr;
                        ALU = dsp_calc_fir(ALU, coefPtr, dataPtr, length);
                    #else // float
                        // TODO
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
            #if DSP_ALU_INT64
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

                #if DSP_ALU_INT64
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
#if DSP_ALU_INT64
            int offset = *cptr++;                   // where is the data space for state data calculation
            dspSample_t * dataPtr  = (dspSample_t*)(rundataPtr+offset);
            int freq = dspSamplingFreqIndex;        // 1 pole per freq
            int * tablePtr = (int*)(cptr+freq);       // point on the offset to be used for the current frequency

            int pole = *tablePtr;                 // pole of the integrator
            // structure of the data space preallocated by encoder
            // 1 prevX  (0)
            // 1 prevY  (1)
            // 2 acc    (2)
            // ALU expected to contain 0.31 sample (typically after LOAD or before STORE)
            dspALU_t * accPtr = (dspALU_t*)(dataPtr+2);
            int prevX = *dataPtr;
            int Xn = ALU;
            *dataPtr = Xn;
            Xn -= prevX;
            ALU = *accPtr;
            dspmacs64_32_32( &ALU, Xn, DSP_QNM(1.0) );     // add (Xn-X[n-1]) scaled 28bit up
            int prevY = *(dataPtr+1);
            dspmacs64_32_32( &ALU, prevY, pole);    // add Y[n-1] * pole (pole is negative and small like -0.001)
            prevY = dspShiftInt( ALU, DSP_MANT);    // reduce precision and store Yn
            *(dataPtr+1) = prevY;
            *accPtr = ALU;
            ALU = prevY;
#endif
        break; }

        /* MPD code for dithering
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
            #if DSP_ALU_INT64
                int offset = *cptr++;                   // where is the data space for state data calculation
                dspALU_t * errorPtr  = (dspALU_t*)(rundataPtr+offset);
                dspALU_t temp1 = *(errorPtr+1);
                dspALU_t temp0 = *(errorPtr+0);
                ALU += temp0;
                *(errorPtr+1) = temp0/2;
                ALU -= temp1;
                ALU += *(errorPtr+2);
                *(errorPtr+2) = temp1;
                dspALU_t sample = ALU;
                ALU += dspTpdf.scaled;      // includes rounding
                ALU &= dspTpdf.notMask;     // truncate
                *(errorPtr+0) = sample - ALU;
            #else // ALU is float
                // TODO
            #endif
        break; }


        case DSP_DITHER_NS2: {
            #if DSP_ALU_INT64
                int offset = *cptr++;                       // where is the data space for state data calculation
                dspSample_t * errorPtr  = (dspSample_t*)(rundataPtr+offset);
                offset = *cptr;
                int freq = dspSamplingFreqIndex * 3;
                int * tablePtr = (int*)(ptr+offset+freq);   // point on the offset to be used for the current frequency
                int  coef0 = *tablePtr++;                   // eg 1.0
                int  coef1 = *tablePtr++;                   // eg -0.5
                int  coef2 = *tablePtr;                     // eg 0.5
                dspSample_t err0 = *(errorPtr+0);
                dspSample_t err1 = *(errorPtr+1);
                dspSample_t err2 = *(errorPtr+2);
                dspmacs64_32_32(&ALU, err0, coef0);
                dspmacs64_32_32(&ALU, err1, coef1);
                dspmacs64_32_32(&ALU, err2, coef2);
                *(errorPtr+1) = err0;
                *(errorPtr+2) = err1;
                dspALU_t sample = ALU;
                ALU += dspTpdf.scaled;      // includes rounding
                ALU &= dspTpdf.notMask;     // truncate
                sample -= ALU;              // compute error
                *(errorPtr+0) = dspShiftInt(sample,DSP_MANT);
            #else // ALU is float
                // TODO
            #endif
        break; }


        case DSP_DISTRIB:{
            int size = *cptr++;     // get size of the array, this factor is also used to scale the ALU
            int offset = *cptr;
            int * dataPtr = (int*) rundataPtr+offset;   // where we have a data space for us
            int index = *dataPtr++; // get position in the table for outputing the value as if it was a clean sample.
            int pos = dspmuls32_32_32(ALU, size); // our sample is now between say -256..+255 for size = 512
            pos += size/2;      // our array is 0..511
            (*(dataPtr+pos))++;   // one more sample counted
            ALU = *(dataPtr+index);
            index++;
            if (index >= size) index = 0;
            *--dataPtr = index;
        break;}

        } // switch (opcode)

        ptr += skip;    // move on to next opcode
        dspprintf2("\n");
        } //while 1

} // while(1)

