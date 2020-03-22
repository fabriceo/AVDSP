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

static int lastOpcodePrint       =  0;      // point on the last opcode generated, ready for printing following code
static int lastIndexPrinted      =  0;      // opaque...

static int lastOpcodeIndexLength = -1;      // point on the latest opcode requiring a quantity of code space not yet known
static int dspDataCounter        =  0;      // point on the next data adress available (relative to begining of data)

static int lastParamNumIndex     =  0;      // index position in the opcode table of the latest ongoing PARAM_NUM or PARAM

static int lastMissingParamIndex =  0;      // index where a parameter is expected to follow
static int lastMissingParamSize  =  0;      // expected minimum size of total codelength for the opcode (verified in calclength)

static int dspDumpStarted        =  0;      // as soon as a dsp_dump is executed, this is set to 1

static int lastSectionOpcode     =  0;      // opcode associated with the latest section declaration
static int lastSectionNumber     =  0;      // number expected of data for the started section
static int lastSectionCount      =  0;      // incremented number each time a dataset is encountered
static int lastSectionIndex      =  0;      // value of the opcode index when a new section was started

static int maxOpcodeValue        =  0;      // represent the higher opcode value used in the encoded program

static int usedInputs            =  0;      // bit patern of all the inputs used by a LOAD command or LOAD_MUX or LOAD_GAIN
static int usedOutputs           =  0;      // bit patern of all the output used by a STORE command

static int ALUformat             =  0;      // represent the current format of the ALU known at compile time 0 = 32bits, 1 = double precision

static int dspFormat;                       // dynamic management of the different type of data when encoding
static int dspIOmax;                        // max number of IO that can be used with Load & Store (to avoid out of boundaries vs samples table)
static int numberFrequencies;               // number of covered frequencies (mainly used in BIQUADS and FIR)

int dspMinSamplingFreq = DSP_DEFAULT_MIN_FREQ;
int dspMaxSamplingFreq = DSP_DEFAULT_MAX_FREQ;

char * dspOpcodeText[DSP_MAX_OPCODE] = {
    "DSP_END_OF_CODE",
    "\nDSP_HEADER",
    "DSP_NOP",
    "\nDSP_CORE",
    "\nDSP_PARAM",
    "\nDSP_PARAM_NUM",
    "DSP_SERIAL",
    "DSP_TPDF",
    "DSP_WHITE",
    "DSP_CLRXY",
    "DSP_SWAPXY",
    "DSP_COPYXY",
    "DSP_COPYYX",
    "DSP_ADDXY",
    "DSP_ADDYX",
    "DSP_SUBXY",
    "DSP_SUBYX",
    "DSP_MULXY",
    "DSP_DIVXY",
    "DSP_DIVYX",
    "DSP_AVGXY",
    "DSP_AVGYX",
    "DSP_NEGX",
    "DSP_NEGY",
    "DSP_SQRTX",
    "DSP_VALUE",
    "DSP_SHIFT",
    "DSP_MUL_VALUE",
    "DSP_DIV_VALUE",
    "DSP_LOAD",
    "DSP_LOAD_GAIN",
    "DSP_LOAD_MUX",
    "DSP_STORE",
    "DSP_LOAD_STORE",
    "DSP_LOAD_MEM",
    "DSP_STORE_MEM",
    "DSP_GAIN",
    "DSP_SAT0DB",
    "DSP_SAT0DB_TPDF",
    "DSP_SAT0DB_GAIN",
    "DSP_SAT0DB_TPDF_GAIN",
    "DSP_DELAY_1",
    "DSP_DELAY",
    "DSP_DELAY_DP",
    "DSP_DATA_TABLE",
    "DSP_BIQUADS",
    "DSP_FIR",
    "DSP_RMS",
    "DSP_DCBLOCK",
    "DSP_DITHER",
    "DSP_DITHER_NS2",
    "DSP_DISTRIB"
};

void dspprintfFatalError(){
    dspprintf("FATAL ERROR : ");
}
#define dspFatalError(...)  { dspprintfFatalError(); dspprintf(__VA_ARGS__); dspprintf("\n"); exit(1); }


// return the current index position in the opcode table
int opcodeIndex() {
    asm volatile("":::"memory"); // memory barier to avoid code reschuffling
    return dspOpcodeIndex;
}
// create a space in the opcode table
static int opcodeIndexAdd(int add) {
    int tmp = opcodeIndex();
    if ((tmp+add) > dspOpcodesMax)   // boundary check
        dspFatalError("YOUR DSP CODE IS TOO LARGE FOR THE ARRAY PROVIDED");
    dspOpcodeIndex += add;
    return tmp;
}
// return an absolute pointer within the opcode table
static opcode_t * opcodePtr(int index){
    asm volatile("":::"memory"); // memory barier to avoid code reschuffling
    return dspOpcodesPtr + index;
}

// return an absolute pointer on the current index position in the opcode table
static opcode_t * opcodeIndexPtr(){
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
    return addCode((code << 16) | (value & 0xFFFF));
}




// add an opcode with a 0 placeholder value for the code lenght below (never possible). Will be solved by calcLenght
static int addOpcodeUnknownLength(int code){
    lastOpcodeIndexLength = addOpcodeValue(code , 0);    // memorize Index where we ll need to update the codelenght
    return lastOpcodeIndexLength;
}

// add a code in the opcode table (at current index)
// pointing on an adress within the codetable (typically PARAM_NUM)
// and encode it as relative value to the given "base" parameter
// which is usually the index pointing on the previous dsp_opcode adresses
// if 0 is provided then it shall mean the adress pointed is just here after this code
static int addCodeOffset(int index, int base){
    int offset;
    if (index) offset = index - base;           // calculate relative value to "base"
    else offset = opcodeIndex() +1 - base;      // calculate the relative value to "base" of the next opcode
    return addCode(offset);                     // store the value calculated
}


// add a word in the opcode table reresenting a quantity of data required and allocated in the data space
static void addDataSpace(int size) {
    addCode(dspDataCounter);              // store the current data index value pointing on the next spare data space
    dspDataCounter += size;               // simulate consumption the expected data space
}
// same as above but push the data index by one if needed
// so that the data adress is alligned on 8 bytes boundaries
static void addDataSpaceAligned8(int size) {
    if(dspDataCounter & 1) dspDataCounter++;
    addDataSpace(size);                 // store the current data index
}

static void addDataSpaceMisAligned8(int size) {
    if((dspDataCounter & 1) == 0) dspDataCounter++;
    addDataSpace(size);                 // store the current data index
}

static void printFromCurrentIndex(){
    lastIndexPrinted = opcodeIndex();
}

// for debugging purpose, print all the opcode generated since the latest dsp_opcode
static void printLastOpcodes() {
    if (lastIndexPrinted < lastOpcodePrint) {
        dspprintf3("%4d : ",lastIndexPrinted);
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
            else dspprintf3("%d(%d) ",val,lastOpcodePrint+val);
        }
        dspprintf3("\n");
    }
    printFromCurrentIndex();
}


// verify if we are within a PARAM or PARAM_NUM section
static void checkInParamNum(){
    if (lastParamNumIndex == 0)
        dspFatalError("Currently not in a PARAM or PARAM_NUM space.");
}


// check if the last section opened shall be closed properly before opening a new one
static void checkFinishedParamSection(){
    if (lastSectionOpcode) {
        if (lastSectionNumber > 0)
            dspFatalError("Section already started and not finished.");
        // a section is finished and the opcode has not been reseted so we must fill the first byte with some info
        opcode_t *first = opcodePtr(lastSectionIndex);
        int code = first->op.opcode;
        switch(code){
        case DSP_BIQUADS: {
            dspprintf2("-> %d biquad cell(s) provided\n",lastSectionCount)
            first->s16.low = lastSectionCount;
            lastSectionOpcode = 0;  // now finished properly
            printFromCurrentIndex();
            break;
        }
        case DSP_LOAD_MUX:{
            printLastOpcodes();
            dspprintf2("-> %d couple(s) provided\n",lastSectionCount)
            first->s16.low = lastSectionCount;
            lastSectionOpcode = 0;  // now finished properly
            break;
        }
        case DSP_FIR: {
            if (lastSectionCount != numberFrequencies)
                dspFatalError("Missing impulse in the fir param section.");
            break;
        }
        }
    }
}

// start a new section in the param num area
static int startParamSection(int opcode, int num){
    checkFinishedParamSection();
    printLastOpcodes();             // flush any opcode printing before starting with new datasets
    checkInParamNum();              // verify that we are inside a started PARAM or PARAM_NUM section
    lastSectionOpcode = opcode;
    lastSectionNumber = num;
    lastSectionCount  = 0;
    lastSectionIndex  = opcodeIndex();
    return lastSectionIndex;
}

static void checkParamSection(int opcode){
    checkInParamNum();
    if (lastSectionOpcode == 0)
        dspFatalError("No section defined or started.");
    if (opcode)
        if (lastSectionOpcode != opcode)
            dspFatalError("Section already started for another opcode.");
}

// shall be used after one section is newly created after startParamSection
static int nextParamSection(int opcode){
    checkParamSection(opcode);
    lastSectionCount++;
    if (lastSectionNumber>0) {
        lastSectionNumber--;
        if (lastSectionNumber == 0) { lastSectionOpcode = 0; } // number of expected section reached
    } else
        if (lastSectionNumber == 0) {
            // flexible so this request is accepted
        } else { // negative number
            if (lastSectionCount > (-lastSectionNumber))
                dspFatalError("too much parameters in this section.");
        }
    return lastSectionOpcode;
}


// define the minimum expected size of code after the current dsp_opcode
// used when a parameter is not provided with its adress in a PARAM space
// then the data are expected to be stored just below
static void setLastMissingParam(int size){
    lastMissingParamIndex  = opcodeIndex();
    lastMissingParamSize = size;
}

static void setLastMissingParamIf0(int paramAddr, int size){
    if (paramAddr == 0) setLastMissingParam(size);
}



// calculate the number of words till the latest call to addOpcodeUnknownLength
// and store it in the LSB16 of the latest dsp_opcode generated
// calclength is called first by all the user dsp_XXX function
static void calcLength(){
    asm volatile("nop":::"memory"); // memory barier to avoid code reschuffling
    if (dspOpcodesPtr == 0)
        dspFatalError("dspEncoderInit has not been launched first.");   // sanity check
    if (lastParamNumIndex) {   // if we were in a Param Num
        checkFinishedParamSection();
        lastParamNumIndex = 0;  // as we are now going to generate a new dsp_opcode then we close the latest PARAM_NUM
    }
    if (lastMissingParamIndex != 0) {  // check if there was a requirement for a minimum code size below the latest dsp_opcode generated
        // check size between now and prev pointer
        int size = opcodeIndex() - lastMissingParamIndex; // represent all the words geenrated below the latest dsp_opcode
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
    lastSectionIndex      = 0;
    lastSectionNumber     = 0;
    lastSectionCount      = 0;
    dspDumpStarted = 0;
    ALUformat      = 0; // by default we consider to be single precision with ALU containing a 0.31 value

    usedInputs = 0;
    usedOutputs = 0;

    addOpcodeUnknownLength(DSP_HEADER);
    opcodeIndexAdd(sizeof(dspHeader_t)/sizeof(int) - 1);
    dspHeaderPtr->totalLength = 0;
    dspHeaderPtr->dataSize  = 0;
    dspHeaderPtr->checkSum  = 0;
    dspHeaderPtr->numCores  = 0;
    dspHeaderPtr->version   = DSP_ENCODER_VERSION;
    dspHeaderPtr->maxOpcode = DSP_MAX_OPCODE-1;
    dspHeaderPtr->freqMin   = minFreq;
    dspHeaderPtr->freqMax   = maxFreq;
    dspHeaderPtr->usedInputs  = 0;
    dspHeaderPtr->usedOutputs = 0;
}

// see DSP_FORMAT in dsp_runtime.h
static int dspFormat64(){
    // return 1 if the DSP ALU is 64bits size (int64 or double)
    return (dspFormat == 2)||(dspFormat == 4)||(dspFormat == 6);
}
static int dspFormatInt(){
    // return 1 if the DSP ALU is integer
    return (dspFormat == 1)||(dspFormat == 2);
}
/* not used
static int dspFormatInt64(){
    // return 1 if the DSP ALU is integer 64bits size
    return (dspFormat == 2);
}
*/
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
        int code = cptr->op.opcode;
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
        int code = cptr->op.opcode;
        int skip = cptr->op.skip;
        int add = 0;
        if (code == DSP_PARAM)     add = 1; // position of the first parameter
        if (code == DSP_PARAM_NUM) add = 2; // position of the first parameter following the PARAM_NUM value
        if (add) {
            begin = pos + add;
            end = (skip) ? (pos + skip) : opcodeIndex();   // if length not yet calculated, we are just within a PARAM NUM sequence
            if ((index >= begin) && (index < end )) { // within this data space
                if ( maxIndex < end )
                    return (begin <<16) | end ;   // great fit
                dspFatalError("memory space expected is too large for this PARAM or PARAM_NUM."); }
        } // continue searching
        if (skip == 0) {
            dspFatalError("Index provided not found in any PARAM or PARAM_NUM space."); }
        pos += skip;
    }
    // unreachable return 0;
}

static int checkInParamSpaceOpcode(int index, int size, int opcode){
    if (opcode)
        if (opcodePtr(index)->op.opcode != opcode)
            dspFatalError("the parameter adress is not pointing on a proper section of data %d.",opcodePtr(index)->op.opcode);
    return checkInParamSpace(index, size);
}


static void checkInRange(int val,int min, int max){
    if ((val<min)||(val>max))
        dspFatalError("value not in expected range");
}

// check that the provided load/store location is within the IOmax range defined in the encoderInit
static void checkIOmax(int IO){
    if ((IO < 0)||(IO >= dspIOmax))
        dspFatalError("IO out of range.");
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
    dspprintf1("dsptotallength = %d\n",opcodeIndex());
    dspHeaderPtr->dataSize = dspDataCounter;
    dspprintf1("dataSize       = %d\n",dspDataCounter);
    // now calculate the simplified checksum of all the opcodes and count number of cores
    unsigned int sum;
    int numCore;
    dspCalcSumCore(opcodePtr(0), &sum, &numCore);
    dspHeaderPtr->checkSum = sum;           // comit checksum
    dspprintf1("check sum      = %d\n", sum);
    if (numCore == 0) numCore = 1;
    dspHeaderPtr->numCores = numCore;       // comit number of declared cores
    dspprintf1("numcore        = %d\n",numCore);
    dspHeaderPtr->maxOpcode   = maxOpcodeValue;
    dspHeaderPtr->usedInputs  = usedInputs;
    dspHeaderPtr->usedOutputs = usedOutputs;
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
static int addSingleOpcode(int code) {
    calcLength();
    return addOpcodeValue(code, 1);
}

// add a single dsp_code without any following parameters
static int addSingleOpcodePrint(int code) {
    int tmp = addSingleOpcode(code);
    dspprintf2("%s\n",dspOpcodeText[code]);
    return tmp;
}


// return the current index position in the opcode table and potentially insert one NOP code for padding to 8 bytes
int opcodeIndexAligned8() {
    if (opcodeIndex() & 1) addSingleOpcode(DSP_NOP);
    return opcodeIndex();
}
// same but to be used when the padding is needed just after a next parameter to come
int opcodeIndexMisAligned8() {
    if ((opcodeIndex() & 1) == 0) addSingleOpcode(DSP_NOP);
    return opcodeIndex();
}


// return the current index position in the opcode table and potentially insert one NOP code for padding to 8 bytes
static int paramAligned8() {
    if (opcodeIndex() & 1) addCode(0);
    return opcodeIndex();
}
// same but to be used when the padding is needed just after a next parameter to come
static int paramMisAligned8() {
    if ((opcodeIndex() & 1) == 0) addCode(0);
    return opcodeIndex();
}



/* unused
// add a dsp_code with its following parameter
static int addOpcodeParam(int code, int param ) {
    calcLength();
    int tmp = addOpcodeValue(code, 2);
    addCode(param);
    return tmp;
}
*/

// add a dsp_opcode that will be followed by an unknowed list of param at this stage
// will be solved by the next dsp_opcode generation, due to call to calcLength()
static int addOpcodeLength(int code) {
    calcLength();
    return addOpcodeUnknownLength(code);
}

static int addOpcodeLengthPrint(int code){
    int tmp = addOpcodeLength(code);
    dspprintf2("%s\n",dspOpcodeText[code]);
    return tmp;
}

static int addGainCodeQNM(dspGainParam_t gain){
    if (dspFormatInt())
         return addCode(DSP_QNM(gain));
    else return addFloat(gain);
}

// indicate No operation
void dsp_NOP() { addSingleOpcodePrint(DSP_NOP); }

// indicate start of a program for a dedicated core/task
void dsp_CORE(){
    addSingleOpcodePrint(DSP_CORE);
    ALUformat = 0;  // reset it as we start a new core
}

// clear ALU X and Y
void dsp_CLRXY(){ addSingleOpcodePrint(DSP_CLRXY); }

// exchange ALU X and Y
void dsp_SWAPXY(){ addSingleOpcodePrint(DSP_SWAPXY); }

// copy ALU X to Y
void dsp_COPYXY(){ addSingleOpcodePrint(DSP_COPYXY); }

// copy ALU Y to X
void dsp_COPYYX(){ addSingleOpcodePrint(DSP_COPYYX); }

// perform ALU X = X + Y
void dsp_ADDXY(){ addSingleOpcodePrint(DSP_ADDXY); }

// perform ALU2 Y = X + Y
void dsp_ADDYX(){ addSingleOpcodePrint(DSP_ADDYX); }

// perform ALU X = X - Y
void dsp_SUBXY(){ addSingleOpcodePrint(DSP_SUBXY); }

// perform ALU Y = Y - X
void dsp_SUBYX(){ addSingleOpcodePrint(DSP_SUBYX); }

// perform ALU X = X * Y
void dsp_MULXY(){ addSingleOpcodePrint(DSP_MULXY); }

// perform ALU X = X / Y
void dsp_DIVXY(){ addSingleOpcodePrint(DSP_DIVXY); }

// perform ALU2 Y = Y / X
void dsp_DIVYX(){ addSingleOpcodePrint(DSP_DIVYX); }

// perform ALU X = X / Y
void dsp_AVGXY(){ addSingleOpcodePrint(DSP_AVGXY); }

// perform ALU2 Y = Y / X
void dsp_AVGYX(){ addSingleOpcodePrint(DSP_AVGYX); }

// perform ALU X = sqrt(X)
void dsp_SQRTX(){ addSingleOpcodePrint(DSP_SQRTX); }

void dsp_NEGX(){ addSingleOpcodePrint(DSP_NEGX); }

void dsp_NEGY(){ addSingleOpcodePrint(DSP_NEGY); }

void dsp_WHITE() { addSingleOpcodePrint(DSP_WHITE); }

void dsp_SAT0DB() {
    addSingleOpcodePrint(DSP_SAT0DB);
    ALUformat = 0;
}

void dsp_SAT0DB_TPDF() {
    addSingleOpcodePrint(DSP_SAT0DB_TPDF);
    ALUformat = 0;
}

void dsp_SAT0DB_TPDF_GAIN_(int paramAddr, int tpdf){
    ALUformat = 0;
    int tmp;
    if (tpdf) tmp = addOpcodeLengthPrint(DSP_SAT0DB_TPDF_GAIN);
    else      tmp = addOpcodeLengthPrint(DSP_SAT0DB_GAIN);
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);   // possibility to define the gain just below the opcode
}

void dsp_SAT0DB_TPDF_GAIN(int paramAddr){
    dsp_SAT0DB_TPDF_GAIN_(paramAddr,1);
}

void dsp_SAT0DB_GAIN(int paramAddr){
    dsp_SAT0DB_TPDF_GAIN_(paramAddr,0);
}

void dsp_SAT0DB_GAIN_Fixed(dspGainParam_t gain){
    dsp_SAT0DB_TPDF_GAIN_(0,0);
    addGainCodeQNM(gain);
}

void dsp_SAT0DB_TPDF_GAIN_Fixed(dspGainParam_t gain) {
    dsp_SAT0DB_TPDF_GAIN_(0,1);
    addGainCodeQNM(gain);
}


void dsp_TPDF(int bits){
    opcodeIndexAligned8();  // garantiee that the 64 bits words below will be alligned8 bytes
    addOpcodeLengthPrint(DSP_TPDF);
    checkInRange(bits,2,32);
    bits = DSP_MANT+32-bits;    // eg 36 for DSPMANT = 28 and 24th bit ditering
    unsigned long long round = 1ULL << (bits-1);    // value (0.5) for rounding sample
    unsigned long long notMask  = ~((1ULL << bits)-1);
    unsigned factor;
    bits = bits-31; // because tpdf value is coded in s.31, so we get 5 for the 36 above
    if (bits>=0) factor = 1ULL<<bits;   // we can use a 32x32=64 mul for getting the tpdf scaled directly from the multiplication
    else factor = (1ULL<<(32+bits));    // we will divide still by using the 32x32 multiplication but keeping only the MSB so 32 bits less
    if (bits == -1) factor--;           // special case, cannot keep factor equal to 1<<31 as this becomes a negaive number in 2's
    addCode(factor);                    // provide both : factor value
    addCode(notMask & 0xFFFFFFFF);
    addCode(notMask >> 32);
    addCode(round & 0xFFFFFFFF);        // these value will be alligned8
    addCode(round >> 32);
    addCode(bits);                      // and bits representing number of shift to be performed
}


void dsp_SHIFT(int bits){
    addOpcodeLengthPrint(DSP_SHIFT);
    addCode(bits);
}
void dsp_SHIFT_FixedInt(int bits){
    dsp_SHIFT(bits);
}


/*
 *
 * LOAD
 *
 */

// load ALU with the physical location provided
void dsp_LOAD(int IO) {
    checkIOmax(IO);
    if (IO<32) usedInputs |= 1ULL<<IO;
    addOpcodeLengthPrint(DSP_LOAD);
    addCode(IO);
}

void dsp_LOAD_GAIN(int IO, int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_LOAD_GAIN);
    checkIOmax(IO);
    if (IO<32) usedInputs |= 1ULL<<IO;
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCode(IO);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);   // possibility to define the gain just below the opcode
}

void dsp_LOAD_GAIN_Fixed(int IO, dspGainParam_t gain) {
    dsp_LOAD_GAIN(IO, 0);
    addGainCodeQNM(gain);   // store the fixed gain just after the opcode
}


// load many inputs and apply a gain to them
void dsp_LOAD_MUX(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint( DSP_LOAD_MUX);
    checkInParamSpaceOpcode(paramAddr, 2, DSP_LOAD_MUX);  // IO-gain matrix only stored in param section
    addCodeOffset(paramAddr, tmp);
}

// must be used to start and list a group of IO-gain
// e.g. int myMuxMatrix = dspLoadMux_Inputs(2);
//          dspLoadMux_Data(1,0.5);
//          dspLoadMux_Data(2,0.5);
int dspLoadMux_Inputs(int number){
    startParamSection(DSP_LOAD_MUX, number);
    int pos = addOpcodeValue(DSP_LOAD_MUX, number);
    dspprintf3("\n%4d : LoadMux section\n",pos);
    return pos;
}
//to be used just below dspLoadMux_Inputs, as many time as defined
void dspLoadMux_Data(int in, dspGainParam_t gain){
    checkIOmax(in);
    if (in<32) usedInputs |= 1ULL<<in;
    int next = nextParamSection(DSP_LOAD_MUX);
    addCode(in);
    addGainCodeQNM(gain);
    if (next == 0) printFromCurrentIndex();
}

/*
 *
 * STORE
 *
 */


void dsp_STORE(int IO) {
    checkIOmax(IO);
    addOpcodeLengthPrint(DSP_STORE);
    addCode(IO);
    if (IO<32) usedOutputs |= 1ULL<<IO;
}


// check if we currently are below a specific opcode just generated
void checkLastMissing(int opcode){
    if (lastMissingParamIndex == 0)
        dspFatalError("no parameter expected here.");
    if (opcode)
        if (opcodePtr(lastMissingParamIndex - 1)->op.opcode != opcode)
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
    int tmp = addOpcodeLengthPrint(DSP_PARAM);
    lastParamNumIndex = tmp; // indicate that we are inside a PARAM_NUM statement
    return tmp;
}

// DSP_PARAM_NUM

int dsp_PARAM_NUM(int num) {
    int tmp = addOpcodeLengthPrint(DSP_PARAM_NUM);
    lastParamNumIndex = tmp;
    addCode(num);
    return tmp;
}


/*
 * GAIN
 */


void dsp_GAIN(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_GAIN);
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);   // possibility to define the gain just below the opcode
}

// can be used only in a param section
int dspGain_Default(dspGainParam_t gain){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = addGainCodeQNM(gain);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}


void dsp_GAIN_Fixed(dspGainParam_t gain){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_GAIN);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(gain);
}


void dsp_VALUE_Fixed(float value){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUE);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(value);
}
void dsp_VALUE_FixedInt(int value){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUE);
    addCodeOffset(0, tmp);  // value is just below
    addCode(value);
}

void dsp_VALUE(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUE);
    checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
}

int  dspValue_Default(float value){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = addGainCodeQNM(value);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}

int  dspValue_DefaultInt(int value){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = addCode(value);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}


void dsp_DIV_Fixed(float value){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_DIV_VALUE);
    addGainCodeQNM(value);
}
void dsp_DIV_FixedInt(int value){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_DIV_VALUE);
    addCode(value);
}

void dsp_MUL_Fixed(float value){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_MUL_VALUE);
    addGainCodeQNM(value);
}
void dsp_MUL_FixedInt(int value){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_MUL_VALUE);
    addCode(value);
}


void dsp_DELAY_1(){
    addOpcodeLengthPrint(DSP_DELAY_1);
    addDataSpaceAligned8(2);    // 2 words for supporting 64bits alu
}

// DSP_SERIAL
void dsp_SERIAL(int N) {
    addOpcodeLengthPrint(DSP_SERIAL);
    addCode(N);
    addCode(~N);
}

// can be used only in a param space for declaring a list of datas (for example to be used by DATA_TABLE)
int dspDataTableInt(int * data, int n){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = opcodeIndex();
    for (int i=0; i<n; i++) addCode(*(data+i));
    lastIndexPrinted = opcodeIndex();
    return tmp;
}

int dspDataTableFloat(float * data, int n){
    printLastOpcodes();
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = opcodeIndex();
    for (int i=0; i<n; i++) addCode(DSP_QNM(*(data+i)));
    printFromCurrentIndex();
    dspprintf2("%4d : data table : %d float numbers\n",tmp,n);
    return tmp;
}

int dspData2(int a,int b){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = opcodeIndex();
    addCode(a);
    addCode(b);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}

int dspData4(int a,int b, int c, int d){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = opcodeIndex();
    addCode(a); addCode(b);
    addCode(c); addCode(d);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}
int dspData6(int a,int b, int c, int d, int e, int f){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = opcodeIndex();
    addCode(a); addCode(b);
    addCode(c); addCode(d);
    addCode(e); addCode(f);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}
int dspData8(int a,int b, int c, int d, int e, int f, int g, int h){
    checkInParamNum();
    checkFinishedParamSection();
    int tmp = opcodeIndex();
    addCode(a); addCode(b); addCode(c); addCode(d);
    addCode(e); addCode(f); addCode(g); addCode(h);
    lastOpcodePrint = opcodeIndex();
    return tmp;
}

// DSP_LOAD_STORE
void dsp_LOAD_STORE(){  // this function must be followed by couples of data (input & output)
    ALUformat = 0;
    addOpcodeLengthPrint(DSP_LOAD_STORE);
    setLastMissingParam(2);   // alway expect the parameters to be provided in the following opcode, at least 2 words
}

void dspLoadStore_Data(int in, int out){
    checkLastMissing(DSP_LOAD_STORE);       // verify that a dsp_LOAD_STORE() is just above
    checkIOmax(in);
    checkIOmax(out);
    addCode(in);
    addCode(out);
    if (in<32)  usedInputs  |= 1ULL<<in;
    if (out<32) usedOutputs |= 1ULL<<out;
}

static void addMemLocation(int index, int base){
    int space = 1 + dspFormat64();
    checkInParamSpace(index, space);
    addCodeOffset(index, base);
}

// load a meory location from a PARAM area
void dsp_LOAD_MEM_Index(int paramAddr, int index) {
    int tmp = addOpcodeLengthPrint(DSP_LOAD_MEM);
    addMemLocation(paramAddr + index*(dspFormat64()+1), tmp);
}

void dsp_STORE_MEM_Index(int paramAddr, int index) {
    int tmp = addOpcodeLengthPrint(DSP_STORE_MEM);
    addMemLocation(paramAddr  + index*(dspFormat64()+1), tmp);
}

// load a meory location from a PARAM area
void dsp_LOAD_MEM(int paramAddr) {
    dsp_LOAD_MEM_Index(paramAddr, 0);
}

void dsp_STORE_MEM(int paramAddr) {
    dsp_STORE_MEM_Index(paramAddr, 0);
}

// generate the space inside the PARAM area for the futur LOAD/STORE_MEM
int dspMem_LocationMultiple(int number) {
    checkFinishedParamSection();
    checkInParamNum();  // check if we are in a PARAM or PARAM_NUM section
    int space = 1;
    if (dspFormat64()) { // if ALU is not 32 bits (then 64 bits needing 8byte allignement)
        paramAligned8();
        space = 2; }
    int tmp = opcodeIndex();
    opcodeIndexAdd(space*number);
    return tmp;
}

// generate the space inside the PARAM area for the futur LOAD/STORE_MEM
int dspMem_Location() {
    return dspMem_LocationMultiple(1);
}

// generate a DELAY instruction, parameter defined in PARAM space

static void dsp_DELAY_(int paramAddr, int opcode){
    checkInParamSpace(paramAddr, 1);
    int tmp = addOpcodeLengthPrint(opcode);
    int size = opcodePtr(paramAddr)->s16.high;  // get max delay line in samples
    //opcodePtr(paramAddr)->s16.high = 0;       // cleanup parameter and only keep LSB (default delay value in uSec)
    addCode(size);                              // store the max size of the delay line for runtime to check due to user potential changes
    if (opcode == DSP_DELAY_DP)
         addDataSpaceMisAligned8(size*2+1);      // now we can request the data space
    else addDataSpace(size+1);
    addCodeOffset(paramAddr, tmp);              // point on where is the delay in uSec
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
    checkFinishedParamSection();
    signed long long maxSamples = (((signed long long)maxus * dspTableFreq[dspMaxSamplingFreq] + 500000)) / 1000000;
    if (maxSamples > 16000) dspFatalError("delay too large.");  // arbitrary value in this code version TODO
    return addOpcodeValue(maxSamples, us);   // temporary storage of the maxsamples and us in a single word
}

int dspDelay_MicroSec_Max(int maxus){
    return dspDelay_MicroSec(maxus, maxus);
}

int dspDelay_MicroSec_Max_Default(int maxus, int us){
    return dspDelay_MicroSec(maxus, us);
}

int dspDelay_MilliMeter_Max(int maxmm, float speed){    // speed in meter per sec
    return dspDelay_MicroSec(maxmm * 1000.0 / speed, maxmm * 1000.0 / speed);
}

int dspDelay_MilliMeter_Max_Default(int maxmm, int mm, float speed){    // speed in meter per sec
    return dspDelay_MicroSec_Max_Default(maxmm * 1000.0 / speed, mm * 1000.0 / speed);
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
    addOpcodeLengthPrint(opcode);
    unsigned long long delayLineFactor = dspTableDelayFactor[dspMaxSamplingFreq];
    unsigned long long maxSamples_ = (delayLineFactor * microSec);
    maxSamples_ >>= 32;
    unsigned maxSamples = maxSamples_;
    addCode(microSec);  // store the expected delay in uSec
    if (DP == 1 ) addDataSpace(1 + maxSamples); // request data space (including index) and store the pointer
    else addDataSpaceMisAligned8(1 + maxSamples*2);
    addCode(0); // this will indicate to runtime that this is a fixed delay line.
}

void dsp_DELAY_FixedMicroSec(int microSec){
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY);
}
void dsp_DELAY_FixedMilliMeter(int mm,float speed){
    dsp_DELAY_FixedMicroSec(mm * 1000.0 / speed);
}

#if (DSP_FORMAT == DSP_FORMAT_INT64)
void dsp_DELAY_DP_FixedMicroSec(int microSec){
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY_DP);
}
void dsp_DELAY_DP_FixedMilliMeter(int mm,float speed){
    dsp_DELAY_DP_FixedMicroSec(mm * 1000.0 / speed);
}
#endif

//DSP_DATA_TABLE

void dsp_DATA_TABLE(int paramAddr, dspGainParam_t gain, int divider, int size){
    ALUformat = 0;
    int tmp = addOpcodeLengthPrint(DSP_DATA_TABLE);
    if (paramAddr) checkInParamSpace(paramAddr, size);
    addGainCodeQNM(gain);   // gain applied to all samples of the table
    addCode(divider);       // e.g. if divider == 3 then take only 1 out of 3 value in the table
    addCode(size);          // total size of the table
    addDataSpace(1);        // addrss of an index in the data space area
    addCodeOffset(paramAddr, tmp);    // pointer to the data table
    setLastMissingParamIf0(paramAddr,size);
}

int dspGenerator_Sine(int samples){
    checkInParamNum();              // check if we are in a PARAM or PARAM_NUM section
    checkFinishedParamSection();    // verify if there is an ongoing section started
    int tmp = opcodeIndex();
    checkInRange(samples,4,1024);
    dspprintf3("dspGenerator : 2.PI sinewave in %d values\n",samples);
    for (int i=0; i<samples; i++) {
        double x = sin((2.0*M_PI * (double)i)/(double)samples);
        addCode(DSP_Q31(x)); }
    printFromCurrentIndex();
    return tmp;
}



/*
 * BIQUAD Related
 */

// user function to define the start of a biquad section containg coefficient.
// e.g.     int myBQ = dspBiquadSection(2);
//
void dsp_BIQUADS(int paramAddr){
    ALUformat = 1;
    int base = addOpcodeLengthPrint(DSP_BIQUADS);
    checkInParamSpaceOpcode(paramAddr,2+6*numberFrequencies, DSP_BIQUADS);  // biquad coef are only store in param section
    int num = opcodePtr(paramAddr)->s16.low;  // get number of sections provided
    checkInParamSpace(paramAddr,(2+6*numberFrequencies)*num);
    if (dspFormatDouble())
        addDataSpaceAligned8(num*8);    // 2 words for each data (xn-1, xn-2, yn-1, yn-2)
    else
        if (dspFormatInt())
            addDataSpaceAligned8(num*6);   // space for standard state data + 64bits remainder
        else
            addDataSpaceAligned8(num*4);   // space for standard state data float: 4  words xn-1,xn-2,yn-1,yn-2
    addCodeOffset(paramAddr, base);     // store pointer on the first bunch of (alligned) coefficients
}

int dspBiquad_Sections(int number){
    startParamSection(DSP_BIQUADS, number); // check and initialize conditions for the follwoing data in the PARAM section
    int pos = paramMisAligned8();
    lastSectionIndex = addOpcodeValue(DSP_BIQUADS, number);    // store the number of following sections
    if (number>0) dspprintf3("\n%4d : biquad section expecting %d cell(s)\n",pos,number)
    else
        if (number<0)
             dspprintf3("\n%4d : biquad section expecting maximum %d cell(s)\n",pos,-number)
        else dspprintf3("\n%4d : biquad section\n",pos);
    addCode(1);                             // this is the bypass parameter (reusing old memory location for gain...)
    return pos;
}

int  dspBiquad_Sections_Flexible(){
    return dspBiquad_Sections(0);
}
int  dspBiquad_Sections_Maximum(int number){
    return dspBiquad_Sections(-number);
}


void sectionBiquadCoeficientsBegin(){
    nextParamSection(DSP_BIQUADS);
}

void sectionBiquadCoeficientsEnd(){
    if (lastSectionOpcode == 0) // last section of biquad
        // cancell printing of coeeficients,  as they have been printed in another way
        printFromCurrentIndex();
}

int addFilterParams(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspGainParam_t gain){
    int tmp = addOpcodeValue(type, freq);
    if (tmp & 1) {
        addFloat(Q);
        addFloat(gain);
    }else
        dspFatalError("Encoder bug (not expected). Adress should be misalligned here");
    return tmp;
}

int addBiquadCoeficients(dspFilterParam_t b0,dspFilterParam_t b1,dspFilterParam_t b2,dspFilterParam_t a1,dspFilterParam_t a2){

    int tmp = paramAligned8();    // this enforce that coefficient are alligned 8, so 6 words per biquads and per frequency
    if (dspFormatInt()) {   // integer alu
        addCode(DSP_QNMBQ(b0));
        addCode(DSP_QNMBQ(b1));
        addCode(DSP_QNMBQ(b2));
        addCode(DSP_QNMBQ((a1 - 1.00))); // concept of mantissa reintegration :)
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
    int pos = paramMisAligned8();
    lastSectionIndex = pos; // to adjust in case the index was not alligned previously
    addOpcodeValue(DSP_FIR, numberFrequencies);
    return pos;
}

// create an opcode for executing a fir filter based on several impulse located at "paramAddr"
// minFreq and maxFreq informs on the number of impulse and supported frequencies
void dsp_FIR(int paramAddr){    // possibility to restrict the number of impulse, not all frequencies covered

    int base = addOpcodeLengthPrint(DSP_FIR);
    int end = checkInParamSpaceOpcode(paramAddr,2*numberFrequencies, DSP_FIR);

    int tableFreq[FMAXpos];
    int lengthMax = 0;
    ALUformat = 0;

    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) { // screen the pointed area to create the list of offset for each freq
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
        addCodeOffset(tableFreq[f], base);
        }
    addDataSpace(lengthMax);      // request a data space in the data area corresponding to the largest impulse discovered
}


// generate a 2 opcode sequence <1> <0> if the fir shall not be executed, otherwise genere a value corresponding to delay like for DELAY instruction
int dspFir_Delay(int value){            // to be used when a frequency is not covered by a proper impulse
    nextParamSection(DSP_FIR);
    int pos = paramMisAligned8(); // represent a dummy Impulse, so should be padded 8 bytes
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
    int pos = paramMisAligned8();
#ifdef DSP_FILEACCESS_H_
    dspFileName = name;
    if ((opcodeIndex() + length) >= dspOpcodesMax)
        dspFatalError("Fir impulse too large for the opcode table size.");
    if (-1 == dspfopenRead("r"))
        dspFatalError("cant open impulse file.");

    addCode(length);
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

// integrate a 0.31 sample during x miliseconds. then moving average in delay line and Sqrt
// result is 0.31. should be used after dsp_STORE or dsp_LOAD or dsp_SAT0DB or dsp_DELAY
void dsp_RMS_(int timetot, int delay, int delayInSteps, int pwr){

    addOpcodeLength(DSP_RMS);
    dspprintf3("%s %dms total integration time, ",dspOpcodeText[DSP_RMS],timetot);
    checkInRange(timetot, 10, 7200000);

    double twoP32 = 1ULL<<32;
    double timesecf = timetot; timesecf /= 1000.0;
    if (delayInSteps == 0) {
        checkInRange(delay, 1, timetot);
        delay = timetot / delay;
    }
    checkInRange(delay, 0, 1000);   // max 1000 steps in the delay line
    dspprintf3("averaged %d time\n",delay);

    double stepsf = delay;
    addDataSpaceMisAligned8( 5 + 4 + delay*2); //
    // 1 counter    (0)
    // 1 index      (1)
    // 1 sqrtlatest (2)
    // 1 sqrtwip    (3)
    // 1 sqrtbit    (4)
    // 2 sumsquare  (5)
    // 2 movingavg  (+1)
    // 2xN delayLine(+2)
    addCode( delay);

    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        // generate list of optimized divisor and counter depending on delayline
        int fs = dspTableFreq[f];
        double fsf = fs;
        double maxCounterf;
        if (delay) maxCounterf= fsf * timesecf / stepsf;
        else       maxCounterf= fsf * timesecf;

        int maxCounter = maxCounterf;
        addCode(maxCounter);
        fsf = maxCounter;

        float multf;    // each sample is multiplied by multf before being squared, in order to scale result to 64bit
        if (delay)
             multf = twoP32 / sqrt(fsf*delay) + 0.5;
        else multf = twoP32 / sqrt(fsf) + 0.5;
        int mult = multf;
        mult *= pwr;    // adjust sign => negative if "powerXY"
        addCode( mult);
        if (f == (dspMinSamplingFreq+1) ) dspprintf2("F = %6d, count = %d, mult = %d\n",fs,maxCounter,mult);
    }
    printFromCurrentIndex();
}


// the delay line is given in number of delays, integration is made during timetot / delay
void dsp_RMS(int timetot, int delaysteps){
    dsp_RMS_(timetot, delaysteps,1,1);
}
// the delay given represent
void dsp_RMS_MilliSec(int timetot, int delayms){
    if (delayms == 0)
        dsp_RMS_(timetot, delayms,1,1);
    else
        dsp_RMS_(timetot, delayms,0,1);
}

// same as RMS but compute X * Y instead of X^2
void dsp_PWRXY(int timetot, int delaysteps){
    dsp_RMS_(timetot, delaysteps,1,-1);
}

void dsp_PWRXY_MilliSec(int timetot, int delayms){
    if (delayms == 0)
        dsp_RMS_(timetot, delayms,1,-1);
    else
        dsp_RMS_(timetot, delayms,0,-1);
}

void dsp_DCBLOCK(int lowfreq){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_DCBLOCK);
    checkInRange(lowfreq, 1, 100);
    float lowf = lowfreq;
    addDataSpaceAligned8(6);

    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        // generate list of pole according to fs
        int fs = dspTableFreq[f];
        double fsf = fs;
        float pole = 2.0*M_PI*lowf/fsf; //-0.00125 -> 10hz@48k, 20hz@96k
        //dspprintf("F = %f, pole = %f\n",fsf,pole);
        addGainCodeQNM(-pole);
    }
}

void dsp_DITHER(){
    addOpcodeLengthPrint(DSP_DITHER);
    addDataSpaceAligned8(8);
}

void dsp_DITHER_NS2(int paramAddr){
    // support only 6 triplets of coefficients in this version
    if ((dspMinSamplingFreq<F44100)||(dspMaxSamplingFreq>F192000))
        dspFatalError("frequency range provided in encoderinit incompatible.");
    int base = addOpcodeLengthPrint(DSP_DITHER_NS2);
    checkInParamSpace(paramAddr,3*numberFrequencies);   // requires 3 coef for each supported frequencies
    addDataSpaceAligned8(4);                    // create space for mantissa reintegration 64bits + the 2 errors
    addCodeOffset(paramAddr, base);             // relative pointer to the data table
}

void dsp_DISTRIB(int size){
    addOpcodeLengthPrint(DSP_DISTRIB);
    addCode(size);
    addDataSpace(1+size);
}
