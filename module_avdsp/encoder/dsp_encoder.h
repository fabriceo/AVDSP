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
int  paramAligned8();

 int  dsp_END_OF_CODE();
 void dsp_NOP();
 void dsp_CORE();
 void dsp_SERIAL(int N);

 void dsp_SWAPXY();
 void dsp_COPYXY();
 void dsp_CLRXY();
 void dsp_TPDF(int bits);

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

 //saturate the ALU to keep value between -1..+1 and transform to 0.31 format (for int64 ALU)
 void dsp_SAT0DB();
 //apply a gain before saturation
 void dsp_SAT0DB_GAIN(int paramAddr);
 void dsp_SAT0DB_GAIN_Fixed(dspGainParam_t gain);
 //apply a TPDF dither between 8 and 24 bits (bit parameter) before saturation
 void dsp_SAT0DB_TPDF();
 void dsp_SAT0DB_TPDF_GAIN(int paramAddr);
 void dsp_SAT0DB_TPDF_GAIN_Fixed(dspGainParam_t gain);

 void dsp_SHIFT(int bits);
 void dsp_SHIFT_FixedInt(int bits); // exact same as above

// load the ALU with the raw sample 0.31. ALU is 33.31 and can be used only for delay or store.
 void dsp_LOAD(int IO);
// load a sample in 0.31 and apply a gain (4.28) result is 8.56 result
 void dsp_LOAD_GAIN(int IO, int paramAddr);
 void dsp_LOAD_GAIN_Fixed(int IO, dspGainParam_t gain);

 //load many sample from many inputs and apply a gain (4.28) for each, resulting in a 8.56 result
 void dsp_LOAD_MUX(int paramAddr);
 int  dspLoadMux_Inputs(int number);
 int  dspLoadMux_Data(int in, dspGainParam_t gain);

 // store a 0.31 sample from ALU lsb
 void dsp_STORE(int IO);

// load inputs and store them imediately (32 bits)
 void dsp_LOAD_STORE();
 void dspLoadStore_Data(int memin, int memout);

// load a memory (64 bits) location in the ALU
 void dsp_LOAD_MEM(int paramAddr);
 //store ALU (64 bits) in memory location
 void dsp_STORE_MEM(int paramAddr);
 int  dspMem_Location();

// initialize an area of data (biquad, gain, delay line, matrix...)
 int  dsp_PARAM();
 int  dsp_PARAM_NUM(int num);

 int  dspDataN(int * data, int n);
 int  dspData2(int a,int b);
 int  dspData4(int a,int b, int c, int d);
 int  dspData6(int a,int b, int c, int d, int e, int f);
 int  dspData8(int a,int b, int c, int d, int e, int f, int g, int h);
 // create a int32 table of n samples, preloaded with a 2.PI sine values
 int  dspGenerator_Sine(int samples);

 //apply a gain (4.28) on the ALU . result is 8.56
 void dsp_GAIN_Fixed(dspGainParam_t gain);
 void dsp_GAIN(int paramAddr);
 int  dspGain_Default(dspGainParam_t gain);

 //load a value in the ALU (previous ALU moved to ALU2)
 void dsp_VALUE_Fixed(float value);
 void dsp_VALUE_FixedInt(int value);
 void dsp_VALUE(int paramAddr);
 int  dspValue_Default(float value);
 int  dspValue_DefaultInt(int value);

 void dsp_DIV_Fixed(float value);
 void dsp_DIV_FixedInt(int value);

 void dsp_MUL_Fixed(float value);
 void dsp_MUL_FixedInt(int value);

// swap the ALU lsb (32 bits) with a delay line. to be used just before store.
 void dsp_DELAY(int paramAddr);
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

// calculate cascaded biquad
 void dsp_BIQUADS(int paramAddr);
 //define the list of biquad within a PARAM structure
 //each filters to be declared below. Number is the number of biquad cell (1storder = 2ndOrder = 1cell)
 int  dspBiquad_Sections(int number);

 // not tested yet
 void dsp_FIR(int paramAddr);
 int  dspFir_Impulses();
 int  dspFir_Delay(int value);
 int  dspFir_ImpulseFile(char * name, int length);

 //sum square with combined accumulation and moving average over a delay line of N Samples
 // the period of accumulation is dependent of the sampling rate and ajusted so that the integration time fit the parameter
 void dsp_RMS(int timems, int delayLine);
 void dsp_RMS_MilliSec(int timems, int delayms);

 // compute the sqrt of the sum of X * Y during a total integration time with optional averaging to get intermediate results
 // same runtime as RMS but with a factor being negative to differentiate
 void dsp_PWRXY(int timems, int delayLine);
 void dsp_PWRXY_MilliSec(int timems, int delayms);

 //void dsp_CIC_I(int delay);
 //void dsp_CIC_D(int delay);

#endif /* DSP_ENCODER_H_ */
