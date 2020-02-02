/*
 * dsp_coder.c
 *
 *  Created on: 1 janv. 2020
 *      Author: fabriceo
 *      this program will create a list of opcodes to be executed lated by the dsp engine
 */

#include "dsp_encoder.h"         // enum dsp codes, typedefs and QNM definition
#include <stdlib.h>             // only for importing exit()

#define DSP_ENCODER_VERSION 1   // will be stored in the program header for further interpretation by the runtime

static opcode_t * dspOpcodesPtr  = 0;       // absolute adress start of the table containing the opcodes and data
static dspHeader_t* dspHeaderPtr = 0;       // point on the header containing program summary
static int dspOpcodesMax         = 0;       // max allowed size of this table (in words)

volatile static int dspOpcodeIndex = 0;     // point on the next available opcode position in the opcode table

static int lastOpcodePrint       =  0;
static int lastIndexPrinted      =  0;
static int lastOpcodeIndexLength = -1;      // point on the latest opcode requiring a quantity of code space not yet known
static int dspDataCounter        =  0;      // point on the next data adress available (relative to begining of data)
static int lastParamNumIndex     =  0;      // index position in the opcode table of the latest ongoing PARAM_NUM or PARAM
static int lastMissingParamIndex =  0;      // index where a parameter is expected to follow
static int lastMissingParamSize  =  0;      // expected minimum size of total codelength for the opcode (verified in calclength)
static int dspDumpStarted        =  0;      // as soon as a dsp_dump is executed, this is set to 1
static int lastSectionOpcode     =  0;
static int lastSectionNumber     =  0;
static int maxOpcodeValue        =  0;      // represent the higher opcode value used in the encoded program
static long long usedInputs      =  0;      // bit patern of all the inputs used by a LOAD command
static long long usedOutputs     =  0;      // bit patern of all the output used by a STORE command

static int dspFormat;                       // dynamic management of the different type of data when encoding
static int dspIOmax;                        // max number of IO that can be used with Load & Store (to avoid out of boundaries vs samples table)
static int numberFrequencies;               // number of covered frequencies (mainly used in FIR)

int dspMinSamplingFreq = DSP_DEFAULT_MIN_FREQ;
int dspMaxSamplingFreq = DSP_DEFAULT_MAX_FREQ;



// print the fatal error message and quit encoder
void dspFatalError(char *msg) {
    dspprintf("FATAL ERROR: %s\n",msg);
    exit(1);    // from stdlib.h
}


// return the current index position in the opcode table
int opcodeIndex() {
    asm volatile("":::"memory"); // memory barier to avoid code reschuffling
    return dspOpcodeIndex;
}
int opcodeIndexAdd(int add) {
    asm volatile("":::"memory"); // memory barier to avoid code reschuffling
    int tmp = opcodeIndex();
    if ((tmp+add) > dspOpcodesMax)   // boundary check
        dspFatalError("YOUR DSP CODE IS TOO LARGE FOR THE ARRAY PROVIDED");
    dspOpcodeIndex += add;
    return tmp;
}
// return an absolute pointer within the opcode table
opcode_t * opcodePtr(int index){
    asm volatile("":::"memory"); // memory barier to avoid code reschuffling
    return dspOpcodesPtr + index;
}

// return an absolute pointer on the current index position in the opcode table
opcode_t * opcodeIndexPtr(){
    return opcodePtr(opcodeIndex());
}

// store an op code or a word at the next available place in the opcod table
int addCode(int code) {
    int tmp = opcodeIndex();
    opcodeIndexPtr()->i32 = code;
    opcodeIndexAdd(1);
    opcodeIndexPtr()->i32 = DSP_END_OF_CODE;    // preventive
    return tmp;
}

int addFloat(float value) {  // tested ok
    union floatInt {
        float F;
        int   I;    } val;
    val.F = value;
    return  addCode(val.I);
}

// add an opcode with a value in the LSB
static int addOpcodeValue(int code, int value){
    return addCode((code << 16) | value);
}

// add an opcode with a 0 placeholder value for the code lenght below (never possible)
static int addOpcodeUnknownLength(int code){
    lastOpcodeIndexLength = addOpcodeValue(code , 0);    // memorize Index where we ll need to update the codelenght
    return lastOpcodeIndexLength;
}

// return the current index position in the opcode table and potentially insert one NOP code for padding to 8 bytes
int opcodeIndexAligned8() {
    if (opcodeIndex() & 1) addCode(DSP_NOP);
    return opcodeIndex();
}
// same but to be used when the padding is needed just after a next parameter to come
int opcodeIndexMisAligned8() {
    if ((opcodeIndex() & 1) == 0) addCode(DSP_NOP);
    return opcodeIndex();
}

int  paramAlign8() {
    return opcodeIndexAligned8();
}

// add a code in the opcode table (at current index)
// pointing on an adress within the codetable (typically PARAM_NUM)
// and encode it as relative value to the given "base" parameter
// which is usually the index pointing on the previous dsp_opcode adresses
// if 0 is provided then it means the adress pointed is just here after this code
int addCodeOffset(int index, int base){
    int offset;
    if (index) offset = index - base;           // calculate relative value to "base"
    else offset = opcodeIndex() +1 - base;      // calculate the relative value to "base" of the next opcode
    return addCode(offset);                     // store the value calculated
}

// same as above with the base parameter predefined as being the NEXT current index
// in the opcode table which is more a really relative adress to be used like this:
//      int offest = *ptr++;
//      int * paramPtr = ptr+offset
int addCodeOffsetIndex(int index){
    return addCodeOffset(index, opcodeIndex() + 1);
}

// add a word in the opcode table reresenting a quantity of data required in the data space
void addDataSpace(int size) {
    addCode(dspDataCounter);              // store the current data index value pointing on the next spare data space
    dspDataCounter += size;               // simulate consumption the expected data space
}
// same as above but push the data index by one if needed
// so that the data adress is alligned on 8 bytes boundaries
void addDataSpaceAligned8(int size) {
    if(dspDataCounter & 1) dspDataCounter++;
    addDataSpace(size);                 // store the current data index
}

void addDataSpaceMisAligned8(int size) {
    if((dspDataCounter & 1) == 0) dspDataCounter++;
    addDataSpace(size);                 // store the current data index
}

// for debugging purpose, print all the opcode generated since the latest dsp_opcode
static void printLastOpcodes() {
    if (lastIndexPrinted < lastOpcodePrint) {
        for (int i = lastIndexPrinted; i< lastOpcodePrint; i++){
            int val = opcodePtr(i)->i32;
            dspprintf3("0x%X ",val);
        }
    } else lastOpcodePrint = lastIndexPrinted;
    if (lastOpcodePrint != opcodeIndex()) {
        opcode_t tmp = *opcodePtr(lastOpcodePrint);
        unsigned int code = tmp.op.opcode;
        unsigned int skip = tmp.op.skip;
        dspprintf3("%4d : %d <%d> ",lastOpcodePrint, code, skip);
        for (int i = lastOpcodePrint+1; i< opcodeIndex(); i++) {
            int val = opcodePtr(i)->i32;
            if (val>=0)  dspprintf3("0x%X ",val)
            else dspprintf3("%d ",val);
        }
        dspprintf3("\n");
    }
    lastIndexPrinted = opcodeIndex();  // store the latest Index that have been already printed so far (might not be an opcode
}


// verify if we are within a PARAM or PARAM_NUM section
void checkInParamNum(){
    if (lastParamNumIndex == 0)
        dspFatalError("Currently not in a PARAM or PARAM_NUM space.");
}


static int startParamSection(int opcode, int num){
    if (lastSectionOpcode)
        dspFatalError("Section already started and not finished.");
    printLastOpcodes(); // flush any opcode printing before starting with new datasets
    checkInParamNum();
    lastSectionOpcode = opcode;
    lastSectionNumber = num;
    return opcodeIndex();
}

static void checkParamSection(int opcode){
    checkInParamNum();
    if (lastSectionOpcode == 0)
        dspFatalError("No section defined or started.");
    if (opcode)
        if (lastSectionOpcode != opcode)
            dspFatalError("Section already started for another opcode.");
}

static int nextParamSection(int opcode){
    checkParamSection(opcode);
    lastSectionNumber--;
    if (lastSectionNumber == 0) {
        lastSectionOpcode=0;
    }
    return lastSectionNumber;
}


// define the minimum expected size of code after the current dsp_opcode
// used when a parameter is not provided with its adress in a PARAM space
// then the data are expected to be stored just below
void setLastMissingParamIf0(int index, int size){
    if (index == 0) {
        lastMissingParamIndex  = opcodeIndex();
        lastMissingParamSize = size; }
}


// calculate the number of words till the latest call to addOpcodeUnknownLength
// and store it in the LSB16 of the latest dsp_opcode generated
// calclength is called first by all the user dsp_XXX function
void calcLength(){
    asm volatile("nop":::"memory"); // memory barier to avoid code reschuffling
    if (dspOpcodesPtr == 0)
        dspFatalError("dspEncoderInit has not been launched first.");   // sanity check
    if (lastParamNumIndex) {   // if we were in a Param Num
        if (lastSectionOpcode)
            dspFatalError("Last parameter not finished regarding expeted numbers of parameters.");
        lastParamNumIndex = 0;  // as we are now going to generate a new dsp_opcode then we close the latest PARAM_NUM
    }
    if (lastMissingParamIndex != 0) {  // check if there was a requirement for a minimum code size below the latest dsp_opcode generated
        // check size between now and prev pointer
        int size = opcodeIndex() - lastMissingParamIndex - 1; // represent all the words geenrated below the latest dsp_opcode
        if (size < lastMissingParamSize) {
            dspprintf("for opcode at %d : ",lastMissingParamIndex);
            dspFatalError("not enough parameters provided below this opcode."); }
        lastMissingParamIndex = 0;
        lastMissingParamSize = 0;   // clear this request until next
    }
    if (lastOpcodeIndexLength != -1) { // check if a lenght of code shall be calculated now (calcLength is called by all opcode functions)
        opcode_t tmp = *opcodePtr(lastOpcodeIndexLength);   // read opcode generated
        tmp.op.skip= (opcodeIndex() - lastOpcodeIndexLength);   // generate the lenght as a futur "skip" so the runtime can do: codePtr += codePtr->skip
        *opcodePtr(lastOpcodeIndexLength) = tmp;          // update the opcode bin location
        lastOpcodeIndexLength = -1;                       // reset the index as we just solved it
         }
    opcode_t prevOpcode = *opcodePtr(lastOpcodePrint);
    if (prevOpcode.op.opcode > maxOpcodeValue) maxOpcodeValue = prevOpcode.op.opcode;
    printLastOpcodes();                                   // used to dump the latest code generated and its subcodes
    lastOpcodePrint = opcodeIndex();

}

// generate a header with following structure
// 0 : total lenght including all opcode and all run-data
// 1 : size of required data for running state
// 2 : checksum of all the program : opcodes + length (doesnt include param or coefs or datas)
// 3 : number of cores found in the opcode program
// 4 : dsp code version : minimum interpreter version required
// 5 : max supported frequency : number of instanciation of biquad coefs and delay lines data

void dspEncoderInit(opcode_t * opcodeTable, int max, int type, int minFreq, int maxFreq, int maxIO) {
    dspprintf("DSP ENCODER : format generated for handling ");
    if      (type == 1) dspprintf("integer 32 bits")
    else if (type == 2) dspprintf("integer 64 bits")
    else if (type == 3) dspprintf("float (32bits)")
    else if (type == 4) dspprintf("double (64bits)")
    else if (type == 5) dspprintf("float with float samples")
    else if (type == 6) dspprintf("double with float samples");
    dspprintf("\n");

    dspOpcodesPtr       = opcodeTable;
    dspHeaderPtr        = (dspHeader_t*)opcodeTable;
    dspOpcodesMax       = max;
    dspFormat           = type;
    dspMinSamplingFreq  = minFreq;
    dspMaxSamplingFreq  = maxFreq;
    numberFrequencies   = maxFreq - minFreq +1;
    dspIOmax            = maxIO;
    dspOpcodeIndex      = 0;
    dspDataCounter      = 0;
    maxOpcodeValue      = 0;

    lastOpcodeIndexLength = -1;
    lastParamNumIndex     = 0;
    lastMissingParamIndex = 0;
    lastIndexPrinted      = 0;
    lastSectionOpcode     = 0;
    lastSectionNumber     = 0;
    dspDumpStarted = 0;

    usedInputs = 0;
    usedOutputs = 0;

    addOpcodeUnknownLength(DSP_HEADER);
    addCode(0); // place holder for total lenght including all opcode and all run-data
    addCode(0); // place holder for data size
    addCode(0); // basic checksum of all the program : opcodes + length (doesnt include param or coefs or datas)
    addCode(0); // number of cores found in the opcode program
    addOpcodeValue(DSP_MAX_OPCODE-1,DSP_ENCODER_VERSION);
    addOpcodeValue(maxFreq, minFreq);
}

// see DSP_FORMAT in dsp_runtime.h
static int dspFormat64(){
    // return 1 if the DSP ALU is 64bits size
    return (dspFormat == 2)||(dspFormat == 4)||(dspFormat == 6);
}
static int dspFormatInt(){
    // return 1 if the DSP ALU is integer
    return (dspFormat == 1)||(dspFormat == 2);
}
static int dspFormatInt64(){
    // return 1 if the DSP ALU is integer 64bits size
    return (dspFormat == 2);
}
static int dspFormatDouble(){
    // return 1 if the DSP ALU is double float
    return (dspFormat == 4)||(dspFormat == 6);
}


// search one PARAM or PARAM_NUM area covering the address provided as a parameter
int findInParamSpace(int addrParam) {
    int pos = 0;
    int num;
    while(1) {
        opcode_t * cptr = opcodePtr(pos);
        short code = cptr->op.opcode;
        int skip   = cptr->op.skip;
        int add = 0;
        if ((code == DSP_PARAM) || (code == DSP_HEADER))   {
            add = 1;
            num = 0;
        }
        if (code == DSP_PARAM_NUM) {
            add = 2;
            num = opcodePtr(pos+1)->i32;
        }
        if (add) {
            int begin = pos + add;
            int end = (skip) ? (pos + skip -1) : (opcodeIndex()-1);   // if length not yet calculated, we are just within a PARAM NUM sequence
            if ((addrParam >= begin)&&(addrParam <= end)){ // found it ?
                if (num == 0) return addrParam; // absolute adress is returned
                else          return (addrParam - begin) | (num<<16);     // relative value to PARAM_NUM start of data
            }
        } // continue searching
        if (skip == 0) {
            dspFatalError("Index provided not found in any PARAM or PARAM_NUM space."); }
        pos += skip;
    }
}

// screen the whole code to find all PARAM_NUM area and
// then verify that the provided adress and the expected size
// is within the PARAM_NUM boundaries
static int checkInParamSpace(int index, int size){
    int maxIndex = index + size - 1; // location of the last word needed
    int pos = 0;
    int begin;
    int end;
    while(1) {
        opcode_t * cptr = opcodePtr(pos);
        short code = cptr->op.opcode;
        int   skip = cptr->op.skip;
        int add = 0;
        if ((code == DSP_PARAM)|| (code == DSP_HEADER))  add = 1; // position of the first parameter
        if (code == DSP_PARAM_NUM) add = 2; // position of the first parameter following the PARAM_NUM value
        if (add) {
            begin = pos + add;
            end = (skip) ? (pos + skip) : (opcodeIndex());   // if length not yet calculated, we are just within a PARAM NUM sequence
            if ((index >= begin) && (index < end )) { // within this data space
                if ( maxIndex < end ) return (begin <<16) | end ;   // great fit
                dspFatalError("memory space required too large for this PARAM or PARAM_NUM."); }
        } // continue searching
        if (skip == 0) {
            dspFatalError("Index provided not found in any PARAM or PARAM_NUM space."); }
        pos += skip;
    }
    // unreachable return 0;
}


void dsp_dump(int addr, int size, char * name){
    asm volatile("nop":::"memory"); // memory barier to avoid code reschuffling
    printLastOpcodes();
    dspDumpStarted = 1;
#ifdef DSP_FILEACCESS_H_
    if (dumpFileIsOpen() == 0)
        if (0 != dumpFileCreate())
            dspFatalError("problem in creating dump file.");
    dumpprintf("%s %d %d %d\n", name, addr & 0xFFFF, addr >> 16, size);
#endif
    dspprintf1("DUMP %s %d %d %d\n", name, addr & 0xFFFF, addr >> 16, size);
}
// generate a dump for the given adress space, could be used by a host application
// to update parameters in the data space, using the information generated here.
// if num is 0 then the absolute adress is written, otherwise relative to the latest PARAM_NUM
void dsp_dumpParameter(int addr, int size, char * name){
    int tmp = findInParamSpace(addr);
    dsp_dump(tmp, size, name);
}

void dsp_dumpParameterNum(int addr, int size, char * name, int num){
    int tmp = findInParamSpace(addr);
    if (num) {
        char buff[256];
        sprintf(buff,"%s_%d",name,num);
        dsp_dump(tmp, size, (char*)&buff);
    } else dsp_dump(tmp, size, name);
}


// DSP_END_OF_CODE
// generate the DSP_EN_OF_CODE and calculate total program length, number of core and checksum.
// return size of code alligned to next 8bytes, so can be used as a dataStart index in same array
int dsp_END_OF_CODE(){
    calcLength();                       // solve latest opcode length
    dspprintf2("DSP_END_OF_CODE\n")
    addOpcodeValue(DSP_END_OF_CODE,0);
    if (opcodeIndex() & 1) addCode(0);
    calcLength();                       // just for executing debug print
    dspHeaderPtr->totalLength = opcodeIndex();  // total size of the program
    dspprintf1("dsptotallength (0) = %d\n",opcodePtr(1)->i32);
    dspHeaderPtr->dataSize = dspDataCounter;
    dspprintf1("dataSize       (1) = %d\n",opcodePtr(2)->i32);
    // now calculate the simplified checksum of all the opcodes and count number of cores
    unsigned int sum;
    int numCore;
    dspCalcSumCore(opcodePtr(0), &sum, &numCore);
    dspHeaderPtr->checkSum = sum;           // comit checksum
    dspprintf1("check sum      (2) = %d\n", opcodePtr(3)->i32);
    if (numCore == 0) numCore = 1;
    dspHeaderPtr->numCores = numCore;       // comit number of declared cores
    dspprintf1("numcore        (3) = %d\n",opcodePtr(4)->i32);
    dspHeaderPtr->maxOpcode = maxOpcodeValue;
    if (dspDumpStarted) {
        dsp_dump(opcodeIndex(),dspDataCounter,"DSP_END_OF_CODE_DATA_SIZE");
        dsp_dump(5,1,"DSP_CORES_NUMBER");
        dsp_dump(6,1,"DSP_ENCODER_VERSION");
        dsp_dump(7,1,"DSP_SUPPORTED_FREQUENCY_RANGE");
#ifdef DSP_FILEACCESS_H_
        dumpFileClose();
#endif
    }
    return opcodeIndex();  // size of the program
}


// add a single dsp_code without any following parameters
int addSingleOpcode(int code) {
    calcLength();
    return addOpcodeValue(code, 1);
}

// add a dsp_code with its following parameter
static int addOpcodeParam(int code, int param ) {
    calcLength();
    int tmp = addOpcodeValue(code, 2);
    addCode(param);
    return tmp;
}

// add a dsp_opcode that will be followed by an unknowed list of param at this stage
// will be solved by the next dsp_opcode generation, due to call to calcLength()
int addOpcodeLength(int code) {
    calcLength();
    return addOpcodeUnknownLength(code);
}

// indicate No operation
void dsp_NOP() { addSingleOpcode(DSP_NOP); }

// indicate start of a program for a dedicated core/task
void dsp_CORE(){ addSingleOpcode(DSP_CORE); }

// clear ALU X and Y
void dsp_CLRXY(){ addSingleOpcode(DSP_CLRXY); }

// exchange ALU X and Y
void dsp_SWAPXY(){ addSingleOpcode(DSP_SWAPXY); }

// copy ALU X to Y
void dsp_COPYXY(){ addSingleOpcode(DSP_COPYXY); }

// perform ALU X = X + Y
void dsp_ADDXY(){ addSingleOpcode(DSP_ADDXY); }

// perform ALU X = X - Y
void dsp_SUBXY(){ addSingleOpcode(DSP_SUBXY); }

// perform ALU X = X * Y
void dsp_MULXY(){ addSingleOpcode(DSP_MULXY); }

// perform ALY X = X / Y (X must be larger)
void dsp_DIVXY(){ addSingleOpcode(DSP_DIVXY); }

// perform ALU X = sqrt(X)
void dsp_SQRTX(){ addSingleOpcode(DSP_SQRTX); }


void dsp_SAT0DB() { addSingleOpcode(DSP_SAT0DB); }

// check that the provided load/store location is within the IOmax range defined in the encoderInit
void checkIOmax(int IO){
    if ((IO < 0)||(IO >= dspIOmax))
        dspFatalError("IO out of range.");
}

// load ALU with the physical location provided
void dsp_LOAD(int IO) {
    checkIOmax(IO);
    if (IO<64) usedInputs |= (long long)1<<IO;
    addOpcodeParam(DSP_LOAD, IO);
    dspprintf3("DSP_LOAD input[%d]\n",IO);
    if (IO>63) IO=63;
    usedInputs |= (long long)1<<IO;
}

void dsp_STORE(int IO) {
    checkIOmax(IO);
    addOpcodeParam(DSP_STORE, IO);
    dspprintf3("DSP_STORE output[%d]\n",IO);
    if (IO>63) IO=63;
    usedOutputs |= (long long)1<<IO;
}

void dsp_STORE_TPDF(int IO, int bits){
    checkIOmax(IO);
    addOpcodeParam(DSP_STORE_TPDF, IO);
    addCode(bits);
    dspprintf3("DSP_STORE_TPDF output[%d], %d bits\n",IO, bits);
    if (IO>63) IO=63;
    usedOutputs |= (long long)1<<IO;
}

void dsp_TPDF(int bits){
    addOpcodeParam(DSP_STORE_TPDF, bits);
    dspprintf3("DSP_TPDF %d bits\n", bits);
}

#if (DSP_FORMAT == DSP_FORMAT_INT64)
void dsp_LOAD_DP(int IO) {
    checkIOmax(IO);
    addOpcodeParam(DSP_LOAD_DP, IO);
    dspprintf3("DSP_LOAD_DP input[%d]\n",IO); }

void dsp_STORE_DP(int IO) {
    checkIOmax(IO);
    addOpcodeParam(DSP_STORE_DP, IO);
    dspprintf3("DSP_STORE_DP output[%d]\n",IO); }
#endif

// check if we currently are below a specific opcode just generated
void checkLastMissing(int opcode){
    if (lastMissingParamIndex == 0)
        dspFatalError("no parameter expected here.");
    if (opcode)
        if (opcodePtr(lastMissingParamIndex)->op.opcode != opcode)
            dspFatalError("incompatible with the previous opcode generated.");
}

// check if we are currently inside a PARAM_NUM section otherwise below a specific opcode just generated
void checkInParamNumOrLastMissing(int opcode){
    if (lastParamNumIndex == 0)
        checkLastMissing(opcode);
    else
        checkInParamNum();
}


int dsp_PARAM() {
    int tmp = addOpcodeLength(DSP_PARAM);
    lastParamNumIndex = tmp; // indicate that we are inside a PARAM_NUM statement
    return tmp;
}

// DSP_PARAM_NUM

int dsp_PARAM_NUM(int num) {
    int tmp = addOpcodeLength(DSP_PARAM_NUM);
    lastParamNumIndex = tmp;
    addCode(num);
    return tmp;
}

/*
 * GAIN
 */

int addGainCodeQ31(dspGainParam_t gain){
if (dspFormatInt())
     return addCode(DSP_Q31(gain));
else return addFloat(gain);
}


int addGainCodeQNM(dspGainParam_t gain){
    if (dspFormatInt())
         return addCode(DSP_QNM(gain));
    else return addFloat(gain);
}

void dsp_GAIN(int paramAddr){
    setLastMissingParamIf0(paramAddr, 2);
    int tmp = addOpcodeLength(DSP_GAIN);
    addCodeOffset(paramAddr, tmp);
}

void dsp_GAIN0DB(int paramAddr){
    setLastMissingParamIf0(paramAddr, 2);
    int tmp = addOpcodeLength(DSP_GAIN0DB);
    addCodeOffset(paramAddr, tmp);
}

// can be used either in a param space, or just below the DSP_GAIN opcode
int dspGain_Default(dspGainParam_t gain){
    checkInParamNumOrLastMissing(DSP_GAIN);
    return addGainCodeQNM(gain);
}

// can be used either in a param space, or just below the DSP_GAIN opcode
int dspGain0DB_Default(dspGainParam_t gain){
    checkInParamNumOrLastMissing(DSP_GAIN0DB);
    return addGainCodeQ31(gain);
}

void dsp_GAIN_Fixed(dspGainParam_t gain){
    int tmp = addOpcodeLength(DSP_GAIN);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(gain);
}

void dsp_GAIN0DB_Fixed(dspGainParam_t gain){
    int tmp = addOpcodeLength(DSP_GAIN0DB);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQ31(gain);
}

void dsp_DELAY_1(){
    addOpcodeLength(DSP_DELAY_1);
    if (dspFormat64())
         addDataSpaceAligned8(2); // 2 words for delaying 64bits alu without any conversion
    else addDataSpace(1);
}

// DSP_SERIAL
void dsp_SERIAL(int N) {
    addOpcodeLength(DSP_SERIAL);
    addCode(N);
    addCode(!N);
}

int dsp_DATAN(int * data, int n){
    checkInParamNum();
    int tmp = opcodeIndex();
    for (int i=0; i<n; i++) addCode(*(data+i));
    return tmp;
}

int dsp_DATA2(int a,int b){
    checkInParamNum();
    int tmp = opcodeIndex();
    addCode(a);
    addCode(b);
    return tmp;
}

int dsp_DATA4(int a,int b, int c, int d){
    checkInParamNum();
    int tmp = opcodeIndex();
    addCode(a); addCode(b);
    addCode(c); addCode(d);
    return tmp;
}
int dsp_DATA6(int a,int b, int c, int d, int e, int f){
    checkInParamNum();
    int tmp = opcodeIndex();
    addCode(a); addCode(b);
    addCode(c); addCode(d);
    addCode(e); addCode(f);
    return tmp;
}
int dsp_DATA8(int a,int b, int c, int d, int e, int f, int g, int h){
    checkInParamNum();
    int tmp = opcodeIndex();
    addCode(a); addCode(b); addCode(c); addCode(d);
    addCode(e); addCode(f); addCode(g); addCode(h);
    return tmp;
}

// DSP_LOAD_STORE
void dsp_LOAD_STORE(){  // this function must be followed by couples of data (input & output)
    setLastMissingParamIf0(0, 2);   // alway expect the parameters to be provided in the following opcode
    addOpcodeLength(DSP_LOAD_STORE);
}

void dspLoadStore_Data(int memin, int memout){
    checkLastMissing(DSP_LOAD_STORE);
    checkIOmax(memin);
    checkIOmax(memout);
    addCode(memin);
    addCode(memout);
}

static void addMemLocation(int index, int base){
    int space = 1 + dspFormat64();
    checkInParamSpace(index, space);
    addCodeOffset(index, base);
}

// load a meory location from a PARAM area
void dsp_LOAD_MEM(int paramAddr) {
    int tmp = addOpcodeLength(DSP_LOAD_MEM);
    addMemLocation(paramAddr, tmp);
    dspprintf3("DSP_LOAD_MEM [%d]\n",paramAddr);
}

void dsp_STORE_MEM(int paramAddr) {
    int tmp = addOpcodeLength(DSP_STORE_MEM);
    addMemLocation(paramAddr, tmp);
    dspprintf3("DSP_STORE_MEM [%d]\n",paramAddr)
}

// generate the space inside the PARAM area for the futur LOAD/STORE_MEM
int dspMem_Location() {
    checkInParamNum();  // check if we are in a PARAM or PARAM_NUM section
    int space = 1;
    if (dspFormat64()) { // if ALU is not 32 bits (then 64 bits needing 8byte allignement)
        opcodeIndexAligned8();
        space = 2; }
    int tmp = opcodeIndex();
    opcodeIndexAdd(space);
    return tmp;
}

// generate a DELAY instruction, parameter defined in PARAM space

static void dsp_DELAY_(int paramAddr, int opcode){
    checkInParamSpace(paramAddr, 1);
    int tmp = addOpcodeLength(opcode);
    int size = opcodePtr(paramAddr)->s16.high;   // get max delay line in samples
    //opcodePtr(paramAddr)->s16.high = 0;        // cleanup parameter and only keep LSB (default delay value in uSec)
    addCode(size);                          // store the max size of the delay line for runtime to check due to user potential changes
    if (opcode == DSP_DELAY_DP) {
        addDataSpaceMisAligned8(size*2+1);  // now we can request the data space
    } else
        addDataSpace(size+1);
    addCodeOffset(paramAddr, tmp);          // point on where is the delay in uSec
}

void dsp_DELAY(int paramAddr){
    dsp_DELAY_(paramAddr, DSP_DELAY);
}

#if (DSP_FORMAT == DSP_FORMAT_INT64)
// exact same as above but double precision
void dsp_DELAY_DP(int paramAddr){
    dsp_DELAY_(paramAddr, DSP_DELAY_DP);
}
#endif

static const int dspTableFreq[FMAXpos] = {
        8000, 16000,
        24000, 32000,
        44100, 48000,
        88200, 96000,
        176400,192000,
        352800,384000,
        705600, 768000 };


// genertae one word code combining the default uS value in LSB and with the max value in MSB
static int dspDelay_MicroSec(unsigned short maxus, unsigned short us){
    if (us<0) us = -us;
    checkInParamNum();  // check if we are in a PARAM or PARAM_NUM section
    signed long long maxSamples = (((signed long long)maxus * dspTableFreq[dspMaxSamplingFreq] + 500000)) / 1000000;
    if (maxSamples > 16000) dspFatalError("delay too large.");  // arbitrary value in this code version TODO
    int tmp = opcodeIndex();
    addOpcodeValue(maxSamples, us);   // temporary storage of the maxsamples
    return tmp;
}

int dspDelay_MicroSec_Max(int maxus){
    return dspDelay_MicroSec(maxus, maxus);
}

int dspDelay_MicroSec_Max_Default(int maxus, int us){
    return dspDelay_MicroSec(maxus, us);
}

int dspDelay_MilliMeter_Max(int maxmm, int speed){    // speed in meter per sec
    return dspDelay_MicroSec(maxmm * 1000 / speed, maxmm * 1000 / speed);
}

int dspDelay_MilliMeter_Max_Default(int maxmm, int mm, int speed){    // speed in meter per sec
    return dspDelay_MicroSec_Max_Default(maxmm * 1000 / speed, mm * 1000 / speed);
}


#define dspDelayFactor 4294.967296  // 2^32/10^6
const unsigned int dspTableDelayFactor[FMAXpos] = {
        dspDelayFactor*8000, dspDelayFactor*16000,
        dspDelayFactor*24000, dspDelayFactor*32000,
        dspDelayFactor*44100, dspDelayFactor*48000,
        dspDelayFactor*88200, dspDelayFactor*96000,
        dspDelayFactor*176400,dspDelayFactor*192000,
        dspDelayFactor*352800,dspDelayFactor*384000,
        dspDelayFactor*705600, dspDelayFactor*768000
};

static void dsp_DELAY_FixedMicroSec_(unsigned short microSec, int opcode){
    int DP = 1;
    if (opcode == DSP_DELAY_DP) DP = 2;
    addOpcodeLength(opcode);
    unsigned long long delayLineFactor = dspTableDelayFactor[dspMaxSamplingFreq];
    unsigned long long maxSamples_ = (delayLineFactor * microSec);
    maxSamples_ >>= 32;
    unsigned maxSamples = maxSamples_;
    dspprintf2("DELAY microsec %d , maxsample %d\n",microSec, maxSamples)
    addCode(microSec);  // store the expected delay in uSec
    if (DP == 1 ) addDataSpace(maxSamples+1); // request data space (including index) and store the pointer
    else addDataSpaceMisAligned8(maxSamples*2+1);
    addCode(0); // this indicate that this is a fixed delay line.
}

void dsp_DELAY_FixedMicroSec(int microSec){
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY);
}
void dsp_DELAY_FixedMilliMeter(int mm,int speed){
    dsp_DELAY_FixedMicroSec(mm * 1000 / speed);
}

#if (DSP_FORMAT == DSP_FORMAT_INT64)
void dsp_DELAY_DP_FixedMicroSec(int microSec){
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY_DP);
}
void dsp_DELAY_DP_FixedMilliMeter(int mm,int speed){
    dsp_DELAY_DP_FixedMicroSec(mm * 1000 / speed);
}
#endif

//DSP_DATA_TABLE

void dsp_DATA_TABLE(int dataPtr, dspGainParam_t gain, int divider, int size){
    setLastMissingParamIf0(dataPtr,size+4);
    int tmp = addOpcodeLength(DSP_DATA_TABLE);
    addGainCodeQ31(gain);
    addCode(divider);
    addCode(size);
    addDataSpace(1); //index
    addCodeOffset(dataPtr, tmp);
}

/*
 * MACC & MUX0DB
 *
 */

//DSP_MACC
int dspParamMaccMux_Data(int memin, dspGainParam_t gain, int opcode){
    checkIOmax(memin);
    int next = nextParamSection(opcode);
    int tmp = addCode(memin);
    addGainCodeQNM(gain);
    if (next == 0) lastIndexPrinted = opcodeIndex();
    return tmp;
}

int dspMacc_Inputs(int number){
    startParamSection(DSP_MACC, number);
    return addOpcodeValue(DSP_MACC, number);
}

int dspMacc_Data(int memin, dspGainParam_t gain){
    return dspParamMaccMux_Data(memin, gain, DSP_MACC);
}

void dsp_MACC_MUX0DB(int paramAddr, int opcode){   // this function must be followed by couples of data (input & gain in qnm ). num is number of couples (data/2)
    checkInParamSpace(paramAddr, 2);         // mac matrix only stored in param section
    if (opcodePtr(paramAddr)->op.opcode != opcode)
        dspFatalError("Number of section not provided in the PARAM.");

    int tmp = addOpcodeLength(opcode);
    addCodeOffset(paramAddr, tmp);
}

void dsp_MACC(int paramAddr){
    dsp_MACC_MUX0DB(paramAddr, DSP_MACC);
}

//DSP_MUX0DB
// same as MACC
int dspMux0DB_Inputs(int number){
    startParamSection(DSP_MUX0DB, number);
    return addOpcodeValue(DSP_MUX0DB, number);
}

int dspMux0DB_Data(int memin, dspGainParam_t gain){
    return dspParamMaccMux_Data(memin, gain, DSP_MUX0DB);
}

void dsp_MUX0DB(int paramAddr, int num){ // this function must be followed by couples of data (input & gain in qnm ). num is number of couples (data/2)
    dsp_MACC_MUX0DB(paramAddr, DSP_MUX0DB);
}


/*
 * BIQUAD Related
 */

// user function to define the start of a biquad section containg coefficient.
// e.g.     int myBQ = dspBiquadSection(2);
//
int dspBiquad_Sections(int number){
    startParamSection(DSP_BIQUADS, number); // check and initialize conditions for the follwoing data in the PARAM section
    int pos = opcodeIndex();
    addOpcodeValue(DSP_BIQUADS, number);    // store the number of following sections
    return pos;
}

void dsp_BIQUADS(int paramAddr){  // number of biquads and adress of the coefs (0 means next from here)
    checkInParamSpace(paramAddr,1+8*numberFrequencies);  // biquad coef are only store in param section
    if (opcodePtr(paramAddr)->op.opcode != DSP_BIQUADS)
        dspFatalError("Error in providing the biquad section adress.");
    int num = opcodePtr(paramAddr)->s16.low;  // get number of sections provided
    checkInParamSpace(paramAddr,1+8*numberFrequencies*num);
    int base = addOpcodeLength(DSP_BIQUADS);
    dspprintf2("DSP_BIQUADS (%d)\n",num);
    if (dspFormatDouble())
        addDataSpaceAligned8(num*8);    // 2 words for each data (xn-1, xn-2, yn-1, yn-2)
    else addDataSpaceAligned8(num*4);   // space for standard state data : 4  words xn-1,xn-2,yn-1,yn-2
    addCodeOffset(paramAddr, base);     // store pointer on the first bunch of (alligned) coefficients
}

void sectionBiquadCoeficientsBegin(){
    nextParamSection(DSP_BIQUADS);
}

void sectionBiquadCoeficientsEnd(){
    if (lastSectionNumber == 0) // last section of biquad
        // cancell printing of coeeficients, or pint them in another way in a later version..
        lastIndexPrinted = opcodeIndex();  // store the latest Index that have been already printed so far (might not be an opcode
}
int addFilterParams(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain){
    int tmp = addOpcodeValue(type, freq);
    addFloat(Q);
    addFloat(gain);
    return tmp;
}

int addBiquadCoeficients(dspFilterParam_t b0,dspFilterParam_t b1,dspFilterParam_t b2,dspFilterParam_t a1,dspFilterParam_t a2){

    int tmp = opcodeIndex();
    if (dspFormatInt()) {   // integer alu
        addCode(DSP_QNMBQ(b0));
        addCode(DSP_QNMBQ(b1));
        addCode(DSP_QNMBQ(b2));
        addCode(DSP_QNMBQ(a1));
        addCode(DSP_QNMBQ(a2));
    } else {
        addFloat(b0);
        addFloat(b1);
        addFloat(b2);
        addFloat(a1);
        addFloat(a2);
    }
    return tmp;
}



int dspFir_Impulses(){
    startParamSection(DSP_FIR, numberFrequencies);
    int pos = opcodeIndexMisAligned8();
    addOpcodeValue(DSP_FIR, numberFrequencies);
    return pos;
}

// create an opcode for executing a fir filter based on several impulse located at "paramAddr"
// minFreq and maxFreq informs on the number of impulse and supported frequencies (should require SRC for other frequencies...in later version)
int dsp_FIR(int paramAddr){    // possibility to restrict the number of impulse, not all frequencies covered

    int end = checkInParamSpace(paramAddr,2*numberFrequencies);

    int tableFreq[FMAXpos];

    int base = addOpcodeLength(DSP_FIR);
    if (opcodePtr(paramAddr++)->op.opcode != DSP_FIR)
        dspFatalError("Error in providing Fir impulse section address.");

    int lengthMax = 0;

    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {                           // screen the pointed area to create the list of offset for each freq
        int length = opcodePtr(paramAddr)->s16.low;      // first code is the length of the next impulse
        int delay = opcodePtr(paramAddr)->s16.high;      // or the delay
        if (delay) {
            delay++;                                // +1 because the first data stored in the delay line is the current index position
            length = 1;
            if (delay > lengthMax)
                lengthMax = delay;                  // calculate filter maximum lenght of data regarded all possible frequencies
        } else
            if (length > lengthMax)
                lengthMax = length;                 // calculate filter maximum lenght regarded all possible frequencies
        tableFreq[f] = paramAddr++;                 // store adress of this impulse array
        paramAddr += length;                        // going through next impulse
        if ((paramAddr & 1) == 0) paramAddr++;      // padd according to how this is supposed to be stored
        if (paramAddr >= end)
           dspFatalError("FIR Impulse list goes outside of PARAM section.(encoder bug?)");
        }
    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ )
        // generate list of offset for each frequency supported by the dsp runtime
        addCodeOffset(tableFreq[f], base);
    addDataSpace(lengthMax);                    // request a data space in the data area corresponding to the largest impulse discovered
    return base;
}


// generate a 2 opcode sequence <1> <0> if the fir shall not be executed, otherwise genere a vale corresponding to delay like for DEAL instruction
int dspFir_Delay(int value){            // to be used when a frequency is not covered by a proper impulse
    nextParamSection(DSP_FIR);
    int pos = opcodeIndexMisAligned8(); // represent a dummy Impulse, so should be padded 8 bytes
    if (value > 1) {
        addOpcodeValue(value, 0);           // store the expected delay (in samples) in msb, same format as for DELAY opcode
    } else
        addCode(1);
    addCode(0);
    return pos;
}

// load an Impulse file (text file with each coef as a float parameter on each line)
// length is the maximum expected size of the impulse in number of taps
int dspFir_ImpulseFile(char * name, int length){ // max lenght expected
    nextParamSection(DSP_FIR);
    int pos = opcodeIndexMisAligned8();
#ifdef DSP_FILEACCESS_H_
    dspFileName = name;
    if ((opcodeIndex() + length) >= dspOpcodesMax)
        dspFatalError("OpcodeTable too small for this impulse file.");
    if (-1 == dspfopenRead("r"))
        dspFatalError("cant open impulse file.");

    int old = addCode(length);
    float * codePtr = (float *)opcodeIndexPtr();
    int tmp = dspfreadImpulse(codePtr, length);
    if (tmp == -1) {
        dspfclose();
        dspFatalError("problem while reading the Impulse file.");
        }
    if (tmp != length)
        dspFatalError("Impulse file too small or access problem.");
    opcodeIndexAdd(length);   // comit size of the impulse
#else
    dspFir_Delay(1);
    dspprintf2("simulating impulse file with a dspFir_Delay(1).")
#endif
    return pos;
}

