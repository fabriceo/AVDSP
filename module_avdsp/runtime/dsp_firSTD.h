/*
 * dsp_firSTD.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */

#if DSP_ALU_INT
extern dspALU_t dsp_calc_fir_int(  dspALU_t ALU,               // original Accumulator when enterring fir contains xn sample in 0.31
                        dspParam_t  * coefPtr,       // absolute pointer on the filter impulse response, 8 byte alligned, 0.31 format
                        dspALU_SP_t * dataPtr,       // table on the filter state variable, same size as filter response
                        int num) ;                   // number of taps
#endif

#if DSP_ALU_FLOAT
dspALU_t dsp_calc_fir_float(dspALU_SP_t xn,
                        dspParam_t  * coefPtr,       // absolute pointer on the filter impulse response
                        dspALU_SP_t * dataPtr,       // table on the filter state variable, same size as filter response
                        int num);
#endif
