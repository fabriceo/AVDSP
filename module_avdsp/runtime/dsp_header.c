/*
 * dsp_header.c
 *
 *  Created on: 11 oct. 2023
 *      Author: Fabrice
 */
#include "dsp_header.h"

//used in both dsp_encoder.c and dsp_runtime
const char * dspOpcodeText[DSP_MAX_OPCODE] = {
    "DSP_END_OF_CODE",      //0
    "DSP_HEADER",
    "DSP_PARAM",
    "DSP_PARAM_NUM",
    "DSP_NOP",
    "DSP_CORE",
    "DSP_SECTION",

    "DSP_LOAD",             //7
    "DSP_STORE",
    "DSP_LOAD_STORE",
    "DSP_STORE_TPDF",
    "DSP_STORE_GAIN",
    "DSP_LOAD_GAIN",
    "DSP_LOAD_MUX",
    "DSP_MIXER",

    "DSP_LOAD_X_MEM",
    "DSP_STORE_X_MEM",
    "DSP_LOAD_Y_MEM",
    "DSP_STORE_Y_MEM",

    "DSP_LOAD_MEM_DATA",

    "DSP_CLRXY",        //20
    "DSP_SWAPXY",
    "DSP_COPYXY",
    "DSP_COPYYX",
    "DSP_ADDXY",
    "DSP_ADDYX",
    "DSP_SUBXY",
    "DSP_SUBYX",
    "DSP_MULXY",
    "DSP_MULYX",
    "DSP_DIVXY",
    "DSP_DIVYX",
    "DSP_AVGXY",
    "DSP_AVGYX",
    "DSP_NEGX",
    "DSP_NEGY",
    "DSP_SHIFT",
    "DSP_VALUEX",
    "DSP_VALUEY",

    "DSP_GAIN",     //39
    "DSP_CLIP",

    "DSP_SAT0DB",   //41
    "DSP_SAT0DB_VOL",
    "DSP_STORE_VOL",
    "DSP_SAT0DB_GAIN",
    "DSP_STORE_VOL_SAT",
    "DSP_SERIAL",

    "DSP_DELAY_1",  //47
    "DSP_DELAY",
    "DSP_DELAY_DP",

    "DSP_BIQUADS",
    "DSP_DCBLOCK",

    "DSP_DATA_TABLE",
    "DSP_TPDF_CALC",
    "DSP_TPDF",
    "DSP_WHITE",
    "DSP_DITHER",
    "DSP_DITHER_NS2",
    "DSP_DISTRIB",
    "DSP_DIRAC",
    "DSP_SQUAREWAVE",
    "DSP_SINE",
    "DSP_SQRTX",
    "DSP_RMS",
    "DSP_FIR",
    "DSP_DELAY_FB_MIX",
    "DSP_INTEGRATOR",
    "DSP_CICUS",
    "DSP_CICN"
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
