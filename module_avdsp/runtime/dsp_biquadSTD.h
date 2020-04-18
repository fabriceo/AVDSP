/*
 * dsp_biquadSTD.c
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabriceo
 */


#if DSP_FORMAT == DSP_FORMAT_INT32
#error biquad 16x16=32 not implemented
extern dspALU_t dsp_calc_biquads_short( dspALU_t ALU, dspParam_t * coefPtr, dspALU_SP_t * dataPtr, int num, const int mantbq, int skip) ;
#endif

#if DSP_FORMAT == DSP_FORMAT_INT64 // TESTED and OK !
extern dspALU_t dsp_calc_biquads_int( dspALU_t xn, dspParam_t * coefPtr, dspALU_SP_t * dataPtr, short num, const int mantbq, int skip) ;
#endif

#if DSP_ALU_FLOAT // not tested
extern dspALU_t dsp_calc_biquads_float(dspALU_SP_t x, dspParam_t * coefPtr, dspALU_SP_t * dataPtr, short num, int skip) ;
#endif
