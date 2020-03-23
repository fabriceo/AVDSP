/*
 * dsp_encoder.h
 *
 *  Created on: 9 janv. 2020
 *      Author: fabrice
 */

#ifndef DSP_ENCODER_H_
#define DSP_ENCODER_H_

#include "dsp_header.h"
#include "dsp_filters.h"
#include "dsp_fileaccess.h"

//create a program strucutre with below functions.

//prototypes from dsp_encoder.c
void dspEncoderInit(opcode_t * opcodeTable, int max, int type, int minFreq, int maxFreq, int maxIO);

void dsp_dumpParameter(int addr, int size, char * name);
void dsp_dumpParameterNum(int addr, int size, char * name, int num);

int  addCode(int code);
int  addFloat(float value);
int  opcodeIndex();
int  opcodeIndexAligned8();
int  opcodeIndexMisAligned8();

 int  dsp_END_OF_CODE();
 void dsp_NOP();
 void dsp_CORE();
 void dsp_SERIAL(int N);

 void dsp_SWAPXY();
 void dsp_COPYXY();
 void dsp_COPYYX();
 void dsp_CLRXY();

 void dsp_ADDXY();
 void dsp_ADDYX();
 void dsp_SUBXY();
 void dsp_SUBYX();
 void dsp_MULXY();
 void dsp_DIVXY();
 void dsp_DIVYX();
 void dsp_AVGXY();
 void dsp_AVGYX();
 void dsp_SQRTX();
 void dsp_NEGX();
 void dsp_NEGY();

 // generate a random number to be used by DITHER or SAT0DB_TPDF
 void dsp_TPDF(int bits);
 // load the ALU with the random number
 void dsp_WHITE();
 //saturate the ALU to keep value between -1..+1 and transform to 0.31 format (for int64 ALU)
 void dsp_SAT0DB();
 //apply a gain before saturation
 void dsp_SAT0DB_GAIN(int paramAddr);
 void dsp_SAT0DB_GAIN_Fixed(dspGainParam_t gain);
 //apply a TPDF dither before saturation
 void dsp_SAT0DB_TPDF();
 //apply a gain then a TPDF dither before saturation
 void dsp_SAT0DB_TPDF_GAIN(int paramAddr);
 void dsp_SAT0DB_TPDF_GAIN_Fixed(dspGainParam_t gain);

// shit ALU left (positive) or right (negative), corresponding to multiply by 2^n or 2^-n
 void dsp_SHIFT(int bits);
 // exact same as above, adding this name for consistency with naming convention
 void dsp_SHIFT_FixedInt(int bits);

// load the ALU with the raw sample 0.31. ALU is 33.31 and can be used only for DELAY or STORE.
 void dsp_LOAD(int IO);
// load a sample in 0.31 and apply a gain (4.28) resulting in format 5.59
 void dsp_LOAD_GAIN(int IO, int paramAddr);
 void dsp_LOAD_GAIN_Fixed(int IO, dspGainParam_t gain);

 //load many sample from many inputs and apply a gain (4.28) for each
 void dsp_LOAD_MUX(int paramAddr);
 // define the section where the couples IO-gain are listed. number can be 0 or negative to maximize the list
 int  dspLoadMux_Inputs(int number);
 // describe each couple IO - gain
 void dspLoadMux_Data(int in, dspGainParam_t gain);

 // store a 0.31 sample from ALU lsb. msb is discarded
 void dsp_STORE(int IO);

// load inputs and store them imediately (32 bits)
 void dsp_LOAD_STORE();
 // used to list each couple of in->out
 void dspLoadStore_Data(int memin, int memout);

// load a memory (64 bits) location in the ALU
 void dsp_LOAD_MEM(int paramAddr);
 //store ALU (64 bits) in memory location
 void dsp_STORE_MEM(int paramAddr);
 // load a memory (64 bits) location in the ALU
  void dsp_LOAD_MEM_Index(int paramAddr, int index);
  //store ALU (64 bits) in memory location
  void dsp_STORE_MEM_Index(int paramAddr, int index);
 // define a memory location within opcode table. to be used within PARAM or PARAM_NUM
 int  dspMem_Location();
 // define multiple locations so that STORE_MEM_Indexed can be used
 int  dspMem_LocationMultiple(int number);

// initialize an area of data (biquad, gain, delay line, matrix...)
 int  dsp_PARAM();
 int  dsp_PARAM_NUM(int num);

 int  dspDataTableInt(int * data, int n);
 int  dspDataTableFloat(float * data, int n);
 int  dspData2(int a,int b);
 int  dspData4(int a,int b, int c, int d);
 int  dspData6(int a,int b, int c, int d, int e, int f);
 int  dspData8(int a,int b, int c, int d, int e, int f, int g, int h);
 // create a int32 table of n samples, preloaded with a 2.PI sine values
 int  dspGenerator_Sine(int samples);

 //directly apply a gain (4.28) on the ALU
 void dsp_GAIN_Fixed(dspGainParam_t gain);
 void dsp_GAIN(int paramAddr);
 int  dspGain_Default(dspGainParam_t gain);

 //load a value in the ALU (previous ALU moved to ALU2)
 void dsp_VALUE_Fixed(float value);
 void dsp_VALUE_FixedInt(int value);
 void dsp_VALUE(int paramAddr);
 int  dspValue_Default(float value);
 int  dspValue_DefaultInt(int value);

 // divide the ALU by a fixed number coded 4.28
 void dsp_DIV_Fixed(float value);
 // divide the ALU by a fixed int32 number
 void dsp_DIV_FixedInt(int value);

 // multiply the ALU by a fixed number coded 4.28
 void dsp_MUL_Fixed(float value);
 // multiply the ALU by a fixed int32 number
 void dsp_MUL_FixedInt(int value);

// apply a delay line. to be used just before STORE or after LOAD as this works only on ALY lsb. msb discarded
 void dsp_DELAY(int paramAddr);
 // used to define the delay , in a PARAM or PARAMNUM section
 int  dspDelay_MicroSec_Max(int maxus);
 int  dspDelay_MicroSec_Max_Default(int maxus, int us);
 int  dspDelay_MilliMeter_Max(int maxmm, float speed);
 int  dspDelay_MilliMeter_Max_Default(int maxmm, int mm, float speed);

 void dsp_DELAY_FixedMicroSec(int microSec);
 void dsp_DELAY_FixedMilliMeter(int mm,float speed);

#if (DSP_FORMAT == DSP_FORMAT_INT64)
 void dsp_DELAY_DP(int paramAddr);
 void dsp_DELAY_DP_FixedMicroSec(int microSec);
 void dsp_DELAY_DP_FixedMilliMeter(int mm,float speed);
#endif

 // used to read a predefined wave form.
 void dsp_DATA_TABLE(int paramAddr, dspGainParam_t gain, int divider, int size);

// calculate cascaded biquads
 void dsp_BIQUADS(int paramAddr);
 //define the list of biquad within a PARAM structure
 //each filters to be declared below. Number is the number of biquad cell (1storder = 2ndOrder = 1cell)
 // negative number is used
 int  dspBiquad_Sections(int number);
 // same, but means the number of cell will be calculated automatically at the end of the section
 int  dspBiquad_Sections_Flexible();
 // same, but means the number of cell cannot be more that the given number (to maximize cpu load at runtime)
 int  dspBiquad_Sections_Maximum(int number);

 // work in progress
 void dsp_FIR(int paramAddr);
 int  dspFir_Impulses();
 int  dspFir_Delay(int value);
 int  dspFir_ImpulseFile(char * name, int length);

 //sum square with combined accumulation and moving average over a delay line of N Samples, N can be 0
 void dsp_RMS(int timems, int delayLine);
 // same but the delay line is given in milisecond and the encoder will adjust the number of sample according to FS
 void dsp_RMS_MilliSec(int timems, int delayms);

 // compute the sqrt of the sum of X * Y during a total integration time with optional averaging to get intermediate results
 // remark: same runtime as RMS but with an internal factor being negative to differentiate functionality
 void dsp_PWRXY(int timems, int delayLine);
 void dsp_PWRXY_MilliSec(int timems, int delayms);

 // apply a first order filter with special rounding mechanism to garantee no DC. yn = pole.yn-1 + xn - xn-1
 void dsp_DCBLOCK(int lowfreq);

 // inject noise and apply 2nd order noise shapping. ( no input = no noise)
 void dsp_DITHER();
 void dsp_DITHER_NS2(int paramAddr);

 void dsp_DIRAC_Fixed(int freq, dspGainParam_t gain);

 void dsp_CLIP_Fixed(dspGainParam_t value);

 // convert a deciBell value to a float number. e.g. dB2gain(10.0) => 3.162277
 static inline dspGainParam_t dB2gain(dspGainParam_t db){
     db /= 20.0;
     return pow(10,db); }

#endif /* DSP_ENCODER_H_ */
