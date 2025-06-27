/* 18h56
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
static opcode_t * symbolStart    = 0;
dspHeader_t* dspHeaderPtr        = 0;       // point on the header containing program summary
static int dspMemoryMax          = 0;       // max allowed size of this table (in words)
static int dspOpcodesMax         = 0;       // max allowed size for code in words
static int symbolNumber          = 0;       //number of symbol added
volatile static int dspOpcodeIndex = 0;     // point on the next available opcode position in the opcode table
static int firstOpcodeIndex      =  0;      // point just after the FIRST dsp_HEADER
static int lastTileNum           =  0;      //incremented each time a dsp_TILE() is called
static int firstTileIndex        =  0;      //point just after the veryfirst header
static int firstTileParamSize    =  0;      //size of all common paramters to be duplicated in each further tiles

static int lastOpcodePrint       =  0;      // point on the last opcode generated, ready for printing following code
static int lastIndexPrinted      =  0;      // opaque...
static int dspOutLabel           =  0;      // incremented 
char * dspOutLabelName;
static int lastOpcodeIndexLength = -1;      // point on the latest opcode requiring a quantity of code space not yet known
static int dspDataCounter        =  0;      // point on the next data adress available (relative to begining of data)

static int lastParamNumIndex     =  0;      // index position in the opcode table of the latest ongoing PARAM_NUM or PARAM

static int lastMissingParamIndex =  0;      // index where a parameter is expected to follow
static int lastMissingParamSize  =  0;      // expected minimum size of total codelength for the opcode (verified in calclength)

static int dspDumpStarted        =  0;      // as soon as a dsp_dump is executed, this is set to 1
static int dspOutIndex           =  0;      //used when generatin advspout.cpp file to differentiate some symbols
static int lastSectionOpcode     =  0;      // opcode associated with the latest section declaration
static int lastSectionNumber     =  0;      // number expected of data for the started section
static int lastSectionCount      =  0;      // incremented number each time a dataset is encountered
static int lastSectionIndex      =  0;      // value of the opcode index when a new section was started
static int lastCoreIndex         =  0;      // Index where was the latest dsp_core , used to store IO related to this core
static int lastCoreData          =  0;      // value of dspDataCounter of the curent/latest core
static int lastCoreNum           =  0;      // number of the current core, incrementing
static int lastCoreOpcode        =  0;      // contains opcode of last core (DSP_CORE or DSP_CORE_EXTERN)
static int maxOpcodeValue        =  0;      // represent the higher opcode value used in the encoded program
static int lastTpdfDataAddress   =  0;      //point on the opcode containg shift and factor for normalizing tpdfvalue
static int lastTpdfDataAddressCore= 0;      //point on the opcode containg shift and factor for normalizing tpdfvalue within current core
static int lastSectionProg       =  0;      //point on last dsp_SECTION
static int lastSectionElse       =  0;      //point on last dsp_SECTION_ELSE
static int lastSectionData       =  0;      //value of dspDataCounter at the begining of the section
static int lastSectionDataMax    =  0;      //value of dspDataCounter at the end of the section (max)
static unsigned long long  usedInputs           =  0;      // bit patern of all the inputs used by a LOAD command or LOAD_MUX or LOAD_GAIN
static unsigned long long  usedOutputs          =  0;      // bit patern of all the output used by a STORE command
static unsigned long long usedInputsCore        =  0;      // at core level : bit patern of all the inputs used by a LOAD command or LOAD_MUX or LOAD_GAIN
static unsigned long long usedOutputsCore       =  0;      // at core level : bit patern of all the output used by a STORE command

static int ALUformat             =  0;      // represent the current format of the ALU known at compile time 0 = s31, 1 = double precision or when a sampled is scaled with a gain
static int dspFormat;                       // dynamic management of the different format when encoding
static int dspMant;                         // dynamic value of the DSP_MANT. initialize in encoderinit. 0 if format not integer
static int dspIOmax;                        // max number of IO that can be used with Load & Store (to avoid out of boundaries vs samples table)
static int numberFrequencies;               // number of covered frequencies (mainly used in BIQUADS and FIR)
static float maxParamValue      = 0.0;      // to hold the maximum value pushed as encoded parameter
static int dspDynamic           = 0;        // 0 means filters are staticaly calculated. 1 means dynamically at FS change

int dspMinSamplingFreq = DSP_DEFAULT_MIN_FREQ;
int dspMaxSamplingFreq = DSP_DEFAULT_MAX_FREQ;
const int dspIOmaximum = 64; 


#ifndef DSP_FILEACCESS_H_
#define dspout(...)
#endif

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
int opcodeIndexAdd(int add) {
    int tmp = opcodeIndex();
    if ((tmp+add) > dspOpcodesMax)   // boundary check
        dspFatalError("ERROR : Dsp generate code is too large");
    dspOpcodeIndex += add;
    return tmp;
}
// return an absolute pointer within the opcode table
opcode_t * opcodePtr(int index){
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

int  addFloat_or_QNM(float value, int M){
    if (dspFormat < DSP_FORMAT_FLOAT)    // integer alu. Using qnm format.
        return addCode( dspQM32(value,(M==0)? dspMant : M ) );
    else
        return addFloat(value);
}

// add an opcode with a value in the LSB
int addOpcodeValue(int code, int value){
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
int addCodeOffset(int index, int base){
    int offset;
    if (index) offset = index - base;           // calculate relative value to "base"
    else offset = opcodeIndex() +1 - base;      // calculate the relative value to "base" of the next opcode
    return addCode(offset);                     // store the value calculated
}


// add a word in the opcode table reresenting the data offset where a space is reserved
static int addDataSpace(int size) {
    int tmp = dspDataCounter;
    addCode(dspDataCounter - lastCoreData);     // store the current data index value pointing on the next spare data space
    dspDataCounter += size;                     // simulate consumption the expected data space
    return tmp;
}

// same as above but push the data index by one if needed
// so that the data adress is alligned on 8 bytes boundaries
int addDataSpaceAligned8(int size) {
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
        dspprintf3("\n");
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

void dspPrintPending() {
    printLastOpcodes();
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
        case DSP_BIQUADS_FS: 
        case DSP_BIQUADS: {
            dspprintf2("-> %d biquad cell(s) provided\n",lastSectionCount)
            for (int i=0; i<lastSectionCount ; i++) dspout("{ 0,0,0,0,0,0},");
            dspout(" }; //%d biquad cell(s) provided\n",lastSectionCount);
            first->s16.low = lastSectionCount;
            lastSectionOpcode = 0;  // now finished properly
            printFromCurrentIndex();
            opcodeIndexAdd(lastSectionCount*6);
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
    switch(opcode) {
        case DSP_BIQUADS_FS :
        case DSP_BIQUADS : {
            dspOutLabel++;
            dspout("const float %s[][6] = {\n",dspOutLabelName); break; }
    }
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
    //this is used to identify the greater value of opcode in this program
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
        dspMant         = 0; //normally not used, only for fast checking int/float format
    } else {
        dspFormat       = format;
        dspMant         = (format < DSP_FORMAT_FLOAT) ? DSP_MANT : 0;
    }
    dspprintf("DSP ENCODER : format generated for handling ");
    if      (dspFormat == DSP_FORMAT_INT32)         dspprintf("integer 32 bits, with %d bits mantissa",dspMant)
    else if (dspFormat == DSP_FORMAT_INT64)         dspprintf("integer 64 bits, with %d bits mantissa",dspMant)
    else if (dspFormat == DSP_FORMAT_FLOAT)         dspprintf("float (32bits) with integer samples")
    else if (dspFormat == DSP_FORMAT_DOUBLE)        dspprintf("double (64bits) with integer samples")
    else if (dspFormat == DSP_FORMAT_FLOAT_FLOAT)   dspprintf("float (32bits) with float (32bits) samples")
    else if (dspFormat == DSP_FORMAT_DOUBLE_FLOAT)  dspprintf("double (64bits) with float (32bits) samples");
    dspprintf("\n");
}

//initialize encoder for a new header
void dspHeaderInit(opcode_t * opcodeTable) {

    dspOpcodesPtr         = opcodeTable;
    dspHeaderPtr          = (dspHeader_t*)opcodeTable;

    dspOpcodeIndex        = 0;
    dspDataCounter        = 0;    //first table is used to store volume assigned to IO
    dspOutLabel           = 0;
    lastOpcodePrint       = 0;
    lastOpcodeIndexLength = -1;
    lastParamNumIndex     = 0;
    lastMissingParamIndex = 0;
    lastIndexPrinted      = 0;
    lastSectionOpcode     = 0;
    lastSectionIndex      = 0;
    lastSectionNumber     = 0;
    lastSectionCount      = 0;
    dspDumpStarted        = 0;
    ALUformat             = 0; // by default we consider to be single precision with ALU containing a 0.31 value
    lastCoreIndex         = 0;
    lastCoreNum           = 0;
    lastCoreOpcode        = 0;
    maxParamValue         = 0.0;
    lastTpdfDataAddress   =  0;
    lastTpdfDataAddressCore = 0;
    lastSectionProg       = 0;
    lastSectionElse       = 0;
    lastSectionData       = 0;
    lastSectionDataMax    = 0;

    usedInputs            = 0;
    usedOutputs           = 0;
    usedInputsCore        = 0;
    usedOutputsCore       = 0;

    addOpcodeUnknownLength(DSP_HEADER);
    opcodeIndexAdd(sizeof(dspHeader_t)/sizeof(int) - 1);
    if (firstOpcodeIndex==0) firstOpcodeIndex = opcodeIndex();
    dspHeaderPtr->totalLength = 0;
    dspHeaderPtr->dataSize  = 0;
    dspHeaderPtr->checkSum  = 0;
    dspHeaderPtr->numCores  = 0;
    dspHeaderPtr->version   = DSP_ENCODER_VERSION;
    dspHeaderPtr->format    = dspMant;    // all value encoded in fixedpoint format
    dspHeaderPtr->mantissa2 = 0;    //default runtime value
    dspHeaderPtr->maxOpcode = DSP_MAX_OPCODE-1;
    dspHeaderPtr->freqMin   = dspMinSamplingFreq;
    dspHeaderPtr->freqMax   = dspMaxSamplingFreq;
    dspHeaderPtr->usedInputs  = 0;
    dspHeaderPtr->usedOutputs = 0;
    dspHeaderPtr->clockcpu    = 0;
    dspHeaderPtr->clock176k   = 0;
    dspHeaderPtr->clock192k   = 0;

    setSerialHash(0);
    firstTileIndex = opcodeIndex();
    calcLength();
}


// type is eiter one of the DSP_FORMAT_XX or 0 for float or N for INT64 with DSP_MANT = N
void dspEncoderInit(opcode_t * opcodeTable, int max, int format, int minFreq, int maxFreq, int maxIO) {

    if (maxIO > dspIOmaximum) dspFatalError("dspEncoderInit too much IO.");
    dspMemoryMax        = max;
    dspOpcodesMax       = max;   //TODO
    dspEncoderFormat(format);
    dspMinSamplingFreq  = minFreq;
    dspMaxSamplingFreq  = maxFreq;
    numberFrequencies   = maxFreq - minFreq +1;
    dspIOmax            = maxIO;
    maxOpcodeValue      =  0;
    firstOpcodeIndex    =  0;
    lastTileNum         =  0;
    firstTileIndex      =  0;
    firstTileParamSize  =  0;
    symbolStart         =  0;
    symbolNumber        =  0;
    dspOutIndex         =  0;
    dspOutLabelName     = "";
    dspDynamic          = 0;
    dspOutFileCreate(); //eventually create an output file containing C source code generated
    dspHeaderInit( opcodeTable );

}

int dsp_FSMIN(int freq){
    if (firstOpcodeIndex != opcodeIndex()) return 0;
    dspMinSamplingFreq = freq;
    dspHeaderPtr->freqMin   = dspMinSamplingFreq;
    numberFrequencies = dspMaxSamplingFreq - dspMinSamplingFreq +1;
    return 1;
}

int dsp_FSMAX(int freq){
    if (firstOpcodeIndex != opcodeIndex()) return 0;
    dspMaxSamplingFreq = freq;
    dspHeaderPtr->freqMax   = dspMaxSamplingFreq;
    numberFrequencies = dspMaxSamplingFreq - dspMinSamplingFreq +1;
    return 1;
}

int dsp_FSDYN(int val) {
    if (firstOpcodeIndex != opcodeIndex()) return 0;
    dspDynamic = val;
    return 1;
}

int dsp_FORMAT(int format, int mant2) {
    if (firstOpcodeIndex != opcodeIndex()) return 0;
    dspEncoderFormat(format);
    if (dspFormat < DSP_FORMAT_FLOAT) {
         dspHeaderPtr->format = dspMant;     // all value encoded in fixedpoint format
         dspHeaderPtr->mantissa2 = mant2;    // all value encoded in fixedpoint format
    } else {
         dspHeaderPtr->format = 0;  // simplified format to describe float encoded parameters
         dspHeaderPtr->mantissa2 = 0;
    }
    return 1;
}

int dsp_IOMAX(int iomax) {
    iomax += 7;
    iomax &= ~7;
    //test if code generation has already started
    if (firstOpcodeIndex != opcodeIndex()) return 0;
    if (iomax>=dspIOmaximum) dspFatalError("IO max out of range.");
    dspIOmax = iomax;
    return 1;
}

int dsp_CLOCK(int cpu, int k176, int k192, int prio) {
    dspHeaderPtr->clockcpu = cpu * 1000000;
    dspHeaderPtr->clock176k = k176 * 1000000;
    dspHeaderPtr->clock192k = k192 * 1000000;
    dspHeaderPtr->coreaesprio = prio;
    return 1;
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
        *(ptr++) = usedInputsCore & 0xFFFFFFFF;
        *(ptr++) = usedOutputsCore & 0xFFFFFFFF;
        //for compatibility with previous version
        *(ptr++) = usedInputsCore >>32;
        *(ptr++) = usedOutputsCore >>32;
        //compute size of data used in this core
        if(dspDataCounter & 1) dspDataCounter++;
        (*ptr)   = dspDataCounter - lastCoreData; 
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
    if (lastSectionElse || lastSectionProg) {
        int data = dspDataCounter - lastSectionData;
        if (dspDataCounter > lastSectionDataMax) lastSectionDataMax = dspDataCounter;
        dspDataCounter = lastSectionDataMax;
        calcLength();
        printLastOpcodes();
        dspprintf3("DSP_SECTION END : data used %d,  dspDataCounter set to %d\n",data,dspDataCounter);
    } 
    if (lastSectionElse) {

        //printf("lastSectionElse=%d\n",lastSectionElse);
        int ofs = opcodeIndex() - lastSectionElse;
        if (opcodePtr(lastSectionElse)->op.opcode == DSP_NOP) {
            opcodePtr(lastSectionElse)->op.skip = ofs;
            dspprintf3("DSP_SECTION ELSE Patching DSP_NOP at %d with ofset %d => %d\n",lastSectionElse,ofs,lastSectionElse+ofs);
        }
        lastSectionElse = 0;
    }
    if (lastSectionProg) {
        //printf("lastSectionProg=%d\n",lastSectionProg);
        int * ptr = (int *)opcodePtr(lastSectionProg);
        if (opcodePtr(lastSectionProg)->op.opcode == DSP_SECTION) {
            ptr++;  // point on displacement
            int ofs = opcodeIndex() - lastSectionProg;
            //printf("old= %d, new = %d\n",*ptr,ofs);
            *ptr = ofs;
            calcLength();
            printLastOpcodes(); //
            dspprintf3("DSP_SECTION Patching at %d with ofset %d => %d\n\n",lastSectionProg,ofs,lastSectionProg+ofs);
        }
        lastSectionProg = 0;
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


//create a dsp_CORE opcode if none has been decalred yet.
static void check_dsp_CORE() {
    if ( (lastCoreNum == 0) || (lastCoreOpcode == DSP_CORE_AES) ) {
        dsp_CORE();
        printLastOpcodes();
        lastOpcodePrint = opcodeIndex();
    }
}


int dspHeaderDone(){
    if (lastCoreOpcode != DSP_CORE_AES) {
        check_dsp_CORE();
        updateLastSection();
        updateLastCoreIOs();
    }
    calcLength();                       // solve latest opcode length
    dspprintf2("DSP_END_OF_CODE\n")
    addOpcodeValue(DSP_END_OF_CODE,0);
    int index = opcodeIndex();
    index &= 3;                        //padding/allignement 16 bytes
    if (index) opcodeIndexAdd(4-index);
    calcLength();                       // just for executing debug print
    dspHeaderPtr->totalLength = opcodeIndex();  // total size of the program including header
    dspprintf1("dsptotallength = %d\n",opcodeIndex());
    dspHeaderPtr->dataSize = dspDataCounter;    //not relevant,as each core is dynamically allocating data
    dspprintf1("dataSize (max) = %d\n",dspDataCounter);
    // now calculate the simplified checksum of all the opcodes and count number of cores
    unsigned int sum;
    int numCore;
    dspCalcSumCore(opcodePtr(0), &sum, &numCore,dspHeaderPtr->totalLength);
    dspHeaderPtr->checkSum = sum;           // comit checksum
    dspprintf1("check sum      = 0x%X\n", sum);
    if (numCore == 0) numCore = 1;
    dspHeaderPtr->numCores = numCore;       // comit number of declared cores
    dspprintf1("cores found    = %d\n",numCore);
    if (dspFormat < DSP_FORMAT_FLOAT) {
        int integ = maxParamValue;
        for (int i=0; i<31; i++) { if (integ) integ >>= 1; else {integ = i; break;} } //compute log2
        integ++; //adding a bit for the sign
        int mant2 = dspHeaderPtr->mantissa2;
        if (mant2 == 0) mant2 = DSP_MANT2;
        dspprintf1("max encoded    = %f = q%d.%d vs q%d.%d (accu %d.%d)\n", maxParamValue,integ,32-integ,32-dspMant,dspMant,64-mant2,mant2);
        if (integ > (32-dspMant)) dspFatalError("some numbers are too large for the choosen encoding format");
    }
    dspHeaderPtr->maxOpcode   = maxOpcodeValue;
    dspHeaderPtr->usedInputs  = usedInputs;
    dspHeaderPtr->usedOutputs = usedOutputs;
    
    dspout("} //end of core %d\n",numCore);
    dspout("int dspDataSpace%d[%d];\n",lastTileNum,dspDataCounter);
    return opcodeIndex();

}
void dspSymbolCreateTable() {
    if (symbolStart) dspFatalError("symbol table already created");
    symbolNumber        = 0;
    dspOpcodesPtr       = &dspOpcodesPtr[dspOpcodeIndex];
    dspOpcodeIndex      = 0;
    symbolStart = opcodeIndexPtr();
    addOpcodeValue(DSP_PARAM_NUM,0);
    addCode(0x7FFFFFFE);    //magic number
}

void dspSymbolAdd(dspSymbol_t * s){
    if (symbolStart == 0) dspFatalError("symbol table was not initiated upfront");
    #if defined(DSP_PRINTF) && ( DSP_PRINTF < 3 )
    if (s->address) 
    #endif
    {
        if (symbolNumber == 0) {
            dspprintf2("EXTERN SYMBOLS TABLE\n")
            dspprintf2("tile, usedin, address, type, len, name\n");
            symbolNumber = 1;
        }
        dspprintf2("%4d    %4X    %5d    %2d  %3d  %s\n",s->tileNum, s->tileUsed, s->address, s->type, s->length, s->name);
    }
    if (s->address) {
        addCode(s->address);
        unsigned f = s->type | (s->tileNum << 8) | (s->tileUsed << 12) | (s->length << 16);
        addCode(f);
        char * p = (char*)opcodeIndexPtr();
        char * q = s->name;
        int w = (s->length+1+3)/4;
        opcodeIndexAdd(w);
        for (int i=0; i < s->length; i++) p[i] = q[i];
    }
}

int dspSymbolEndOfTable(){
    if (symbolStart == 0) dspFatalError("symbol table was not initiated upfront");
    symbolStart->op.skip = opcodeIndex();
    return opcodeIndex();  // size of the program
}

// DSP_END_OF_CODE
// generate the DSP_EN_OF_CODE and calculate total program length, number of core and checksum.
// return size of code alligned to next 8bytes, so can be used as a dataStart index in same array
int dsp_END_OF_CODE(){

    dspHeaderDone();

    if (dspDumpStarted) {
        dsp_dump(opcodeIndex(),dspDataCounter,"DSP_END_OF_CODE_DATA_SIZE");
        dsp_dump(5,1,"DSP_CORES_NUMBER");
        dsp_dump(6,1,"DSP_ENCODER_VERSION");
        dsp_dump(7,1,"DSP_SUPPORTED_FREQUENCY_RANGE");
#ifdef DSP_FILEACCESS_H_
        dumpFileClose();
        dspOutFileClose();
#endif
    }

    return opcodeIndex();  // size of the program
}



// add a single dsp_code without any following parameters
static int addSingleOpcode(int code) {
    calcLength();
    check_dsp_CORE();
    return addOpcodeValue(code, 1);
}

// add a single dsp_code without any following parameters
static int addSingleOpcodePrint(int code) {
    int tmp = addSingleOpcode(code);
    dspprintf2("%s\n",dspOpcodeText[code]);
    return tmp;
}


#if 0 // removed january 2025 (not used)
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

#endif

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
    check_dsp_CORE();
    return addOpcodeUnknownLength(code);
}

static int addOpcodeLengthPrint(int code){
    int tmp = addOpcodeLength(code);
    dspprintf2("%s\n",dspOpcodeText[code]);
    return tmp;
}

//used only for dsp_PARAM and dsp_CORE and DSP_CORE_EXTERN, to avoid recursivity
static int addOpcodeLengthPrint_without_dsp_CORE(int code){
    calcLength();
    int tmp = addOpcodeUnknownLength(code);
    dspprintf2("%s\n",dspOpcodeText[code]);
    return tmp;
}

static void calcMaxParamValue(float val){
    if (val  > maxParamValue) maxParamValue = val;
    if (-val > maxParamValue) maxParamValue = -val;
}

int addGainCodeQNM(dspGainParam_t gain){
    calcMaxParamValue(gain);
    if (dspFormat < DSP_FORMAT_FLOAT) {
        float max = 1ULL << (31 - dspMant);
        float min = -max;
        if ((gain >= max) || (gain < min))
            dspprintf(">>>> WARNING : float parameter does not fit in integer format chosen (%d.%d).\n",31-dspMant,dspMant);
        return addCode(dspQM32( gain, dspMant));
    } else
        return addFloat(gain);
}

int addGainCodeQ31(dspGainParam_t gain){
    calcMaxParamValue(gain);
    if (dspFormat < DSP_FORMAT_FLOAT) {
        if ((gain > 1.0) || (gain < -1.0))
            dspprintf(">>>> WARNING : float parameter does not fit in 31 bit mantissa.\n");
        return addCode(dspQM32( gain, 31));
    } else
        return addFloat(gain);
}

int addDoubleCodeQ31(double value){
    calcMaxParamValue(value);
    if (dspFormat < DSP_FORMAT_FLOAT) {
        if ((value > 1.0) || (value < -1.0))
            dspprintf(">>>> WARNING : float parameter does not fit in 31 bit mantissa.\n");
        return addCode(dspQM32( value, 31));
    } else
        return addFloat(value);
}

// indicate No operation
void dsp_NOP() { addSingleOpcodePrint(DSP_NOP); }


// indicate start of a program for a dedicated core/task
//a core will be authorized if any bit in the 1st mask is set to 1, OR any bit in the 2nd mask is set to 0
int dsp_CORE_Prog_(unsigned opcode, unsigned progAny1, unsigned progAny0){
    calcLength();
    checkFinishedParamSection();
    updateLastSection();
    updateLastCoreIOs();
    printLastOpcodes();             // flush any opcode printing before starting with new datasets
    if (lastTileNum == 0) {
        firstTileParamSize = opcodeIndex() - firstTileIndex;
        lastTileNum++;
    }
    lastCoreNum++;
    usedInputsCore  = 0;
    usedOutputsCore = 0;
    lastTpdfDataAddressCore = 0;
    if (lastCoreNum > 1) dspout("} //end of core %d\n\n",lastCoreNum-1);
    dspout("void dsp_CORE%d() {\n   if (0==dsp_CORE(0x%x,0x%x)) return;\n",lastCoreNum,progAny1,progAny0);
    int tmp = addOpcodeLengthPrint_without_dsp_CORE(opcode);  //avoid potential recusivity!
    lastCoreOpcode = opcode;
    lastCoreIndex  = tmp;
    lastCoreData   = dspDataCounter;
    addCode(0);addCode(0);addCode(0);addCode(0); // space for 4 words for input output tracking
    addCode(0);             // space for dataSize
    addCode(progAny1);      // add a 32bit value representing compatibility of the code with 32 user programs
    addCode(progAny0);      // add a 32bit value representing compatibility of the code with 32 user programs
    ALUformat = 0;          // reset it as we start a new core
    return lastCoreNum;
}

int dsp_CORE_Prog(unsigned progAny1, unsigned progAny0){
    return dsp_CORE_Prog_(DSP_CORE,progAny1,progAny0);
}

int dsp_CORE_EXTERN_Prog(unsigned progAny1, unsigned progAny0){
    return dsp_CORE_Prog_(DSP_CORE_AES,progAny1,progAny0);
}
//a core will be authorized if any bit in the 1st mask is set to 1, AND all bits set in 2nd mask are 0
int dsp_CORE(){
    return dsp_CORE_Prog(0xFFFFFFFF,0);
}

int dsp_CORE_num() {
    return lastCoreNum;
}

void dsp_SECTION(unsigned progAny1, unsigned progOnly0){
    calcLength();
    check_dsp_CORE();
    updateLastSection();
    lastSectionData    = dspDataCounter;
    lastSectionDataMax = dspDataCounter;
    if ((progAny1 == 0xFFFFFFFF) && (progOnly0 == 0)) return;
    lastSectionProg = addOpcodeLengthPrint(DSP_SECTION);
    addCode(0);             //offset for jump (at least 4 !)
    addCode(progAny1);      //add a 32bit value representing compatibility of the code with 32 user programs
    addCode(progOnly0);     //add a 32bit value representing compatibility of the code with 32 user programs
    dspprintf3("DSP_SECTION : initial datacounter %d\n",lastSectionData);
}

void dsp_SECTION_ELSE(unsigned progAny1, unsigned progOnly0){
    calcLength();   //used to print late data
    if (lastSectionProg == 0) dspFatalError("no SECTION identified before SECTION ELSE");
    int tmp = addSingleOpcodePrint(DSP_NOP);
    updateLastSection();
    dspprintf3("DSP_SECTION ELSE : reinitialize datacounter %d\n",lastSectionData);
    dspDataCounter = lastSectionData;
    int old = lastSectionDataMax;
    dsp_SECTION(progAny1,progOnly0);
    lastSectionDataMax = old;
    lastSectionElse = tmp;
}



// clear ALU X and Y
void dsp_CLRXY(){ 
    dspout("   dsp_CLRXY();\n");
    addSingleOpcodePrint(DSP_CLRXY); }

// exchange ALU X and Y
void dsp_SWAPXY(){ 
    dspout("   dsp_SWAPXY();\n");
    addSingleOpcodePrint(DSP_SWAPXY); }

// copy ALU X to Y
void dsp_COPYXY(){ 
    dspout("   dsp_COPYXY();\n");
    addSingleOpcodePrint(DSP_COPYXY); }

// copy ALU Y to X
void dsp_COPYYX(){ 
    dspout("   dsp_COPYYX();\n");    
    addSingleOpcodePrint(DSP_COPYYX); }

// perform ALU X = X + Y
void dsp_ADDXY(){ 
    dspout("   dsp_ADDXY();\n");
    addSingleOpcodePrint(DSP_ADDXY); }

// perform ALU2 Y = X + Y
void dsp_ADDYX(){ 
    dspout("   dsp_ADDYX();\n");
    addSingleOpcodePrint(DSP_ADDYX); }

// perform ALU X = X - Y
void dsp_SUBXY(){ 
    dspout("   dsp_SUBXY();\n");
    addSingleOpcodePrint(DSP_SUBXY); }

// perform ALU Y = Y - X
void dsp_SUBYX(){ 
    dspout("   dsp_SUBYX();\n");
    addSingleOpcodePrint(DSP_SUBYX); }

// perform ALU X = X * Y
void dsp_MULXY(){ 
    dspout("   dsp_MULXY();\n");
    addSingleOpcodePrint(DSP_MULXY); }

// perform ALU Y = X * Y
void dsp_MULYX(){ 
    dspout("   dsp_MULYX();\n");
    addSingleOpcodePrint(DSP_MULYX); }

// perform ALU X = X / Y
void dsp_DIVXY(){ 
    dspout("   dsp_DIVXY();\n");
    addSingleOpcodePrint(DSP_DIVXY); }

// perform ALU2 Y = Y / X
void dsp_DIVYX(){ 
    dspout("   dsp_DIVYX();\n");
    addSingleOpcodePrint(DSP_DIVYX); }

// perform ALU X = X / Y
void dsp_AVGXY(){ 
    dspout("   dsp_AVGXY();\n");
    addSingleOpcodePrint(DSP_AVGXY); }

// perform ALU2 Y = Y / X
void dsp_AVGYX(){ 
    dspout("   dsp_AVGYX();\n");
    addSingleOpcodePrint(DSP_AVGYX); }

// perform ALU X = sqrt(X)
void dsp_SQRTX(){ 
    dspout("   dsp_SQRTX();\n");
    addSingleOpcodePrint(DSP_SQRTX); }

void dsp_NEGX(){ 
    dspout("   dsp_NEGX();\n");
    addSingleOpcodePrint(DSP_NEGX); }

void dsp_NEGY(){ 
    dspout("   dsp_NEGY();\n");
    addSingleOpcodePrint(DSP_NEGY); }

int dsp_FUNC_MEM(int op, int paramAddr) {
    dspout("   dsp_FUNC_MEM(%d,%d);\n",op,opcodeIndex()-paramAddr);
    int tmp = addOpcodeLengthPrint(op);
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);
    return tmp;
}


void dsp_FULL_LOAD(int IO) {
    checkIOmax(IO);
    if (IO<64) usedOutputs |= 1ULL<<IO;
    if (IO<64) usedOutputsCore |= 1ULL<<IO;
    dspout("   dsp_FULL_LOAD();\n");
    int tmp = addOpcodeLengthPrint(DSP_FULL_LOAD); 
    addCode(IO);
    int size = opcodePtr(lastCoreIndex)->op.skip;
    int start = lastCoreIndex+size;
    if (tmp != start) dspFatalError("dsp_FULL_LOAD must be first instruction in a core ");
}


void dsp_WHITE() {
    checkCalcTpdf();
    dspout("   dsp_WHITE();\n");
    addSingleOpcodePrint(DSP_WHITE); }

void dsp_SAT0DB_VOL(){
    dspout("   dsp_SAT0DB_VOL();\n");
    addSingleOpcodePrint(DSP_SAT0DB_VOL);
    ALUformat = 0;  //after this instruction, the ALU contains a 32bit value unscaled, ready to be stored to a DAC output.
}

void dsp_SAT0DB() {
    dspout("   dsp_SAT0DB();\n");
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
    dspout("   dsp_SAT0DB_GAIN(%f);\n",gain);
    dsp_SAT0DB_GAIN(0);
    addGainCodeQNM(gain);
}

int dsp_TPDF_CALC(int dither){
    if (lastTpdfDataAddress == 0) {
        dspout("   dsp_TPDF_CALC(%d);\n",dither);
        addOpcodeLengthPrint(DSP_TPDF_CALC);
    } else {
        dspout("   dsp_TPDF(%d);\n",dither);
        addOpcodeLengthPrint(DSP_TPDF);
    }
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
    dspout("   dsp_SHIFT(%d);\n",bits);
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
    if (IO<64) usedInputs |= 1ULL<<IO;      //keep track of inputs used
    if (IO<64) usedInputsCore |= 1ULL<<IO;
    dspout("   dsp_LOAD(%d);\n",IO);
    addOpcodeLengthPrint(dspMant?DSP_LOAD:DSP_FLOAD);
    addCode(IO);
}

void dsp_LOAD_GAIN(int IO, int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(DSP_LOAD_GAIN);
    checkIOmax(IO);
    if (IO<64) usedInputs |= 1ULL<<IO;
    if (IO<64) usedInputsCore |= 1ULL<<IO;
    if (paramAddr) checkInParamSpace(paramAddr,1);
    addCode(IO);
    addCodeOffset(paramAddr, tmp);
    setLastMissingParamIf0(paramAddr, 1);   // possibility to define the gain just below the opcode
}

void dsp_LOAD_GAIN_Fixed(int IO, dspGainParam_t gain) {
    dspout("   dsp_LOAD_GAIN(%d,%f);\n",IO,gain);
    dsp_LOAD_GAIN(IO, 0);
    addGainCodeQNM(gain);   // store the fixed gain just after the opcode
}

// load many inputs and apply a gain to them
int dsp_LOAD_MUX(int paramAddr){
    ALUformat = 1;
    dspout("//dsp_LOAD_MUX(&mux);  //TODO\n");
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
    if (in<64) usedInputs |= 1ULL<<in;
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
        if (out<64) usedOutputs |= 1ULL<<out;
        if (out<64) usedOutputsCore |= 1ULL<<out;
        IO >>= 8;
        if (IO==0) break;
    }
}

void dsp_STORE(int IO) {
    dspout("   dsp_STORE(%d);\n",IO);
    addOpcodeLengthPrint(dspMant?DSP_STORE:DSP_FSTORE);
    dsp_STORE_IO(IO);
}

void dsp_STORE_VOL(int IO) {
    dspout("   dsp_STORE_VOL(%d);\n",IO);
    addOpcodeLengthPrint(DSP_STORE_VOL);
    dsp_STORE_IO(IO);
}

void dsp_STORE_VOL_SAT(int IO) {
    dspout("   dsp_STORE_VOL_SAT(%d);\n",IO);
    addOpcodeLengthPrint(DSP_STORE_VOL_SAT);
    dsp_STORE_IO(IO);
}

void dsp_STORE_TPDF(int IO) {
    int addrtpdf = checkCalcTpdf();
    dspout("   dsp_STORE_TPDF(%d);\n",IO);
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
    dspout("   dsp_STORE_GAIN(%d,%f);\n",IO,gain);
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
    dspout("// dsp_PARAM section start\n");
    int tmp = addOpcodeLengthPrint_without_dsp_CORE(DSP_PARAM);
    lastParamNumIndex = tmp; // indicate that we are inside a PARAM_NUM statement
    return tmp;
}

// DSP_PARAM_NUM

int dsp_PARAM_NUM(int num) {
    dspout("// dsp_PARAM_NUM(%d) section start\n",num);
    int tmp = addOpcodeLengthPrint_without_dsp_CORE(DSP_PARAM_NUM);
    lastParamNumIndex = tmp;
    addCode(num);
    return tmp;
}


int dsp_TILE() {    //do not generate opcode
    if (lastTileNum == 0) { //first time we see a tile keyword.
        lastTileNum++;
        firstTileParamSize = opcodeIndex() - firstTileIndex;
    } else {
        //this is a new TILE so finish previous header and start a new one.
        dspHeaderDone();
        lastTileNum++;
        opcode_t * oldCodePtr;
        oldCodePtr = dspOpcodesPtr;
        int index = opcodeIndex();
        dspprintf1("NEW TILE %d\n",lastTileNum);
        dspHeaderInit(&dspOpcodesPtr[index]);
        //recopie param num declared before first tile keyword
        for (int i=0; i<firstTileParamSize ; i++) {
            addCode(oldCodePtr[firstTileIndex+i].i32);
        }
    }
    return lastTileNum;
}

int  dsp_TILE_num() {
    return lastTileNum;
}

/*
 * GAIN
 */


void dsp_GAIN(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(dspMant?DSP_GAIN:DSP_FGAIN);
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
    dspout("   dsp_GAIN(%f);\n",gain);
    int tmp = addOpcodeLengthPrint(dspMant?DSP_GAIN:DSP_FGAIN);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(gain);
}


void dsp_VALUEX_Fixed(float value){
    ALUformat = 1;
    dspout("   dsp_VALUEX(%f);\n",value);
    int tmp = addOpcodeLengthPrint(dspMant?DSP_VALUEX:DSP_FVALUEX);
    addCodeOffset(0, tmp);  // value is just below
    addGainCodeQNM(value);
}
 
void dsp_VALUEX(int paramAddr){
    ALUformat = 1;
    int tmp = addOpcodeLengthPrint(dspMant?DSP_VALUEX:DSP_FVALUEX);
    checkInParamSpace(paramAddr,1);
    addCodeOffset(paramAddr, tmp);
}

void dsp_VALUEY_Fixed(float value){
    ALUformat = 1;
    dspout("   dsp_VALUEY(%f);\n",value);
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


void dsp_INTEGRATOR(){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_INTEGRATOR);
    int data = addDataSpaceAligned8(2);    // 2 words for supporting 64bits alu
    dspout("   dsp_INTEGRATOR(%d);\n",data);
}


void dsp_DELAY_1(){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_DELAY_1);
    int data = addDataSpaceAligned8(2);    // 2 words for supporting 64bits alu
    dspout("   dsp_DELAY_1(%d);\n",data);
}

// DSP_SERIAL
void dsp_SERIAL(unsigned hash) {
    dspout("   dsp_SERIAL(0x%X);\n",hash);
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
    dspout("   dsp_LOAD_STORE(0,0); //TODO missing parameters\n");
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
    if (in<64)  usedInputs  |= 1ULL<<in;
    if (in<64)  usedInputsCore  |= 1ULL<<in;
    if (out<64) usedOutputs |= 1ULL<<out;
    if (out<64) usedOutputsCore |= 1ULL<<out;
}

// DSP_MIXER
void dsp_MIXER(){  // this function must be followed by couples of data (input & output)
    ALUformat = 1;
    dspout("//dsp_MIXER(); //TODO list of parameters\n");
    addOpcodeLengthPrint(DSP_MIXER);
    setLastMissingParam(2);   // alway expect the parameters to be provided in the following opcode,
                              // at least 2 words

}

void dspMixer_Data(int in,  dspGainParam_t gain){

    checkLastMissing(DSP_MIXER);       // verify that a dsp_MIXER() is just above
    checkIOmax(in);
    addCode(in);
    addGainCodeQNM(gain);
    if (in<64)  usedInputs  |= 1ULL<<in;
    if (in<64)  usedInputsCore  |= 1ULL<<in;
}

static void addMemLocation(int index, int base){
    checkInParamSpace(index, 2);
    addCodeOffset(index, base);
}

// load a meory location from a PARAM area
void dsp_LOAD_X_MEM_Index(int paramAddr, int index) {
    ALUformat = 1;
    dspout("//dsp_LOAD_X_MEM(); //TODO\n");
    int tmp = addOpcodeLengthPrint(DSP_LOAD_X_MEM);
    addMemLocation(paramAddr + index*2, tmp);
}

void dsp_STORE_X_MEM_Index(int paramAddr, int index) {
    dspout("//dsp_STORE_X_MEM(); //TODO\n");
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
    dspout("//dsp_LOAD_Y_MEM(); //TODO\n");
    int tmp = addOpcodeLengthPrint(DSP_LOAD_Y_MEM);
    addMemLocation(paramAddr + index*2, tmp);
}

void dsp_STORE_Y_MEM_Index(int paramAddr, int index) {
    dspout("//dsp_STORE_Y_MEM(); //TODO\n");
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
    int size = opcodePtr(paramAddr)->i32;       // get max delay line in samples
    addCode(size);                              // store the max size of the delay line for runtime to check due to user potential changes
    if (size) {
        if (opcode == DSP_DELAY_DP)
            addDataSpaceMisAligned8(size*2+1);      // now we can request the data space
        else addDataSpace(size+1);
    } else addCode(0);
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

static void dsp_DELAY_max_(int paramAddr, int max, int opcode){
    checkInParamSpace(paramAddr, 1);
    int tmp = addOpcodeLengthPrint(opcode);
    addCode(max);                              // store the max size of the delay line for runtime to check due to user potential changes
    if (opcode == DSP_DELAY_DP)
         addDataSpaceMisAligned8(max*2+1);      // now we can request the data space
    else addDataSpace(max+1);
    addCodeOffset(paramAddr, tmp);              // point on where is the delay in uSec
}

void dsp_DELAY_max(int paramAddr, int max){
    dsp_DELAY_max_(paramAddr,max,DSP_DELAY);
}

void dsp_DELAY_DP_max(int paramAddr, int max){
    dsp_DELAY_max_(paramAddr,max,DSP_DELAY_DP);
}

// genertae one word code combining the default uS value in LSB and with the max value in MSB
static int dspDelay_MicroSec(int maxus, int us){
    checkInParamNum();  // check if we are in a PARAM or PARAM_NUM section
    checkFinishedParamSection();
    signed long long maxSamples = (((signed long long)maxus * dspConvertFrequencyFromIndex(dspMaxSamplingFreq) + 500000)) / 1000000;
    addCode(us);    //changed from short to int.  runtime to be verified
    return addCode(maxSamples);
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

static void dsp_DELAY_FixedMicroSec_(int microSec, int opcode){
    int DP = 1;
    if (opcode == DSP_DELAY_DP) { DP = 2; ALUformat = 1; } else ALUformat = 0;
    addOpcodeLengthPrint(opcode);
    unsigned long long delayLineFactor = dspTableDelayFactor[dspMaxSamplingFreq];
    unsigned long long maxSamples_ = (delayLineFactor * microSec);
    maxSamples_ >>= 32;
    unsigned maxSamples = maxSamples_;
    delayLineFactor = dspTableDelayFactor[dspMinSamplingFreq];
    unsigned long long minSamples_ = (delayLineFactor * microSec);
    minSamples_ >>= 32;
    unsigned minSamples = minSamples_;
    int fshi = dspConvertFrequencyFromIndex(dspMaxSamplingFreq);
    int fslo = dspConvertFrequencyFromIndex(dspMinSamplingFreq);
    dspprintf2("    DELAY %dus -> %d samples @%d -> %.0fus. @%d -> %.0fus\n",microSec,maxSamples,fshi,(float)maxSamples / (float)fshi * 1000000.0,fslo,(float)minSamples / (float)fslo * 1000000.0);
    if (maxSamples) {
        addCode(microSec);  // store the expected delay in uSec
        int data;
        if (DP == 1 ) {
            data = addDataSpace(1 + maxSamples); // request data space (including index) and store the pointer
            if (opcode == DSP_DELAY) dspout("   dsp_DELAY(%d,%d,%d);\n",microSec,data,maxSamples);
            else dspout("//dsp_DELAY_FB_MIX(..); //TODO\n");
        } else {
            data = addDataSpaceMisAligned8(1 + maxSamples*2);
            dspout("   dsp_DELAY_DP(%d,%d,%d);\n",microSec,data,maxSamples);
        }
    } else {
        addCode(0);addCode(0);
    }
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
    //dspout("   dsp_DELAY_FB_MIX(%d,%f,%f,%f,%f);\n",microSec,source,fb,delayed,mix);
    dsp_DELAY_FixedMicroSec_(microSec, DSP_DELAY_FB_MIX);
    addGainCodeQ31(source);
    addGainCodeQ31(fb);
    addGainCodeQ31(delayed);
    addGainCodeQ31(mix);
}


void dsp_CIC_FixedMicroSec(int microSec){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_CICUS);
    unsigned long long delayLineFactor = dspTableDelayFactor[dspMaxSamplingFreq];
    unsigned long long maxSamples_ = (delayLineFactor * (unsigned)microSec);
    maxSamples_ >>= 32;
    unsigned maxSamples = maxSamples_;
    delayLineFactor = dspTableDelayFactor[dspMinSamplingFreq];
    unsigned long long minSamples_ = (delayLineFactor * microSec);
    minSamples_ >>= 32;
    if (minSamples_<2) dspFatalError("minimum 2 samples required");
    int fs = dspConvertFrequencyFromIndex(dspMaxSamplingFreq);
    dspprintf2("    CIC FILTER %dus -> %d samples @%d -> %.0fus\n",microSec,maxSamples,fs,(float)maxSamples / (float)fs * 1000000.0);
    addCode(microSec);  // store the expected delay in uSec
    int data = addDataSpaceMisAligned8(1 + (maxSamples+1)*2);
    dspout("   dsp_CIC(%d,%d,%d);\n",microSec, data, 1 + (maxSamples+1)*2);
    for (int f = dspMinSamplingFreq; f <= dspMaxSamplingFreq; f++ ) {
        // generate list of divider according to number of samples depending on fs
        delayLineFactor = dspTableDelayFactor[f];
        unsigned long long samples = (delayLineFactor * (unsigned)microSec);
        samples >>= 32;
        double coef = samples;
        coef = 2.0 / coef;
        addDoubleCodeQ31(coef);
    }
}

void dsp_CIC_N(int maxSamples){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_CICN);
    if (maxSamples<2) dspFatalError("minimum 2 samples required");
    addCode(maxSamples);
    int data = addDataSpaceMisAligned8(1 + (maxSamples+1)*2);
    // generate coef according to maxSamples
    double coef = maxSamples;
    coef = 2.0 / coef;
    dspout("   dsp_CIC_N(%d,%d,%d,%f);\n",data,maxSamples,1 + (maxSamples+1)*2,coef);
    addDoubleCodeQ31(coef);
}

void dsp_EXPMA(double alpha) {
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_EXPMA);
    int data = addDataSpaceAligned8(2);            //book a 64bit location
    dspout("   dsp_EXPMA(%d,%f);\n",data,alpha);
    addDoubleCodeQ31(alpha);
}


void dsp_THDCOMP(double c2, double c3){
    ALUformat = 1;
    addOpcodeLengthPrint(DSP_THDCOMP);
    dspout("   dsp_THDCOMP(%f,%f);\n",c2,c3);
    addDoubleCodeQ31(c2);
    addDoubleCodeQ31(c3);
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
        addDoubleCodeQ31(x); }
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
    int base = addOpcodeLengthPrint(dspMant?DSP_BIQUADS:DSP_FBIQUADS);
    checkInParamSpaceOpcode(paramAddr,2+6*numberFrequencies, DSP_BIQUADS);  // biquad coef are only store in param section
    int num = opcodePtr(paramAddr)->s16.low;  // get number of sections provided
    checkInParamSpace(paramAddr,(2+6*numberFrequencies)*num);
    int addrValue = addDataSpaceAligned8(num*(dspMant?6:8));  // 2 words for mantissa reintegration + 4 words for each data (xn-1, xn-2, yn-1, yn-2)
    dspout("   dsp_BIQUADS(&%s,%d,%d,%d); //TODO\n",dspOutLabelName,num,addrValue,num*6);
    addCodeOffset(paramAddr, base);        // store pointer on the table of coefficients
    // from release 1.0 this returns the adress where the Biquaed calculated value is stored
    return addrValue+((num-1)*6);           // to be tested
}

int dsp_BIQUADS_FS(int paramAddr){
    ALUformat = 1;
    int base = addOpcodeLengthPrint(DSP_BIQUADS_FS);
    checkInParamSpaceOpcode(paramAddr,1+6*2, DSP_BIQUADS_FS);
    int num = opcodePtr(paramAddr)->s16.low;  // get number of sections provided
    checkInParamSpace(paramAddr,(1+6*num*2));
    addCode(num);   //number of section
    addCodeOffset(paramAddr+1+6*num, base);     //ofset of the computed , in the code space
    int data = addDataSpaceAligned8(num*6);     // 2 words for mantissa reintegration + 4 words for each data (xn-1, xn-2, yn-1, yn-2)
    dspout("   dsp_BIQUADS_FS(&%s[%d],%d,%d,%d);\n",dspOutLabelName,num,num,data,num*6);
    return base;
}

int dspBiquad_Sections(int number){
    startParamSection(dspDynamic? DSP_BIQUADS_FS:DSP_BIQUADS, number); // check and initialize conditions for the follwoing data in the PARAM section
    int pos = paramMisAligned8();
    lastSectionIndex = addOpcodeValue(dspDynamic? DSP_BIQUADS_FS:DSP_BIQUADS, number);    // store the number of following sections
    if (number>0) dspprintf3("\n%4d : biquad section expecting %d cell(s)\n",pos,number)
    else
        if (number<0)
             dspprintf3("\n%4d : biquad section expecting maximum %d cell(s)\n",pos,-number)
        else dspprintf3("\n%4d : biquad section\n",pos);
    if (dspDynamic==0) addCode(1);  // this is the bypass parameter
    return pos;
}

int  dspBiquad_Sections_Flexible(){
    return dspBiquad_Sections(0);
}
int  dspBiquad_Sections_Maximum(int number){
    return dspBiquad_Sections(-number);
}


void sectionBiquadCoeficientsBegin(){
    nextParamSection(dspDynamic? DSP_BIQUADS_FS:DSP_BIQUADS);
}

void sectionBiquadCoeficientsEnd(){
    if (lastSectionOpcode == 0) // last section of biquad
        // cancell printing of coeeficients,  as they have been printed in another way
        printFromCurrentIndex();
}

int addFilterParams(int type, dspFilterParam_t freq, dspFilterParam_t Q, dspFilterParam_t freq2, dspFilterParam_t Q2, dspGainParam_t gain){
    if (dspDynamic == 0) {
        int tmp = addOpcodeValue(type, freq);
        if (tmp & 1) {
            addFloat(Q);
            addFloat(gain);
        }else
            dspFatalError("Encoder bug (not expected). Adress should be misalligned here");
        return tmp;
    } else {
        int tmp = addCode(type);
        if (tmp & 1) dspFatalError("Encoder bug (not expected). Adress should be 64bits alligned here");
        addFloat(freq);
        addFloat(Q);
        addFloat(gain);
        addFloat(freq2);
        addFloat(Q2);
        return tmp;
    }
}

int addBiquadCoeficients(dspFilterParam_t b0,dspFilterParam_t b1,dspFilterParam_t b2,dspFilterParam_t a1,dspFilterParam_t a2){
    calcMaxParamValue(b0);
    calcMaxParamValue(b1);
    calcMaxParamValue(b2);
    calcMaxParamValue(a1-(dspMant?1.0:0.0));
    calcMaxParamValue(a2);
    if (dspDynamic==0) {
        int tmp = paramAligned8();    // this enforce that coefficient are alligned 8, so 6 words per biquads and per frequency
        addGainCodeQNM(b0);
        addGainCodeQNM(b1);
        addGainCodeQNM(b2);
        addGainCodeQNM(a1 - (dspMant?1.0:0.0)); // to make things bette for integer routines
        addGainCodeQNM(a2);
        return tmp;
    } else {
        return opcodeIndex();   //no coefficient creation in dynamic mode
    }
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
    dspout("//dsp_FIR(&taps); //TODO\n");
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

int dsp_CONVOL() {
    int temp = addOpcodeLengthPrint(DSP_FIR);
    addCode(0); //size will be defined later, put a zero as place holders
    return temp;
}
int dsp_WARPCONVOL() {
    int temp = addOpcodeLengthPrint(DSP_WFIR);
    addCode(0); //size will be defined later, put a zero as place holders
    return temp;
}

// integrate a s.31 sample during x miliseconds. then moving average in delay line and Sqrt
// result is s.31. should be used after dsp_STORE or dsp_LOAD or dsp_SAT0DB or dsp_DELAY
void dsp_RMS_(int timetot, int delay, int delayInSteps, int pwr){
    ALUformat = 1;
    dspout("//dsp_RMS(...); //TODO\n");
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
    dspout("   dsp_DCBLOCK(%d);\n",lowfreq);
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
    dspout("   dsp_DITHER();\n");
    addOpcodeLengthPrint(DSP_DITHER);
    addDataSpaceAligned8(6);    // might be double so 3x2
}

void dsp_DITHER_NS2(int paramAddr){
    checkCalcTpdf();
    ALUformat = 1;
    dspout("//dsp_DITHER_NS2(&params); //TODO\n");
    // support only 6 triplets of coefficients in this version
    if ((dspMinSamplingFreq<F44100)||(dspMaxSamplingFreq>F192000))
        dspFatalError("frequency range provided in encoderinit incompatible.");
    int base = addOpcodeLengthPrint(DSP_DITHER_NS2);
    checkInParamSpace(paramAddr,3*numberFrequencies);   // requires 3 coef for each supported frequencies
    addDataSpaceAligned8(3);                    // create space for 3 errors bin potentially 2 words
    addCodeOffset(paramAddr, base);             // relative pointer to the data table
}

void dsp_DISTRIB(int IO, int size){
    dspout("   dsp_DISTRIB(%d,%d);\n",IO,size);
    addOpcodeLengthPrint(DSP_DISTRIB);
    checkIOmax(IO);
    addCode(IO);
    if (IO<64) usedOutputs |= 1ULL<<IO;
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
    dspout("   dsp_DIRAC(%d,%f);\n", freq, gain);
    addOpcodeLengthPrint(DSP_DIRAC);
    dsp_DIRAC_(freq, gain);
}

void dsp_SQUAREWAVE_Fixed(int freq, dspGainParam_t gain){
    dspout("   dsp_SQUAREWAVE(%d,%f);\n",freq,gain);
    addOpcodeLengthPrint(DSP_SQUAREWAVE);
    dsp_DIRAC_(freq, gain);
}


void dsp_CLIP_Fixed(dspGainParam_t value){
    ALUformat = 1;
    dspout("   dsp_CLIP(%f);\n",value);
    addOpcodeLengthPrint(DSP_CLIP);
    if ((value > 1.0) || (value < 0.0))
        dspFatalError("value not in range 0..1.0");
    addGainCodeQNM(value);
}

void dsp_SINE_Fixed(int freq, dspGainParam_t gain){
    ALUformat = 1;
    dspout("   dsp_SINE(%d,%f);\n",freq,gain);
    addOpcodeLengthPrint(DSP_SINE);
    int fmax = dspConvertFrequencyFromIndex(dspMaxSamplingFreq);
    checkInRange(freq, 10, fmax/2-1);
    addDataSpaceAligned8(4);    //data space for computing xn and yn each in 64bits
    addGainCodeQNM(gain);
    for (int f=dspMinSamplingFreq; f<=dspMaxSamplingFreq; f++){
        int fs = dspConvertFrequencyFromIndex(f);
        double omega = 2.0*M_PI*(double)freq / (double)fs;
        double alpha = cos(omega);
        double start = sin(omega);
        addDoubleCodeQ31(alpha);
        addDoubleCodeQ31(start);
    }
}


void dspoutFilters3(int type, int order, float freq,float Q,float gain, const char * name) {
    dspout("   { %d, %f, %f, %f, 0, 0 }, //%s\n",type, freq, Q, gain, name);
    order +=1; order >>= 2;
    for (int i=0; i<order;i++) dspout("   { 0, 0, 0, 0, 0, 0 },\n");
}
void dspoutFilters5(int type, int order, float freq,float Q, float freq2,float Q2,float gain, const char * name) {
    dspout("   { %d, %f, %f, %f, %f, %f }, //%s\n",type, freq, Q, freq2, Q2, gain, name);
    order +=1; order >>= 2;
    for (int i=0; i<order;i++) dspout("   { 0,0,0,0 },\n");
}