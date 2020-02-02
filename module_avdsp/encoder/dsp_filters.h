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
        BEna1,BELP2,BELP3,BELP4,BELP5,BELP6,BELP7,BELP8,            // bessel
        BEna2,BEHP2,BEHP3,BEHP4,BEHP5,BEHP6,BEHP7,BEHP8,
        BEna3,BE3LP2,BE3LP3,BE3LP4,BE3LP5,BE3LP6,BE3LP7,BE3LP8,    // bessel at -3db cutoff
        BEna4,BE3HP2,BE3HP3,BE3HP4,BE3HP5,BE3HP6,BE3HP7,BE3HP8,
        BUna1,BULP2,BULP3,BULP4,BULP5,BULP6,BULP7,BULP8,            // buterworth
        BUna2,BUHP2,BUHP3,BUHP4,BUHP5,BUHP6,BUHP7,BUHP8,
        Fna1,LRLP2,Fna2,LRLP4,Fna3,LRLP6,Fna4,LRLP8,
        Fna5,LRHP2,Fna6,LRHP4,Fna7,LRHP6,Fna8,LRHP8,            // linkwitz rilley
        FLP1,FLP2,FHP1,FHP2,FLS1,FLS2,FHS1,FHS2,
        FAP1,FAP2,FPEAK,FNOTCH,                  // other shelving, allpass, peaking, notch
};



//prototypes from dsp_filter.c
int dsp_Filter2ndOrder(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain);
int dsp_LP_BES2(dspFilterParam_t freq);
int dsp_HP_BES2(dspFilterParam_t freq);
int dsp_LP_BES2_3DB(dspFilterParam_t freq);
int dsp_HP_BES2_3DB(dspFilterParam_t freq);
int dsp_LP_BUT2(dspFilterParam_t freq);
int dsp_HP_BUT2(dspFilterParam_t freq);
int dsp_LP_LR2(dspFilterParam_t freq);
int dsp_HP_LR2(dspFilterParam_t freq);
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
