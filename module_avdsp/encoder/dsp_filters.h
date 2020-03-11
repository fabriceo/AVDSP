/*
 * dsp_biquadcalc.h
 *
 *  Created on: 9 janv. 2020
 *      Author: fabrice
 */

#ifndef DSP_BIQUADCALC_H_
#define DSP_BIQUADCALC_H_

#include "dsp_header.h"
#include <math.h>

enum filterTypes {
        BEna1,LPBE2,LPBE3,LPBE4,LPBE5,LPBE6,LPBE7,LPBE8,            // bessel
        BEna2,HPBE2,HPBE3,HPBE4,HPBE5,HPBE6,HPBE7,HPBE8,
        BEna3,LPBE3db2,LPBE3db3,LPBE3db4,LPBE3db5,LPBE3db6,LPBE3db7,LPBE3db8,    // bessel at -3db cutoff
        BEna4,HPBE3db2,HPBE3db3,HPBE3db4,HPBE3db5,HPBE3db6,HPBE3db7,HPBE3db8,
        BUna1,LPBU2,LPBU3,LPBU4,LPBU5,LPBU6,LPBU7,LPBU8,            // buterworth
        BUna2,HPBU2,HPBU3,HPBU4,HPBU5,HPBU6,HPBU7,HPBU8,
        Fna1,LPLR2,LPLR3,LPLR4,Fna3,LPLR6,Fna4,LPLR8,
        Fna5,HPLR2,HPLR3,HPLR4,Fna7,HPLR6,Fna8,HPLR8,            // linkwitz rilley
        FLP1,FLP2,FHP1,FHP2,FLS1,FLS2,FHS1,FHS2,
        FAP1,FAP2,FPEAK,FNOTCH,                  // other shelving, allpass, peaking, notch
};



//prototypes from dsp_filter.c
int dsp_Filter2ndOrder(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain);
int dsp_Filter1stOrder(int type, dspFilterParam_t freq,  dspGainParam_t gain);
int dsp_LP_BES2(dspFilterParam_t freq);
int dsp_HP_BES2(dspFilterParam_t freq);
int dsp_LP_BES2_3DB(dspFilterParam_t freq);
int dsp_HP_BES2_3DB(dspFilterParam_t freq);
int dsp_LP_BUT2(dspFilterParam_t freq);
int dsp_HP_BUT2(dspFilterParam_t freq);
int dsp_LP_LR2(dspFilterParam_t freq);
int dsp_HP_LR2(dspFilterParam_t freq);
int dsp_LP_BES3(dspFilterParam_t freq);
int dsp_HP_BES3(dspFilterParam_t freq);
int dsp_LP_BES3_3DB(dspFilterParam_t freq);
int dsp_HP_BES3_3DB(dspFilterParam_t freq);
int dsp_LP_BUT3(dspFilterParam_t freq);
int dsp_HP_BUT3(dspFilterParam_t freq);
int dsp_LP_LR3(dspFilterParam_t freq);
int dsp_HP_LR3(dspFilterParam_t freq);
int dsp_LP_BES4(dspFilterParam_t freq);
int dsp_HP_BES4(dspFilterParam_t freq);
int dsp_LP_BES4_3DB(dspFilterParam_t freq);
int dsp_HP_BES4_3DB(dspFilterParam_t freq);
int dsp_LP_BUT4(dspFilterParam_t freq);
int dsp_HP_BUT4(dspFilterParam_t freq);
int dsp_LP_LR4(dspFilterParam_t freq);
int dsp_HP_LR4(dspFilterParam_t freq);
int dsp_LP_BES6(dspFilterParam_t freq);
int dsp_HP_BES6(dspFilterParam_t freq);
int dsp_LP_BES6_3DB(dspFilterParam_t freq);
int dsp_HP_BES6_3DB(dspFilterParam_t freq);
int dsp_LP_BUT6(dspFilterParam_t freq);
int dsp_HP_BUT6(dspFilterParam_t freq);
int dsp_LP_LR6(dspFilterParam_t freq);
int dsp_HP_LR6(dspFilterParam_t freq);
int dsp_LP_BES8(dspFilterParam_t freq);
int dsp_HP_BES8(dspFilterParam_t freq);
int dsp_LP_BES8_3DB(dspFilterParam_t freq);
int dsp_HP_BES8_3DB(dspFilterParam_t freq);
int dsp_LP_BUT8(dspFilterParam_t freq);
int dsp_HP_BUT8(dspFilterParam_t freq);
int dsp_LP_LR8(dspFilterParam_t freq);
int dsp_HP_LR8(dspFilterParam_t freq);


int dsp_filter(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain);

#endif /* DSP_BIQUADCALC_H_ */
