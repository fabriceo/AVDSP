/*
 * sigmadspfilter.c
 *
 *  Created on: 14 may 2020
 *      Author: fabriceo
 *
 */

#include <stdio.h>

#include "sigmadsppresets.h"    // this also include the sigmastudio generic file
#include "sigmadspresponse.h"   // include types

// some function to deal with complex number

dspComplex_t cOne  = { 1.0, 0.0 };
dspComplex_t cZero = { 0.0, 0.0 };

static dspComplex_t cComp(dspFloat_t re, dspFloat_t im){
    dspComplex_t c = { re, im};
    return c;
}

static dspFloat_t cMag(dspComplex_t a) {
    return sqrt( a.re*a.re + a.im*a.im );
}

static dspComplex_t cConj(dspComplex_t a) {
    a.im = - a.im;
    return a;
}

static dspComplex_t cAdd(dspComplex_t a,dspComplex_t b){
    a.re += b.re;
    a.im += b.im;
    return a;
}

static dspComplex_t cMul(dspComplex_t a, dspComplex_t b){
    dspComplex_t c;
    c.re = a.re*b.re - a.im*b.im;
    c.im = a.im*b.re + a.re*b.im;
    return c;
}
static dspComplex_t cMulReal(dspComplex_t a, float real){
    a.re *= real;
    a.im *= real;
    return a;
}


static dspFloat_t cArg(dspComplex_t a) {
    return atan2(a.im, a.re);
}


static dspComplex_t cDiv(dspComplex_t a, dspComplex_t b){
    return cMulReal( cMul( a, cConj(b) ), 1.0/(b.re*b.re + b.im*b.im) );
}

static dspComplex_t cExp(dspComplex_t a){
    dspComplex_t c;
    dspFloat_t ex = exp(a.re);
    c.re = ex * cos(a.im);
    c.im = ex * sin(a.im);
    return c;
}

// calculate the multiplier for producing a geometrical list of frequencies between start-stop
dspFloat_t dspFreqMultiplier(dspFloat_t start, dspFloat_t stop, int N){
    return exp(log(stop/start)/(N-1));
}

// initialize the table containing the response for further printing as a bode plot
void dspBodeResponseInit(dspBode_t * t, int N, dspFloat_t gain){
    for (int i=0; i< N; i++) {
        t[i].H = cComp( gain, 0);
        t[i].mag = 0.0;
        t[i].phase = 0.0; }
}

// initialize the table of frequencies with the geometrical list between start and stop
void dspBodeFreqInit(dspFloat_t start, dspFloat_t stop, int round, dspBode_t * t, int N, dspFloat_t fs){
    dspFloat_t mult = dspFreqMultiplier(start, stop, N);
    //printf("multiplier = %f\n",mult);
    dspSamplingFreq = fs;
    for (int i=0; i< N; i++) {
        dspFloat_t ff = start;
        if (round) {
            ff = (start + round/2)/round;
            int f = ff;
            f *= round;
            if (f == 0) f = 1;
            ff = f;}
        t[i].freq = ff;
        dspFloat_t w = 2 * M_PI * ff / fs;
        t[i].Z = cExp( cComp( 0, w ) );
        start *= mult; }
}


// apply a biquad on the response table to further print as a bode plot
void dspBodeApplyBiquad(dspBode_t * t, int N, dspBiquadCoefs_t * bq){
    //printf("bq : b0 %f, b1 %f, b2 %f, a1 %f, a2 %f\n",bq->b0,bq->b1,bq->b2,bq->a1,bq->a2);
    for (int i=0; i < N; i++) {
        dspComplex_t Z = t[i].Z;
        dspComplex_t Z2 = cMul( Z, Z);
        dspComplex_t cNum = cAdd( cComp( bq->b0, 0.0), cAdd( cMulReal( Z, bq->b1 ), cMulReal( Z2, bq->b2 ) ) );
        // coef a1 & a2 are negated due to filter calculation which invert the sign
        dspComplex_t cDen = cAdd( cOne, cAdd( cMulReal( Z, - bq->a1 ), cMulReal( Z2, - bq->a2 ) ) );
        dspComplex_t Hz = cDiv( cNum, cDen);
        t[i].H = cMul( t[i].H, Hz ); }
}

// apply a filter made of multiple biquad on the response table
void dspBodeApplyFilter(dspBode_t * t, int N, dspFilter_t * f){
    if (f->ftype == FNONE) return;
    tempBiquadIndex = 0;
    int cells = dsp_filter(f->ftype, f->freq, f->Q, f->gain);
    for (int i=0; i < cells; i++) {
        dspBodeApplyBiquad(t, N, &tempBiquad[i]);}
}

// apply all the bank filter on the response table
void dspBodeApplyFilterBank(dspBode_t * t, int N, dspFilterBlock_t * fb){
    for (int i=0; i < fb->numberOfFilters; i++)
        dspBodeApplyFilter(t, N, &fb->filter[i]);
}

// compute the magnitude of the response in deciBell. return also min and max
void dspBodeMagnitudeDB(dspBode_t * t, int N, dspFloat_t gain, dspFloat_t *minDB, dspFloat_t *maxDB){
    *minDB = 200.0; *maxDB = -200.0;
    dspFloat_t max = (1ULL<<32); dspFloat_t min = 1.0/max;
    for (int i=0; i < N; i++) {
        dspFloat_t mag = cMag( t[i].H) * gain;
        if (mag > max) mag=max;
        if (mag < min) mag=min;
        mag = 20.0 * log10(mag);
        t[i].mag = mag;
        if (mag>*maxDB) *maxDB=mag;
        if (mag<*minDB) *minDB=mag; }
}

void dspBodePhase(dspBode_t * t, int N){
    for (int i=0; i < N; i++) {
        dspFloat_t phase = cArg( t[i].H ) * 180.0/M_PI;
        t[i].phase = phase; }
}
