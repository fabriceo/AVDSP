/*
 * dsp_header.c
 *
 *  Created on: 11 oct. 2023
 *      Author: Fabrice
 */

#include "dsp_header.h"

#if defined(DSP_PRINTF)
int dspPrintfVal = DSP_PRINTF;
#else
int dspPrintfVal = 0;
#endif

//used in both dsp_encoder.c and dsp_runtime

#if defined(DSP_PRINTF)
const char * dspOpcodeText_[DSP_LAST_OPCODE] = {
"DSP_END_OF_CODE" ,
"DSP_HEADER" ,
"DSP_PARAM" ,
"DSP_PARAM_NUM" ,
"DSP_NOP" ,
"DSP_CORE" ,
"DSP_SECTION" ,
"DSP_LOAD" ,
"DSP_STORE" ,
"DSP_LOAD_STORE" ,
"DSP_STORE_TPDF" ,
"DSP_STORE_GAIN" ,
"DSP_LOAD_GAIN" ,
"DSP_LOAD_MUX" ,
"DSP_MIXER" ,
"DSP_LOAD_X_MEM" ,
"DSP_STORE_X_MEM" ,
"DSP_LOAD_Y_MEM" ,
"DSP_STORE_Y_MEM" ,
"DSP_LOAD_MEM_DATA" ,
"DSP_CLRXY" ,
"DSP_SWAPXY" ,
"DSP_COPYXY" ,
"DSP_COPYYX" ,
"DSP_ADDXY" ,
"DSP_ADDYX" ,
"DSP_SUBXY" ,
"DSP_SUBYX" ,
"DSP_MULXY" ,
"DSP_MULYX" ,
"DSP_DIVXY" ,
"DSP_DIVYX" ,
"DSP_AVGXY" ,
"DSP_AVGYX" ,
"DSP_NEGX" ,
"DSP_NEGY" ,
"DSP_SHIFT" ,
"DSP_VALUEX" ,
"DSP_VALUEY" ,
"DSP_GAIN" ,     //39
"DSP_CLIP" ,
"DSP_SAT0DB" ,   //41
"DSP_SAT0DB_VOL" ,
"DSP_STORE_VOL" ,
"DSP_SAT0DB_GAIN" ,
"DSP_STORE_VOL_SAT" ,
"DSP_SERIAL" ,
"DSP_DELAY_1" ,  //47
"DSP_DELAY" ,
"DSP_DELAY_DP" ,
"DSP_BIQUADS" ,
"DSP_DCBLOCK" ,
"DSP_DATA_TABLE" ,
"DSP_TPDF_CALC" ,
"DSP_TPDF" ,
"DSP_WHITE" ,
"DSP_DITHER" ,
"DSP_DITHER_NS2" ,
"DSP_DISTRIB" ,
"DSP_DIRAC" ,
"DSP_SQUAREWAVE" ,
"DSP_SINE" ,
"DSP_SQRTX" ,
"DSP_RMS" ,
"DSP_FIR" ,
"DSP_WFIR" ,
"DSP_DELAY_FB_MIX" ,
"DSP_INTEGRATOR" ,
"DSP_CICUS" ,
"DSP_CICN" ,
"DSP_EXPMA" ,
"DSP_THDCOMP" ,
"DSP_BIQUAD_FS" ,
"DSP_BIQUAD_FS_FAST" ,
"DSP_BIQUAD_FS_FAST8" ,
"DSP_SEND" ,
"DSP_RECEIVE" ,
"DSP_DRC_ENV_PEAK" ,
"DSP_DRC_ENV_RMS" ,
"DSP_DRC_LIM_PEAK" ,
"DSP_DRC_LIM_RMS" ,
"DSP_DRC_LIM_PEAK_CLIP" ,
"DSP_DRC_COMPRESSOR" ,
"DSP_DRC_EXPANDER" ,
"DSP_DRC_NOISE_GATE" ,
"DSP_MEMCLR" ,
"DSP_SWAPMEM" ,
"DSP_ADDMEM" ,
"DSP_MEMADD" ,
"DSP_SUBMEM" ,
"DSP_MEMSUB" ,
"DSP_MULMEM" ,
"DSP_DIVMEM" ,
"DSP_AVGMEM" ,
"DSP_MEMAVG" ,
"DSP_NEGMEM" ,
"DSP_MEMSAVE" ,
"DSP_LOADMEM" ,
"DSP_MEMSAVEXY" ,
"DSP_LOADMEMXY" ,
"DSP_MEMVALUE" ,
"DSP_MIXERMEM" ,
"DSP_MEMGAIN" ,
"DSP_MEMINPUT" ,
"DSP_CORE_EXTERN" ,
"DSP_FULL_LOAD" ,
"DSP_PRIO_ON" ,
"DSP_PRIO_OFF" ,
"DSP_TRANSFER2" ,
"DSP_TRANSFER8" ,
"DSP_LOADY" ,
"DSP_LOADXY" ,
"DSP_STOREY" ,
"DSP_STOREXY" ,
"DSP_GAINY" ,
"DSP_GAINXY" ,
"DSP_GAIN_X_Y" ,
"DSP_DELAYY" ,
"DSP_SAT0DBXY" ,
"DSP_SAT0DBXY_VOL" ,
"DSP_BIQUADSY" ,
"DSP_BIQUADSY_FS" ,
"DSP_BIQUADSXY" ,
"DSP_BIQUADSXY_FS" ,
"DSP_DOWNSAMPLE" ,
"DSP_UPSAMPLE" ,
"DSP_FLOAD" ,
"DSP_FSTORE" ,
"DSP_FVALUEX" ,
"DSP_FGAIN" ,
"DSP_FBIQUADS" ,
};
#else
const char * dspOpcodeText_[1] = { "" };
#endif


long long dspQNM(double x, int n, int m){
    return DSP_QNM(x,n,m);
}

long long dspQM64(double x, int m) {
    return DSP_QM64(x,m);
}

int dspQM32(double x, int m){
    return DSP_QM32(x,m);
}

void dspCalcSumCore(opcode_t * ptr, unsigned int * sum, int * numCore, unsigned int maxcode){
    XCunsafe {
    *sum = 0;
    *numCore = 0;
    while(1){
        enum dspOpcodesEnum code = ptr->op.opcode;
        int skip = ptr->op.skip;
        if ( (code == DSP_END_OF_CODE) || (skip == 0) ){
            if (*numCore == 0) *numCore = 1;
            break;   // end of program encountered
        }
        if (code == DSP_CORE) (*numCore)++;
        if ( ( *numCore == 0 ) &&   //any first opcode will generate a core
                (code != DSP_HEADER) &&
                (code != DSP_NOP) &&
                (code != DSP_CORE_EXTERN) &&
                (code != DSP_PARAM) &&
                (code != DSP_PARAM_NUM) )  *numCore = 1;
        *sum += ptr->u32;
        ptr += skip;
    } // while(1)
} }

