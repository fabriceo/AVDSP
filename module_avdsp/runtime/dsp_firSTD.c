/*
 * dsp_firSTD.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabriceo
 */

#include "dsp_runtime.h"
#include "dsp_ieee754.h"

#if DSP_ALU_INT
dspALU_t dsp_calc_fir_int(  dspALU_t ALU,               // original Accumulator when enterring fir contains xn sample in 0.31
                        dspParam_t  * coefPtr,       // absolute pointer on the filter impulse response, 8 byte alligned, 0.31 format
                        dspALU_SP_t * dataPtr,       // table on the filter state variable, same size as filter response
                        int num) {                   // number of taps
    int num2 = num /2;
    dspSample_t xn = ALU ;  // expecting ALU to store the xn sample in 0.31 so no conversion here
    ALU = 0;
    dspAligned64_t *cPtr = (dspAligned64_t*)coefPtr;    // convert pointers for accessing 64 bits data
    dspAligned64_t *dPtr = (dspAligned64_t*)dataPtr;
    for (int i=0; i< num2; i++) {
        dspAligned64_t coef = *cPtr++;
        dspAligned64_t data = *dPtr;
        dspSample_t x2 = data >> 32; // msb
        dspParam_t c1 = coef & 0xFFFFFFFF;
        ALU += (signed long long)xn * c1;
        dspParam_t c2 = coef >> 32;
        dspSample_t x1 = data & 0xFFFFFFFF;
        ALU += (signed long long)x1 * c2;
        //dspprintf("coef1 = %d, coef2 = %d\n",c1,c2);
        *dataPtr++ = (signed long long)xn | ((signed long long)x1 << 32);
        xn = x2;
    }
    if (num & 1) {
        coefPtr = (dspParam_t*)cPtr;
        ALU += (signed long long)xn * (*cPtr);
    }
    return ALU;
}

#elif DSP_ALU_FLOAT
dspALU_t dsp_calc_fir_float(dspALU_SP_t xn,
                        dspParam_t  * coefPtr,       // absolute pointer on the filter impulse response
                        dspALU_SP_t * dataPtr,       // table on the filter state variable, same size as filter response
                        int num) {                   // number of taps

    dspALU_t ALU = 0;

    for (int i=0; i < num ; i++) {
        dspALU_SP_t prev = *(dataPtr+i);
        *(dataPtr+i) = xn;
        dspMaccFloatFloat( &ALU, xn, *(coefPtr+i) );
        xn = prev;
    }
    return ALU;
}
#endif
