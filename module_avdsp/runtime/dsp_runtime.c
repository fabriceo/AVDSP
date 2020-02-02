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

int dspMinSamplingFreq = DSP_DEFAULT_MIN_FREQ;
int dspMaxSamplingFreq = DSP_DEFAULT_MAX_FREQ;
int dspNumSamplingFreq;
int dspSamplingFreqIndex;
int dspDelayLineFactor;
int dspBiquadFreqSkip;
int dspBiquadFreqOffset;



opcode_t * dspFindCore(opcode_t * ptr, const int numCore){  // search core and return begining of core code
    int num = 0;
    while (1) {
        short code = ptr->op.opcode;
        short skip = ptr->op.skip;
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
        short code = ptr->op.opcode;
        short skip = ptr->op.skip;
        //printf("2ptr @0x%X = %d-%d\n",(int)ptr,code, skip);
        if (skip == 0) return 0; // = END_OF_CORE
        if ( (code == DSP_NOP) ||
             (code == DSP_PARAM) ||
             (code == DSP_PARAM_NUM) ||
             (code == DSP_SERIAL) )  // skip any data or serial statement if at begining of code
            ptr += skip;
         else
             if ((code == DSP_END_OF_CODE)||(code == DSP_CORE)) return 0;
             else
                 break;    // seems the code is now a valid code to process
    }
    return ptr;
}


#if 0   // not really needed
static inline float dsp64QNM_tofloat(long long a, int mant){ // a is like 8.56 and mant is 56
    a >>= (mant/2); // convert 8.56 to say 4.28
    return ((float)(a)/(float)((long long)1<<(mant)));
}

static inline double dsp64QNM_todouble(long long a, int mant){ // a is like 8.56 and mant is 56
    return ((double)(a)/(double)((long long)1<<mant));
}
#endif


static const int dspTableFreq[FMAXpos] = {
        8000, 16000,
        24000, 32000,
        44100, 48000,
        88200, 96000,
        176400,192000,
        352800,384000,
        705600, 768000 };

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

static inline int dspFindFrequencyIndex(int freq){
    for (int i=0; i<FMAXpos; i++)
        if (freq == dspTableFreq[i]) return i;
    return 0;
}



// make the basic sanity check, predefined some FS related variable  and clear the data area
// to be run once before dspRuntime() and at EACH frequency change.
int dspRuntimeInit( opcode_t * codePtr,             // pointer on the dspprogram
                    int maxSize,                    // size of the opcode table available in memory
                    const int fs){                  // sampling frequency currently in use
    opcode_t* cptr = codePtr;
    dspHeader_t * headerPtr = (dspHeader_t*)codePtr;
    short code = cptr->op.opcode;
    if (code == DSP_HEADER) {
        int freqIndex = dspFindFrequencyIndex(fs); // no check of coherence, up to the programer
        int min = headerPtr->freqMin;
        int max = headerPtr->freqMax;
        if ((freqIndex < min) || (freqIndex > max)) {
             dspprintf("ERROR : supported sampling freq not compatible with program.\n");
             return -2;
        }
        //printf("INIT ** frequencies : min %d, max %d, num %d, index %d\n",min, max, max-min+1,freqIndex - min);
        dspMinSamplingFreq = min;
        dspMaxSamplingFreq = max;
        dspSamplingFreqIndex = freqIndex - min;
        dspNumSamplingFreq = max - min +1;
        dspBiquadFreqSkip = 8*dspNumSamplingFreq;   // 3 words for filter user params (type+freq, Q, gain) + 5 coef per biquad
        dspBiquadFreqOffset = 8*dspSamplingFreqIndex+3; // skip also the 3 first words
        dspDelayLineFactor = dspTableDelayFactor[freqIndex];

        unsigned sum ;
        int numCores ;
        dspCalcSumCore(codePtr, &sum, &numCores);
        if (numCores < 1) {
            dspprintf("ERROR : no cores defined in the program.\n");
            return -3;
        }
        if (sum != headerPtr->checkSum) {
            dspprintf("ERROR : checksum problem with the program.\n");
            return -4;
        }
        if (headerPtr->maxOpcode >= DSP_MAX_OPCODE) {
            dspprintf("ERROR : some opcodes in the program are not supported in this runtime version.\n");
            return -5;
        }
        int length = headerPtr->totalLength;    // lenght of the program
        int size = headerPtr->dataSize;         // size of the data needed
        if ((size+length) > maxSize){
            dspprintf("ERROR : total size (program+data = %d) is over the allowed size (%d).\n",length+size, maxSize);
            return -6;
        }
        int * intPtr = (int*)codePtr + length;  // point on data
        for (int i = 0; i < size; i++) *(intPtr+i) = 0;
        return length;  // ok
    } else {
        dspprintf("ERROR : no header in dsp program.\n");
        return -1;
    }
}

// write a buffer in a param space
int dspDataWrite(int offset, int length, int * source){
return 0;
}



// dsp interpreter implementation
int DSP_RUNTIME_FORMAT(dspRuntime)( opcode_t * ptr,         // pointer on the coree to be executed
                                    int * rundataPtr,       // pointer on the data area (end of code)
                                    dspSample_t * sampPtr) {// pointer on the working table where the IO samples are available

    //int random1; int random2;   // place holder for random number required for TPDF. 1 couple is generated at each FS and used for all program

    dspALU_t ALU2 = 0;
    dspALU_t ALU = 0;
    while (1) {
        int * cptr = (int*)ptr;
        short opcode = ptr->op.opcode;
        short skip = ptr->op.skip;
        dspprintf2("[%2d] <+%3d> : ", opcode, skip);
        cptr++;
        switch (opcode) {   // read opcode

        case DSP_END_OF_CODE:   // tested
        case DSP_CORE:
            dspprintf2("END");
            return 0;

        case DSP_NOP:
            dspprintf2("NOP");
            break;

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

        case DSP_CLRXY: { // tested
            dspprintf2("CLRXY");
            ALU2 = 0;
            ALU = 0;
            break;}

        case DSP_ADDXY: {
            dspprintf2("ADDXY");
            ALU += ALU2;
            break;}

        case DSP_SUBXY: {
            dspprintf2("SUBXY");
            ALU -= ALU2;
            break;}

        case DSP_MULXY: {
            dspprintf2("MULXY");
            #if DSP_ALU_INT64
                dspmul64_64QNM( &ALU, &ALU2, DSP_MANTDP);
            #else   // float
                ALU *= ALU2;
            #endif
            break;}

        case DSP_DIVXY: {
            dspprintf2("DIVXY");
            #if DSP_ALU_INT64
                dspdiv64_64QNM( &ALU, &ALU2, DSP_MANTDP);
                dspprintf3("ALU = %f\n",FDP28(ALU));
            #else   // float
                ALU /= ALU2;
            #endif
            break;}

        case DSP_SQRTX: {  // TODO
            dspprintf2("SQRTX");
            #if DSP_ALU_INT64
                dspsqrt64QNM( &ALU, DSP_MANTDP); // sqrt(8.56) gives 4.28, converted back to 8.56
            #else   // float
                //ALU = sqrt(ALU);
            #endif
            break;}

        case DSP_SAT0DB: {  // TODO
            dspprintf2("SAT0DB");
            #if DSP_ALU_INT64
                ALU = dsp64QNM_to031(ALU, DSP_MANTDP);
            #else   // float
                // TODO
            #endif
            break;}


        case DSP_TPDF: {  // TODO
            int tpdf = *cptr;
            dspprintf2("TPDF %d bits",tpdf);
            ALU += 0;
            break;}


        case DSP_LOAD: {
            ALU2 = ALU;     // save ALU in Y in case someone want to use it later
            int index = *cptr;
            dspprintf2("LOAD input[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64
                ALU = (unsigned)(*samplePtr);   // we dont want to sign extend here to force ALU msb == 0
            #elif DSP_SAMPLE_INT
                ALU = DSP_F31(*samplePtr);    // convert sample to a float number
            #else //if DSP_SAMPLE_FLOAT
                ALU = *samplePtr;           // no conversion required
            #endif
            break; }

        case DSP_STORE: {
            int index = *cptr;
            dspprintf2("STORE output[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64           // convert 8.56 to int32
                *samplePtr = ALU;//dsp64QNM_check031( ALU, DSP_MANTDP);
            #elif DSP_SAMPLE_INT
                *samplePtr = DSP_Q31(ALU);      // convert to int32
            #else // DSP_SAMPLE_FLOAT
                *samplePtr = ALU;               // no conversion needed
            #endif
            break; }

        case DSP_STORE_TPDF: {  // TODO
            int index = *cptr++;
            int tpdf = *cptr;
            dspprintf2("STORE_TPDF output[%d] %d bits",index, tpdf);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64           // convert 8.56 to int32
                *samplePtr = ALU;//dsp64QNM_check031( ALU, DSP_MANTDP);
            #elif DSP_SAMPLE_INT
                *samplePtr = DSP_Q31(ALU);      // convert to int32
            #else // DSP_SAMPLE_FLOAT
                *samplePtr = ALU;               // no conversion needed
            #endif
            break; }


        case DSP_LOAD_DP: {    // tested
            ALU2 = ALU;     // save ALU in Y in case someone want to use it later
            int index = *cptr;
            dspprintf2("LOAD_DP input[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64
                ALU = dsp031_to64QNM( *samplePtr , DSP_MANTDP);   // sample are considered 0.31 first bit being sign and others mantissa so -1..+1
            #elif DSP_SAMPLE_INT
                ALU = DSP_F31(*samplePtr);      // convert sample to a float number
            #else //if DSP_SAMPLE_FLOAT
                ALU = *samplePtr;           // no conversion required
            #endif
            break; }

        case DSP_STORE_DP: {   // tested
            int index = *cptr;
            dspprintf2("STORE_DP output[%d]",index);
            dspSample_t * samplePtr = sampPtr+index;
            #if DSP_ALU_INT64           // convert 8.56 to int32
                *samplePtr = dsp64QNM_to031( ALU , DSP_MANTDP);
            #elif DSP_SAMPLE_INT
                *samplePtr = DSP_Q31(ALU);      // convert to int32
            #else // DSP_SAMPLE_FLOAT
                *samplePtr = ALU;               // no conversion needed
            #endif
            break; }

        case DSP_GAIN:{
            int offset = *cptr;
            dspParam_t * gainPtr = (dspParam_t*)ptr+offset;
            dspParam_t gain = *gainPtr;
            dspprintf2("GAIN");
            #if DSP_ALU_INT64   // check if double precision has been used
                if (ALU>>32) ALU = dspmul64_32QNM( ALU, gain, DSP_MANT);
                else ALU = dspmul031_32QNM( ALU, gain, DSP_MANT, DSP_MANTDP);
                if ((ALU>>32)==0) ALU = (long long)1<<32;  // minimum DP value
            #else // DSP_ALU_FLOAT
                ALU *= gain;
            #endif
            break;}

        case DSP_GAIN0DB:{
            int offset = *cptr;
            dspParam_t * gainPtr = (dspParam_t*)ptr+offset;
            dspParam_t gain = *gainPtr;
            dspprintf2("GAIN0DB");
            #if DSP_ALU_INT64
                if (ALU>>32) ALU = dspmul64_32QNM( ALU, gain, 31);
                else ALU = dspmul031_32QNM( ALU, gain, 31, DSP_MANTDP);
                if ((ALU>>32)==0) ALU = (long long)1<<32;  // minimum DP value
            #else // DSP_ALU_FLOAT
                ALU *= gain;
            #endif
            break;}

        case DSP_DELAY_1:{
            int offset = *cptr; //point on the area where we have the sample storage
            int * dataPtr = rundataPtr+offset;
            dspprintf2("DELAY_1 @%d", offset);
            dspALU_t * memPtr = (dspALU_t*)dataPtr;   // formal type casting to enable double word instructions
            dspALU_t tmp = *memPtr;
            *memPtr = ALU;
            ALU = tmp;
            break;}

        case DSP_LOAD_STORE: {
            int max = skip-1;   // length in words
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
            int * offsetPtr = (int*)ptr+offset;
            dspprintf2("LOAD_MEM @%d ",offset);
            dspALU_t * memPtr = (dspALU_t*)offsetPtr;
            ALU = *memPtr;
            break; }

        case DSP_STORE_MEM:{
            int offset = *cptr;    //point on the area where we have the memory location in 32 or 64 bits
            int * offsetPtr = (int*)ptr+offset;
            dspprintf2("STORE_MEM @%d ",offset);
            dspALU_t * memPtr = (dspALU_t*)offsetPtr;
            *memPtr = ALU;
            break; }

        case DSP_DELAY: {   // TODO all bad
            dspprintf2("DELAY ...");    // TODO
            unsigned  maxSize = *cptr++;                // maximum size in number of samples, OR delay in uSec
            int offset  = *cptr++;                      // adress of data table for storage
            int * dataPtr = rundataPtr+offset;          // now points on the delay line data, starting with current index
            unsigned short microSec;
            unsigned nSamples;
            offset = *cptr;                             // get the offset where we can find the delay in microsseconds
            if (offset == 0) {                          // 0 means it is a fixed delay already defined in maxSize
                nSamples = dspmulu32_32_32(maxSize, dspDelayLineFactor);    // microSec was stored in maxSize by design for DELA_Fixed
            } else {
                int * microSecPtr = (int*)ptr+offset;   // where is stored the delay
                microSec = *microSecPtr;                // read microsec
                nSamples = dspmulu32_32_32(microSec, dspDelayLineFactor);
                if (nSamples > maxSize) nSamples = maxSize; // security check as the host aplication may have put an higher data...
            }
            int index = *dataPtr++;
            dspALU_SP_t * linePtr = (dspALU_SP_t*)dataPtr+index;
            #if DSP_ALU_INT64
                if(ALU >> 32){  // check if ALU is 64QNM
                    dspALU_t value = dsp031_to64QNM( *linePtr, DSP_MANTDP );
                    *linePtr = dsp64QNM_to031( ALU, DSP_MANTDP );
                    if ((value >> 32)==0) ALU = (long long)1<<32;  // minimum DP value
                    else ALU = value;
                } else {
                    dspALU_SP_t value = *linePtr;
                    *linePtr = ALU;
                    ALU = (unsigned)value;  // we dont want to sign-extend, to keep msb=0;
                }
            #elif DSP_ALU_FLOAT
                dspALU_SP_t value = *linePtr;
                *linePtr = ALU;
                ALU = value;
            #endif
            index++;
            if (index >= nSamples) index = 0;
            *(--dataPtr) = index;
            break;}

        case DSP_DELAY_DP: {   // TODO all bad
            dspprintf2("DELAY ...");    // TODO
            unsigned maxSize = *cptr++;                      // maximum size in number of samples, OR delay in uSec
            int offset  = *cptr++;                      // adress of data table for storage
            int * dataPtr = rundataPtr+offset;        // now points on the delay line data, starting with current index
            unsigned microSec;
            unsigned nSamples;
            offset = *cptr;                             // get the offset where we can find the delay in microsseconds
            if (offset == 0) {
                nSamples = dspmulu32_32_32(maxSize, dspDelayLineFactor);    // microSec was stored in maxSize by design for DELA_Fixed
            } else {
                int * microSecPtr = (int*)ptr+offset;
                microSec = *microSecPtr & 0xFFFF;                // read microsec
                nSamples = dspmulu32_32_32(microSec, dspDelayLineFactor);
                if (nSamples > maxSize) nSamples = maxSize;
            }
            int index = *dataPtr++;
            dspALU_t * linePtr = (dspALU_t*)dataPtr+index;
            #if DSP_ALU_INT64
                dspALU_t value = *linePtr;
                *linePtr = ALU;
                ALU = value;
            #elif DSP_ALU_FLOAT
                dspALU_SP_t value = *linePtr;
                *linePtr = ALU;
                ALU = value;
            #endif
            index++;
            if (index >= nSamples) index = 0;
            *(--dataPtr) = index;
            break;}

        case DSP_BIQUADS: {
            dspprintf2("BIQUAD ");
            int offset = *cptr++;  // pointer on the array of state data accumulated
            dspSample_t * dataPtr = (dspSample_t*)rundataPtr+offset;
            offset = *cptr;   // pointer on the array of coeficints
            int * numPtr = (int*)ptr+offset;    // point on the number of sections
            unsigned short num = *numPtr++ ;
            dspprintf2("%d sections\n",num);
            dspParam_t * coefPtr = (dspParam_t*)numPtr+dspBiquadFreqOffset;
            #if DSP_ALU_INT64
                // ALU is expected to contain the sample in 0.31, otherwise convertion happens to reduce DP to 0.31
                #if 0//  defined(__XS2A__)
                    // assembly routine returns a 32 bit saturated value
                    //ALU = (unsigned)dsp_filters_biquads( dsp64QNM_to031check(ALU, DSP_MANTDP) , coefPtr, dataPtr, num, DSP_MANTBQ, dspBiquadFreqSkip);
                    ALU = (unsigned)bq( dsp64QNM_to031check(ALU, DSP_MANTDP) , coefPtr, dataPtr, num, DSP_MANTBQ, dspBiquadFreqSkip);
                #else
                    ALU = dsp_calc_biquads(ALU, coefPtr, dataPtr, num, DSP_MANTBQ, dspBiquadFreqSkip);
                #endif
            #else // DSP_ALU_FLOAT
                ALU = dsp_calc_biquads_float(ALU, coefPtr, dataPtr, num,dspBiquadFreqSkip);
            #endif
            break; }

        case DSP_PARAM: {
            dspprintf2("PARAM");
            break; }

        case DSP_PARAM_NUM: {
            dspprintf2("PARAM_NUM ...");    // TODO
            break; }

        case DSP_SERIAL:{    //
            //int serial= *cptr++;  // supposed to be the serial number of the product, or a specif magic code TBD
            //dspprintf2("SERIAL %d",serial);
            //int check = *cptr++;
            // serial check to do here
            break; }


        case DSP_MUX0DB:{
            dspprintf2("MUX0DB");
            int offset = *cptr;
            int * tablePtr = (int*)ptr+offset;
            int max = (*tablePtr++) & 0xFFFF;   //number of sections
            ALU = 0;
            while (max)  {
                int index = *tablePtr++;
                dspSample_t sample = *(sampPtr+index);
                dspParam_t gain   = (dspParam_t)(*tablePtr++);
                #if DSP_ALU_INT64
                    ALU += dspmul031_32QNM( sample, gain, 31, DSP_MANTDP);
                #else // DSP_ALU_FLOAT
                    ALU += sample * gain;
                #endif
                max--;
            }
            break; }

        case DSP_MACC: {
            dspprintf2("MACC");
            int offset = *cptr;
            int * tablePtr = (int*)ptr+offset;
            int max = (*tablePtr++) & 0xFFFF;
            while (max)  {
                int index = *tablePtr++;
                dspSample_t sample = *(sampPtr+index);
                dspParam_t gain   = (dspParam_t)(*tablePtr++);
                #if DSP_ALU_INT64 // 8.56 * 4.28 = 12.84 => 12.56 (83-56=)
                    ALU += dspmul031_32QNM( sample, gain, DSP_MANT, DSP_MANTDP);
                #else // DSP_ALU_FLOAT
                    ALU += sample * gain;
                #endif
                max--;
            }
            break; }

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
                dspSample_t * dataPtr = (dspSample_t*)rundataPtr+offset;
                //dspprintf3("state data @0x%X, length %d\n",(int)dataPtr,length);
                int delay = length>>16;
                if (delay) {    // simple delay line
                    int index = *dataPtr++; // read position in the dalay line
                    dspALU_SP_t * linePtr = (dspALU_SP_t*)dataPtr+index;
                    #if DSP_ALU_INT64
                        dspALU_SP_t value = dsp32QNM_to64( *linePtr, DSP_MANT );
                        *linePtr = dspconv64_32QNM( ALU, DSP_MANT );
                        ALU = value;
                    #elif DSP_ALU_FLOAT
                        dspALU_SP_t value = *linePtr;
                        *linePtr = ALU;
                        ALU = value;
                    #endif
                    index++;
                    if (index >= delay) index = 0;
                    *(--dataPtr) = index;
                } else {
                    dspParam_t * coefPtr = (dspParam_t*)tablePtr;
                    if (length > 0) {
                    #if DSP_ALU_INT64
                        ALU = dsp_calc_fir(ALU, coefPtr, dataPtr, length);
                    #else // float
                    #endif
                    }
                }
            }
            break; }

        case DSP_DATA_TABLE: {  // Index should be store in rundata TODO
            dspprintf2("DATA_TABLE");
            //int offset=*cptr++;      // get adress ofset for data
            int gain = *cptr++;      // 0_31 format and 0x7FFFFFFF = 1
            int div =  *cptr++;      // frequency divider
            int size=  *cptr++;      // table size
            int index = *cptr++;
            int *indexPtr = rundataPtr+index;
            int offset= *cptr;        // current position of the data in the table
            int * dataPtr = (int*)ptr+offset;
            index = *indexPtr;
            ALU = *(dataPtr+index);
            index += div;
            if (index >= size) index -= size;
            *indexPtr = index;
            #if DSP_ALU_INT64 // 8.56 * 4. 28 = 12.84 converted back in 8.56
                ALU = dspmul64_32QNM( ALU, gain, DSP_MANT);
            #else // DSP_ALU_FLOAT
                ALU *= gain;
            #endif
            break; }

        } // switch (opcode)

        ptr += skip;    // move on to next opcode
        dspprintf2("\n");
        } //while 1

} // while(1)


