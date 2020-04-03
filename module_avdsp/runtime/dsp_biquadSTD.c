/*
 * dsp_biquadSTD.c
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */

#include "dsp_runtime.h"
#include "dsp_ieee754.h"

#if (DSP_FORMAT == DSP_FORMAT_INT32)
#error biquad 16x16=32 not implemented
dspALU_t dsp_calc_biquads_short( dspALU_t ALU, dspParam_t * coefPtr, dspALU_SP_t * dataPtr, int num, const int mantbq, int skip) {
    return ALU;
}
#endif

#if (DSP_FORMAT == DSP_FORMAT_INT64)
dspALU_t dsp_calc_biquads_int( dspALU_t xn, dspParam_t * coefPtr, dspALU_SP_t * dataPtr, short num, const int mantbq, int skip) {
    //xn >>= mantbq;  done in the caller
    dspALU_t ALU;
    while (num--) {
        dspParam_t * cPtr = coefPtr;
        dspParam_t b0 = * coefPtr++;
        dspParam_t b1 = * coefPtr++;
        dspParam_t b2 = * coefPtr++;
        dspParam_t a1 = * coefPtr++;
        dspParam_t a2 = * coefPtr++;
        coefPtr = cPtr + skip;  // because we have many coef depending on frequency span
        // dataptr map : prevlsb, prevmsb, xn-1, xn-2, yn-1, yn-2
        dspALU_t* p = (dspALU_t*)dataPtr;
        ALU = *p; dataPtr += 2;      // load latest value of the biquad
        ALU += (xn * b0);
        dspALU_t prev = (*dataPtr); //load xn-1
        ALU += (prev * b1);         // b1*xn-1
        (*dataPtr++) = xn;          // store xn => xn-1
        xn = (*dataPtr);            // load xn-2
        ALU += (xn * b2);           //b2*xn-2
        (*dataPtr++) = prev;        //store xn-1 => xn-2
        xn = (*dataPtr++);          //load yn-1
        ALU += (xn * a1);           //yn-1*a1
        prev = (*dataPtr);          // load yn-2
        ALU += (prev * a2);         //yn-2*a2
        (*dataPtr--) = xn;          // store yn-1 => yn-2 and point on yn-1
        *p = ALU;                   // store last biquad compute in "prev" for mantissa reintegration
        xn = ALU >> (mantbq);       // convert double precision to original format by removing mantissa from biquad coeficcients
        (*dataPtr++) = xn;          // store yn => yn-1
        dataPtr++;
    }
    //return xn;    prefer returning scaled value
    return ALU;                     // the result is scaled with the biquad precision (28) that was removed before the call
}
#endif

#if DSP_ALU_FLOAT // not tested

dspALU_t dsp_calc_biquads_float(dspALU_SP_t xn, dspParam_t * coefPtr, dspALU_SP_t * dataPtr, short num, int skip) {
    dspALU_t ALU;

    while (num--) {
        dspParam_t * cPtr = coefPtr;
        dspParam_t b0 = (*coefPtr++);
        dspParam_t b1 = (*coefPtr++);
        dspParam_t b2 = (*coefPtr++);
        dspParam_t a1 = (*coefPtr++);
        dspParam_t a2 = (*coefPtr);
        coefPtr = cPtr + skip;  // because we have many coef depending on frequency span

        dspALU_t* p = (dspALU_t*)dataPtr;
        ALU = *p; dataPtr += 2;                 // load latest value of the biquad (potentially dual precision)

        dspMaccFloatFloat( &ALU, xn, b0);
        dspALU_SP_t xn1 = (*dataPtr);           //load xn-1
        dspMaccFloatFloat( &ALU , xn1, b1);     // b1*xn-1
        (*dataPtr++) = xn;                      // store xn => xn-1
        dspALU_SP_t xn2 = (*dataPtr);           // load xn-2
        dspMaccFloatFloat( &ALU , xn2, b2);     //b2*xn-2
        (*dataPtr++) = xn1;                     //store xn-1 => xn-2
        dspALU_SP_t yn1 = (*dataPtr++);         //load yn-1
        dspMaccFloatFloat( &ALU , yn1, a1);     //yn-1*a1 (this coef is reduced by 1.0 by encoder)
        dspALU_SP_t yn2 = (*dataPtr);           // load yn-2
        dspMaccFloatFloat( &ALU , yn2, a2);     //yn-2*a2
        *p = ALU;               // store last biquad computed in "prev" for mantissa reintegration at next cycle
        (*dataPtr--) = yn1;     // store yn-1 => yn-2 and point on yn-1
        dspALU_SP_t yn = ALU;
        (*dataPtr++) = yn;      // store yn => yn-1
        dataPtr++;
        xn = yn;
    }
    return ALU;
}

#endif
