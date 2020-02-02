/*
 * dsp_biquadSTD.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */



#if DSP_ALU_INT32
#error biquad 16x16=32 not implemented
dspALU_t dsp_calc_biquads_32_16( dspALU_t ALU, dspParam_t * coefPtr, dspSample_t * dataPtr, int num, int skip, int q) {
    return ALU;
}
#endif

#if DSP_ALU_INT64 // TESTED and OK !
dspALU_t dsp_calc_biquads( dspSample_t x, dspParam_t * coefPtr, dspSample_t * dataPtr, int num, const int mantbq, int skip) {
    dspALU_t xn = x;
    dspALU_t ALU;
    while (num--){
        dspParam_t * cPtr = coefPtr;
        dspParam_t b0 = * coefPtr++; // format is 4.28
        dspParam_t b1 = * coefPtr++;
        dspParam_t b2 = * coefPtr++;
        dspParam_t a1 = * coefPtr++;
        dspParam_t a2 = * coefPtr++;
        coefPtr = cPtr + skip;  // becaue we have many coef depending on frequency span
        // dataptr map : xn-1, xn-2, yn-1, yn-2
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
        xn = ALU >> (mantbq);       // convert 8.56 to original format
        (*dataPtr++) = xn;          // store yn => yn-1     //4->6
        dataPtr++;
    }
    return xn;
}
#endif

#if DSP_ALU_FLOAT // not tested
dspALU_t dsp_calc_biquads_float(dspALU_SP_t x, dspParam_t * coefPtr, dspSample_t * dataPtrParam, int num, int skip) {
#if (DSP_ALU_FLOAT32 == 1)
    float * dataPtr = (float*)dataPtrParam;
#else
    double * dataPtr = (double*)dataPtrParam;
#endif
    dspALU_t xn = x;    // x is float (single precision)
    dspALU_t ALU;

    for (int n=0; n< num; n++){
        dspParam_t * cPtr = coefPtr;
        dspParam_t b0 = * coefPtr++;
        dspParam_t b1 = * coefPtr++;
        dspParam_t b2 = * coefPtr++;
        dspParam_t a1 = * coefPtr++;
        dspParam_t a2 = * coefPtr++;
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
    return ALU;
}

#endif
