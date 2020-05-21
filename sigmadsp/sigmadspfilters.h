
#include <math.h>

typedef float dspFloat_t;       // 32bits IEEE754 by default

#ifdef DSP_FILTER_PARAM_T
typedef DSP_FILTER_PARAM_T dspFilterParam_t;
#else
typedef float dspFilterParam_t; // 32bits IEEE754 by default
#endif

// the list of all supported filters.
enum filterTypes { FNONE,
        BEna1,LPBE2,LPBE3,LPBE4,BEna5,LPBE6,BEna9,LPBE8,   // bessel
        BEna2,HPBE2,HPBE3,HPBE4,BEna6,HPBE6,BEna10,HPBE8,
        BEna3,LPBE3db2,LPBE3db3,LPBE3db4,BEna7,LPBE3db6,BEna11,LPBE3db8,    // bessel at -3db cutoff
        BEna4,HPBE3db2,HPBE3db3,HPBE3db4,BEna8,HPBE3db6,BEna12,HPBE3db8,
        BUna1,LPBU2,LPBU3,LPBU4,BUna3,LPBU6,BUna5,LPBU8, // buterworth
        BUna2,HPBU2,HPBU3,HPBU4,BUna4,HPBU6,BUna6,HPBU8,
        Fna1,LPLR2,LPLR3,LPLR4,Fna3,LPLR6,Fna4,LPLR8,   // linkwitz rilley
        Fna5,HPLR2,HPLR3,HPLR4,Fna7,HPLR6,Fna8,HPLR8,
        FLP1,FLP2,FHP1,FHP2,FAP1,FAP2,FBP0DB, FBPQ,
        FLS1,FLS2,FHS1,FHS2,FPEAK,FNOTCH,         // other shelving, allpass, peaking, notch, bandpass
};


// basic structure for holding the biquad coefficient computed for a given sampling rate
typedef struct {
    dspFilterParam_t b0;
    dspFilterParam_t b1;
    dspFilterParam_t b2;
    dspFilterParam_t a1;
    dspFilterParam_t a2;
} dspBiquadCoefs_t;

#define tempBiquadMax 4
extern dspBiquadCoefs_t tempBiquad[tempBiquadMax];         // temporary array of max 4 biquad cell
extern int tempBiquadIndex;
extern dspFloat_t dspSamplingFreq;

extern const char * dspFilterNames[];   // short comprehensive name for the filter ftype, 5 char max
extern const char   dspFilterCells[];   // number of biquad cells needed for the filter ftype (1..4) 0 for fnone

// search for the name of a filter in the table and return index
extern enum filterTypes dspFilterNameSearch(char *s);
// compute filter coefficients and store them in "tempBiquad"
extern int dsp_filter(int type, dspFloat_t freq, dspFloat_t Q, dspFloat_t gain);

