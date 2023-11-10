/*
 * dsp_header.c
 *
 *  Created on: 11 oct. 2023
 *      Author: Fabrice
 */
#include "dsp_header.h"

//used in both dsp_encoder.c and dsp_runtime
const char * dspOpcodeText[DSP_MAX_OPCODE] = {
    "DSP_END_OF_CODE",
    "\nDSP_HEADER",
    "DSP_NOP",
    "\nDSP_CORE",
    "\nDSP_PARAM",
    "\nDSP_PARAM_NUM",
    "DSP_SERIAL",
    "DSP_TPDF_CALC",
    "DSP_TPDF",
    "DSP_WHITE",
    "DSP_CLRXY",
    "DSP_SWAPXY",
    "DSP_COPYXY",
    "DSP_COPYYX",
    "DSP_ADDXY",
    "DSP_ADDYX",
    "DSP_SUBXY",
    "DSP_SUBYX",
    "DSP_MULXY",
    "DSP_DIVXY",
    "DSP_DIVYX",
    "DSP_AVGXY",
    "DSP_AVGYX",
    "DSP_NEGX",
    "DSP_NEGY",
    "DSP_SQRTX",
    "DSP_SHIFT",
    "DSP_VALUE",
    "DSP_VALUE_INT",
    "DSP_MUL_VALUE",
    "DSP_MUL_VALUE_INT",
    "DSP_DIV_VALUE",
    "DSP_DIV_VALUE_INT",
    "DSP_AND_VALUE_INT",
    "DSP_LOAD",
    "DSP_LOAD_GAIN",
    "DSP_LOAD_MUX",
    "DSP_STORE",
    "DSP_LOAD_STORE",
    "DSP_LOAD_MEM",
    "DSP_STORE_MEM",
    "DSP_GAIN",
    "DSP_SAT0DB",
    "DSP_SAT0DB_TPDF",
    "DSP_SAT0DB_GAIN",
    "DSP_SAT0DB_TPDF_GAIN",
    "DSP_DELAY_1",
    "DSP_DELAY",
    "DSP_DELAY_DP",
    "DSP_DATA_TABLE",
    "DSP_BIQUADS",
    "DSP_FIR",
    "DSP_RMS",
    "DSP_DCBLOCK",
    "DSP_DITHER",
    "DSP_DITHER_NS2",
    "DSP_DISTRIB",
    "DSP_DIRAC",
    "DSP_SQUAREWAVE",
    "DSP_CLIP",
    "DSP_LOAD_MEM_DATA",
    "DSP_SINE"
};


long long dspQNM(double x, int n, int m){
    return DSP_QNM(x,n,m);
}

long long dspQM64(double x, int m) {
    return DSP_QM64(x,m);
}

int dspQM32(double x, int m){
    return DSP_QM32(x,m);
}
