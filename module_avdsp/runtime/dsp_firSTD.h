/*
 * dsp_firSTD.h
 *
 *  Created on: 11 janv. 2020
 *      Author: Fabrice
 */

#if DSP_ALU_INT64
dspALU_t dsp_calc_fir(  dspALU_t ALU,               // original Accumulator when enterring fir contains xn sample in 0.31
                        dspParam_t  * coefPtr,       // absolute pointer on the filter impulse response, 8 byte alligned, 0.31 format
                        dspSample_t * dataPtr,       // table on the filter state variable, same size as filter response
                        int num) {                   // number of taps

dspSample_t xn = ALU ;  // expecting ALU to store the xn sample in 0.31 so no conversion here

    dspAligned64_t *cPtr = (dspAligned64_t*)coefPtr;    // convert pointers for accessing 64 bits data
    dspAligned64_t *dPtr = (dspAligned64_t*)dataPtr;
    for (int i=0; i< (num & (~1)); i++) {
        dspAligned64_t coef = *cPtr++;
        dspAligned64_t data = *dPtr;
        dspSample_t x2 = data >> 32; // msb
        dspParam_t c1 = coef & 0xFFFFFFFF;
        ALU += (signed long long)xn * c1;
        dspParam_t c2 = coef >> 32;
        dspSample_t x1 = data & 0xFFFFFFFF;
        ALU += (signed long long)x1 * c2;
        dspprintf("coef1 = %d, coef2 = %d\n",c1,c2);
        *dataPtr++ = (signed long long)xn | ((signed long long)x1 << 32);
        xn = x2;
    }
    if (num & 1) {
        coefPtr = (dspParam_t*)cPtr;
        ALU += (signed long long)xn * (*cPtr);
    }
    return ALU;
}
#endif
