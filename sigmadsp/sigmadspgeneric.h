
// put the path of the PARAM.h file generated by sigma studio
#include "generic2x4v3_IC_1_PARAM.h"

//verify each of the defines below in accordance with the sigmastudio generic file syntax you adopted
#define DEF_FILTER_ADDR(x)    defined( MOD_FILTER##x##_ALG0_STAGE0_B2_ADDR )
#define DSP_FILTER_ADDR(x)    MOD_FILTER##x##_ALG0_STAGE0_B2_ADDR
#define DEF_FILTER_COUNT(x)   defined( MOD_FILTER##x##_COUNT )
#define DSP_FILTER_COUNT(x)   MOD_FILTER##x##_COUNT

#define DEF_FIR_ADDR(x)       defined( MOD_FIR##x##_ALG0_FIRSIGMA300ALG1FIRCOEFF0_ADDR )
#define DSP_FIR_ADDR(x)       MOD_FIR##x##_ALG0_FIRSIGMA300ALG1FIRCOEFF0_ADDR
#define DEF_FIR_COUNT(x)      defined( MOD_FIR##x##_COUNT )
#define DSP_FIR_COUNT(x)      MOD_FIR##x##_COUNT

#define DSP_GAIN_IN_ADDR      MOD_GAIN_IN_ALG0_GAIN0_ADDR
#define DSP_GAIN_IN_COUNT     MOD_GAIN_IN_COUNT
#define DSP_GAIN_OUT_ADDR     MOD_GAIN_OUT_ALG0_GAIN0_ADDR
#define DSP_GAIN_OUT_COUNT    MOD_GAIN_OUT_COUNT

#define DEF_RMS_ADDR(x)       defined( MOD_RMS##x##_ALG0_SINGLEBANDLEVELREADS3007LEVEL_ADDR )
#define DSP_RMS_ADDR(x)       MOD_RMS##x##_ALG0_SINGLEBANDLEVELREADS3007LEVEL_ADDR

#define DEF_MIXER_IN(x)       defined( MOD_MIXER_IN_ALG0_NXNMIXALG2VOL0##x##00_ADDR )
#define DSP_MIXER_IN_ADDR(x)  MOD_MIXER_IN_ALG0_NXNMIXALG2VOL0##x##00_ADDR
#define DEF_MIXER_OUT(x)      defined( MOD_MIXER_OUT_ALG0_NXNMIXALG1VOL0##x##00_ADDR )
#define DSP_MIXER_OUT_ADDR(x) MOD_MIXER_OUT_ALG0_NXNMIXALG1VOL0##x##00_ADDR

#define DSP_DELAY_OUT_ADDR    MOD_DELAY_OUT_ALG0_DELAYAMT_ADDR
#define DSP_DELAY_OUT_COUNT   MOD_DELAY_OUT_COUNT
#define DSP_DELAY_OUT_MAX     (600) // samples

//////////////////////////////////////////////////////////////////////////////////////////

// number of inputs and outputs are considered based on the GAIN_IN and GAIN_OUT algorythm presence
#define DSP_INPUTS  DSP_GAIN_IN_COUNT
#define DSP_OUTPUTS DSP_GAIN_OUT_COUNT

// total number of filter bank, including both Inputs and Outputs
#define DSP_IO_TOTAL (DSP_INPUTS + DSP_OUTPUTS)

// now indentify the larger filterbanks by srutenizing the FILTERx algorythms
#if DEF_FILTER_COUNT(1)
#define FILTER1_COUNT DSP_FILTER_COUNT(1)
#else
#error no filter defined. Expecting filter block with name "FILTERx" with x starting 1
#endif
#if DEF_FILTER_COUNT(2) && (DSP_FILTER_COUNT(2) > FILTER1_COUNT)
#define FILTER2_COUNT DSP_FILTER_COUNT(2)
#else
#define FILTER2_COUNT FILTER1_COUNT
#endif
#if DEF_FILTER_COUNT(3) && (DSP_FILTER_COUNT(3) > FILTER2_COUNT)
#define FILTER3_COUNT DSP_FILTER_COUNT(3)
#else
#define FILTER3_COUNT FILTER2_COUNT
#endif
#if DEF_FILTER_COUNT(4) && (DSP_FILTER_COUNT(4) > FILTER3_COUNT)
#define FILTER4_COUNT DSP_FILTER_COUNT(4)
#else
#define FILTER4_COUNT FILTER3_COUNT
#endif
#if DEF_FILTER_COUNT(5) && (DSP_FILTER_COUNT(5) > FILTER4_COUNT)
#define FILTER5_COUNT DSP_FILTER_COUNT(5)
#else
#define FILTER5_COUNT FILTER4_COUNT
#endif
#if DEF_FILTER_COUNT(6) && (DSP_FILTER_COUNT(6) > FILTER5_COUNT)
#define FILTER6_COUNT DSP_FILTER_COUNT(6)
#else
#define FILTER6_COUNT FILTER5_COUNT
#endif
#if DEF_FILTER_COUNT(7) && (MOD_FILTER7_COUNT > FILTER6_COUNT)
#define FILTER7_COUNT DSP_FILTER_COUNT(7)
#else
#define FILTER7_COUNT FILTER6_COUNT
#endif
#if DEF_FILTER_COUNT(8) && (DSP_FILTER_COUNT(8) > FILTER7_COUNT)
#define FILTER8_COUNT DSP_FILTER_COUNT(8)
#else
#define FILTER8_COUNT FILTER7_COUNT
#endif
#if DEF_FILTER_COUNT(9) && (DSP_FILTER_COUNT(9) > FILTER8_COUNT)
#define FILTER9_COUNT DSP_FILTER_COUNT(9)
#else
#define FILTER9_COUNT FILTER8_COUNT
#endif
#if DEF_FILTER_COUNT(10) && (DSP_FILTER_COUNT(10) > FILTER9_COUNT)
#define FILTER10_COUNT DSP_FILTER_COUNT(10)
#else
#define FILTER10_COUNT FILTER9_COUNT
#endif
#if DEF_FILTER_COUNT(11) && (DSP_FILTER_COUNT(11) > FILTER10_COUNT)
#define FILTER11_COUNT DSP_FILTER_COUNT(11)
#else
#define FILTER11_COUNT FILTER10_COUNT
#endif
#if DEF_FILTER_COUNT(12) && (DSP_FILTER_COUNT(12) > FILTER11_COUNT)
#define FILTER12_COUNT DSP_FILTER_COUNT(12)
#else
#define FILTER12_COUNT FILTER11_COUNT
#endif
#if DEF_FILTER_COUNT(13) && (DSP_FILTER_COUNT(13) > FILTER12_COUNT)
#define FILTER13_COUNT DSP_FILTER_COUNT(13)
#else
#define FILTER13_COUNT FILTER12_COUNT
#endif
#if DEF_FILTER_COUNT(14) && (DSP_FILTER_COUNT(14) > FILTER13_COUNT)
#define FILTER14_COUNT DSP_FILTER_COUNT(14)
#else
#define FILTER14_COUNT FILTER13_COUNT
#endif
#if DEF_FILTER_COUNT(15) && (DSP_FILTER_COUNT(15) > FILTER14_COUNT)
#define FILTER15_COUNT DSP_FILTER_COUNT(15)
#else
#define FILTER15_COUNT FILTER14_COUNT
#endif
#if DEF_FILTER_COUNT(16) && (DSP_FILTER_COUNT(16) > FILTER15_COUNT)
#define FILTER16_COUNT DSP_FILTER_COUNT(16)
#else
#define FILTER16_COUNT FILTER15_COUNT
#endif

// this define the maximum number of successive biquad seen in the generic sigmastudio file
#define FILTER_BANK_SIZE (FILTER16_COUNT / 5)

#if DEF_RMS_ADDR(1)
extern const int rmsAddr[8];
#endif

extern const int mixerInAddr;   //address within sigmadsp parameter for MIXER_IN
extern const int mixerOutAddr;
extern const int gainInAddr;
extern const int gainOutAddr;
extern const int delayOutAddr;

extern const int filterBankAddr[16];    //address within sigmadsp parameter for FILTERx
extern const int filterBankSize[16];    //size of each FILTERx

