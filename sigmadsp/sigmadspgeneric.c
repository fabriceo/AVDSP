/*
 * sigmadspgeneric.c
 *
 *  Created on: 14 may 2020
 *      Author: fabriceo
 *
 */

#include "sigmadspgeneric.h" // this also includes the generic_PARAM.h file from sigmastudio

const int mixerInAddr  = DSP_MIXER_IN_ADDR(0);
const int mixerOutAddr = DSP_MIXER_OUT_ADDR(0);
const int gainInAddr   = DSP_GAIN_IN_ADDR;
const int gainOutAddr  = DSP_GAIN_OUT_ADDR;
const int delayOutAddr = DSP_DELAY_OUT_ADDR;


// inventory of the number of generic filter banks with name FILTERX when X is between 1 and 16 max in this version
const int filterBankAddr[16] = {
#if DEF_FILTER_ADDR(1)
        DSP_FILTER_ADDR(1),
#else
        0,
#endif
#if DEF_FILTER_ADDR(2)
        DSP_FILTER_ADDR(2),
#else
        0,
#endif
#if DEF_FILTER_ADDR(3)
        DSP_FILTER_ADDR(3),
#else
        0,
#endif
#if DEF_FILTER_ADDR(4)
        DSP_FILTER_ADDR(4),
#else
        0,
#endif
#if DEF_FILTER_ADDR(5)
        DSP_FILTER_ADDR(5),
#else
        0,
#endif
#if DEF_FILTER_ADDR(6)
        DSP_FILTER_ADDR(6),
#else
        0,
#endif
#if DEF_FILTER_ADDR(7)
        DSP_FILTER_ADDR(7),
#else
        0,
#endif
#if DEF_FILTER_ADDR(8)
        DSP_FILTER_ADDR(8),
#else
        0,
#endif
#if DEF_FILTER_ADDR(9)
        DSP_FILTER_ADDR(9),
#else
        0,
#endif
#if DEF_FILTER_ADDR(10)
        DSP_FILTER_ADDR(10),
#else
        0,
#endif
#if DEF_FILTER_ADDR(11)
        DSP_FILTER_ADDR(11),
#else
        0,
#endif
#if DEF_FILTER_ADDR(12)
        DSP_FILTER_ADDR(12),
#else
        0,
#endif
#if DEF_FILTER_ADDR(13)
        DSP_FILTER_ADDR(13),
#else
        0,
#endif
#if DEF_FILTER_ADDR(14)
        DSP_FILTER_ADDR(14),
#else
        0,
#endif
#if DEF_FILTER_ADDR(15)
        DSP_FILTER_ADDR(15),
#else
        0,
#endif
#if DEF_FILTER_ADDR(16)
        DSP_FILTER_ADDR(16)
#else
        0
#endif
};

const int filterBankSize[16] = {
#if DEF_FILTER_COUNT(1)
        DSP_FILTER_COUNT(1) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(2)
        DSP_FILTER_COUNT(2) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(3)
        DSP_FILTER_COUNT(3) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(4)
        DSP_FILTER_COUNT(4) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(5)
        DSP_FILTER_COUNT(5) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(6)
        DSP_FILTER_COUNT(6) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(7)
        DSP_FILTER_COUNT(7) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(8)
        DSP_FILTER_COUNT(8) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(9)
        DSP_FILTER_COUNT(9) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(10)
        DSP_FILTER_COUNT(10) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(11)
        DSP_FILTER_COUNT(11) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(12)
        DSP_FILTER_COUNT(12) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(13)
        DSP_FILTER_COUNT(13) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(14)
        DSP_FILTER_COUNT(14) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(15)
        DSP_FILTER_COUNT(15) / 5,
#else
        0,
#endif
#if DEF_FILTER_COUNT(16)
        DSP_FILTER_COUNT(16) / 5
#else
        0
#endif
};


// inventory of the RMSx algorythms found in the generic sigmastudio file, if any!
#if DEF_RMS_ADDR(1)
const int rmsAddr[8] = {
        DSP_RMS_ADDR(1),

#if DEF_RMS_ADDR(2)
        DSP_RMS_ADDR(2),
#else
        0,
#endif
#if DEF_RMS_ADDR(3)
        DSP_RMS_ADDR(3),
#else
        0,
#endif
#if DEF_RMS_ADDR(4)
        DSP_RMS_ADDR(4),
#else
        0,
#endif
#if DEF_RMS_ADDR(5)
        DSP_RMS_ADDR(5),
#else
        0,
#endif
#if DEF_RMS_ADDR(6)
        DSP_RMS_ADDR(6),
#else
        0,
#endif
#if DEF_RMS_ADDR(7)
        DSP_RMS_ADDR(7),
#else
        0,
#endif
#if DEF_RMS_ADDR(8)
        DSP_RMS_ADDR(8)
#else
        0
#endif
};
#endif

