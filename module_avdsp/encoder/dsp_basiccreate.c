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
    _input, _output, _transfer, _inputgain, _outputgain, _outputpdf, _outputvol, _outputvolsat,
    _mixer, _mixergain, _gain, _clip,
    _clrxy,_swapxy,_copyxy,_copyyx,_addxy,_addyx,_subxy,_subyx,_mulxy,_mulyx, _divxy,_divyx,_avgxy,_avgyx,_negx,_negy,_shift,_valuex,_valuey,
    _saturate, _saturatevol, _saturategain,
    _delayone, _delayus, _delaydpus, _delayusfbmix,
    _savexmem, _loadxmem,  _saveymem, _loadymem,
    _dcblock, _biquad, _biquad8, _convol,
    _tpdf, _white, _sine,_square,_dirac,
    _integrator, _cicus, _cicn,_expma,
    _envpeak,_envrms,_limiterpeak,_limiterrms,_limiterpeakhard,_compressor,_expander,_noisegate,
    _tile,_send,_receive,
    dspKeywordsNumber
};
static const char * dspKeywords[dspKeywordsNumber] = {
    "end", "param", "nop", "core", "section",
    "input", "output","transfer", "inputgain", "outputgain", "outputtpdf", "outputvol", "outputvolsat", "mixer","mixergain","gain","clip",
    "clrxy","swapxy","copyxy","copyyx","addxy","addyx","subxy","subyx","mulxy","mulyx","divxy","divyx","avgxy","avgyx","negx","negy","shift","valuex","valuey",
    "saturate", "saturatevol","saturategain",
    "delayone", "delayus", "delaydpus", "delayusfbmix",
    "savexmem", "loadxmem","saveymem", "loadymem",
    "dcblock", "biquad", "biquad8", "convol",
    "tpdf", "white", "sine","square","dirac",
    "integrator","movingavgus","movingavgn","expmovingavg",
    "envpeak","envrms","limiterpeak","limiterrms","limiterpeakhard","compressor","expander","noisegate",
    "tile","send","receive"
};

enum paramkeywords_e {
        _MEMORY,            //folowed optionally by the number of 64bits memory locations otherwise 1
        _TAPS,              //folowed by a list of taps number, or a filename with list of data
        _VALUE,             //followed by 0 to n values in range -8..+8
        _VALUEINT,          //followed by 0 to n values in range -int32 ...+int32
        _DRCIN,             //followed by 2,4,or 5,6 values (attack, release,thresold,gain,slope,inv) and 2 variables for alpha
        _DRCOUT,            //empty. working area enveloppe+gain
        _FILTER,            //followed by a list of biquads filters
        _FILTER8,           //followed by a list of 1 to 8 biquads. fixed size for 8.
        paramKeywordsNumber
};
static const char * paramKeywords[paramKeywordsNumber] = {
        "MEMORY","TAPS","VALUE","VALUEINT","DRCIN","DRCOUT","FILTER","FILTER8" };

enum   tvalue_e           { _tIO, _tfreq, _tvalue32,_tvalue64,    _tint32,  _tdelay, _tfilterQ, _tmem, _tshift, _ttpdf, _tpercent, _ttile, _tdrc_attack, _tnone };
double valueMin[_tnone] = {    0,     10,        -8,     -128,-0x7FFFFFFF,        0,        0 ,     1,     -32,      8,         0,      1,  0.001 };
double valueMax[_tnone] = {   31,  40000,        +8,     +128, 0x7FFFFFFF, 10000000,       20 ,    16,      32,     31,         1,      4,  4.0  };

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

static inline int isCharOfString(char ch) {
    return ( ((ch >= 32) && (ch != 34 )) || (ch == 9) ) ? ch : 0;
}


static inline char * skipSpaces(char * * s);


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


//check if current delimiter character is in the given string then return its value otherwise 0
static int searchString(char * * s, char * * str ) {
    char * p = skipSpaces(s);
    if (*p == 34) {
        p++;
        *str = p;   //begining of string
        int len=0;
        while (isCharOfString(*p)) { len++; p++; }
        if (*p == 34) *s = p+1; else *s = p;
        return len;
    }
    return -1;
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
    _value = 1,         //contains a float (double)
    _valuedb = 2,       //contains a value that was expressed in decidels
    _valueint = 3,      //contains a value that properly fits as a 32bits integer
    label_filter = 4,   //point on a list of filters in param space
    label_memory,       //points on a 64bit data location in param space
    label_taps,         //points on a list of Taps stored in param space
    label_value,        //points on a value stored in param space
    label_valueint,     //points on a value declared formally as integer, stored in param space
    label_drcin,        //points on a record type DRCIN, stored in param space
    label_drcout,       //points on a record type DRCIN, stored in param space
    label_filters,      //points on a list of filters, followed by their biquad coefficients (initialized to 0)
    label_filter8,      //points on a list of 8 filters, followed by their biquad coefficients (initialized to 0)
};

enum { label_max_values = 8 };

typedef struct label_s {
    struct label_s *   next;
    double       value;          //value typically when labelType < 4
    int          numValues;      //number of values in the table
    double       values[label_max_values];      //value typically when labelType >= 4
    dspSymbol_t s;              //s.name will be extended during malloc : KEEP s at bottom of structure
} label_t;

int numTile = 0;    //current tile number

typedef label_t * labelptr_t;

//contains pointer on the list of labels created
static labelptr_t firstLabel = NULL, lastLabel = NULL;

//add a new label in the list and return its allocated pointer.
//character pointer expected to point on label and then moved after label.
static labelptr_t appendNewLabel(char * * s) {
    char * p = *s;
    int len = 0;
    while ( isAlphanum(*p) ) { p++; len++; }
    labelptr_t l = malloc(sizeof(label_t)+len);
    l->s.length = len;
    l->next = NULL;
    l->s.type = _empty;
    if (firstLabel) { lastLabel->next = l; lastLabel = l; }
    else { firstLabel = lastLabel = l; }
    int i;
    for (i=0, p=*s; i<len; i++) l->s.name_[i] = *(p++);
    l->s.name_[len] = 0;
    l->s.name = l->s.name_; //set pointer
    l->value = 0;
    l->s.address = 0;
    l->numValues = 0;
    l->s.tileNum = dsp_TILE_num();
    l->s.tileUsed = 0;
    *s = p;
    return l;
}

//search a possible label in *s then returns its pointer otherwise 0
static label_t * searchLabel(char * *s){
    char * p = skipSpaces(s);
    for (labelptr_t l = firstLabel; l ; l=l->next) {
        if ( p == strstr( p, l->s.name ) ) {
            int len = l->s.length;
            if ( isAlphanum( p[len]) ) continue; //not yet
            *s = &p[len];
            return l;
        }
    }
    return NULL;
}

void usedLabelInTile(label_t * l) {
    if (l) l->s.tileUsed |= (1UL<<dsp_TILE_num());
}

//search any label in "line" and replace them with their numerical value.
//assuming line is long enough to hold replaced strings
static void replaceLabels(char * line) {
    labelptr_t l = firstLabel;
    char buf[512];
    while (l) {
        int len = l->s.length;
        //prepare buf for comparaison with [label]
        buf[0] = '[';
        memcpy(&buf[1],l->s.name,len);
        buf[len+1]=']';
        buf[len+2]=0;
        //search occurence
        char * p = strstr( line, buf );
        if (p) {
            //found one, replace it
            int size = len+2; //to include '[]'
            char * after = &p[size]; //after points just after ]
            int lenafter = strlen( after ) + 1; //compute remainings characters in the line, including last 0
            int lval = l->value;
            double lvalue = lval;
            if (l->value == lvalue) sprintf(buf, "%d",lval);
            else sprintf(buf, "%f",l->value);
            int lenbuf = strlen( buf );
            //eventually adjust bin space
            if (size != lenbuf) memmove(&p[lenbuf],after,lenafter); //resize to fit requirement
            memcpy(p,buf,lenbuf);
            continue; //retry with same label, in case of double occurence ...
        }
        l=l->next;
    }
}


static void freeAllLabels(){
    labelptr_t l = firstLabel;
    while (l) {
        volatile labelptr_t next = l->next;
        free(l);
        l = next;
    }
}


void createSymbolTable() {
    dspSymbolCreateTable();
    labelptr_t l = firstLabel;
    while (l) {
        dspSymbolAdd(&l->s);
        l = l->next;
    }
    dspSymbolEndOfTable();
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
            if ( (base==10) && (p[0]=='%') ) {
                *value = result * sign / 100.0;
                *s = p+1;
                return _value; }
            if ( (base==10) && (p[0]=='m') ) {
                *value = result * sign / 1000.0;
                *s = p+1;
                return _value; }
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
                if( l->s.type >= label_filter ) return errNum = -27;
                temp = l->value;
                //test an optional index
                char * old = *s;
                res = searchDelimiter( s, "[");
                if (res) {
                    if( l->s.type != _valueint ) return errNum = -11;
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
                res = l->s.type;
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


static void fatalError(); //prototype as it is located at botom of this file
static void fatalErrorNum(int num){
    errNum = num;
    fatalError();
}



static char line[512] = ""; //buffer for one line of code
static int lineNum;
static char * fgetLine() {
    return fgets( line, sizeof(line), dspbasicInput );
}


static inline char * skipSpaces(char * * s){
    char * p = *s;
    loop:
    while (isSpaceOrTab(*p)) p++;
    if (*p == 0x5C) { // backslash"\"
        p++;
        while (isSpaceOrTab(*p)) p++;
        if ((*p == 0) || (*p == '#') || (*p == 0x0A) || (*p == 0x0D)) { } else fatalErrorNum(32);
        if (fgetLine()) {
            p = line;
            errPtr = p;
            goto loop;
        } else {
            line[0] = 0;
            p = line;
        }
    }
    *s = p;
    return p;
}



int dspbasicCreate(char * dspbasicName, int argc, char **argv){
    char * nextName = dspbasicName;
    int size    = 0;
    lineNum = 0;
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

    while (1) {
        if (lineNum == 0) {
            //analyse all the parameters given on the command line
            if (countarg != argc) {
                strcpy(line, argv[countarg]);
                dspprintf2("option %d %s\n",countarg,line);
                countarg++;
            } else lineNum = 1;
        }
        if (lineNum) {
            if ( fgetLine() == 0) break;   //read line from file
            lineNum++;
        }
        errPtr = p = line;
        //main loop to analyse the line
        while (*p ) {
            //skip any spaces or tab
            if ( isSpaceOrTab(*p) ) { p++; continue; }
            //check special case '#-' as a prefix for printable comments
            if ( (p[0] == '#') && (p[1] == '-') ) {
                char *s = &p[2];
                replaceLabels(s);
                while (*s) {
                    if ((*s == 0x0A)||(*s == 0x0D)) {
                        *s = 0; s = &p[2];
                        fprintf(stderr,"%s\n",s);
                        break; }
                    s++; }
                break;
            }
            //load a new line when finding # or cr/lf
            if ( (*p == '#')  || (*p == 0x5C ) || (*p == 0x0A) || (*p == 0x0D) ) break; //goto next line
            //expecting either a label definition or a dsp keyword, all starting by a letter
            if ( isLetter( *p ) ) {
                errPtr = p;
                int res;
                int keyw = searchKeywords( &p, dspKeywords, dspKeywordsNumber);
                if (keyw > _param) clearParamSection();
                switch(keyw) {
                case _end:   { goto finished; break; }
                case _param: {
                    if ((res = testExpression( &p, &input )) < _error) fatalError();
                    if (res) {
                        if (res != _valueint) fatalErrorNum(11);
                        int num = input;
                        dsp_PARAM_NUM(num);
                        errPtr = p;
                        switch (num) {
                        case 200: {
                            res = searchDelimiter( &p, "," );
                            errPtr = p;
                            if (res ==  0) fatalErrorNum(30);
                            char * str;
                            res = searchString( &p, &str);
                            if (res < 0) fatalErrorNum(34);
                            int code = res; int n = 1;  //first char is string length
                            for (int i=0; i<= res; i++) {
                                if (i < res) code |= (*str)<<(n*8);
                                n++; str++;
                                if ((n==4) || (i == res)) { addCode(code); code = 0; n = 0; }
                            }
                            errPtr = p;
                            break;
                        }
                        } //switch
                        clearParamSection();
                    } else
                        checkAndCreateParam();
                    break; }
                case _nop : {
                    dsp_NOP();
                    break; }
                case _section:
                case _core:  {
                    unsigned progAny1 = 0xFFFFFFFF;
                    unsigned progOnly0 = 0;
                    if ((res = testExpression( &p, &input )) < _error) fatalError();
                    if (res) {
                        if (res != _valueint) fatalErrorNum(11);
                        progAny1 = input;
                    }
                    errPtr = p;
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res) {
                        if ((res = searchExpression( &p, &input )) < _error) fatalError();
                        if (res != _valueint) fatalErrorNum(11);
                        progOnly0 = input;
                    }
                    if (keyw == _core)    numCore = dsp_CORE_Prog(progAny1,progOnly0);
                    if (keyw == _section) dsp_SECTION(progAny1,progOnly0);
                    break; }
                case _tile : {  //expect a number between 0..7
                    numTile = dsp_TILE();
                    if( outOfRange( numTile, valueMin[_ttile], valueMax[_ttile])) fatalError();;
                break; }
                case _send: //falltrough to _receive
                case _receive: {    //syntax send channel : io,io ...
                    //todo

                break; }
                case _input: {
                    if ((res = searchExpressionRange( &p, &input, _tIO  )) < _error) fatalError();
                    dsp_LOAD( input);
                    break; }
                case _outputpdf:
                case _outputvol:
                case _outputvolsat:
                case _output: {
                    int outcount;
                    int finalIO;
                    do {
                        outcount=0;
                        finalIO=0;
                        do {
                            if ((res = searchExpressionRange( &p, &output , _tIO )) < _error) fatalError();
                            int out = output;   //convert to integer
                            finalIO |= (out << (8*outcount));
                            outcount++;
                            res = searchDelimiter( &p, "," );
                            errPtr = p;
                        } while ( res && (outcount<4) );
                        if (keyw == _output) dsp_STORE( finalIO );
                        else  if (keyw == _outputpdf) dsp_STORE_TPDF( finalIO );
                        else  if (keyw == _outputvol) dsp_STORE_VOL( finalIO );
                        else dsp_STORE_VOL_SAT( finalIO );
                    } while(res);
                    break; }
                case _transfer: {
                    int transferNum=0;
                    do {
                        errPtr = p;
                        res = searchDelimiter( &p, "(" );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(28);
                        if ((res = searchExpressionRange( &p, &input , _tIO )) < _error) fatalError();
                        errPtr = p;
                        res = searchDelimiter( &p, "," );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(30);
                        if ((res = searchExpressionRange( &p, &output, _tIO )) < _error) fatalError();
                        res = searchDelimiter( &p, ")" );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(29);

                        if (transferNum == 0) dsp_LOAD_STORE();
                        transferNum++;
                        dspLoadStore_Data(input,output);
                        if ((res = testDelimiter( &p, "(" ))) continue;
                        res = searchDelimiter( &p, "," );
                    } while (res);
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
                case _negx   : { dsp_NEGX();  break; }
                case _negy   : { dsp_NEGY();  break; }
                case _shift  : {
                    if ((res = searchExpressionRange( &p, &input, _tshift  )) < _error) fatalError();
                    dsp_SHIFT_FixedInt( input);
                    break; }
                case _valuex : {
                    double result;
                    if ((res = searchExpressionRange( &p, &result, _tvalue32)) < _error) fatalError();
                    dsp_VALUEX_Fixed(result);
                    break; }
                case _valuey : {
                    double result;
                    if ((res = searchExpressionRange( &p, &result, _tvalue32)) < _error) fatalError();
                    dsp_VALUEY_Fixed(result);
                    break; }
                case _gain : {
                    errPtr = p;
                    labelptr_t l = searchLabel( &p );
                    if ((l) && ((l->s.type == label_drcout) || (l->s.type == label_value))) {
                        dsp_GAIN( l->s.address + ( (l->s.type == label_drcout) ? 1 : 0 ) );
                        usedLabelInTile(l);
                    } else {
                        p = errPtr;
                        double result;
                        if ((res = searchExpressionRange( &p, &result, _tvalue32)) < _error) fatalError();
                        dsp_GAIN_Fixed(result);
                    }
                    break; }
                case _clip : {
                    double result;
                    if ((res = searchExpressionRange( &p, &result, _tvalue32)) < _error) fatalError();
                    dsp_CLIP_Fixed(result);
                    break; }
                case _mixer: {
                    int mixerNum = 0;
                    do {
                        if ((res = searchExpressionRange( &p, &input , _tIO )) < _error) fatalError();
                        if(mixerNum == 0) dsp_MIXER();
                        dspMixer_Data(input, 1.0);
                        mixerNum++;
                        res = searchDelimiter( &p, "," );
                        errPtr = p;
                    } while (res);
                    break; }
                case _mixergain:
                case _inputgain:
                case _outputgain: {
                    int inputgainNum=0;
                    do {
                        errPtr = p;
                        res = searchDelimiter( &p, "(" );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(28);
                        if ((res = searchExpressionRange( &p, &input , _tIO )) < _error) fatalError();
                        res = searchDelimiter( &p, "," );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(30);
                        if ((res = searchExpressionRange( &p, &gain, _tvalue32 )) < _error) fatalError();
                        errPtr = p;
                        res = searchDelimiter( &p, ")" );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(19);
                        if (keyw == _inputgain) {
                            dsp_LOAD_GAIN_Fixed( input, gain );
                            break; }
                        if (keyw == _outputgain) {
                            dsp_STORE_GAIN_Fixed( input, gain );
                            break; }
                        if (inputgainNum == 0) dsp_MIXER();
                        dspMixer_Data(input, gain);
                        inputgainNum++;
                        if ((res = testDelimiter( &p, "(" ))) continue;
                        res = searchDelimiter( &p, "," );
                    } while(res);
                    break; }
                case _saturate: {
                    dsp_SAT0DB();
                    break; }
                case _white: {
                    dsp_WHITE();
                    break; }
                case _saturatevol:{
                    dsp_SAT0DB_VOL();
                    break; }
                case _saturategain:{
                    if ((res = searchExpressionRange( &p, &gain, _tvalue32 )) < _error) fatalError();
                    dsp_SAT0DB_GAIN_Fixed(gain);
                    break; }
                case _delayone: {
                    dsp_DELAY_1(); break; }
                case _delaydpus:
                case _delayus: {
                    errPtr = p;
                    labelptr_t l = searchLabel( &p );
                    if (l && (l->s.type == label_valueint)) {
                        errPtr = p;
                        usedLabelInTile(l);
                        if ((res = searchDelimiter( &p, "," )) == 0) fatalErrorNum(30);
                        errPtr = p;
                        double max=0;
                        if ((res = searchExpressionRange( &p, &max, _tdelay )) < _error) fatalError();
                        if (keyw == _delaydpus)  dsp_DELAY_DP_max( l->s.address, max );
                        else dsp_DELAY_max( l->s.address, max );
                    } else {
                        p = errPtr;
                        if ((res = searchExpressionRange( &p, &delay, _tdelay )) < _error) fatalError();
                        if (keyw == _delaydpus)  dsp_DELAY_DP_FixedMicroSec( delay );
                        else dsp_DELAY_FixedMicroSec( delay );
                    }
                    break; }
                case _delayusfbmix: {
                    double source,feed,delayed,mix;
                    if ((res = searchExpressionRange( &p, &delay, _tdelay )) < _error) fatalError();
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(30);
                    if ((res = searchExpressionRange( &p, &source, _tpercent )) < _error) fatalError();
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(30);
                    if ((res = searchExpressionRange( &p, &feed, _tpercent )) < _error) fatalError();
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(30);
                    if ((res = searchExpressionRange( &p, &delayed, _tpercent )) < _error) fatalError();
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(30);
                    if ((res = searchExpressionRange( &p, &mix, _tpercent )) < _error) fatalError();
                    dsp_DELAY_FB_MIX_FixedMicroSec( delay, source, feed, delayed, mix );
                    break; }
                case _loadxmem:
                case _savexmem:
                case _loadymem:
                case _saveymem:{
                    input = 0.0;
                    labelptr_t l = searchLabel( &p );
                    if ((l==NULL)||(l->s.type != label_memory)) fatalErrorNum(10);
                    usedLabelInTile(l);
                    int bracket = searchDelimiter( &p, "[.");
                    if (bracket) {
                        if ((res = testExpression( &p, &input )) < _error) fatalError();
                        if (res == _empty) {
                            if (l->value > 1.0) fatalErrorNum(2);
                        } else if (res != _valueint) fatalErrorNum(11);
                        if (outOfRange( input, 0.0, l->value-1.0 )) fatalError();
                    }
                    if (bracket)
                        if (bracket != searchDelimiter( &p, "]")) fatalErrorNum(26);
                    if       (keyw == _savexmem) dsp_STORE_X_MEM( l->s.address + input*2.0 );
                    else  if (keyw == _saveymem) dsp_STORE_Y_MEM( l->s.address + input*2.0 );
                    else  if (keyw == _loadxmem) dsp_LOAD_X_MEM( l->s.address + input*2.0 );
                    else  if (keyw == _loadymem) dsp_LOAD_Y_MEM( l->s.address + input*2.0 );
                    break; }
                case _dcblock: {
                    if ((res = searchExpressionRange( &p, &freq, _tfreq )) < _error) fatalError();
                    dsp_DCBLOCK( freq );
                    break; }
                case _biquad8:
                case _biquad: {
                    errPtr = p;
                    labelptr_t l = searchLabel( &p );
                    if (l == NULL) fatalErrorNum(3);
                    if ( (l->s.type != label_filter)&&(l->s.type != label_filters)&&(l->s.type != label_filter8) ) fatalErrorNum(3);
                    usedLabelInTile(l);
                    if (l->s.type == label_filter) dsp_BIQUADS( l->s.address );
                    else dsp_BIQUADS_FS( l->s.address );
                    break; }
                case _convol: {
                    int numFilt = 0;
                    do {
                        errPtr = p;
                        labelptr_t l = searchLabel( &p );
                        if (l == NULL) fatalErrorNum(33);
                        if (l->s.type != label_taps) fatalErrorNum(33);
                        usedLabelInTile(l);
                        if (numFilt == 0) ; //generate opcode
                        else ; // generates taps adresses
                        numFilt++;
                        //TODO generated opcode with adress of taps
                        res = searchDelimiter( &p, ",");
                    } while(res);
                    break; }
                case _integrator: {
                    dsp_INTEGRATOR(); break; }
                case _cicus: {
                    if ((res = searchExpressionRange( &p, &delay, _tdelay )) < _error) fatalError();
                    dsp_CIC_FixedMicroSec( delay );
                    break; }
                case _cicn: {
                    if ((res = searchExpressionRange( &p, &delay, _tdelay )) < _error) fatalError();
                    dsp_CIC_N( delay );
                    break; }
                case _expma : {
                    double value = 0;
                    if ((res = searchExpressionRange( &p, &value, _tpercent )) < _error) fatalError();
                     dsp_EXPMA(value);
                    break; }
                case _tpdf: {
                    if ((res = searchExpressionRange( &p, &tpdf, _ttpdf )) < _error) fatalError();
                    dsp_TPDF(tpdf);
                    break; }
                case _sine:
                case _square:
                case _dirac: {
                    res = searchDelimiter( &p, "(" );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(28);
                    if ((res = searchExpressionRange( &p, &freq, _tfreq )) < _error) fatalError();
                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(30);
                    if ((res = searchExpressionRange( &p, &gain, _tvalue32 )) < _error) fatalError();
                    errPtr = p;
                    res = searchDelimiter( &p, ")" );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(29);

                    switch(keyw) {
                    case _sine:     dsp_SINE_Fixed(freq,gain); break;
                    case _square:   dsp_SQUAREWAVE_Fixed(freq,gain); break;
                    case _dirac:    dsp_DIRAC_Fixed(freq,gain); break;
                    }
                    break; }
                case _envpeak :
                case _envrms :
                case _limiterpeak :
                case _limiterrms :
                case _limiterpeakhard :
                case _noisegate :
                case _compressor :
                case _expander :{
                    errPtr = p;
                    labelptr_t drcin = searchLabel( &p );
                    if ((drcin == NULL) || (drcin->s.type != label_drcin)) fatalErrorNum(36);
                    usedLabelInTile(drcin);
                    if ((keyw == _limiterpeak)||(keyw == _limiterrms)||(keyw == _limiterpeakhard)||(keyw == _noisegate))
                        if (drcin->numValues < 4) fatalErrorNum(38);
                    if (keyw == _compressor)
                        if (drcin->numValues < 5) fatalErrorNum(39);
                    if (keyw == _expander)
                        if (drcin->numValues < 6) fatalErrorNum(39);

                    res = searchDelimiter( &p, "," );
                    errPtr = p;
                    if (res ==  0) fatalErrorNum(30);
                    labelptr_t drcout = searchLabel( &p );
                    if ((drcout == NULL) || (drcout->s.type != label_drcout)) fatalErrorNum(37);
                    usedLabelInTile(drcout);
                    break; }
                } //end of switch keyw

                if (keyw == -1) {   //this is not a keyword so it has to be a label then
                    //if (paramsection == 0) { errNum=17; fatalErrorNum(); }
                    labelptr_t l = searchLabel( &p );
                    errPtr = p;
                    if (l) {
                        //label found, verify if user wants to overload label definition (?)
                        res = searchDelimiter( &p, "?" );
                        if (res == 0) fatalErrorNum(12);
                        //yes overloading
                        errPtr = p;
                    } else {
                        //label unknown. add it to the list
                        l = appendNewLabel( &p );
                        res = searchDelimiter( &p, "?" );
                        errPtr = p;
                    }
                    int filterNum=0, filterType=0, filterkeyw = 0;
            filter_entry:
                    errPtr = p;
                    filterType = searchKeywords( &p, filterNames, filterTypesNumber );
                    if ( filterType >= 0 ) checkAndCreateParam();
            filter_retry:
                    if ( filterType >= 0 ) {
                        if ( filterNum == 0 ) {
                            if (filterkeyw == 0) {
                                //this new label is followed by a filter name for the first time
                                if ((l->s.type != _empty)&&(l->s.type != label_filter)) fatalErrorNum(12);
                                l->s.type = label_filter;
                                l->s.address = dspBiquad_Sections_Flexible();
                            } else {
                                addOpcodeValue(DSP_BIQUADS_FS,1); //numsection place older
                                addCode(0); //
                            }
                        }
                        filterNum++;
                        errPtr = p;
                        res = searchDelimiter( &p, "(" );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(19);
                        //read filter cutoff frequency
                        if ((res = testExpression( &p, &freq )) < _error) fatalError();
                        if (res == _empty) fatalErrorNum(14);
                        if (res == _valuedb) fatalErrorNum(25);
                        errPtr = p;
                        if (outOfRange( freq, valueMin[_tfreq], valueMax[_tfreq])) fatalError();
                        //read filter Q only for generic second order filters
                        gain = 1.0;  //default value
                        filterQ = 1.0; //default value
                        if (filterType >= 46) {
                            res = searchDelimiter( &p, "," );
                            errPtr = p;
                            if (res ==  0) fatalErrorNum(21);
                            if ((res = testExpression( &p, &filterQ )) < _error) fatalError();
                            if (res ==  _empty) fatalErrorNum(15);
                            errPtr = p;
                            if (outOfRange( filterQ, valueMin[_tfilterQ], valueMax[_tfilterQ] )) fatalError();
                            if (filterType == 55) { //FLT
                                res = searchDelimiter( &p, "," );
                                errPtr = p;
                                if (res ==  0) fatalErrorNum(21);
                                //read frequency fp & qp
                                if ((res = testExpression( &p, &freqLT )) < _error) fatalError();
                                if (res == _empty) fatalErrorNum(14);
                                if (res == _valuedb) fatalErrorNum(25);
                                errPtr = p;
                                if (outOfRange( freqLT, valueMin[_tfreq], valueMax[_tfreq])) fatalError();
                                //read filter Q
                                res = searchDelimiter( &p, "," );
                                errPtr = p;
                                if (res ==  0) fatalErrorNum(21);
                                if ((res = testExpression( &p, &filterQLT )) < _error) fatalError();
                                if (res ==  _empty) fatalErrorNum(15);
                                errPtr = p;
                                if (outOfRange( filterQLT, valueMin[_tfilterQ], valueMax[_tfilterQ] )) fatalError();
                            }
                        }
                        //read filter gain
                        res = searchDelimiter( &p, "," );
                        errPtr = p;
                        if (res>0)
                            if ((res = searchExpressionRange( &p, &gain, _tvalue32 )) < _error) fatalError();
                        errPtr = p;
                        res = searchDelimiter( &p, ")" );
                        errPtr = p;
                        if (res ==  0) fatalErrorNum(20);
                        if (filterType == 55) {
                            //add filter characteristics in the dsp code (param section)
                            if (filterkeyw == 0) {
                                dspprintf2("filter %s LT with F0=%f, Q0=%f, Fp=%f, Qp=%f, G=%f\n",l->s.name,freq,filterQ,freqLT,filterQLT,gain);
                                dsp_FilterLT( freq, filterQ, freqLT, filterQLT, gain );
                            } else
                            { addCode(0); addCode(0); addCode(0); addCode(0); addCode(0); addCode(0);}
                        } else
                        if (filterType == 54) {
                            dspprintf2("filter %s HILBERT with xx=%f, xx=%f, G=%f\n",l->s.name,freq,filterQ,gain);
                            //TODO dsp_hilbert
                             { addCode(0); addCode(0); addCode(0); addCode(0); addCode(0); addCode(0);}
                        } else {
                            //add filter characteristics in the dsp code (param section)
                            if (filterkeyw == 0) {
                                dspprintf2("filter %s type %s created with F=%f, Q=%f, G=%f\n",l->s.name,filterNames[filterType],freq,filterQ,gain);
                                dsp_filter( filterTypes[filterType], freq, filterQ, gain );
                            } else
                            { addCode(0); addCode(0); addCode(0); addCode(0); addCode(0); addCode(0);}
                        }
                        //accept potentially other filters on the same line, separated with comma
                        errPtr = p;
                        filterType = searchKeywords( &p, filterNames, filterTypesNumber );
                        if (filterType >= 0) goto filter_retry;
                        errPtr = p;
                        res = testDelimiter( &p, "#\n\r");
                        if ( res == 0) fatalErrorNum(32);
                        else continue;
                    } else {
                        //not a filter
                        res = searchKeywords( &p, paramKeywords, paramKeywordsNumber );
                        switch (res) {
                        case _MEMORY : {
                            // MEMORY keyword recognized
                            if (l->s.type != _empty) fatalErrorNum(12);
                            l->s.type = label_memory;
                            errPtr = p;
                            //accept a potential MEMORY size parameter as an integer
                            double memSize = 1.0;
                            res = searchValue( &p, &memSize, 0 );
                            if (res) {
                                // check if it is an integer value
                                if (res != _valueint) fatalErrorNum(11);
                                if (outOfRange( memSize, valueMin[_tmem], valueMax[_tmem] )) fatalError();
                            }
                            l->value = memSize;
                            int memSizeInt = memSize;
                            //allocate space in the dsp code
                            checkAndCreateParam();
                            l->s.address = dspMem_LocationMultiple(memSizeInt);
                            break; }
                        case _TAPS: { //TAPS
                            checkAndCreateParam();
                            int numTaps=0;
                            if (l->s.type != _empty) fatalErrorNum(12);
                            l->s.type = label_taps;
                            l->s.address = opcodeIndex();
                            do {
                                errPtr = p;
                                double tap = 0.0;
                                if ((res = searchExpressionRange( &p, &tap, _tvalue32 )) < _error) fatalError();
                                l->value = 0;
                                numTaps++;
                                errPtr = p;
                                res = searchDelimiter( &p, "," );
                            } while (res);
                            //convert all taps
                            dspprintf2("*** %d TAPS ***\n",numTaps);
                            break; }
                        case _VALUE :
                        case _VALUEINT : { //
                            checkAndCreateParam();
                            int numValues = 0;
                            if (l->s.type != _empty) fatalErrorNum(12);
                            if (res == _VALUE) l->s.type = label_value;
                            else l->s.type = label_valueint;
                            l->s.address = opcodeIndex();
                            int res2;
                            do {
                                errPtr = p;
                                double value = 0.0;
                                if ((res2 = searchExpressionRange( &p, &value, (res == _VALUE) ? _tvalue32 : _tint32 )) < _error) fatalError();
                                l->values[numValues] = value;
                                if (numValues == 0) l->value = value;
                                if (res == _VALUE) addGainCodeQNM(value); else addCode(value);
                                errPtr = p;
                                numValues++;
                                res2 = searchDelimiter( &p, "," );
                                if (res2 && ((numValues >= label_max_values) ) ) fatalErrorNum(35);
                            } while (res2);
                            l->numValues = numValues;
                            errPtr = p;
                            break; }
                        case _DRCIN : {
                            checkAndCreateParam();
                            int numValues = 0;
                            if (l->s.type != _empty) fatalErrorNum(12);
                            l->s.type = label_drcin;
                            l->s.address = opcodeIndex();
                            double value = 0.0;
                            if ((res = searchExpressionRange( &p, &value, _tdrc_attack )) < _error) fatalError();
                            l->value = value;       //attack
                            l->values[0] = value;   //attack
                            addGainCodeQNM(value);
                            numValues++;
                            errPtr = p;
                            if ( (res = searchDelimiter( &p, "," ) == 0) ) fatalErrorNum(30);;
                            errPtr = p;
                            if ((res = searchExpressionRange( &p, &value, _tdrc_attack )) < _error) fatalError();
                            l->values[1] = value;   //release
                            addGainCodeQNM(value);
                            addCode(0); //alpha computed upon fs change
                            addCode(0); //alpha computed upon fs change
                            numValues++;
                            errPtr = p;
                            res = searchDelimiter( &p, "," );
                            if (res) {
                                if ((res = searchExpressionRange( &p, &value, _tvalue32 )) < _error) fatalError();
                                l->values[2] = value;   //threshold
                                addGainCodeQNM(value);
                                numValues++;
                                errPtr = p;
                                if ( (res = searchDelimiter( &p, "," ) == 0) ) fatalErrorNum(30);;
                                errPtr = p;
                                if ((res = searchExpressionRange( &p, &value, _tvalue32 )) < _error) fatalError();
                                l->values[3] = value;   //release
                                addGainCodeQNM(value);
                                numValues++;
                                errPtr = p;
                                res = searchDelimiter( &p, "," );
                                errPtr = p;
                                if (res) {
                                    if ((res = searchExpressionRange( &p, &value, _tvalue32 )) < _error) fatalError();
                                    l->values[4] = value;   //slope
                                    addGainCodeQNM(value);
                                    numValues++;
                                    res = searchDelimiter( &p, "," );
                                    if (res) {
                                        l->values[5] = 0;
                                        addCode(0);         //used to store inv_threshold
                                        numValues++;
                                    }
                                }
                            }
                            l->numValues = numValues;
                            break; }
                        case _DRCOUT : {
                            checkAndCreateParam();
                            if (l->s.type != _empty) fatalErrorNum(12);
                            l->s.type = label_drcout;
                            l->s.address = opcodeIndex();
                            l->value = 0;
                            l->values[0] = 0;   //enveloppe computed
                            l->values[1] = 0;   //new gain computed
                            l->values[2] = 0;   //tbd
                            l->numValues = 3;
                            addCode(0);
                            addCode(0);
                            addCode(0);
                            errPtr = p;
                            break; }
                        case _FILTER :
                        case _FILTER8 : { //
                            if (l->s.type != _empty) fatalErrorNum(12);
                            if (keyw == _FILTER) l->s.type = label_filters;
                            else l->s.type = label_filter8;
                            checkAndCreateParam();
                            l->s.address = opcodeIndex();
                            filterkeyw = keyw;
                            goto filter_entry;
                            break; }
                        default :  {
                            // then should be an numerical expression
                            res = searchDelimiter( &p, "=" );
                            errPtr = p;
                            enum label_type_e old = l->s.type;
                            double temp;
                            if (res) {
                                //value or label authorized after an "="
                                if ((res = searchExpression( &p, &temp )) < _error) fatalError();
                                errPtr = p;
                                old = _empty;  //clear previous definition if any
                            } else {
                                char * prevp = p;
                                res = searchValue( &p, &temp, 1 );  //otherwise only numerical
                                errPtr = p;
                                if (res == _empty)  fatalErrorNum(7);
                                if (testDelimiter( &p, "+-*/")) { errPtr= prevp; fatalErrorNum(31); }
                            }
                            if (old == _empty) {
                                l->value = temp;
                                l->s.type = res;
                                int lval = l->value;
                                double lvalue = lval;
                                if (lvalue == l->value) {
                                    //dspprintf3("label type %d %s = %d\n",l->s.type,l->s.name, lval);
                                } else {
                                    //dspprintf3("label type %d %s = %f\n",l->s.type,l->s.name, l->value);
                                }
                            } else
                                dspprintf2("label type %d %s already set with value %f\n",l->s.type,l->s.name, l->value);
                            break;
                        } // default
                        } //switch (keyword)
                    } // not a filter
                } //if keyw == -1
                if (lineNum > 0) {
                    if (testDelimiter( &p, "#")) continue;
                    int res = searchDelimiter( &p, ";\n\r\\" );     //optional delimiter on a line
                    errPtr=p;
                    if (res ==0 ) fatalErrorNum(13);
                    if (res != ';') {errPtr=p; break; } //skip end of line
                }
            } else {
                //a strange caracter is encountered
                errPtr=p; fatalErrorNum(6);  }
        } // while(*p)
    } //while (1)
finished:

    fclose(dspbasicInput);

    dspprintf1("\n");
    } // while (nextname);

    size = dsp_END_OF_CODE();

    createSymbolTable();
    freeAllLabels();
    return size;
}

void fatalError(){
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
    case -33: fprintf(stderr,"Error: TAPS label expected\n"); break;
    case -34: fprintf(stderr,"Error: string expected\n"); break;
    case -35: fprintf(stderr,"Error: too much values\n"); break;
    case -36: fprintf(stderr,"Error: DRCIN label expected\n"); break;
    case -37: fprintf(stderr,"Error: DRCOUT label expected\n"); break;
    case -38: fprintf(stderr,"Error: DRCIN requires 4 parameter\n"); break;
    case -39: fprintf(stderr,"Error: DRCIN requires 5 parameter\n"); break;
    case -40: fprintf(stderr,"Error: VALUEINT label expected\n"); break;
    case -41: fprintf(stderr,"Error: MEMORY label expected\n"); break;
    default: break;
    }
    fprintf(stderr,"l%d: %s",lineNum-1,line);
    fprintf(stderr,"l%d: ",lineNum-1);
    char *q = line;
    while (isSpaceOrTab(*errPtr)) errPtr++;
    while (q != errPtr) { fprintf(stderr,"%c",(*q==9 ? *q : ' ')); q++; }
    fprintf(stderr,"^\n");
    freeAllLabels();
    fclose(dspbasicInput);
    exit(errNum);
}
