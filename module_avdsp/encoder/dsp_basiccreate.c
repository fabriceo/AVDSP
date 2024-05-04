#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dsp_filters.h"
#include "dsp_encoder.h"

static FILE* dspbasicInput  = NULL;    //user basic dsp code as text file

#define filterTypesNumber 56

const char * filterNames[filterTypesNumber] = {
    "LPBE2","LPBE3","LPBE4","LPBE6","LPBE8",
    "HPBE2","HPBE3","HPBE4","HPBE6","HPBE8",
    "LPBE3db2","LPBE3db3","LPBE3db4","LPBE3db6","LPBE3db8",
    "HPBE3db2","HPBE3db3","HPBE3db4","HPBE3db6","HPBE3db8",
    "LPBU2","LPBU3","LPBU4","LPBU6","LPBU8",
    "HPBU2","HPBU3","HPBU4","HPBU6","HPBU8",
    "LPLR2","LPLR3","LPLR4","LPLR6","LPLR8",
    "HPLR2","HPLR3","HPLR4","HPLR6","HPLR8",
    "LP1","HP1","LS1","HS1","AP1","BP0DB",
    "LP2","HP2", "LS2","HS2", "AP2",
    "PEAK","NOTCH","BPQ","HILB","LT"
};

const char filterTypes[filterTypesNumber] = {
    LPBE2,LPBE3,LPBE4,LPBE6,LPBE8,   // bessel
    HPBE2,HPBE3,HPBE4,HPBE6,HPBE8,
    LPBE3db2,LPBE3db3,LPBE3db4,LPBE3db6,LPBE3db8,    // bessel at -3db cutoff
    HPBE3db2,HPBE3db3,HPBE3db4,HPBE3db6,HPBE3db8,
    LPBU2,LPBU3,LPBU4,LPBU6,LPBU8, // buterworth
    HPBU2,HPBU3,HPBU4,HPBU6,HPBU8,
    LPLR2,LPLR3,LPLR4,LPLR6,LPLR8,   // linkwitz rilley
    HPLR2,HPLR3,HPLR4,HPLR6,HPLR8,
    FLP1,FHP1,FLS1,FHS1,FAP1,FBP0DB,
    FLP2,FHP2,    // low pass and high pass
    FLS2,FHS2,    // low shelf and high shelf
    FAP2,FPEAK,FNOTCH, // allpass, eak and notch
    FBPQ, FHILB, FLT      // bandpass and hilbert
};

const char filterOrders[filterTypesNumber] = {
    2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8,
    1,1,1,1,1,2,2,2,2,2,2,2,2,2,99,2
};

enum keywords_e {
    _end, _param, _nop, _core, _section,
    _input, _output, _transfer, _inputgain, _outputgain, _outputpdf, _mixer, _mixergain, _gain, _clip,
    _clrxy,_swapxy,_copyxy,_copyyx,_addxy,_addyx,_subxy,_subyx,_mulxy,_mulyx, _divxy,_divyx,_avgxy,_avgyx,_negx,_negy,_shift,_valuex,_valuey,
    _saturate, _saturatevol, _saturategain, _saturatetpdf, _saturategaintpdf,
    _delayone, _delayus, _delaydpus,
    _savexmem, _loadxmem,  _saveymem, _loadymem,
    _dcblock, _biquad, _biquad8, _convol,
    _tpdf, _white, _sine,_square,_dirac,
    dspKeywordsNumber
};
static const char * dspKeywords[dspKeywordsNumber] = {
    "end", "param", "nop", "core", "section",
    "input", "output","transfer", "inputgain", "outputgain", "outputtpdf", "mixer","mixergain","gain","clip",
    "clrxy","swapxy","copyxy","copyyx","addxy","addyx","subxy","subyx","mulxy","mulyx","divxy","divyx","avgxy","avgyx","negx","negy","shift","valuex","valuey",
    "saturate", "saturatevol","saturategain", "saturatetpdf", "saturategaintpdf",
    "delayone", "delayus", "delaydpus",
    "savexmem", "loadxmem","saveymem", "loadymem",
    "dcblock", "biquad", "biquad8", "convol",
    "tpdf", "white", "sine","square","dirac",
};

#define paramKeywordsNumber 2
static const char * paramKeywords[paramKeywordsNumber] = { "MEMORY","TAPS" };

enum   tvalue_e { _tIO, _tfreq, _tgain,_tdelay, _tfilterQ, _tmem, _tshift, _ttpdf, _ttaps, _tnone };
double valueMin[_tnone] = {  0,    10, -8,    0,  0 ,  1, -32, 8,   1  };
double valueMax[_tnone] = { 31, 40000, +8, 1000, 20 , 16,  32, 31, -1 };
double errMin=0.0, errMax=0.0;

static inline int isNumber(char ch){
    return ( (ch >= '0' ) && ( ch <= '9' ) ) ? ch : 0;
}

static inline int isLetter(char ch){
    return ( ((ch >= 'a' ) && ( ch <= 'z' )) ||
             ((ch >= 'A' ) && ( ch <= 'Z' )) ||
              (ch == '_') ) ? ch : 0;
}

static inline int isHexaNum(char ch) {
    return ( ((ch >= 'a' ) && ( ch <= 'f' )) ||
             ((ch >= 'A' ) && ( ch <= 'F' )) ||
             isNumber(ch) ) ? ch : 0;
}

static inline int isBinary(char ch){
    return ( (ch >= '0' ) && ( ch <= '1' ) ) ? ch : 0;
}

static inline int isAlphanum(char ch){
    return ( isLetter(ch) || isNumber(ch) ) ? ch : 0;
}


static inline int isSpaceOrTab(char ch) {
    return ( (ch == ' ' ) || ( ch == 9 ) ) ? ch : 0;
}

static inline char * skipSpaces(char * * s){
    char * p = *s;
    while (isSpaceOrTab(*p)) p++;
    *s = p;
    return p;
}

//check if current delimiter character is in the given string then return its value otherwise 0
static int searchDelimiter(char * * s, char * delim) {
    char * p = skipSpaces(s);
    int len = strlen(delim);
    for (int i = 0; i<len; i++)
        if (*p == delim[i]) {
            *s = p+1;
            return delim[i]; }
    return 0;
}

static int testDelimiter(char * * s, char * delim) {
    int res = searchDelimiter( s , delim);
    if (res) (*s)--;
    return res;
}


//search a possible keyword in *s and return its index otherwise -1
//pass a table of string and a number of keywords to be screened
static int searchKeywords(char * * s, const char * * keywords, int num){
    char * p = skipSpaces( s );
    for (int i = 0; i < num; i++) {
        if (p == strstr( p, keywords[i]) ) {
            int len = strlen( keywords[i] );
            if ( isAlphanum( p[len] ) ) continue; //not exactly same
            *s = &p[len];
            return i; }
    }
    return -1;
}

enum label_type_e {
    _error = -1,
    _empty = 0,         //not yet computed
    _value = 1,         //contains a float
    _valuedb = 2,       //contains a value in decibel
    _valueint = 3,      //contains an integer
    label_filter = 4,   //point on a list of filters in param space
    label_memory = 5,    //points on a 64bit data location in param space
    label_taps = 6      //points on a list of Taps
};

typedef struct label_s {
    struct label_s *   next;
    enum label_type_e labelType; //store the type of label
    double       value;          //value when labelType = 1;
    int          address;        //adress of the filter bank in the dspcode or MEM location
    char         name[1];        //labelname will be extended during malloc : KEEP it at bottom of structure
} label_t;

typedef label_t * labelptr_t;

//contains pointer on the list of labels created
static labelptr_t firstLabel = NULL, lastLabel = NULL;

//add a new label in the list and return its allocated pointer.
//character pointer expected to point on label and then moved after label.
static labelptr_t appendNewLabel(char * * s) {
    char * p = *s;
    int len = 0;
    while ( isAlphanum(*p) ) { p++; len++; }
    labelptr_t l = malloc(sizeof(label_t)+len); //todo potential error...
    l->next = NULL;
    l->labelType = _empty;
    if (firstLabel) { lastLabel->next = l; lastLabel = l; }
    else { firstLabel = lastLabel = l; }
    int i;
    for (i=0, p=*s; i<len; i++) l->name[i] = *(p++);
    l->name[len] = 0;
    *s = p;
    return l;
}

//search a possible label in *s then returns its pointer otherwise 0
static label_t * searchLabel(char * *s){
    char * p = skipSpaces(s);
    for (labelptr_t l = firstLabel; l ; l=l->next) {
        if ( p == strstr( p, l->name ) ) {
            int len = strlen( l->name );
            if ( isAlphanum( p[len]) ) continue; //not yet
            *s = &p[len];
            return l;
        }
    }
    return NULL;
}

static void freeAllLabels(){
    labelptr_t l = firstLabel;
    while (l) {
        labelptr_t next = l->next;
        free(l);
        l = next;
    }
}


//points on previous set of character where an error is detected
static char * errPtr;
static char * errPtrPrev;
static int errNum;


//try to extract a numerical value from the charater pointer.
//then returns 1 for real or 2 for decibel and 3 for integer and provides result in "value" param
//otherwise return 0
static int searchValue(char * * s, double * value, int enableDB) {
    int state = 0;
    double mantisse = 1.0, sign = 1.0, base = 10.0, result = 0.0;
    char * p = skipSpaces(s);
    while (*p && (*p != '#')) {
        if (state == 0) {
            //sign authorized only at the begining
            if ( (*p) == '-') { sign = -sign; p++; continue; }
            if ( (*p) == '+') { p++; continue; }
            if ( isSpaceOrTab(*p) ) { p++; continue; }
            if ( (*p) == 'b') { base=2;  p++;state |= 1; continue; }
            if ( (*p) == 'x') { base=16; p++;state |= 1; continue; }
            if ( (*p) == 'o') { base=8;  p++;state |= 1; continue; }
        }
        if (state == 1) {
            //point authorised only after at least one digit
            if (*p =='.') { state |= 2; p++; }
        }
        if (state) {
            if ( (p[0]=='d')||(p[0]=='D'))
                if ((p[1]=='b')||(p[1]=='B')) {
                    //check if authorized
                    if ((enableDB == 0)||(base != 10.0)) {
                        *s = p; //should return a specific error
                        return _empty; }
                    *value = pow( 10, result * sign / 20.0 );
                    *s = p+2;
                    return _valuedb; } //decibel
        }
        if ( ((base==8.0)  && isNumber(  *p ) && (*p<'9')) ||
             ((base==10.0) && isNumber(  *p )) ||
             ((base==16.0) && isHexaNum( *p )) ||
             ((base==2.0)  && isBinary(  *p )) ) {
            state |= 1;
            if (state & 2) mantisse /= 10.0;
            else result *= base;
            int num;
            if ((base == 16.0) && isLetter(*p)) num = (*p & 7)+9;
            else num = *p - '0';
            result += num * mantisse;
            p++;
        } else break;  //unknown character
    } // while
    if (state) {
        *s = p;
        result *= sign; *value = result;
        int integer = result; double check = integer;
        if (check == result) return _valueint;
        else return _value;
    }
    return _empty;
}


//search for a direct numerical value, or a label of type value
static int testExpression(char * * s, double * value){
    double sum = 0.0, mul = 0.0, temp;
    int prevop = 0, modeDB = 0;
    int res;
    errPtrPrev = skipSpaces ( s );
    do {
        errPtr = *s;    //position for a potential error
        //get a numerical value. postfix db authorized only if first or already activated
        res = searchValue( s, &temp, (prevop == 0) || modeDB );
        if (res == 0) {
            if ( isLetter( **s ) ) {
                labelptr_t l = searchLabel(s);
                if (l == NULL) return errNum = -2;
                if( l->labelType >= label_filter ) return errNum = -27;
                temp = l->value;
                //test an optional index
                char * old = *s;
                res = searchDelimiter( s, "[");
                if (res) {
                    if( l->labelType != _valueint ) return errNum = -11;
                    double index;
                    char * e = *s;
                    res = searchValue( s, &index, 0);
                    //check for an integer
                    if (res != _valueint) { errPtr = e; return errNum = -11;}
                    if (index<0) { errPtr = e; errMin=0; errMax=99; return errNum = -5; }
                    temp += index;
                    res = searchDelimiter( s, "]");
                    errPtr = *s;
                    if (res == 0) return errNum = -26;
                } else *s = old;
                res = l->labelType;
            }
        }
        if (res) {
            if (res == _valuedb) {
                if (prevop == 0) modeDB = 1;
                else if (modeDB == 0) return errNum = -25;
            } else {
                if (modeDB) return errNum = -25;
            }
            //test potential operators
            res = searchDelimiter( s, "+-*/");
            errPtr = *s;
            switch (res) {
            case 0: //fall through
            case '-' : case '+' : {
                if (modeDB) {
                    switch (prevop) {
                    case 0 : sum = temp; break;
                    case '+': sum *= temp; break;
                    case '-': sum /= temp; break;}
                } else switch (prevop) {
                    case 0 : //fall through
                    case '+': sum += temp; break;
                    case '-': sum -= temp; break;
                    case '*': mul *= temp; sum += mul; break;
                    case '/': mul /= temp; sum += mul; break;}
                break; }
            case '*': case '/': {
                if (modeDB) return errNum = -25;
                switch (prevop) {
                case 0:
                case '+': sum += mul ; mul =  temp; break;
                case '-': sum += mul ; mul = -temp;  break;
                case '*': mul *= temp; break;
                case '/': mul /= temp; break; }
                break; }
            }
            //check if last delimiter was an operator
            if (res) prevop = res;
        } else {
            //no more value, check if an operator was pending
            if (prevop) return errNum = -18;
            else return _empty;
        }
    } while(res);
    *value = sum;
    if (modeDB) return _valuedb;
    int integer = sum;
    double check = integer;
    if (check == sum) return _valueint;
    else return _value;
}

static int outOfRange(double x,double Min,double Max){
    errMin = Min; errMax = Max;
    if ((x<Min)||(x>Max)) return  errNum = -5;
    return 0;
}

static int searchExpression(char * * s, double * value){
    int res = testExpression( s , value );
    if (res == 0 ) return errNum = -2;
    return res;
}

static int searchExpressionRange(char * * s, double * value, int range){
    int res = searchExpression( s , value );
    if (res <= 0) return res;
    if (range != _tnone) {
        if ((res != _valueint) && ((range == _tIO) || (range = _tmem))) return errNum = 11;
        if( outOfRange( *value, valueMin[range], valueMax[range])) return errNum;
    }
    return res;
}


static int paramsection = 0;

void checkAndCreateParam(){
    if (paramsection == 0)
        paramsection = dsp_PARAM();
}

void clearParamSection() {
    paramsection = 0;
}


int dspbasicCreate(char * dspbasicName, int argc, char **argv){
    char line[512] = ""; //buffer for one line of code
    char * nextName = dspbasicName;
    int lineNum = 0;
    int size    = 0;

    while (nextName && (*nextName)) {

    dspbasicName = nextName;

    char * plus = strchr(dspbasicName, '+');
    if (plus) {
        *plus = 0;
        plus++; while ((*plus) == ' ') plus++;
        if (*plus) nextName = plus;
        else nextName = 0;
    } else nextName = 0;

    dspprintf1("reading file %s\n\n",dspbasicName);
    if (nextName) dspprintf2("...next file %s\n\n",nextName);

    dspbasicInput = fopen( dspbasicName, "r" );
    if (dspbasicInput == NULL) {
        fprintf(stderr,"Error: Failed to open %s file.\n",dspbasicName);
        return -1;
    }
    if(fseek( dspbasicInput, 0, SEEK_END ) ) {
      fprintf(stderr,"Error: Failed to discover %s size.\n",dspbasicName);
      fclose(dspbasicInput);
      return -1;
    }

    int dspbasicFileSize = ftell( dspbasicInput );

    if(fseek( dspbasicInput, 0, SEEK_SET ) ) {
      fprintf(stderr,"Error: Failed to set file pointer for %s file (size=%d).\n",dspbasicName,dspbasicFileSize);
     return -1;
    }
    double input, output, gain, delay, freq, filterQ, freqLT,filterQLT, tpdf;
    char * p;   //pointer on the character to analyse
    int countarg = 0;

    //initialize dsp code with a PARAM section. this authorize
    checkAndCreateParam();
    while (1) {
        if (lineNum == 0) {
            //analyse all the parameters given on the command line
            if (countarg != argc) {
                strcpy(line, argv[countarg]);
                dspprintf2("option %d %s\n",countarg,line);
                countarg++;
            } else lineNum = 1;
        }
        if (lineNum) if (fgets( line, 512, dspbasicInput ) == 0) break;   //read line from file
        errPtr = p = line;
        while (*p ) {
            //skip any spaces or tab
            if ( isSpaceOrTab(*p) ) { p++; continue; }
            //load a new line when finding # or cr/lf
            if ( (*p == '#')  || (*p == 0x0A) || (*p == 0x0D) ) { p++; break; } //goto next line
            //expecting either a label definition or a dsp keyword, all starting by a letter
            if ( isLetter( *p ) ) {
                errPtr = p;
                int res;
                int keyw = searchKeywords( &p, dspKeywords, dspKeywordsNumber);
                if (keyw > _param) clearParamSection();
                switch(keyw) {
                case _end:   { goto finished; break; }
                case _param: { checkAndCreateParam(); break; }
                case _nop : {
                    dsp_NOP();
                    break; }
                case _section:
                case _core:  {
                    unsigned progAny1 = 0xFFFFFFFF;
                    unsigned progOnly0 = 0;
                    if ((res = testExpression( &p, &input )) < _error) goto error;
                    if (res) {
                        if (res != _valueint) { errNum = 11; goto error; }
                        progAny1 = input;
                    }
                    errPtr = p;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res) {
                        if ((res = searchExpression( &p, &input )) < _error) goto error;
                        if (res != _valueint) { errNum = 11; goto error; }
                        progOnly0 = input;
                    }
                    if (keyw == _core)    dsp_CORE_Prog(progAny1,progOnly0);
                    if (keyw == _section) dsp_SECTION(progAny1,progOnly0);
                    break; }
                case _input: {
                    if ((res = searchExpressionRange( &p, &input, _tIO  )) < _error) goto error;
                    dsp_LOAD( input);
                    break; }
                case _outputpdf:
                case _output: {
                output_retry:
                    if ((res = searchExpressionRange( &p, &output , _tIO )) < _error) goto error;
                    if (keyw == _output) dsp_STORE( output );
                    else dsp_STORE_TPDF( output );
                    res = searchDelimiter( &p, "," );
                    if(res) goto output_retry;
                    break; }
                case _transfer: {
                    int transferNum=0;
                transfer_retry:
                    res = searchDelimiter( &p, "(" );
                    errPtr = p;
                    if (res ==  0) { errNum = -28; goto error; }
                    if ((res = searchExpressionRange( &p, &input , _tIO )) < _error) goto error;
                    errPtr = p;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) { errNum = 30; goto error; }
                    if ((res = searchExpressionRange( &p, &output, _tIO )) < _error) goto error;
                    res = searchDelimiter( &p, ")" );
                    errPtr = p;
                    if (res ==  0) { errNum = 29; goto error; }

                    if (transferNum == 0) dsp_LOAD_STORE();
                    transferNum++;
                    dspLoadStore_Data(input,output);
                    if (testDelimiter( &p, "(" )) goto transfer_retry;
                    break; }
                case _clrxy  : { dsp_CLRXY(); break; }
                case _swapxy : { dsp_SWAPXY(); break; }
                case _copyxy : { dsp_COPYXY(); break; }
                case _copyyx : { dsp_COPYYX(); break; }
                case _addxy  : { dsp_ADDXY(); break; }
                case _addyx  : { dsp_ADDYX(); break; }
                case _subxy  : { dsp_SUBXY(); break; }
                case _subyx  : { dsp_SUBYX(); break; }
                case _mulxy  : { dsp_MULXY(); break; }
                case _mulyx  : { dsp_MULYX(); break; }
                case _divxy  : { dsp_DIVXY(); break; }
                case _divyx  : { dsp_DIVYX(); break; }
                case _avgxy  : { dsp_AVGXY(); break; }
                case _avgyx  : { dsp_AVGYX(); break; }
                case _negx   : { dsp_NEGX(); break; }
                case _negy   : { dsp_NEGY();break; }
                case _shift  : {
                    if ((res = searchExpressionRange( &p, &input, _tshift  )) < _error) goto error;
                    dsp_SHIFT_FixedInt( input);
                    break; }
                case _valuex : {
                    double result;
                    if ((res = searchExpression( &p, &result)) < _error) goto error;
                    dsp_VALUEX_Fixed(result);
                    break; }
                case _valuey : {
                    double result;
                    if ((res = searchExpression( &p, &result)) < _error) goto error;
                    dsp_VALUEY_Fixed(result);
                    break; }
                case _gain : {
                    double result;
                    if ((res = searchExpression( &p, &result)) < _error) goto error;
                    dsp_GAIN_Fixed(result);
                    break; }
                case _clip : {
                    double result;
                    if ((res = searchExpression( &p, &result)) < _error) goto error;
                    dsp_CLIP_Fixed(result);
                    break; }

                case _mixer: {
                    int mixerNum = 0;
                mixer_rety:
                    if ((res = searchExpressionRange( &p, &input , _tIO )) < _error) goto error;
                    if(mixerNum == 0) dsp_MIXER();
                    dspMixer_Data(input, 1.0);
                    mixerNum++;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res > 0) goto mixer_rety;
                    break; }
                case _mixergain:
                case _inputgain: {
                    int inputgainNum=0;
                inputgain_retry:
                    res = searchDelimiter( &p, "(" );
                    errPtr = p;
                    if (res ==  0) { errNum = -28; goto error; }
                    if ((res = searchExpressionRange( &p, &input , _tIO )) < _error) goto error;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) { errNum = 30; goto error; }
                    if ((res = searchExpressionRange( &p, &gain, _tgain )) < _error) goto error;
                    errPtr = p;
                    res = searchDelimiter( &p, ")" );
                    errPtr = p;
                    if (res ==  0) { errNum = 19; goto error; }
                    if (keyw == _inputgain) {
                        dsp_LOAD_GAIN_Fixed( input, gain );
                        break;
                    } else { // _mixer
                        if(inputgainNum == 0) dsp_MIXER();
                        dspMixer_Data(input, gain);
                    }
                    inputgainNum++;
                    if (testDelimiter( &p, "(" )) goto inputgain_retry;
                    break; }
                case _outputgain: {
                    res = searchDelimiter( &p, "(" );
                    errPtr = p;
                    if (res ==  0) { errNum = -28; goto error; }
                    if ((res = searchExpressionRange( &p, &output , _tIO )) < _error) goto error;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) { errNum = 30; goto error; }
                    if ((res = searchExpressionRange( &p, &gain, _tgain )) < _error) goto error;
                    errPtr = p;
                    res = searchDelimiter( &p, ")" );
                    errPtr = p;
                    if (res ==  0) { errNum = 19; goto error; }
                    dsp_STORE_GAIN_Fixed( output, gain );
                    break; }
                case _saturate: {
                    dsp_SAT0DB();
                    break; }
                case _saturatetpdf: {
                    dsp_SAT0DB_TPDF();
                    break; }
                case _white: {
                    dsp_WHITE();
                    break; }
                case _saturatevol:{
                    if ((res = searchExpressionRange( &p, &input, _tIO )) < _error) goto error;
                    dsp_SAT0DB_VOL(input);
                    break; }
                case _saturategaintpdf:
                case _saturategain:{
                    if ((res = searchExpressionRange( &p, &gain, _tgain )) < _error) goto error;
                    if (keyw == _saturategain) dsp_SAT0DB_GAIN_Fixed(gain);
                    if (keyw == _saturategaintpdf) dsp_SAT0DB_TPDF_GAIN_Fixed(gain);
                    break; }
                case _delayone: {
                    dsp_DELAY_1(); break; }
                case _delaydpus:
                case _delayus: {
                    //if (saturated == 0)   { errNum = 8; goto error; }
                    if ((res = searchExpressionRange( &p, &delay, _tdelay )) < _error) goto error;
                    if (keyw == _delaydpus)  dsp_DELAY_DP_FixedMicroSec( delay );
                    else dsp_DELAY_FixedMicroSec( delay );
                    break; }
                case _loadxmem:
                case _savexmem:
                case _loadymem:
                case _saveymem:{
                    labelptr_t l = searchLabel( &p );
                    if ((l==NULL)||(l->labelType != label_memory)) { errNum = 10; goto error; }
                    int bracket = searchDelimiter( &p, "[");
                    if ((res = testExpression( &p, &input )) < _error) goto error;
                    if (res == _empty) {
                        if (l->value > 1.0) { errNum =2; goto error; }
                        else input = 0.0;
                    } else if (res != _valueint) { errNum = 11; goto error; }
                    if (outOfRange( input, 0.0, l->value-1.0 )) goto error;
                    if (bracket)
                        if (0 == searchDelimiter( &p, "]")) { errNum=26; goto error; }
                    if       (keyw == _savexmem) dsp_STORE_X_MEM( l->address + input*2.0 );
                    else  if (keyw == _saveymem) dsp_STORE_Y_MEM( l->address + input*2.0 );
                    else  if (keyw == _loadxmem) dsp_LOAD_X_MEM( l->address + input*2.0 );
                    else  if (keyw == _loadymem) dsp_LOAD_Y_MEM( l->address + input*2.0 );
                    break; }
                case _dcblock: {
                    if ((res = searchExpressionRange( &p, &freq, _tfreq )) < _error) goto error;
                    dsp_DCBLOCK( freq );
                    break; }
                case _biquad8:
                case _biquad: {
                    labelptr_t l = searchLabel( &p );
                    if (l == NULL) { errNum = 3; goto error; }
                    if (l->labelType != label_filter) { errNum = 4; goto error; }
                    dsp_BIQUADS( l->address );
                    break; }
                case _convol: {
                    int numFilt = 0;
                    do {
                        errPtr = p;
                        labelptr_t l = searchLabel( &p );
                        if (l == NULL) { errNum = 33; goto error; }
                        if (l->labelType != label_taps) { errNum = 33; goto error; }
                        if (numFilt == 0) ; //generate opcode
                        else ; // generates taps adresses
                        numFilt++;
                        //TODO generated opcode with adress of taps
                        res = searchDelimiter( &p, ",");
                    } while(res);
                    break; }
                case _tpdf: {
                    if ((res = searchExpressionRange( &p, &tpdf, _ttpdf )) < _error) goto error;
                    dsp_TPDF(tpdf);
                    break; }
                case _sine:
                case _square:
                case _dirac: {
                    res = searchDelimiter( &p, "(" );
                    errPtr = p;
                    if (res ==  0) { errNum = 28; goto error; }
                    if ((res = searchExpressionRange( &p, &freq, _tfreq )) < _error) goto error;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) { errNum = 30; goto error; }
                    if ((res = searchExpressionRange( &p, &gain, _tgain )) < _error) goto error;
                    errPtr = p;
                    res = searchDelimiter( &p, ")" );
                    errPtr = p;
                    if (res ==  0) { errNum = 29; goto error; }

                    switch(keyw) {
                    case _sine:     dsp_SINE_Fixed(freq,gain); break;
                    case _square:   dsp_SQUAREWAVE_Fixed(freq,gain); break;
                    case _dirac:    dsp_DIRAC_Fixed(freq,gain); break;
                    }
                    break; }
                } //end of switch keyw

                if (keyw == -1) {   //this is not a keyword so it has to be a label then
                    if (paramsection == 0) { errNum=17; goto error; }
                    labelptr_t l = searchLabel( &p );
                    errPtr = p;
                    if (l) {
                        //label found, verify if user wants to overload label definition (?)
                        res = searchDelimiter( &p, "?" );
                        if (res == 0) { errNum = 12; goto error; }
                        //yes overloading
                        errPtr = p;
                    } else {
                        //label unknown. add it to the list
                        l = appendNewLabel( &p );
                        res = searchDelimiter( &p, "?" );
                        errPtr = p;
                    }
                    int filterNum=0, filterType;
                    errPtr = p;
                    filterType = searchKeywords( &p, filterNames, filterTypesNumber );
            filter_retry:
                    if ( filterType >= 0 ) {
                        if ( filterNum == 0 ) {
                            //this new label is followed by a filter name for the first time
                            if ((l->labelType != _empty)&&(l->labelType != label_filter)) { errNum = 12; goto error; };
                            l->labelType = label_filter;
                            l->address = dspBiquad_Sections_Flexible();
                        }
                        filterNum++;
                        errPtr = p;
                        res = searchDelimiter( &p, "(" );
                        errPtr = p;
                        if (res ==  0) { errNum = 19; goto error; }
                        //read filter cutoff frequency
                        if ((res = testExpression( &p, &freq )) < _error) goto error;
                        if (res == _empty) { errNum = 14; goto error; }
                        if (res == _valuedb) { errNum = 25; goto error; }
                        errPtr = p;
                        if (outOfRange( freq, valueMin[_tfreq], valueMax[_tfreq])) goto error;
                        //read filter Q only for generic second order filters
                        gain = 1.0;  //default value
                        filterQ = 1.0; //default value
                        if (filterType >= 46) {
                            res = searchDelimiter( &p, "," );
                            errPtr = p;
                            if (res ==  0) { errNum = 21; goto error; }
                            if ((res = testExpression( &p, &filterQ )) < _error) goto error;
                            if (res ==  _empty) { errNum = 15; goto error; }
                            errPtr = p;
                            if (outOfRange( filterQ, valueMin[_tfilterQ], valueMax[_tfilterQ] )) goto error;
                            if (filterType == 55) { //FLT
                                res = searchDelimiter( &p, "," );
                                errPtr = p;
                                if (res ==  0) { errNum = 21; goto error; }
                                //read frequency fp & qp
                                if ((res = testExpression( &p, &freqLT )) < _error) goto error;
                                if (res == _empty) { errNum = 14; goto error; }
                                if (res == _valuedb) { errNum = 25; goto error; }
                                errPtr = p;
                                if (outOfRange( freqLT, valueMin[_tfreq], valueMax[_tfreq])) goto error;
                                //read filter Q
                                res = searchDelimiter( &p, "," );
                                errPtr = p;
                                if (res ==  0) { errNum = 21; goto error; }
                                if ((res = testExpression( &p, &filterQLT )) < _error) goto error;
                                if (res ==  _empty) { errNum = 15; goto error; }
                                errPtr = p;
                                if (outOfRange( filterQLT, valueMin[_tfilterQ], valueMax[_tfilterQ] )) goto error;
                            }
                        }
                        //read filter gain
                        res = searchDelimiter( &p, "," );
                        errPtr = p;
                        if (res>0)
                            if ((res = searchExpressionRange( &p, &gain, _tgain )) < _error) goto error;
                        errPtr = p;
                        res = searchDelimiter( &p, ")" );
                        errPtr = p;
                        if (res ==  0) { errNum = 20; goto error; }
                        if (filterType == 55) {
                            dspprintf2("filter %s LT with F0=%f, Q0=%f, Fp=%f, Qp=%f, G=%f\n",l->name,freq,filterQ,freqLT,filterQLT,gain);
                            //add filter characteristics in the dsp code (param section)
                            dsp_FilterLT( freq, filterQ, freqLT, filterQLT, gain );
                        } else
                        if (filterType == 54) {
                            dspprintf2("filter %s HILBERT with xx=%f, xx=%f, G=%f\n",l->name,freq,filterQ,gain);
                            //TODO dsp_hilbert
                        } else {
                            dspprintf2("filter %s type %s created with F=%f, Q=%f, G=%f\n",l->name,filterNames[filterType],freq,filterQ,gain);
                            //add filter characteristics in the dsp code (param section)
                            dsp_filter( filterTypes[filterType], freq, filterQ, gain );
                        }
                        //accept potentially other filters on the same line, separated with comma
                        errPtr = p;
                        filterType = searchKeywords( &p, filterNames, filterTypesNumber );
                        if (filterType >= 0) goto filter_retry;
                        errPtr = p;
                        res = testDelimiter( &p, "#\n\r");
                        if ( res == 0) {errNum = -32; goto error; }
                        else continue;
                    } else {
                        //not a filter
                        res = searchKeywords( &p, paramKeywords, paramKeywordsNumber );
                        if (res == 0 ) {
                            // MEM keyword recognized
                            if ((l->labelType != _empty)&&(l->labelType != label_memory)) { errNum = 12; goto error; };
                            l->labelType = label_memory;
                            errPtr = p;
                            //accept a potential MEM size parameter as an integer
                            double memSize = 1.0;
                            res = searchValue( &p, &memSize, 0 );
                            if (res) {
                                // check if it is an integer value
                                if (res != _valueint) { errNum = 11; goto error; }
                                if (outOfRange( memSize, valueMin[_tmem], valueMax[_tmem] )) goto error;
                            }
                            l->value = memSize;
                            int memSizeInt = memSize;
                            //allocate space in the dsp code
                            l->address = dspMem_LocationMultiple(memSizeInt);
                            dspprintf2("label %s created with MEM adr = %d size = %f\n",l->name,l->address, l->value);
                        } else
                        if (res == 1) { //TAPS
                            int numTaps=0;
                            do {
                                if ((l->labelType != _empty)&&(l->labelType != label_taps)) { errNum = 12; goto error; };
                                l->labelType = label_taps;
                                errPtr = p;
                                //accept a potential MEM size parameter as an integer
                                double tap = 0.0;
                                if ((res = searchExpressionRange( &p, &tap, _ttaps )) < _error) goto error;
                                numTaps++;
                                errPtr = p;
                                res = searchDelimiter( &p, "," );
                                l->value = 0;
                                l->address = 0;
                            } while (res);
                            //convert all taps
                            dspprintf2("*** %d TAPS ***\n",numTaps);
                        } else
                        {
                            // not a MEMORY nor TAPS, then should be an numerical expression
                            res = searchDelimiter( &p, "=" );
                            errPtr = p;
                            enum label_type_e old = l->labelType;
                            double temp;
                            if (res) {
                                //value or label authorized after an "="
                                if ((res = searchExpression( &p, &temp )) < _error) goto error;
                                errPtr = p;
                                old = _empty;  //clear previous definition if any
                            } else {
                                char * prevp = p;
                                res = searchValue( &p, &temp, 1 );  //otherwise only numerical
                                errPtr = p;
                                if (res == _empty)  { errNum = 7; goto error; }
                                if (testDelimiter( &p, "+-*/")) { errPtr= prevp; errNum = 31; goto error; }
                            }
                            if (old == _empty) {
                                l->value = temp;
                                l->labelType = res;
                                dspprintf2("label %s type %d created with value %f\n",l->name,l->labelType, l->value);
                            } else
                                dspprintf2("label %s type %d already set with value %f\n",l->name,l->labelType, l->value);
                        } // expression value
                    } // not a filter
                } //if keyw == -1
                if (lineNum > 0) {
                    int res = searchDelimiter( &p, ";#\n\r" );     //optional delimiter on a line
                    errPtr=p;
                    if (res ==0 ) { errNum = 13; goto error; }
                    if (res != ';') {errPtr=p; break; } //skip end of line
                }
            } else {
                //a strange caracter is encountered
                errPtr=p; errNum=6; goto error;  }
        } // while(*p)
    } //while (1)
finished:

    fclose(dspbasicInput);

    dspprintf1("\n");
    } // while (nextname);

    size = dsp_END_OF_CODE();

    freeAllLabels();
    return size;

error:
    if (errNum > 0) errNum = - errNum;
    switch (errNum) {
    case -1:  fprintf(stderr,"Error: numerical value expected\n"); break;
    case -2:  fprintf(stderr,"Error: numerical value or label expected\n"); break;
    case -3:  fprintf(stderr,"Error: filter label expected\n"); break;
    case -4:  fprintf(stderr,"Error: filter label expected, this one has a numerical value\n"); break;
    case -5:  fprintf(stderr,"Error: value out of range (%f ... %f)\n",errMin, errMax); break;
    case -6:  fprintf(stderr,"Error: dsp keyword or user label starting with a..z letter exptected, found char <0x%X> : \n",*errPtr); break;
    case -7:  fprintf(stderr,"Error: filter or numerical value exptected after a label definition\n"); break;
    case -8:  fprintf(stderr,"Error: this dsp instruction requires saturation before execution\n"); break;
    case -9:  fprintf(stderr,"Error: dsp accumulator already saturated by previous instruction\n"); break;
    case -10: fprintf(stderr,"Error: special label defined with MEMORY is expected\n"); break;
    case -11: fprintf(stderr,"Error: integer value expected\n"); break;
    case -12: fprintf(stderr,"Error: label already exists\n"); break;
    case -13: fprintf(stderr,"Error: end of line or ; separator expected\n"); break;
    case -14: fprintf(stderr,"Error: filter frequency (or label) expected\n"); break;
    case -15: fprintf(stderr,"Error: filter Q (or label) expected\n"); break;
    case -16: fprintf(stderr,"Error: filter gain (or label) expected\n"); break;
    case -17: fprintf(stderr,"Error: label cannot be created outside of a \"param\" section\n"); break;
    case -18: fprintf(stderr,"Error: missing numerical value to finish expression \n"); break;
    case -19: fprintf(stderr,"Error: after filter type, a bracket \"(\" is expected\n"); break;
    case -20: fprintf(stderr,"Error: missing closing bracket : \")\" is expected\n"); break;
    case -21: fprintf(stderr,"Error: filter parameters separated with a coma \",\" is expected\n"); break;
    case -22: fprintf(stderr,"Error: special label defined with MEMORY is expected\n"); break;
    case -23: fprintf(stderr,"Error: a gain value is expected\n"); break;
    case -24: fprintf(stderr,"Error: problem while evaluating expression\n"); break;
    case -25: fprintf(stderr,"Error: problem with decibel while evaluating expression\n"); break;
    case -26: fprintf(stderr,"Error: missing closing bracket : \"]\" is expected\n"); break;
    case -27: fprintf(stderr,"Error: a label with numerical value is expected\n"); break;
    case -28: fprintf(stderr,"Error: opening bracket \"(\" is expected\n"); break;
    case -29: fprintf(stderr,"Error: closing bracket \")\" is expected\n"); break;
    case -30: fprintf(stderr,"Error: comma \",\" is expected\n"); break;
    case -31: fprintf(stderr,"Error: expression with operator requires equal \"=\" operator\n"); break;
    case -32: fprintf(stderr,"Error: end of line expected\n"); break;
    case -33: fprintf(stderr,"Error: taps label expected\n"); break;
    default: break;
    }
    fprintf(stderr,"l%d: %s",lineNum,line);
    fprintf(stderr,"l%d: ",lineNum);
    char *q = line;
    skipSpaces(&errPtr);
    while (q != errPtr) { fprintf(stderr,"%c",(*q==9 ? *q : ' ')); q++; }
    fprintf(stderr,"^\n");
    freeAllLabels();
    fclose(dspbasicInput);
    return -errNum;
}
