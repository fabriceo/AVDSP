/*
 * dsp_coder.c
 *
 *  Created on: 1 janv. 2020
 *      Author: fabriceo
 *      this program will create a list of opcodes to be executed lated by the dsp engine
 */

#include "dsp_encoder.h"         // enum dsp codes, typedefs and QNM definition
#include <stdlib.h>             // only for importing exit()

//MOVED in dsp_encoder.h
//#define DSP_ENCODER_VERSION ((1<<8) | (1 <<4) | 0) // will be stored in the program header for further interpretation by the runtime

static opcode_t * dspOpcodesPtr  = 0;       // absolute adress start of the table containing the opcodes and data
dspHeader_t* dspHeaderPtr = 0;              // point on the header containing program summary
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
static int lastCoreIndex         =  0;      // Index where was the latest dsp_core , used to store IO related to this core
static int maxOpcodeValue        =  0;      // represent the higher opcode value used in the encoded program
static int lastTpdfDataAddress   =  0;      //point on the opcode containg shift and factor for normalizing tpdfvalue
static int lastTpdfDataAddressCore= 0;      //point on the opcode containg shift and factor for normalizing tpdfvalue within current core
static int LastSectionIndex      = 0;       //point on last dsp_SECTION
static int usedInputs            =  0;      // bit patern of all the inputs used by a LOAD command or LOAD_MUX or LOAD_GAIN
static int usedOutputs           =  0;      // bit patern of all the output used by a STORE command
static unsigned long long usedInputsCore        =  0;      // at core level : bit patern of all the inputs used by a LOAD command or LOAD_MUX or LOAD_GAIN
static unsigned long long usedOutputsCore       =  0;      // at core level : bit patern of all the output used by a STORE command

static int ALUformat             =  0;      // represent the current format of the ALU known at compile time 0 = s31, 1 = double precision or when a sampled is scaled with a gain
static int dspFormat;                       // dynamic management of the different format when encoding
static int dspMant;                         // dynamic value of the DSP_MANT. initialize in encoderinit
static int dspIOmax;                        // max number of IO that can be used with Load & Store (to avoid out of boundaries vs samples table)
static int numberFrequencies;               // number of covered frequencies (mainly used in BIQUADS and FIR)
static float maxParamValue      = 0.0;


int dspMinSamplingFreq = DSP_DEFAULT_MIN_FREQ;
int dspMaxSamplingFreq = DSP_DEFAULT_MAX_FREQ;


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


// add a word in the opcode table reresenting the data offset where a space is reserved
static int addDataSpace(int size) {
    int tmp = dspDataCounter;
    addCode(dspDataCounter);              // store the current data index value pointing on the next spare data space
    dspDataCounter += size;               // simulate consumption the expected data space
    return tmp;
}

// same as above but push the data index by one if needed
// so that the data adress is alligned on 8 bytes boundaries
static int addDataSpaceAligned8(int size) {
    if(dspDataCounter & 1) dspDataCounter++;
    return addDataSpace(size);                 // store the current data index
}

static int addDataSpaceMisAligned8(int size) {
    if((dspDataCounter & 1) == 0) dspDataCounter++;
    return addDataSpace(size);                 // store the current data index
}

static void printFromCurrentIndex(){
    lastIndexPrinted = opcodeIndex();
}

// for debugging purpose, print all the opcode generated since the latest dsp_opcode
static void printLastOpcodes() {
    if (lastIndexPrinted < lastOpcodePrint) {
        dspprintf3("%4d : ",lastIndexPrinted);
        for (int i = lastIndexPrinted; i< lastOpcodePrint; i++)
            dspprintf3("%X ",opcodePtr(i)->i32 );
    } else lastOpcodePrint = lastIndexPrinted;
    if (lastOpcodePrint != opcodeIndex()) {
#if defined(DSP_PRINTF) && ( DSP_PRINTF >=3 )
        opcode_t tmp = *opcodePtr(lastOpcodePrint);
        dspprintf3("%4d : [#%d +%d] ",lastOpcodePrint, tmp.op.opcode, tmp.op.skip);
#endif
        for (int i = lastOpcodePrint+1; i< opcodeIndex(); i++) {
            int val = opcodePtr(i)->i32;
            if (val>=0)  dspprintf3("%X ",val)
            else dspprintf3("%X(@%d) ",val,lastOpcodePrint+val);
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

void setSerialHash(unsigned hash) {
    dspHeaderPtr->serialHash  = hash;
}


void dspEncoderFormat(int format){
    if (format > DSP_FORMAT_DOUBLE_FLOAT) { // this is the mantissa for an INT64 format (simplified parameter)
        dspFormat       = DSP_FORMAT_INT64;
        dspMant         = format;
    } else
    if (format == 0){   // this is considered as float (simplified parameter)
        dspFormat       = DSP_FORMAT_FLOAT;
        dspMant         = 0; //normally not used
    } else {
        dspFormat       = format;
        dspMant         = DSP_MANT; }
    dspprintf("DSP ENCODER : format generated for handling ");
    if      (dspFormat == DSP_FORMAT_INT32)         dspprintf("integer 32 bits, with %d bits mantissa",dspMant)
    else if (dspFormat == DSP_FORMAT_INT64)         dspprintf("integer 64 bits, with %d bits mantissa",dspMant)
    else if (dspFormat == DSP_FORMAT_FLOAT)         dspprintf("float (32bits)")
    else if (dspFormat == DSP_FORMAT_DOUBLE)        dspprintf("double (64bits)")
    else if (dspFormat == DSP_FORMAT_FLOAT_FLOAT)   dspprintf("float with float samples")
    else if (dspFormat == DSP_FORMAT_DOUBLE_FLOAT)  dspprintf("double with float samples");
    dspprintf("\n");
    if (dspFormat < DSP_FORMAT_FLOAT)
         dspHeaderPtr->format = dspMant;    // all value encoded in fixedpoint format
    else
         dspHeaderPtr->format = 0;  // simplified format to describe float encoded parameters
}
// type is eiter one of the DSP_FORMAT_XX or 0 for float or N for INT64 with DSP_MANT = N
void dspEncoderInit(opcode_t * opcodeTable, int max, int format, int minFreq, int maxFreq, int maxIO) {

    if (maxIO>32) dspFatalError("dspEncoderInit supports maximum 32 IO.");
    dspOpcodesPtr       = opcodeTable;
    dspHeaderPtr        = (dspHeader_t*)opcodeTable;
    dspOpcodesMax       = max;

    dspMinSamplingFreq  = minFreq;
    dspMaxSamplingFreq  = maxFreq;
    numberFrequencies   = maxFreq - minFreq +1;
    dspIOmax            = maxIO;
    dspOpcodeIndex      = 0;
    dspDataCounter      = maxIO;    //first table is used to store volume assigned to IO
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
    lastCoreIndex  = 0;
    maxParamValue = 0.0;
    lastTpdfDataAddress     =  0;
    lastTpdfDataAddressCore = 0;
    LastSectionIndex        = 0;

    usedInputs = 0;
    usedOutputs = 0;
    usedInputsCore = 0;
    usedOutputsCore = 0;

    addOpcodeUnknownLength(DSP_HEADER);
    opcodeIndexAdd(sizeof(dspHeader_t)/sizeof(int) - 1);
    dspHeaderPtr->totalLength = 0;
    dspHeaderPtr->dataSize  = 0;
    dspHeaderPtr->checkSum  = 0;
    dspHeaderPtr->numCores  = 0;
    dspHeaderPtr->version   = DSP_ENCODER_VERSION;
    dspEncoderFormat(format);
    dspHeaderPtr->maxOpcode = DSP_MAX_OPCODE-1;
    dspHeaderPtr->freqMin   = minFreq;
    dspHeaderPtr->freqMax   = maxFreq;
    dspHeaderPtr->usedInputs  = 0;
    dspHeaderPtr->usedOutputs = 0;
    setSerialHash(0);
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


static void updateLastCoreIOs(){
    if (lastCoreIndex) {
        int * ptr = (int *)opcodePtr(lastCoreIndex);
        ptr++;  // point on usedInputs
        *ptr++ = usedInputsCore & 0xFFFFFFFF;
        *ptr++ = usedOutputsCore & 0xFFFFFFFF;
        //for compatibility with previous version
        *ptr++ = usedInputsCore >>32;
        *ptr  = usedOutputsCore >>32;
        lastCoreIndex = 0;
    }
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

static int checkCalcTpdf() {
    if (lastTpdfDataAddress == 0) {
        dsp_TPDF_CALC(24);
    }
    if (lastTpdfDataAddressCore) return lastTpdfDataAddressCore;
    return lastTpdfDataAddress;
}

static void updateLastSection(){
    if (LastSectionIndex) {
        //printf("LastSectionIndex=%d\n",LastSectionIndex);
        int * ptr = (int *)opcodePtr(LastSectionIndex);
        ptr++;  // point on displacement
        int ofs = opcodeIndex() - LastSectionIndex;
        //printf("old= %d, new = %d\n",*ptr,ofs);
        *ptr = ofs;
        LastSectionIndex = 0;
    }
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
    //check if some instructions are using TPDF or dithering
    updateLastCoreIOs();
    updateLastSection();
    calcLength();                       // solve latest opcode length
    dspprintf2("DSP_END_OF_CODE\n")
    addOpcodeValue(DSP_END_OF_CODE,0);
    if (opcodeIndex() & 1) addCode(0);  // padding
    calcLength();                       // just for executing debug print
    dspHeaderPtr->totalLength = opcodeIndex();  // total size of the program
    dspprintf1("dsptotallength = %d\n",opcodeIndex());
    dspHeaderPtr->dataSize = dspDataCounter;
    dspprintf1("dataSize       = %d\n",dspDataCounter);
    // now calculate the simplified checksum of all the opcodes and count number of cores
    unsigned int sum;
    int numCore;
    dspCalcSumCore(opcodePtr(0), &sum, &numCore,dspHeaderPtr->totalLength);
    dspHeaderPtr->checkSum = sum;           // comit checksum
    dspprintf1("check sum      = 0x%X\n", sum);
    if (numCore == 0) numCore = 1;
    dspHeaderPtr->numCores = numCore;       // comit number of declared cores
    dspprintf1("cores declared = %d\n",numCore);
    if (dspFormat < DSP_FORMAT_FLOAT) {
        int integ = maxParamValue;
        for (int i=0; i<31; i++) { if (integ) integ >>= 1; else {integ = i; break;} } //compute log2
        integ++; //adding a bit for the sign
        dspprintf1("max encoded    = %f = %d:%d vs %d:%d\n", maxParamValue,integ,32-integ,32-dspMant,dspMant);
    }
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

static void calcMaxParamValue(float val){
    if (val  > maxParamValue) maxParamValue = val;
    if (-val > maxParamValue) maxParamValue = -val;
}

static int addGainCodeQNM(dspGainParam_t gain){
    calcMaxParamValue(gain);
    if (dspFormat < DSP_FORMAT_FLOAT) {
        float max = 1 << (31 - dspMant);
        float min = -max;
        if ((gain>=max) || (gain<min))
            dspprintf(">>>> WARNING : float parameter doent fit in integer format chosen (%d.%d).\n",31-dspMant,dspMant);
        return addCode(DSP_QM32(gain,dspMant));
    } else
        return addFloat(gain);
}

// indicate No operation
void dsp_NOP() { addSingleOpcodePrint(DSP_NOP); }

// indicate start of a program for a dedicated core/task
//a core will be authorized if any bit in the 1st mask is set to 1, OR any bit in the 2nd mask is set to 0
void dsp_CORE_Prog(unsigned progAny1, unsigned progAny0){
    updateLastCoreIOs();
    usedInputsCore  = 0;
    usedOutputsCore = 0;
    lastTpdfDataAddressCore = 0;
    updateLastSection();
    int tmp = addOpcodeLengthPrint(DSP_CORE);
    lastCoreIndex = tmp;
    addCode(0);addCode(0);addCode(0);addCode(0); // space for 4 words for input output tracking
    addCode(progAny1);    //add a 32bit value representing compatibility of the code with 32 user programs
    addCode(progAny0);    //add a 32bit value representing compatibility of the code with 32 user programs
    ALUformat = 0;       // reset it as we start a new core
}

//a core will be authorized if any bit in the 1st mask is set to 1, AND all bits set in 2nd mask are 0
void dsp_CORE(){
    dsp_CORE_Prog(0xFFFFFFFF,0);
}

void dsp_SECTION(unsigned progAny1, unsigned progAny0){
    updateLastSection();
    int tmp = addOpcodeLengthPrint(DSP_SECTION);
    LastSectionIndex = tmp;
    addCode(0);             //offset for jump (at least 4 !)
    addCode(progAny1);      //add a 32bit value representing compatibility of the code with 32 user programs
    addCode(progAny0);      //add a 32bit value representing compatibility of the code with 32 user programs
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

// perform ALU Y = X * Y
void dsp_MULYX(){ addSingleOpcodePrint(DSP_MULYX); }

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

void dsp_WHITE() {
    checkCalcTpdf();
    addSingleOpcodePrint(DSP_WHITE); }

void dsp_SAT0DB_VOL(){
    addSingleOpcodePrint(DSP_SAT0DB_VOL);
    ALUformat = 0;  //after this instruction, the ALU contains a 32bit value unscaled, ready to be stored to a DAC output.
}

void dsp_SAT0DB() {
    addSingleOpcodePrint(DSP_SAT0DB);
    ALUformat = 0;  //after this instruction, the ALU contains a 32bit value unscaled, ready to be stored to a DAC output.
}


void dsp_SAT0DB_GAIN(int paramAddr){
    ALUformat = 0;
    int tmp = addOpcodeLengthPrint(DSP_SAT0DB_GAIN);
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);   // possibility to define the gain just below the opcode
}

void dsp_SAT0DB_GAIN_Fixed(dspGainParam_t gain){
    dsp_SAT0DB_GAIN(0);
    addGainCodeQNM(gain);
}

int dsp_TPDF_CALC(int dither){
    if (lastTpdfDataAddress == 0) addOpcodeLengthPrint(DSP_TPDF_CALC);
    else addOpcodeLengthPrint(DSP_TPDF);
    checkInRange(dither,8,31);
    lastTpdfDataAddressCore = addCode(dither);
    if (lastTpdfDataAddress == 0) lastTpdfDataAddress = lastTpdfDataAddressCore;
    //value will be calculated by runtime during initphase
    addCode(0);         //shifts
    addCode(0);         //mask or factor
    addCode(0);         //mask lo
    addCode(0);         //mask hi
    return lastTpdfDataAddressCore;
}


int dsp_TPDF(int dither){
    return dsp_TPDF_CALC(dither);
}


void dsp_SHIFT(int bits){
    addOpcodeLengthPrint(DSP_SHIFT);
    addCode(bits);
}

void dsp_SHIFT_FixedInt(int bits){  //same :)
    dsp_SHIFT(bits);
}


/*
 *
 * LOAD
 *
 */

// load ALU with the 32bits value of the physical location (or sample) provided
void dsp_LOAD(int IO) {
    ALUformat = 0;
    checkIOmax(IO);
    if (IO<32) usedInputs |= 1ULL<<IO;      //keep track of inputs used
    if (IO<64) usedInputsCore |= 1ULL<<IO;
    addOpcodeLengthPrint(DSP_LOAD);
    addCode(IO);
}

void dsp_LOAD_GAIN(int IO, int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_LOAD_GAIN);
    checkIOmax(IO);
    if (IO<32) usedInputs |= 1ULL<<IO;
    if (IO<64) usedInputsCore |= 1ULL<<IO;
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
int dsp_LOAD_MUX(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint( DSP_LOAD_MUX);
    checkInParamSpaceOpcode(paramAddr, 2, DSP_LOAD_MUX);  // IO-gain matrix only stored in param section
    addCodeOffset(paramAddr, tmp);
    // from release 1.0 this returns the adress where the MUXed value is stored
    return addDataSpaceAligned8(2);    // 2 words for storing calculated value

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
    if (in<64) usedInputsCore |= 1ULL<<in;
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

static void dsp_STORE_IO(int IO) {
    addCode(IO);
    for (int i=0; i<4; i++) {
        int out = IO & 0xFF;
        checkIOmax(out);
        if (out<32) usedOutputs |= 1ULL<<out;
        if (out<64) usedOutputsCore |= 1ULL<<out;
        IO >>= 8;
        if (IO==0) break;
    }
}

void dsp_STORE(int IO) {
    addOpcodeLengthPrint(DSP_STORE);
    dsp_STORE_IO(IO);
}

void dsp_STORE_VOL(int IO) {
    addOpcodeLengthPrint(DSP_STORE_VOL);
    dsp_STORE_IO(IO);
}

void dsp_STORE_VOL_SAT(int IO) {
    addOpcodeLengthPrint(DSP_STORE_VOL_SAT);
    dsp_STORE_IO(IO);
}

void dsp_STORE_TPDF(int IO) {
    int addrtpdf = checkCalcTpdf();
    int tmp = addOpcodeLengthPrint(DSP_STORE_TPDF);
    dsp_STORE_IO(IO);
    addCodeOffset(addrtpdf, tmp);
}

void dsp_STORE_GAIN(int IO, int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_STORE_GAIN);
    dsp_STORE_IO(IO);
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);   // possibility to define the gain just below the opcode
}

void dsp_STORE_GAIN_Fixed(int IO, dspGainParam_t gain) {
    dsp_STORE_GAIN(IO, 0);
    addGainCodeQNM(gain);   // store the fixed gain just after the opcode
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


void dsp_VALUEX_Fixed(float value){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUEX);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(value);
}

void dsp_VALUEX(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUEX);
    checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
}

void dsp_VALUEY_Fixed(float value){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUEY);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(value);
}

void dsp_VALUEY(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_VALUEY);
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


void dsp_DELAY_1(){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_DELAY_1);
    addDataSpaceAligned8(2);    // 2 words for supporting 64bits alu
}

// DSP_SERIAL
void dsp_SERIAL(unsigned hash) {
    addOpcodeLengthPrint(DSP_SERIAL);
    addCode(hash);
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
    for (int i=0; i<n; i++) addGainCodeQNM(*(data+i));
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
    setLastMissingParam(2);   // alway expect the parameters to be provided in the following opcode,
                              // at least 2 words
}

void dspLoadStore_Data(int in, int out){
    checkLastMissing(DSP_LOAD_STORE);       // verify that a dsp_LOAD_STORE() is just above
    checkIOmax(in);
    checkIOmax(out);
    addCode(in);
    addCode(out);
    if (in<32)  usedInputs  |= 1ULL<<in;
    if (in<64)  usedInputsCore  |= 1ULL<<in;
    if (out<32) usedOutputs |= 1ULL<<out;
    if (out<64) usedOutputsCore |= 1ULL<<out;
}

// DSP_MIXER
void dsp_MIXER(){  // this function must be followed by couples of data (input & output)
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_MIXER);
    setLastMissingParam(2);   // alway expect the parameters to be provided in the following opcode,
                              // at least 2 words

}

void dspMixer_Data(int in,  dspGainParam_t gain){

    checkLastMissing(DSP_MIXER);       // verify that a dsp_MIXER() is just above
    checkIOmax(in);
    addCode(in);
    addGainCodeQNM(gain);
    if (in<32)  usedInputs  |= 1ULL<<in;
    if (in<64)  usedInputsCore  |= 1ULL<<in;
}

static void addMemLocation(int index, int base){
    checkInParamSpace(index, 2);
    addCodeOffset(index, base);
}

// load a meory location from a PARAM area
void dsp_LOAD_X_MEM_Index(int paramAddr, int index) {
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_LOAD_X_MEM);
    addMemLocation(paramAddr + index*2, tmp);
}

void dsp_STORE_X_MEM_Index(int paramAddr, int index) {
    int tmp = addOpcodeLengthPrint(DSP_STORE_X_MEM);
    addMemLocation(paramAddr  + index*2, tmp);
}

// load a meory location from a PARAM area
void dsp_LOAD_X_MEM(int paramAddr) {
    dsp_LOAD_X_MEM_Index(paramAddr, 0);
}

void dsp_STORE_X_MEM(int paramAddr) {
    dsp_STORE_X_MEM_Index(paramAddr, 0);
}


// load a meory location from a PARAM area
void dsp_LOAD_Y_MEM_Index(int paramAddr, int index) {
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_LOAD_Y_MEM);
    addMemLocation(paramAddr + index*2, tmp);
}

void dsp_STORE_Y_MEM_Index(int paramAddr, int index) {
    int tmp = addOpcodeLengthPrint(DSP_STORE_Y_MEM);
    addMemLocation(paramAddr  + index*2, tmp);
}

// load a meory location from a PARAM area
void dsp_LOAD_Y_MEM(int paramAddr) {
    dsp_LOAD_Y_MEM_Index(paramAddr, 0);
}

void dsp_STORE_Y_MEM(int paramAddr) {
    dsp_STORE_Y_MEM_Index(paramAddr, 0);
}


// generate the space inside the PARAM area for the futur LOAD/STORE_MEM
int dspMem_LocationMultiple(int number) {
    checkFinishedParamSection();
    checkInParamNum();  // check if we are in a PARAM or PARAM_NUM section
    paramAligned8();
    int tmp = opcodeIndex();
    opcodeIndexAdd(2*number);
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
    addCode(size);                              // store the max size of the delay line for runtime to check due to user potential changes
    if (opcode == DSP_DELAY_DP)
         addDataSpaceMisAligned8(size*2+1);      // now we can request the data space
    else addDataSpace(size+1);
    addCodeOffset(paramAddr, tmp);              // point on where is the delay in uSec
}

void dsp_DELAY(int paramAddr){
    ALUformat = 0;
    dsp_DELAY_(paramAddr, DSP_DELAY);
}

// exact same as above but double precision
void dsp_DELAY_DP(int paramAddr){
    ALUformat = 1;
    dsp_DELAY_(paramAddr, DSP_DELAY_DP);
}

// genertae one word code combining the default uS value in LSB and with the max value in MSB
static int dspDelay_MicroSec(unsigned short maxus, unsigned short us){
    checkInParamNum();  // check if we are in a PARAM or PARAM_NUM section
    checkFinishedParamSection();
    signed long long maxSamples = (((signed long long)maxus * dspConvertFrequencyFromIndex(dspMaxSamplingFreq) + 500000)) / 1000000;
    if (maxSamples > 16000) dspFatalError("delay too large.");  // arbitrary value in this code version TODO
    //dspprintf("DELAY maxus = %d, maxsamples = %lld, value = %dus\n",maxus,maxSamples,us);
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
    if (opcode == DSP_DELAY_DP) { DP = 2; ALUformat = 1; } else ALUformat = 0;
    addOpcodeLengthPrint(opcode);
    unsigned long long delayLineFactor = dspTableDelayFactor[dspMaxSamplingFreq];
    unsigned long long maxSamples_ = (delayLineFactor * microSec);
    maxSamples_ >>= 32;
    unsigned maxSamples = maxSamples_;
    dspprintf2("DELAY %d -> MAXSAMPLE = %d\n",microSec,maxSamples);
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

void dsp_DELAY_DP_FixedMicroSec(int microSec){
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY_DP);
}
void dsp_DELAY_DP_FixedMilliMeter(int mm,float speed){
    dsp_DELAY_DP_FixedMicroSec(mm * 1000.0 / speed);
}

void dsp_DELAY_FB_MIX_FixedMicroSec(int microSec, float source, float fb, float delayed, float mix) {
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY_FB_MIX);
    if (dspFormat < DSP_FORMAT_FLOAT) {   // integer alu. Using q31 format.
        addCode(DSP_QM32(source,31));
        addCode(DSP_QM32(fb,31));
        addCode(DSP_QM32(delayed,31));
        addCode(DSP_QM32(mix,31));
    } else {
        addFloat(source);
        addFloat(fb);
        addFloat(delayed);
        addFloat(mix);
    }
}


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
        addCode(DSP_QM32(x,31)); }
    printFromCurrentIndex();
    return tmp;
}



/*
 * BIQUAD Related
 */

// user function to define the start of a biquad section containg coefficient.
// e.g.     int myBQ = dspBiquadSection(2);
//
int dsp_BIQUADS(int paramAddr){
    ALUformat = 1;
    int base = addOpcodeLengthPrint(DSP_BIQUADS);
    checkInParamSpaceOpcode(paramAddr,2+6*numberFrequencies, DSP_BIQUADS);  // biquad coef are only store in param section
    int num = opcodePtr(paramAddr)->s16.low;  // get number of sections provided
    checkInParamSpace(paramAddr,(2+6*numberFrequencies)*num);
    int addrValue = addDataSpaceAligned8(num*6);           // 2 words for mantissa reintegration + 4 words for each data (xn-1, xn-2, yn-1, yn-2)
    addCodeOffset(paramAddr, base);        // store pointer on the table of coefficients
    // from release 1.0 this returns the adress where the Biquaed calculated value is stored
    return addrValue+((num-1)*6);           // to be tested

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
    calcMaxParamValue(b0);
    calcMaxParamValue(b1);
    calcMaxParamValue(b2);
    calcMaxParamValue(a1-1.0);
    calcMaxParamValue(a2);

    if (dspFormat < DSP_FORMAT_FLOAT) {   // integer alu

        addCode(DSP_QM32(b0,DSP_MANTBQ));
        addCode(DSP_QM32(b1,DSP_MANTBQ));
        addCode(DSP_QM32(b2,DSP_MANTBQ));
        addCode(DSP_QM32(a1-1.0,DSP_MANTBQ)); // concept of mantissa reintegration/noise shapping
        addCode(DSP_QM32(a2,DSP_MANTBQ));
        //addCode(DSP_MANTBQ);    //including the MANTBQ value within the filters so that the asm routine can saturate and extract properly
    } else {
        addFloat(b0);
        addFloat(b1);
        addFloat(b2);
        addFloat(a1 - 1.0); // to make things easier, even float model is using mantissa reintegration
        addFloat(a2);
        //addFloat(1.0);      //for compatibility with int version
    }
    return tmp;
}

int dspFir_Impulses(){
    startParamSection(DSP_FIR, numberFrequencies);
    int pos = paramMisAligned8();
    lastSectionIndex = pos; // to adjust in case the index was not alligned previously
    addOpcodeValue(DSP_FIR, numberFrequencies); // header , will be folowwed by impulses
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
            length = 1;                             // 1 dummy data is stored just after the delay value
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
        addCodeOffset(tableFreq[f], base);          // add pointer on the impulse, relative to DSP_FIR opcode
        }
    addDataSpaceAligned8(lengthMax);                // request a data space in the data area corresponding to the largest impulse discovered
}


// generate a 2 opcode sequence <1> <0> if the fir shall not be executed, otherwise genere a value corresponding to delay like for DELAY instruction
int dspFir_Delay(int value){            // to be used when a frequency is not covered by a proper impulse
    nextParamSection(DSP_FIR);
    int pos = paramMisAligned8(); // represent a dummy Impulse, so should be padded 8 bytes
    if (value > 1) {
        addOpcodeValue(value, 0);           // store the expected delay (in samples) in msb, same format as for DELAY opcode
    } else
        addCode(1);             // simulate an impulse of length 1
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

    addCode(length);    // first word (misalligned) is the length of the impulse
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

// integrate a s.31 sample during x miliseconds. then moving average in delay line and Sqrt
// result is s.31. should be used after dsp_STORE or dsp_LOAD or dsp_SAT0DB or dsp_DELAY
void dsp_RMS_(int timetot, int delay, int delayInSteps, int pwr){
    ALUformat = 1;

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
        int fs = dspConvertFrequencyFromIndex(f);
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
    addDataSpaceAligned8(4);    // 2 words for ACC, 1 word for prevX prevY

    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        // generate list of pole according to fs
        int fs = dspConvertFrequencyFromIndex(f);
        double fsf = fs;
        float pole = 2.0*M_PI*lowf/fsf; //-0.00125 -> 10hz@48k, 20hz@96k
        //dspprintf("F = %f, pole = %f\n",fsf,pole);
        addGainCodeQNM(-pole);
    }
}

void dsp_DITHER(){
    checkCalcTpdf();
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_DITHER);
    addDataSpaceAligned8(6);    // might be double so 3x2
}

void dsp_DITHER_NS2(int paramAddr){
    checkCalcTpdf();
    ALUformat = 1;
    // support only 6 triplets of coefficients in this version
    if ((dspMinSamplingFreq<F44100)||(dspMaxSamplingFreq>F192000))
        dspFatalError("frequency range provided in encoderinit incompatible.");
    int base = addOpcodeLengthPrint(DSP_DITHER_NS2);
    checkInParamSpace(paramAddr,3*numberFrequencies);   // requires 3 coef for each supported frequencies
    addDataSpaceAligned8(3);                    // create space for 3 errors bin potentially 2 words
    addCodeOffset(paramAddr, base);             // relative pointer to the data table
}

void dsp_DISTRIB(int IO, int size){
    addOpcodeLengthPrint(DSP_DISTRIB);
    checkIOmax(IO);
    addCode(IO);
    if (IO<32) usedOutputs |= 1ULL<<IO;
    if (IO<64) usedOutputsCore |= 1ULL<<IO;
    checkInRange(size, 8,1024);
    addCode(size);
    addDataSpace(1+size);
}

void dsp_DIRAC_(int freq, dspGainParam_t gain){
    ALUformat = 1;
    int fmin = dspConvertFrequencyFromIndex(dspMinSamplingFreq);
    checkInRange(freq, 0,fmin/2);
    addDataSpace(1);    // one word in data space as counter for recreating the dirac impulse at frequency "freq"
    addGainCodeQNM(gain);
    for (int f=dspMinSamplingFreq; f<=dspMaxSamplingFreq; f++){
        int fs = dspConvertFrequencyFromIndex(f);
        int count = fs / freq;
        addCode(count);
    }
}

void dsp_DIRAC_Fixed(int freq, dspGainParam_t gain){
    addOpcodeLengthPrint(DSP_DIRAC);
    dsp_DIRAC_(freq, gain);
}

void dsp_SQUAREWAVE_Fixed(int freq, dspGainParam_t gain){
    addOpcodeLengthPrint(DSP_SQUAREWAVE);
    dsp_DIRAC_(freq, gain);
}


void dsp_CLIP_Fixed(dspGainParam_t value){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_CLIP);
    if ((value > 1.0) || (value < 0.0))
        dspFatalError("value not in range 0..1.0");
    addGainCodeQNM(value);
}

void dsp_SINE_Fixed(int freq, dspGainParam_t gain){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_SINE);
    int fmin = dspConvertFrequencyFromIndex(dspMinSamplingFreq);
    checkInRange(freq, 20, fmin/4);
    addDataSpaceAligned8(4);    //data space for computing xn and yn each in 64bits
    addGainCodeQNM(gain);       //initial value for yn (cosine) or when xn == 0
    for (int f=dspMinSamplingFreq; f<=dspMaxSamplingFreq; f++){
        int fs = dspConvertFrequencyFromIndex(f);
        // for frequency > fs / 32, the value of epsilon must be adjusted...
        // example measured at 48khz:
        // 750 => 750.3
        // 1500 => 1502
        // 3000 => 3019
        // 6000 => 6166
        float epsilon = 2.0*M_PI*(float)freq / (float)fs;
        addGainCodeQNM( epsilon );  //using standard dspmantissa (not specific biquad mantissa)
    }
}


