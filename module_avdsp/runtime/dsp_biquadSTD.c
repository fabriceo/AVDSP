/*
 * dsp_biquadSTD.c
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */

#include "dsp_runtime.h"

#if DSP_ALU_INT32
#error biquad 16x16=32 not implemented
dspALU_t dsp_calc_biquads_short( dspALU_t ALU, dspParam_t * coefPtr, dspSample_t * dataPtr, int num, const int mantbq, int skip) {
    return ALU;
}
#endif

#if DSP_ALU_INT64 // TESTED and OK with matissa reintegration and a1 minus 1.0 ! missing code for Saturation
dspALU_t dsp_calc_biquads_int( dspALU_t xn, dspParam_t * coefPtr, dspSample_t * dataPtr, short num, const int mantbq, int skip) {
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
        xn = ALU >> (mantbq);       // convert 8.56 to original format by removing mantissa from biquad coeficcients
        (*dataPtr++) = xn;          // store yn => yn-1
        dataPtr++;
        *p = ALU;                   // store last biquad compute in "prev" for mantissa reintegration
    }
    //return xn;    prefer returning scaled value
    return ALU;                     // the result is scaled with the biquad precision (28) that was removed before the call
}
#endif

#if DSP_ALU_FLOAT // not tested
extern int dspMantissa;
#include "dsp_inlineSTD.h" // import DSP_PTR_TO_FLOAT
dspALU_t dsp_calc_biquads_float(dspALU_SP_t x, dspParam_t * coefPtr, dspSample_t * dataPtrParam, short num, int skip) {
#if DSP_ALU_FLOAT32
    float * dataPtr = (float*)dataPtrParam;
#else
    double * dataPtr = (double*)dataPtrParam;   // TODO check if original program is not encoded FLOAT !
#endif
    dspALU_t xn = x;    // x is float (single precision)
    dspALU_t ALU;

    while (num--) {
        dspParam_t * cPtr = coefPtr;
        dspParam_t b0 = DSP_PTR_TO_FLOAT(coefPtr++);
        dspParam_t b1 = DSP_PTR_TO_FLOAT(coefPtr++);
        dspParam_t b2 = DSP_PTR_TO_FLOAT(coefPtr++);
        dspParam_t a1 = DSP_PTR_TO_FLOAT(coefPtr++);
        if (dspMantissa) a1 += 1.0; // correcting the special coding made by the encoder if in integer mode
        dspParam_t a2 = DSP_PTR_TO_FLOAT(coefPtr);
        coefPtr = cPtr + skip;  // because we have many coef depending on frequency span

        ALU = (xn * b0);
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
        xn = ALU;
        (*dataPtr++) = xn;          // store yn => yn-1     //4->6
        dataPtr++;
    }
    return xn;
}

#endif
