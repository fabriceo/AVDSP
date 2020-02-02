/*
 * dsp_biquadSTD.c
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */


#if DSP_ALU_INT32
#error biquad 16x16=32 not implemented
extern dspALU_t dsp_calc_biquads_32_16( dspALU_t ALU, dspParam_t * coefPtr, dspSample_t * dataPtr, int num, int skip, int q) ;
#endif

#if DSP_ALU_INT64 // TESTED and OK !
extern dspALU_t dsp_calc_biquads( dspSample_t x, dspParam_t * coefPtr, dspSample_t * dataPtr, int num, const int mantbq, int skip) ;
#endif

#if DSP_ALU_FLOAT // not tested
extern dspALU_t dsp_calc_biquads_float(dspALU_SP_t x, dspParam_t * coefPtr, dspSample_t * dataPtrParam, int num, int skip) ;
#endif
