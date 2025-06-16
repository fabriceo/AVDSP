/*
 * dsp_header.c
 *
 *  Created on: 11 oct. 2023
 *      Author: Fabrice
 */
#include "dsp_header.h"

//used in both dsp_encoder.c and dsp_runtime
const char * dspOpcodeText[DSP_LAST_OPCODE] = {
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
    "DSP_WFIR",

    "DSP_DELAY_FB_MIX",
    "DSP_INTEGRATOR",
    "DSP_CICUS",
    "DSP_CICN",
    "DSP_EXPMA",
    "DSP_THDCOMP",
    "DSP_BIQUAD_FS",
    "DSP_BIQUAD_FS_FAST",
    "DSP_BIQUAD_FS_FAST8",
    "DSP_SEND",
    "DSP_RECEIVE",

    "DSP_DRC_ENV_PEAK",
    "DSP_DRC_ENV_RMS",
    "DSP_DRC_LIM_PEAK",
    "DSP_DRC_LIM_RMS",
    "DSP_DRC_LIM_PEAK_CLIP",
    "DSP_DRC_COMPRESSOR",
    "DSP_DRC_EXPANDER",
    "DSP_DRC_NOISE_GATE",

    "DSP_CLRMEM",
    "DSP_SWAPMEM",
    "DSP_ADDMEM",
    "DSP_MEMADD",
    "DSP_SUBMEM",
    "DSP_MEMSUB",
    "DSP_MULMEM",
    "DSP_DIVMEM",
    "DSP_AVGMEM",
    "DSP_MEMAVG",
    "DSP_NEGMEM",
    "DSP_SAVEMEM",
    "DSP_LOADMEM",
    "DSP_VALUEMEM",
    "DSP_MIXERMEM",
    "DSP_GAIN_MEM",
    "DSP_INPUTMEM",
    "DSP_INPUTGAINMEM",

    "DSP_CORE_EXTERN",

    "DSP_FLOAD",
    "DSP_FSTORE",
    "DSP_FVALUEX",
    "DSP_FGAIN",
    "DSP_FBIQUADS",

/*
    "DSP_OP109",
    "DSP_OP110",
    "DSP_OP111",
    "DSP_OP112",
    "DSP_OP113",
    "DSP_OP114",
    "DSP_OP115",
    "DSP_OP116",
    "DSP_OP117",
    "DSP_OP118",
    "DSP_OP119",
    "DSP_OP120",
    "DSP_OP121",
    "DSP_OP122",
    "DSP_OP123",
    "DSP_OP124",
    "DSP_OP125",
    "DSP_OP126",
    "DSP_OP127", */
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
