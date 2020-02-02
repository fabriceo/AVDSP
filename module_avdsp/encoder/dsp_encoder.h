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
int  paramAlign8();

 int  dsp_END_OF_CODE();
 void dsp_NOP();
 void dsp_CORE();
 void dsp_SWAPXY();
 void dsp_COPYXY();
 void dsp_CLRXY();
 void dsp_ADDXY();
 void dsp_SUBXY();
 void dsp_MULXY();
 void dsp_DIVXY();
 void dsp_SQRTX();
 void dsp_SAT0DB();

 void dsp_SERIAL(int N);

 void dsp_TPDF(int bits);

 void dsp_LOAD(int IO);
 void dsp_STORE(int IO);
 void dsp_STORE_TPDF(int IO, int bits);

#if (DSP_FORMAT == DSP_FORMAT_INT64)
 void dsp_LOAD_DP(int mem);
 void dsp_STORE_DP(int mem);
#endif


 void dsp_LOAD_STORE();
 void dspLoadStore_Data(int memin, int memout);


 void dsp_LOAD_MEM(int paramAddr);
 void dsp_STORE_MEM(int paramAddr);
 int  dspMem_Location();


 int  dsp_PARAM();
 int  dsp_PARAM_NUM(int num);
 int  dsp_DATAN(int * data,int n);
 int  dsp_DATA2(int a,int b);
 int  dsp_DATA4(int a,int b, int c, int d);
 int  dsp_DATA6(int a,int b, int c, int d, int e, int f);
 int  dsp_DATA8(int a,int b, int c, int d, int e, int f, int g, int h);

 void dsp_GAIN(int paramAddr);
 int  dspGain_Default(dspGainParam_t gain);
 void dsp_GAIN_Fixed(dspGainParam_t gain);

 void dsp_GAIN0DB(int paramAddr);
 int  dspGain0DB_Default(dspGainParam_t gain);
 void dsp_GAIN0DB_Fixed(dspGainParam_t gain);

 void dsp_COPY();
 void dsp_COPY_Data(int memin,int memout);

 void dsp_DELAY(int paramAddr);
 int  dspDelay_MicroSec_Max(int maxus);
 int  dspDelay_MicroSec_Max_Default(int maxus, int us);
 int  dspDelay_MilliMeter_Max(int maxmm, int speed);
 int  dspDelay_MilliMeter_Max_Default(int maxmm, int mm, int speed);

 void dsp_DELAY_FixedMicroSec(int microSec);
 void dsp_DELAY_FixedMilliMeter(int mm,int speed);

#if (DSP_FORMAT == DSP_FORMAT_INT64)
 void dsp_DELAY_DP(int paramAddr);
 void dsp_DELAY_DP_FixedMicroSec(int microSec);
 void dsp_DELAY_DP_FixedMilliMeter(int mm,int speed);
#endif

 void dsp_DATA_TABLE(int dataPtr, dspGainParam_t gain, int divider, int size);

 int  dspMacc_Inputs(int number);
 int  dspMacc_Data(int memin, dspGainParam_t gain);
 void dsp_MACC(int paramAddr);

 int  dspMux0DB_Inputs(int number);
 int  dspMux0DB_Data(int memin, dspGainParam_t gain);
 void dsp_MUX0DB(int paramAddr, int num);

 int  dspBiquad_Sections(int number);
 void dsp_BIQUADS(int paramAddr);

 int dsp_FIR(int paramAddr);
 int dspFir_Impulses();
 int dspFir_Delay(int value);
 int dspFir_ImpulseFile(char * name, int length);


#endif /* DSP_ENCODER_H_ */
