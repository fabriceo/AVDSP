
#include "sigmadspgeneric.h"

#include "sigmadspfilters.h"

#if 0
//below structure using bitfield is not used in this version
typedef struct {
    int mantissa:24;
    int integer:8;
} dsp824_t;
_Static_assert(sizeof(dsp824_t)==4,"dsp8_24_t is not packed as 32bits by this compiler");
#else
typedef int dsp824_t;
#endif


// each user filter definition will be stored in this structure
typedef struct  {    // basic structure requires 16 bytes per user defined filter
    enum filterTypes ftype;         // this is a binary number based on "enum filterTypes" above
    char bypass;                    // 0 : normal  , 1 : not used/bypassed
    char invert;                    // 1 : invert phase by applying negative gain
    char locked;                    // 1 to indicate that the filter should not be changed
    dspFloat_t freq;                // corner frequency of the filter
    dspFloat_t Q;                   // quality factor of the filter (if relevant)
    dspFloat_t gain;                // boost gain for this filter
} dspFilter_t;

// each input or output is associated with a filter bank and gain delay like this
typedef struct {
    dspFloat_t   mixer[DSP_INPUTS];             // gain coefficient for mixing each relevant input
    int          numberOfFilters;               // number of filters supported in this bank according to sigma studio generic file loaded
    dspFilter_t  filter[FILTER_BANK_SIZE];      // table containing the list of filters
    dspFloat_t   gain;                          // gain attached with this filter bank (upfront for input, otherwise after for output)
    dspFloat_t   delayMicroSec;                 // delay added at the end of the filter bank in micro seconds
    int mute;                                   // 1 if muted
    int invert;                                 // 1 if inverted
} dspFilterBlock_t;


// structure defining a full preset containing mixers, gains, filter and delay for all the inputs & outputs
typedef struct  {
    int presetNumber;               // index of the preset as seen by user
    int checksum;                   // checksum of all the data stored in the preset
    int filterBankSize;             // same as FILTER_BANK_SIZE : maximum number of filters seen for all banks
    int numberOfInputs;             // number of input channels stored in the preset
    int numberOfOutputs;            // number of output channels stored in the preset
    dspFilterBlock_t fb[DSP_IO_TOTAL];   // a table of filter block defined in the structure above
} dspPreset_t;



//same structure but as 8.24 integer, compatible with sigmadsp > ADAU1452 (not 1701)
typedef struct {
    dsp824_t b0;
    dsp824_t b1;
    dsp824_t b2;
    dsp824_t a1;
    dsp824_t a2;
} dspBiquadCoefs824_t;


// buffer required to download a filter bank configuration to the sigmadsp param memory
#define DSP_PARAM_BUFFER_SIZE (2+DSP_INPUTS  +2 +2 +2+FILTER_BANK_SIZE*sizeof(dspBiquadCoefs824_t) + 1)

typedef int sigmadspBuffer_t[DSP_PARAM_BUFFER_SIZE];

#if 0
// convert a deciBell value to a float number. e.g. dB2gain(10.0) => 3.162277
// expected to be optimized by compiler where dB is known at compile time
static inline dspFloat_t dB2gain(dspFloat_t db){
    db /= 20.0;
    return pow(10,db); }
#endif

// convert a float number to a fixed point integer with a mantissa of 24 bit
// eg : the value 0.5 will be coded as 0x00800000
static inline dsp824_t dspQ8_24(float f){
    float maxf = (1 << 7);
    if (f >=   maxf)  return 0x7fffffff;
    if (f < (-maxf))  return -1;
    unsigned mul = 1 << 24;
    f *= mul;
    return f;   // will convert to integer
}

// this macro makes life easier to go trough each of the filter banks , including inputs & outputs
#define DSP_FOR_ALL_CHANNELS(p,ch) for (int ch=0; ch < (p->numberOfInputs + p->numberOfOutputs); ch++)

extern const dspFilter_t dspFilterNone; // default content for fnone filter

// return the checksum of a preset
extern int  dspPresetChecksum(dspPreset_t * p);
// calculate and then update the checksum of a preset (typically used before saving)
extern void dspPresetChecksumUpdate(dspPreset_t * p);
// calculate and then verify the checksum of a preset (typically used after loading a preset)
extern int  dspPresetChecksumVerify(dspPreset_t * p);
// reset a preset with defaut mixer, gain, delay and fnone in each filter bank
extern void dspPresetReset(dspPreset_t * p, int numPreset);
// prepare the data and biquad for downloading to sigmadsp, for a given channel within a given preset
extern int  dspPresetConvert(dspPreset_t * p, int channel, int * dest, dspFloat_t fs, dspFloat_t gainMul, dspFloat_t delayAdd);
// set a filter in the bank of a given channel of a given preset
extern void dspPresetSetFilter(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter);
extern int dspFilterNeedQ(enum filterTypes ftype);
extern int dspFilterNeedGain(enum filterTypes ftype);
// compute the number of biquad cells needed for the filter bank associated to a given channel of a given preset
extern int  dspPresetCalcCellsUsed(dspPreset_t * p, int ch);
// return a comprehensive table indicating how the biquad cells available are fulfilled with the filters.
extern int  dspPresetCalcTableCellsUsed(dspPreset_t * p, int ch, char * table);
// go through the list of filters and ensure that any FNONE is at the end of the list
extern void dspPresetMoveNoneFilters(dspPreset_t * p, int channel);
//sort the filters by increasing frequencies. Move the FNONE filters at the end
extern void dspPresetSortFilters(dspPreset_t * p, int channel);
