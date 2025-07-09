#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "dsp_filters.h"
#include "dsp_encoder.h"

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
    "LP1","HP1","LS1","HS1","AP1",
    "BP0DB","LP2","HP2", "LS2","HS2", "AP2",
    "PEAK","NOTCH","BPQ","HILB","LT",
};

const char filterTypes[filterTypesNumber] = {
    LPBE2,LPBE3,LPBE4,LPBE6,LPBE8,   // bessel
    HPBE2,HPBE3,HPBE4,HPBE6,HPBE8,
    LPBE3db2,LPBE3db3,LPBE3db4,LPBE3db6,LPBE3db8,    // bessel normalized at -3db cutoff
    HPBE3db2,HPBE3db3,HPBE3db4,HPBE3db6,HPBE3db8,
    LPBU2,LPBU3,LPBU4,LPBU6,LPBU8, // buterworth
    HPBU2,HPBU3,HPBU4,HPBU6,HPBU8,
    LPLR2,LPLR3,LPLR4,LPLR6,LPLR8,   // linkwitz rilley
    HPLR2,HPLR3,HPLR4,HPLR6,HPLR8,
    FLP1,FHP1,FLS1,FHS1,FAP1,   //first order low pass, high pass, shelves and allpass
    FBP0DB,       //bandpass normalized to produce its peak with 0db gain
    FLP2,FHP2,    // low pass and high pass
    FLS2,FHS2,    // low shelf and high shelf
    FAP2,FPEAK,FNOTCH, // allpass, peak and notch
    FBPQ, FHILB, FLT,      // bandpass, hilbert, LT
};

const char filterOrders[filterTypesNumber] = {
    2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8, 2,3,4,6,8,
    1,1,1,1,1, 2, 2,2, 2,2, 2,2,2, 2,99,2 
};

enum keywords_e {
    _DSPFSMIN, _DSPFSMAX, _DSPFSDYN, _DSPMANT, _DSPFLOAT, _DSPIOMAX, _DSPCLOCK, _DSPCOND, _DSPXS2, _DSPXS3,_DSPPRINTF,
    _end, _include, _if, _param, _nop, _core, _section, _sectionelse, _coreaes,
    _input, _output, _transfer, _inputgain, _outputgain, _outputpdf, _outputvol, _outputvolsat,
    _mixer, _mixergain, _gain, _clip,
    _clrxy,_swapxy,_copyxy,_copyyx,_addxy,_addyx,_subxy,_subyx,_mulxy,_mulyx, _divxy,_divyx,_avgxy,_avgyx,_negx,_negy,_shift,_valuex,_valuey,
    _saturate, _saturatevol, _saturategain,
    _delayone, _delayus, _delaydpus, _delayusfbmix,
    _savexmem, _loadxmem,  _saveymem, _loadymem,
    _dcblock, _biquad, _biquad8, _convol, _warpconvol,
    _tpdf, _white, _sine,_square,_dirac,
    _integrator, _cicus, _cicn,_expma,_thdcomp,
    _envpeak,_envrms,_limiterpeak,_limiterrms,_limiterpeakhard,_compressor,_expander,_noisegate,
    _tile,_send,_receive,_instructions,_priorityon,_priorityoff,
    _memclr,_swapmem,_addmem,_memadd,_submem,_memsub,_mulmem,_divmem,_avgmem,_memavg,_memneg,_memsave,_loadmem,
    _memvalue,_mixermem,_memgain,_meminput,
    dspKeywordsNumber
};
static const char * dspKeywords[dspKeywordsNumber] = {
    "DSPFSMIN","DSPFSMAX","DSPFSDYN","DSPMANT","DSPFLOAT","DSPIOMAX","DSPCLOCK","DSPCOND","DSPXS2","DSPXS3","DSPPRINTF",
    "end", "include", "if", "param", "nop", "core", "section", "sectionelse", "coreaes",
    "input", "output","transfer", "inputgain", "outputgain", "outputtpdf", "outputvol", "outputvolsat", "mixer","mixergain","gain","clip",
    "clrxy","swapxy","copyxy","copyyx","addxy","addyx","subxy","subyx","mulxy","mulyx","divxy","divyx","avgxy","avgyx","negx","negy","shift","valuex","valuey",
    "saturate", "saturatevol","saturategain",
    "delayone", "delayus", "delaydpus", "delayusfbmix",
    "savexmem", "loadxmem","saveymem", "loadymem",
    "dcblock", "biquad", "biquad8", "convol", "warpconvol",
    "tpdf", "white", "sine","square","dirac",
    "integrator","movingavgus","movingavgn","expmovingavg","thdcomp",
    "envpeak","envrms","limiterpeak","limiterrms","limiterpeakhard","compressor","expander","noisegate",
    "tile","send","receive","instructions","priorityon","priorityoff",
    "memclr","swapmem","addmem","memadd","submem","memsub","mulmem","divmem","avgmem","memavg","memneg","memsave","loadmem","memvalue","mixermem","memgain","meminput",
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

enum { dspIOmaximum = 64};

enum   tvalue_e                  {   _tIO          , _tfreq, _tvalue32, _tq31,  _tvalue64,    _tint32,  _tdelay, _tfilterQ, _tmem, _tshift, _ttpdf, _tpercent, _ttile, _tmant, _tmant2, _tdrc_attack, _tnone };
static double valueMin[_tnone] = {                0,     10,      -8.0,  -1.0,  -128.0,  -0x7FFFFFFF,        0,        0 ,     1,     -32,      8,         0,      1,     15,      31,  0.001 };
static double valueMax[_tnone] = {   dspIOmaximum-1,  95999,      +8.0,  +1.0,  +128.0,   0x7FFFFFFF, 10000000,       20 ,    32,      32,     31,         1,      4,     30,      62,  4.0   };


int numTile = 0;    //current tile number 0 means all following tile have visibility on current symbols

static double errMin=0.0, errMax=0.0;

//points on previous set of character where an error is detected
static char * errPtr;
static int errNum;


static void fatalError(); //prototype as it is located at botom of this file
static void fatalErrorNum(int num);
static void fatalErrorNumIf(int num, int cond);


//max 3 include files level (4 open in total)
#define maxIncludedFiles 4         
static FILE* dspInputArray[maxIncludedFiles]; 
static FILE* * dspInput = dspInputArray;

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
    return ( ((ch >= 32) && (ch != '\"' )) || (ch == 9) ) ? ch : 0;
}

//test End of Line charachters
static inline int isCharEOL(char ch) {
    return (((ch == 0) || (ch == '#') || (ch == 0x0A) || (ch == 0x0D)) ? (ch?ch:1) : 0);
}

//basic function to move pointer forward, bypassing all spaces and tabs
static inline char * skipSpacesBasic(char * * s) {
    char * p = *s;
    while (isSpaceOrTab(p[0])) p++;
    *s = p;
    return p;
}

//prototype
static inline char * skipSpaces(char * * s);    


//check if current delimiter character is in the given list then return its value otherwise 0
static int searchDelimiter(char * * s, char * delim) {
    char * p = skipSpaces(s);
    int len = strlen(delim);
    for (int i = 0; i<len; i++)
        if (p[0] == ((delim[i]==1) ? 0 : delim[i])) {
            if (p[0]) *s = p+1;
            return delim[i]; }
    return 0;
}

//search the next character compatible with one of the list of delimiter provided, then point after, otherwise return 0
static int goAfterDelimiter(char * * s, char * delim) {
    char * p = *s;
    int len = strlen(delim);
    while (p[0]) {
        for (int i=0; i< len; i++)
            if (p[0] == delim[i]) {
                //found one
                errPtr = p;
                *s = p+1;
                return p[0];
            }
        p++;
    }
    return 0;
}
    
 
//test the next character against the given list of delimiter and return its value
static int testDelimiter(char * * s, char * delim) {
    int res = searchDelimiter( s , delim);
    if (res) (*s)--;
    return res;
}

//expect one of the given delimiter otherwise raise an error
static void getDelimiterError(char * * s, char delim, int err) {
    char * p = skipSpaces(s); 
    fatalErrorNumIf(err, p[0] != delim );
    errPtr = p+1;
    *s = p+1;
}

//expect an end of line (typically after include "file") otherwise raise error 32
static void getEOLError(char * * s) {
    skipSpacesBasic( s );
    if ( (**s == '\\') || (0 == searchDelimiter( s, "#\r\n\01" )) ) fatalErrorNum(32);
}

//expect an end of line or a ; typically between two instructions, otherwise raise error 13
static void getSeparatorEOLError(char * * s) {
    if (searchDelimiter( s, ";" )) {
        while (searchDelimiter( s, ";" ));
        return;
    }
    if (0 == testDelimiter( s, "#\r\n\01" )) fatalErrorNum(13);
}

//check if current delimiter character is in the given string then return its value otherwise 0
static int searchString(char * * s, char * * str ) {
    char * p = skipSpaces(s);
    if (p[0] == '\"') {
        p++;
        *str = p;   //begining of string
        int len=0;
        while (isCharOfString(p[0])) { len++; p++; }
        if (p[0]) {   //expect a quote character
            p[0] = 0;
            errPtr = p+1;
            *s = p+1; 
        } else {
            *s = p;
            errPtr = p;
        }
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

enum { label_max_values = 8 };  //max number of values possible for a symbol

typedef struct label_s {
    struct label_s *   next;
    double       value;          //value typically when labelType < 4
    int          numValues;      //number of values in the table
    double       values[label_max_values];      //value typically when labelType >= 4
    dspSymbol_t s;              //s.name will be extended during malloc : KEEP s at bottom of structure
} label_t;

typedef label_t * labelptr_t;

//contains pointer on the list of labels created
static labelptr_t firstLabel = NULL, lastLabel = NULL;
char * lastLabelName = "";

//add a new label in the list and return its allocated pointer.
//character pointer expected to point on label and then moved after label.
static labelptr_t appendNewLabel(char * s) {
    char * p = s;
    int len = 0;
    while ( isAlphanum(p[0]) ) { p++; len++; }
    labelptr_t l = malloc(sizeof(label_t)+len);
    l->s.length = len;
    l->next = NULL;
    l->s.type = _empty;
    if (firstLabel) { lastLabel->next = l; lastLabel = l; }
    else { firstLabel = lastLabel = l; }
    int i;
    for (i=0, p=s; i<len; i++) l->s.name_[i] = *(p++);
    l->s.name_[len] = 0;
    l->s.name = l->s.name_; //set pointer
    lastLabelName = l->s.name;
    l->value = 0;
    l->s.address = 0;
    l->numValues = 0;
    l->s.tileNum = dsp_TILE_num();
    l->s.tileUsed = 0;
    return l;
}
 
static labelptr_t appendNewLabelConst(const char * cp) {
    char * p = (char *)cp;
    return appendNewLabel(p);
}



//search a possible label
static label_t * findLabel(char * p) {
   for (labelptr_t l = firstLabel; l ; l=l->next) {
        if ( p == strstr( p, l->s.name ) ) {
            int len = l->s.length;
            if ( isAlphanum( p[len]) ) continue; //not yet
            //check tile, either 0, either current
            if ( (l->s.tileNum == 0) || (l->s.tileNum == dsp_TILE_num()) ) {
                lastLabelName = l->s.name;
                return l;
            }
        }
    }
    return NULL;
}

//search a possible label in *s then returns its pointer otherwise 0
static label_t * searchLabel(char * *s){
    char * p = skipSpaces(s);
    labelptr_t l = findLabel(p);
    if (l) {
        int len = l->s.length;
        *s = &p[len];
        return l;
        }
    return NULL;
}

void usedLabelInTile(label_t * l) {
    if (l) l->s.tileUsed |= (1UL<<dsp_TILE_num());
}


static void freeAllLabels(){
    labelptr_t l = firstLabel;
    while (l) {
        volatile labelptr_t next = l->next;
        free(l);
        l = next;
    }
    lastLabelName = "";
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

enum { withoutDB = 0, acceptDB = 1 };

//try to extract a numerical value from the charater pointer.
//then returns 1 for real or 2 for decibel and 3 for integer and provides result in "value" param
//otherwise return 0
static int searchNumerical(char * * s, double * value, int enableDB) {
    int state = 0;
    double mantisse = 1.0, sign = 0.0, base = 10.0, result = 0.0;
    char * p = skipSpaces(s);
    char* begin = p;
    errPtr = p;
    *value = 0.0;
    while (p[0]) {
        if (state == 0) {
            //sign authorized only at the begining
            if ( (p[0]) == '-') { 
                if (sign == 0.0) sign = -1.0; else sign = -sign; 
                p++; continue; }
            if ( (p[0]) == '+') { 
                if (sign == 0.0) sign = 1.0;
                p++; continue; }
            if ( isSpaceOrTab(p[0]) ) { p++; continue; }
            if ( ((p[0]) == 'b') || ((p[0]) == 'x') || ((p[0]) == 'o') ) {
                //potential labels are prioritized
                char * pos = p;
                labelptr_t l = searchLabel(&pos);
                if (l) { *s = begin; return _empty; }
            }
            if ( (p[0]) == 'b') { base=2.0;  p++;state |= 1; continue; }
            if ( (p[0]) == 'x') { base=16.0; p++;state |= 1; continue; }
            if ( (p[0]) == 'o') { base=8.0;  p++;state |= 1; continue; }
        }
        if ((state == 1) && (base==10.0)) {
            //point authorised only after at least one digit
            if (p[0] =='.') { state |= 2; p++; continue; }
        }
        if (state) {
            if ( (base==10.0) && ((p[0]=='%')||(p[0]=='m')) ) {
                if (sign == 0.0) sign = 1.0;
                *value = result * sign / ((p[0]=='%')? 100.0 : 1000.0);
                *s = &p[1];
                return _value; 
            }
            if ( ((p[0]=='d')||(p[0]=='D')) && ((p[1]=='b')||(p[1]=='B'))) {
                //check if authorized
                if ((enableDB == 0)||(base != 10.0)) fatalErrorNum(25);
                if (sign == 0.0) sign = 1.0;
                *value = pow( 10, result * sign / 20.0 );
                *s = &p[2];
                return _valuedb; 
            } //decibel
        }
        if ( ((base==8.0)  && isNumber(  p[0] ) && (p[0]<'8')) ||
             ((base==10.0) && isNumber(  p[0] )) ||
             ((base==16.0) && isHexaNum( p[0] )) ||
             ((base==2.0)  && isBinary(  p[0] ) ) ) {
            state |= 1; //number description started
            if (state & 2) mantisse /= 10.0;
            else result *= base;
            int num;
            if ((base == 16.0) && isLetter(p[0])) num = (p[0] & 7)+9;
            else num = p[0] - '0';
            result += num * mantisse;
            p++; continue;
        } else break;  //unknown character
    } // while
    if (state) { //number started?
        *s = p;
        if (sign == 0.0) sign = 1.0;
        result *= sign; *value = result;
        int integer = result; double check = integer;
        if (check == result) return _valueint;
        else return _value;
    }
    if (sign == -1.0) *value = sign;
    return _empty;
}


static int outOfRange(double x,double Min,double Max){
    errMin = Min; errMax = Max;
    if ((x<Min)||(x>Max)) return  errNum = -5;
    return 0;
}

static void outOfRangeError(double x,double Min,double Max){
    if (outOfRange(x, Min, Max) <= _error) fatalError();
}


static int getLabelMemory(char ** s) {
    char * p = skipSpaces(s);
    labelptr_t l = findLabel(p);
    if (l) {
        if (l->s.type != label_memory) fatalErrorNum(10);
        double val=l->s.address;
        int len = l->s.length;
        *s = &p[len];
        usedLabelInTile(l);
        int bracket = searchDelimiter(s,"[.");
        if (bracket) {
            int res2 = searchNumerical(s, &val, withoutDB);
            if (res2 == _empty) {
                if (l->value > 1.0) fatalErrorNum(2);
            } else if (res2 != _valueint) fatalErrorNum(11);
            outOfRangeError( val, 0.0, l->value-1.0 );
            if (bracket == '[') getDelimiterError( s, ']', 26);
            val += val + l->s.address;
        }
        return val;
    } else fatalErrorNum(41);
    return 0;
}



//search for a direct numerical value, or a label of type value eventually with an index [x] or .x
static int testExpression(char * * s, double * value){
    const int depthMax = 4;
    static int depth=0;
    int modeDB=0;
    double sum = 0.0, mul = 0.0, temp = 0.0;
    int prevop = 0;
    int res;
    //if (depth == 0) modeDB = 0;
    skipSpaces ( s );
    do {
        //get a numerical value. postfix db authorized only if first one or if already detected
        res = searchNumerical( s, &temp, (prevop == 0) || modeDB );
        if (res == _empty) {
            int neg = (temp == -1.0);
            int braket = searchDelimiter( s, "(");
            if (braket || neg){
                //recursive!
                if (depth < depthMax) depth++; else fatalErrorNum(48);
                res = testExpression(s, &temp);
                if (res == _empty) fatalErrorNum(2);
                if (braket) getDelimiterError( s, ')', 29);
                depth--;
                if (neg) {
                    if (res == _valuedb) temp = 1.0/temp;
                    else temp = -temp;
                }
            }
        }
        if (res == _empty) {
            if ( isLetter( **s ) ) {
                labelptr_t l = searchLabel(s);
                if (l == NULL) fatalErrorNum(-2);  //numericalValueOrLabelExpected
                if( l->s.type > _valueint ) fatalErrorNum(27);
                temp = l->value;
                //test an optional index
                int bracket = searchDelimiter( s, "[.");
                if ((res=bracket)) {
                    if( l->s.type != _valueint ) fatalErrorNum(11);
                    double index;
                    char * e = *s;
                    if (bracket == '.') res = searchNumerical( s, &index, withoutDB);
                    else {
                        if (depth < depthMax) depth++; else fatalErrorNum(48);
                        res = testExpression(s, &index); //recursive!
                        depth--;
                    }
                    //check for an integer
                    if (res != _valueint) { errPtr = e; fatalErrorNum(11);}
                    if ((index<0)||(index>255)) { 
                        errPtr = e; errMin=0; errMax=256; fatalErrorNum(5); }
                    temp += index;
                    if (bracket == '[') getDelimiterError( s, ']', 26);
                } 
                res = l->s.type;
            }
        }
        if (res) {
            if (res == _valuedb) {
                if (prevop == 0) modeDB = 1;
                else if (modeDB == 0) fatalErrorNum(25);
            } else {
                if (modeDB) fatalErrorNum(25);
            }
            //test potential operators
            res = searchDelimiter( s, "+-*/");
            //dspprintf("tmp %f, prevop %c, op %c\n",temp,prevop?prevop:'_',res?res:'_');
            switch (res) {
            case 0: //fall through
            case '-' : case '+' : { 
                if (modeDB) {
                    switch (prevop) {
                    case 0 : sum = temp; break;
                    case '+': sum *= temp; break;
                    case '-': sum /= temp; break;}
                } else 
                    switch (prevop) {   //4*3+5*2
                    case 0 : //fall through
                    case '+': sum += temp; break;
                    case '-': sum -= temp; break;
                    case '*': mul *= temp; sum += mul; mul = 0; break;
                    case '/': mul /= temp; sum += mul; mul = 0; break;}
                break; }
            case '*': case '/': {
                if (modeDB) fatalErrorNum(25);
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
            //dspprintf("sum %f, mul %f, temp %f, prevop %c\n",sum,mul,temp,prevop?prevop:'_');
        } else {
            //no more value, check if an operator was pending
            if (prevop) fatalErrorNum(18);
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

//expect an expression, otherwise error. Return result type (value, valuedb, valueint)
static int searchExpression(char * * s, double * value){
    int res;
    res = testExpression( s , value );
    if (res == _empty ) fatalErrorNum(2);
    return res;
}

static int searchExpressionRangeError(char * * s, double * value, int range){
    int res = searchExpression( s , value );
    if (range != _tnone) { 
        if ((res != _valueint) && 
            ((range == _tIO) || (range == _tmem)|| (range == _ttpdf)|| (range == _tint32)|| (range == _tshift)|| (range == _ttile)|| (range == _tmant)|| (range == _tmant2))) fatalErrorNum(11);
        outOfRangeError( *value, valueMin[range], valueMax[range]);
    } 

    return res;
}

static void replaceExpressions( char * * s) {
    char buf[30];
    int res;
    while ((res = goAfterDelimiter(s, "["))) {
        double value = 0;
        char * begin = *s -1;  //begining of the expression just after []
        res = searchExpression(s, &value);
        getDelimiterError( s, ']', 26);
        char * end = *s;     //point on the ]
        int lenafter = strlen( end ) + 1; //compute remainings characters in the line, including last 0
        unsigned long long delta = (end - begin); 
        unsigned size = delta; 
        int lenbuf;
        if      (res == _valueint)  lenbuf = sprintf(buf, "%d",(int)value);
        else if (res == _value)     lenbuf = sprintf(buf, "%f",value);
        else if (res == _valuedb)   lenbuf = sprintf(buf, "%fdb",20*log10(value));
        else                        lenbuf = sprintf(buf, "???");
        //dspprintf("begin %c, end %c, len %d, size %d, lenbuf %d\n",begin[0],end[0],lenafter,size,lenbuf);
        //eventually adjust bin space (dest, source, size)
        if (size != lenbuf) memmove( &begin[lenbuf], end, lenafter); //resize to fit requirement
        memcpy(begin,buf,lenbuf);
        *s = &begin[lenbuf];
    }
}


static int paramSection = 0;

void checkAndCreateParam(){
    if (paramSection == 0)
        paramSection = dsp_PARAM();
}

void clearParamSection() {
    paramSection = 0;
}


static char line[32768] = ""; //buffer for one line of code
static int  lineNumArray[maxIncludedFiles];
static int * lineNum = lineNumArray;


static char * fgetLine() {
    return fgets( line, sizeof(line), *dspInput );
}

static char * skipSpaces(char * * s){
    while (1) {
        skipSpacesBasic( s );
        if(**s != '\\') break;
        (*s)++;
        skipSpacesBasic( s );
        errPtr = *s;
        if (isCharEOL(**s)==0) fatalErrorNum(32);
        if (fgetLine()) {
            lineNum[0]++;
            *s = line;
            errPtr = *s;
            continue;   //while 1
        } 
        line[0] = 0;
        *s = line;
    }
    errPtr = *s;     //error position points on active character (non space)
    return *s;
}

char * dspoutheader = "//welcome to cplusplus\n\n" \
                      "#include \"avdspinclude.hpp\"\n\n";
char * dspoutfilename = "avdspout.cpp";

int dspbasicCreate(char * dspbasicName, int argc, char **argv){
    int fileNum = 0;    //depth for included files
    dspInput = dspInputArray;
    lineNum = lineNumArray;
    lineNum[0] = 0;
    char * nextName = dspbasicName;
    int size = 0;
    int numCore = 1;
    if (numCore) {} //just to please compiler
    int dspModeDynamic = 0;
    int tapsinclude = 0;
    int ifcondition = 1;
    dsp_PROCESSOR(0);
    char *txtDSPLINE = "DSPLINE";
    labelptr_t pDSPLINE = appendNewLabel(txtDSPLINE);
    pDSPLINE->s.type =_valueint;
    dsp_COND(0);
    labelptr_t pDSPCOND = appendNewLabelConst(dspKeywords[_DSPCOND]);
    pDSPCOND->s.type = _valueint;
    dsp_CLOCK(528,496,528,0);
    labelptr_t pDSPCLOCK = appendNewLabelConst(dspKeywords[_DSPCLOCK]);
    pDSPCLOCK->s.type = _valueint;
    pDSPCLOCK->value = 528;
    labelptr_t pDSPFSMIN = appendNewLabelConst(dspKeywords[_DSPFSMIN]);
    pDSPFSMIN->s.type = _valueint;
    pDSPFSMIN->value = dspConvertFrequencyFromIndex(dspMinSamplingFreq);
    labelptr_t pDSPFSMAX = appendNewLabelConst(dspKeywords[_DSPFSMAX]);
    pDSPFSMAX->s.type = _valueint;
    pDSPFSMAX->value = dspConvertFrequencyFromIndex(dspMaxSamplingFreq);
    labelptr_t pDSPMANT = appendNewLabelConst(dspKeywords[_DSPMANT]);
    pDSPMANT->s.type = _valueint;
    pDSPMANT->value = dspMant;

    dspOutFileInit(dspoutfilename,dspoutheader);
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

    *dspInput = fopen( dspbasicName, "r" );
    if (*dspInput == NULL) {
        fprintf(stderr,"Error: Failed to open %s file.\n",dspbasicName);
        return -1;
    }
    if(fseek( *dspInput, 0, SEEK_END ) ) {
      fprintf(stderr,"Error: Failed to discover %s size.\n",dspbasicName);
      fclose(*dspInput);
      return -1;
    }

    int dspbasicFileSize = ftell( *dspInput );

    if(fseek( *dspInput, 0, SEEK_SET ) ) {
      fprintf(stderr,"Error: Failed to set file pointer for %s file (size=%d).\n",dspbasicName,dspbasicFileSize);
     return -1;
    }
    int countarg = 0;

    while (1) {
nextline:
        if (lineNum[0] == 0) {
            //analyse all the parameters given on the command line
            if (countarg != argc) {
                strcpy(line, argv[countarg]);
                dspprintf1("option %d %s\n",countarg,line);
                countarg++;
            } else 
                lineNum[0] = 1;
        }
        pDSPLINE->value = lineNum[0];
        if (lineNum[0]) {
            //read next line
            while ( fgetLine() == 0) {
                //end of file detected
                if (fileNum) {
                    fclose(*dspInput);
                    //restore previous file informations
                    fileNum--;
                    lineNum--;
                    dspInput--;
                    dspprintf2("back to previous file, line %d\n",lineNum[0]);
                } else
                    goto finished;
            }
            if (ifcondition) dspprintf4("%4d %s",lineNum[0],line);
            lineNum[0]++;

        }
        double input, output, gain, delay, freq, filterQ, freqLT,filterQLT, tpdf;
        char * p = line;   //pointer on the character to analyse
        //main loop to analyse the line
        while ( p[0] ) {
            //skip any spaces or tab
            if ( isSpaceOrTab(p[0]) || ((p[0]==';')) ) { p++; continue; }
            else errPtr = p;
            //check special case '#-' as a prefix for printable comments
            if ( ifcondition && (p[0] == '#') && (p[1] == '-') ) {
                p += 2;
                char * line = p;
                replaceExpressions( &p );
                fprintf(stderr,"%s",line);
                break; // next line
            }
 
            //load a new line when finding # or cr/lf
            if ( (p[0] == 0) || (p[0] == '#')  || (p[0] == 0x5C ) || (p[0] == 0x0A) || (p[0] == 0x0D) ) break; //goto next line

            if (tapsinclude) goto labeltaps;
            //expecting either a label definition or a dsp keyword, all starting by a letter
            fatalErrorNumIf( 6, isLetter( p[0] ) == 0 );
            int res;
            int keyw = searchKeywords( &p, dspKeywords, dspKeywordsNumber);
            if (ifcondition == 0) {
                if (keyw == _if) {
                    ifcondition = 1;
                } else goto nextline;
            }
            if (keyw > _param) {
                clearParamSection();
                calcLength();
            }
            switch(keyw) {
            case _DSPFSMIN : 
            case _DSPFSMAX : {
                fatalErrorNumIf(45, dsp_checkCodeAlready());
                static int minFreq = -1;
                static int maxFreq = -1;
                double freq = 0;
                searchExpression( &p, &freq );
                int index = dspConvertFrequencyToIndex(freq);
                fatalErrorNumIf(44, index >= FMAXpos );
                if (keyw == _DSPFSMIN) {
                    fatalErrorNumIf(44, minFreq>=0);
                    if ((maxFreq>=0) && (index>maxFreq)) fatalErrorNum(44);
                    minFreq = index;
                    res = dsp_FSMIN(index);
                } else {
                    fatalErrorNumIf(1,maxFreq>=0);
                    if ((minFreq>=0) && (index<minFreq)) fatalErrorNum(44);
                    maxFreq = index;
                    res = dsp_FSMAX(index);
                }
                fatalErrorNumIf(45, res == 0 );
                pDSPFSMIN->value = dspConvertFrequencyFromIndex(dspMinSamplingFreq);
                pDSPFSMAX->value = dspConvertFrequencyFromIndex(dspMaxSamplingFreq);
                break ;}
            case _DSPMANT : 
            case _DSPFLOAT : {
                fatalErrorNumIf(45, dsp_checkCodeAlready());
                static int mantissa = -1;
                fatalErrorNumIf(46, (mantissa >= 0));
                if (keyw == _DSPMANT) {
                    double mant = 0;
                    searchExpressionRangeError( &p, &mant, _tmant );
                    mantissa = mant;
                    //mantissa2 should be less or equal to mantissa+32
                    valueMax[_tmant2] = mant+32;
                    valueMax[_tvalue32] = 1ULL<<(31-mantissa);
                    valueMin[_tvalue32] = -valueMax[_tvalue32];
                    if ((res = searchDelimiter( &p, ","))) {
                        searchExpressionRangeError( &p, &mant, _tmant2 );
                        valueMax[_tvalue64] = 1ULL<< ( 63-(int)mant );
                        valueMin[_tvalue64] = -valueMax[_tvalue64];
                    }
                    res = dsp_FORMAT(mantissa, res ? mant:0);
                } else {
                    res = dsp_FORMAT(0,0);
                    valueMax[_tvalue32] =  128.0;
                    valueMin[_tvalue32] = -128.0;
                    valueMax[_tvalue64] =  128.0;
                    valueMin[_tvalue64] = -128.0;
                }
                fatalErrorNumIf(46, res == 0 );
                pDSPMANT->value = dspMant;
                break; }
            case _DSPIOMAX : {
                fatalErrorNumIf(45, dsp_checkCodeAlready());
                double value = 0;
                if (_valueint != searchNumerical( &p, &value, withoutDB)) fatalErrorNum(5);
                outOfRangeError(value,8,256);
                int val = value;
                if ((val != value) || (val & 7)) fatalErrorNum(47);
                res = dsp_IOMAX(value);
                valueMax[_tIO] = value;
                fatalErrorNumIf(45, res == 0 );
                break; }
            case _DSPFSDYN : {  //allow biquad filter to be calculated when FS is changed and not staticaly
                double value = 0;
                if (_valueint != searchNumerical( &p, &value, withoutDB)) fatalErrorNum(5);
                outOfRangeError(value,0,1);
                res = dsp_FSDYN(value);
                fatalErrorNumIf(45, res == 0 );
                dspModeDynamic = value;
                break; }
            case _DSPCLOCK : {
                fatalErrorNumIf(45, dsp_checkCodeAlready());
                double value = 0.0;
                if (_valueint != searchNumerical( &p, &value, withoutDB)) fatalErrorNum(5);
                outOfRangeError(value,480,648);
                int clockcpu = value;
                if ((clockcpu != value) || (clockcpu & 3)) fatalErrorNum(50);
                int clock176k=0,clock192k=0,prio=0;
                if ((res = searchDelimiter( &p, ","))) {
                    if (_valueint != searchNumerical( &p, &value, withoutDB)) fatalErrorNum(5);
                    outOfRangeError(value,480,508);
                    clock176k = value;
                    if ((clock176k != value) || (clock176k & 3)) fatalErrorNum(50);
                    getDelimiterError( &p, ',',30);
                    if (_valueint != searchNumerical( &p, &value, withoutDB)) fatalErrorNum(5);
                    outOfRangeError(value,500,548);
                    clock192k = value;
                    if ((clock192k != value) || (clock192k & 3)) fatalErrorNum(50);
                    if ((res = searchDelimiter( &p, ","))) {
                        if (_valueint != searchNumerical( &p, &value, withoutDB)) fatalErrorNum(5);
                        outOfRangeError(value,0,3);
                        prio=value;
                    }
                }
                dsp_CLOCK(clockcpu,clock176k,clock192k,prio);
                pDSPCLOCK->value = clockcpu;
                break; }
            case _DSPCOND : {
                double value = 0.0;
//                fatalErrorNumIf(45, dsp_checkCodeAlready());
                res = searchExpressionRangeError( &p, &value,_tint32);
                dsp_COND(value);
                pDSPCOND->value = value;
                break; }
            case _DSPXS2 :
            case _DSPXS3 : {
                dsp_PROCESSOR(keyw == _DSPXS2 ? 2 : 3 );
                double value = 0.0;
//                fatalErrorNumIf(45, dsp_checkCodeAlready());
//                res = searchExpressionRangeError( &p, &value, _tint32);
                res = testExpression( &p,  &value);
                if (res != _empty) {
                    outOfRangeError(value,0,0xFFFFFFFF);
                    dsp_COND(value);
                    pDSPCOND->value = value;
                }
                int freqmax = dspConvertFrequencyFromIndex(dspMaxSamplingFreq);
                int lastclock = pDSPCLOCK->value;
                lastclock *= 1000000/5;
                int inst = lastclock / freqmax;
                dspprintf2("XMOS instructions per samples %d, enabling condition 0x%X\n",inst,dspCondition);
                break;}
                case _DSPPRINTF : {
                    calcLength();
                    clearParamSection();
                    double value;
                    if (_valueint != searchExpression( &p, &value)) fatalErrorNum(5);
                    outOfRangeError(value,0,4);
                    dspPrintfVal = value;
                    break;}
            case _end:   { 
                if (fileNum==0) goto finished; 
                fprintf(stdout,"warning, 'end' instruction found in included file. Ignored\n");
                fclose(*dspInput);
                //restore previous file informations
                fileNum--;
                lineNum--;
                dspInput--;
                dspprintf2("back to previous file, line %d\n",lineNum[0]);
                goto nextline;
                break; }
            case _include : {
                gotoinclude:
                if (fileNum >= (maxIncludedFiles-1)) fatalErrorNum(43);
                char * str;
                if (searchString( &p, &str) <= _error) fatalErrorNum(42);
                getEOLError( &p );
                fileNum ++;
                lineNum++;
                dspInput++;
                lineNum[0] = 1;
                nextName = str;
                goto nextfile;  //restart by opening the next file name
                break; }

            case _if : {
                skipSpacesBasic( &p );
                res = testDelimiter( &p, ";#\r\n\01" );
                if (res) {
                    if (res>=32) p--;
                    ifcondition = 1; //end of line found : everything is now accepted
                } else {
                    double value;
                    getDelimiterError( &p, '(', 51);
                    res = searchExpressionRangeError( &p, &value,_tint32);
                    int var = value;
                    int num=0;
                    int cond=0;
                    while ((searchDelimiter( &p, ","))) {
                        res = searchExpressionRangeError( &p, &value,_tint32);
                        if (res != _valueint) fatalErrorNum(2);
                        int val = value;
                        if (num & 1) {
                            if (var & val) cond &= 0xFFFFFFFE;
                            else if (cond & 1) cond |= 2;
                        } else {
                            if (var & val) cond |= 1;
                        }
                        num++;
                    }
                    getDelimiterError( &p, ')', 29);
                    if (num) ifcondition = (cond ? 1 : 0);
                    else 
                        ifcondition = var ? 1 : 0;
                }
                if (ifcondition == 0) {
                    skipSpacesBasic( &p );
                    res = testDelimiter( &p, "#\r\n\01" );
                    if (res == 0) { 
                        dspprintf3("%4d IF condition 0, ignoring only this line %s",lineNum[0]-1,p);
                        ifcondition = 1;
                    } else 
                        dspprintf3("%4d IF condition 0, ignoring all next lines\n",lineNum[0]-1);
                    goto nextline;
                } else {
                    dspprintf3("%4d IF condition 1\n",lineNum[0]-1);
                    continue; }
                break;}
            case _param: {
                res = testExpression( &p, &input );
                if (res) {
                    if (res != _valueint) fatalErrorNum(11);
                    int num = input;
                    dsp_PARAM_NUM(num);
                    switch (num) {
                    case 200: {
                        getDelimiterError( &p, ',' , 30);
                        char * str;
                        if ((res = searchString( &p, &str)) <= _error) fatalErrorNum(34);
                        int code = res; int n = 1;  //first char is string length
                        for (int i=0; i<= res; i++) {
                            if (i < res) code |= (*str)<<(n*8);
                            n++; str++;
                            if ((n==4) || (i == res)) { addCode(code); code = 0; n = 0; }
                        }
                        break; }
                    } //switch
                    clearParamSection();
                } else
                    checkAndCreateParam();
                break; }

            case _nop : { dsp_NOP(); break; }

            case _section:
            case _sectionelse:
            case _core: 
            case _coreaes: {
                int num=0;
                do {        //multiple x,y condition accepted
                    unsigned progAny1 = 0xFFFFFFFF;
                    unsigned progOnly0 = 0;
                    res = testExpression( &p, &input );
                    if (res) {
                        if (res != _valueint) fatalErrorNum(11);
                        progAny1 = input;
                        num++;
                        res = searchDelimiter( &p, "," );
                        if (res) {
                            res = searchExpression( &p, &input );
                            if (res != _valueint) fatalErrorNum(11);
                            progOnly0 = input;
                            num++;
                        } 
                    } 
                    if (num <= 2) {
                        if (keyw == _core) numCore = dsp_CORE_Prog(progAny1,progOnly0);  
                        if (keyw == _section) dsp_SECTION(progAny1,progOnly0);
                        if (keyw == _sectionelse) dsp_SECTION_ELSE(progAny1,progOnly0);
                        if (keyw == _coreaes) dsp_CORE_EXTERN_Prog(progAny1,progOnly0);
                    } else {
                        addCode(progAny1); addCode(progOnly0);
                    }
                    //stop searching parameter when a true condition is seen
                    if ((progAny1 == 0xFFFFFFFF) && (progOnly0 == 0)) break;
                    res = searchDelimiter( &p, "," );
                } while(res);
                calcLength();
                break; }

            case _tile : {  
                numTile = dsp_TILE();
                if (numTile>1) clearParamSection();
                outOfRangeError( numTile, valueMin[_ttile], valueMax[_ttile]);
                break; }

            case _send: //falltrough to _receive
            case _receive: {    //syntax send channel : io,io ...
                //TODO this requires the opcode calclengthprint to be launched!
                double tile=0;
                searchExpressionRangeError( &p, &tile, _ttile);
                getDelimiterError( &p, ',', 30);
                int index = addCode(0); //placeholder for opcode+skip
                opcode_t * ptr = opcodePtr(index);
                unsigned int outcount;
                unsigned int finalIO;
                do {
                    outcount=0;
                    finalIO=0;
                    do {
                        searchExpressionRangeError( &p, &output , _tIO );
                        unsigned int out = output;   //convert to integer
                        finalIO |= (out << (8*outcount));
                        outcount++;
                        res = searchDelimiter( &p, "," );
                    } while ( res && (outcount<4) );
                    addCode(finalIO);                
                } while(res);
                index = opcodeIndex() - index;
                //TODO, update skip value according to total numbers
                ptr->op.opcode = (keyw==_send) ? DSP_SEND : DSP_RECEIVE;
                calcLength();
                //ptr->op.skip   = index;
                break; }

            case _input: {
                searchExpressionRangeError( &p, &input, _tIO  );
                dsp_LOAD( input);
                break; }

            case _outputpdf:
            case _outputvol:
            case _outputvolsat:
            case _output: {
                unsigned int outcount;
                unsigned int finalIO;
                unsigned int out;
                int breaking=0;
                do {
                    outcount=0;
                    finalIO=0;
                    do {
                        if (breaking == 0) {
                            searchExpressionRangeError( &p, &output , _tIO );
                            out = output;   //convert to integer
                        } else {
                            breaking = 0; out = 0; //from last loop
                        }
                        if ((out == 0) && (outcount)) { breaking = 1; break; }
                        finalIO |= (out << (8*outcount));
                        outcount++;
                        res = searchDelimiter( &p, "," );
                    } while ( res && (outcount<4) );
                    if (keyw == _output) dsp_STORE( finalIO );
                    else  if (keyw == _outputpdf) dsp_STORE_TPDF( finalIO );
                    else  if (keyw == _outputvol) dsp_STORE_VOL( finalIO );
                    else dsp_STORE_VOL_SAT( finalIO );
                } while(res || breaking);   //force an additional loop if "breaking" was used
                break; }

            case _transfer: {
                int transferNum=0;
                do {
                    getDelimiterError( &p, '(', 28 );
                    searchExpressionRangeError( &p, &input , _tIO );
                    getDelimiterError( &p, ',', 30 );
                    searchExpressionRangeError( &p, &output, _tIO );
                    getDelimiterError( &p, ')', 29 );
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
                searchExpressionRangeError( &p, &input, _tshift  );
                dsp_SHIFT_FixedInt( input);
                break; }

            case _valuex : {
                double result;
                searchExpressionRangeError( &p, &result, _tvalue32);
                dsp_VALUEX_Fixed(result);
                break; }
            case _valuey : {
                double result;
                searchExpressionRangeError( &p, &result, _tvalue32);
                dsp_VALUEY_Fixed(result);
                break; }

            case _gain : {
                char * oldp = skipSpaces( &p);
                labelptr_t l = searchLabel( &p );
                if ((l) && ((l->s.type == label_drcout) || (l->s.type == label_value))) {
                    dsp_GAIN( l->s.address + ( (l->s.type == label_drcout) ? 1 : 0 ) );
                    usedLabelInTile(l);
                } else {
                    p = oldp;
                    double result;
                    searchExpressionRangeError( &p, &result, _tvalue32);
                    dsp_GAIN_Fixed(result);
                }
                break; }

            case _clip : {
                double result;
                searchExpressionRangeError( &p, &result, _tvalue32);
                dsp_CLIP_Fixed(result);
                break; }
            case _mixer: {
                int mixerNum = 0;
                do {
                    searchExpressionRangeError( &p, &input , _tIO );
                    if(mixerNum == 0) dsp_MIXER();
                    dspMixer_Data(input, 1.0);  // default gain 1.0
                    mixerNum++;
                    res = searchDelimiter( &p, "," );
                } while (res);
                break; }

            case _mixergain:
            case _inputgain:
            case _outputgain: {
                int inputgainNum=0;
                do {
                    getDelimiterError( &p, '(', 28 );
                    searchExpressionRangeError( &p, &input , _tIO );
                    getDelimiterError( &p, ',', 30 );
                    searchExpressionRangeError( &p, &gain, _tvalue32 );
                    getDelimiterError( &p, ')', 19 );
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
            case _saturatevol:{
                dsp_SAT0DB_VOL();
                break; }
            case _saturategain:{
                searchExpressionRangeError( &p, &gain, _tvalue32 );
                dsp_SAT0DB_GAIN_Fixed(gain);
                break; }

            case _delayone: {
                dsp_DELAY_1(); 
                break; }
            case _delaydpus:
            case _delayus: {
                char * oldp = skipSpaces( &p);
                labelptr_t l = searchLabel( &p );
                if (l && (l->s.type == label_valueint)) {
                    usedLabelInTile(l);
                    getDelimiterError( &p, ',', 30 );
                    double max=0;
                    searchExpressionRangeError( &p, &max, _tdelay );
                    if (keyw == _delaydpus)  dsp_DELAY_DP_max( l->s.address, max );
                    else dsp_DELAY_max( l->s.address, max );
                } else {
                    p = oldp;
                    searchExpressionRangeError( &p, &delay, _tdelay );
                    if (keyw == _delaydpus)  dsp_DELAY_DP_FixedMicroSec( delay );
                    else dsp_DELAY_FixedMicroSec( delay );
                }
                break; }
            case _delayusfbmix: {
                double source,feed,delayed,mix;
                searchExpressionRangeError( &p, &delay, _tdelay );
                getDelimiterError( &p, ',', 30 );
                searchExpressionRangeError( &p, &source, _tpercent );
                getDelimiterError( &p, ',', 30 );
                searchExpressionRangeError( &p, &feed, _tpercent );
                getDelimiterError( &p, ',', 30 );
                searchExpressionRangeError( &p, &delayed, _tpercent );
                getDelimiterError( &p, ',', 30 );
                searchExpressionRangeError( &p, &mix, _tpercent );
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
                    res = testExpression( &p, &input );
                    if (res == _empty) {
                        if (l->value > 1.0) fatalErrorNum(2);
                    } else if (res != _valueint) fatalErrorNum(11);
                    outOfRangeError( input, 0.0, l->value-1.0 );
                }
                if (bracket == '[') getDelimiterError( &p, ']', 26);
                if       (keyw == _savexmem) dsp_STORE_X_MEM( l->s.address + input*2.0 );
                else  if (keyw == _saveymem) dsp_STORE_Y_MEM( l->s.address + input*2.0 );
                else  if (keyw == _loadxmem) dsp_LOAD_X_MEM( l->s.address + input*2.0 );
                else  if (keyw == _loadymem) dsp_LOAD_Y_MEM( l->s.address + input*2.0 );
                break; }

            case _dcblock: {
                searchExpressionRangeError( &p, &freq, _tfreq );
                dsp_DCBLOCK( freq );
                break; }

            case _biquad8:
            case _biquad: {
                labelptr_t l = searchLabel( &p );
                if (l == NULL) { lastLabelName = ""; fatalErrorNum(3); }
                if ( (l->s.type != label_filter) 
                  && (l->s.type != label_filters) 
                  && (l->s.type != label_filter8) ) fatalErrorNum(3);
                usedLabelInTile(l);
                dspOutLabelName = l->s.name;
                dspprintf3("biquad filter %s, %d\n",dspOutLabelName,l->s.address);
                if ((l->s.type == label_filter) && (dspModeDynamic==0)) dsp_BIQUADS( l->s.address );
                else dsp_BIQUADS_FS( l->s.address );
                break; }

            case _warpconvol: //falthrough
            case _convol: {
                int numFilt = 0;
                int base = 0;
                int max = 0;
                double lambda = 0.0;
                do {
                    if (keyw == _warpconvol) {
                        getDelimiterError( &p, '(',28);
                        searchExpressionRangeError( &p, &lambda, _tq31 );
                        getDelimiterError( &p, ',',30);
                        if (numFilt == 0) base = dsp_WARPCONVOL();
                    } else 
                        if (numFilt == 0) base = dsp_CONVOL();
                    labelptr_t l = searchLabel( &p );
                    if (l == NULL) fatalErrorNum(33);
                    if (l->s.type != label_taps) fatalErrorNum(33);
                    usedLabelInTile(l);
                    numFilt++;  //TODO upper bundaries for the max number of frequencies allowed
                    int num = l->numValues;
                    if (num > max) max = num;
                    addCodeOffset(l->s.address,base); // generates taps adresses
                    addCode(num);   //generate number of taps for this impulse
                    if (keyw == _warpconvol) {
                        addDoubleCodeQ31(lambda);
                        getDelimiterError( &p, ')',20);
                    }
                    res = searchDelimiter( &p, ",");
                } while(res);
                int n=dspMaxSamplingFreq-dspMinSamplingFreq+1;
                for (int i=numFilt; i<n; i++) {
                    addCodeOffset(base & 1,base);   //alligned 8 !
                    addCode(1); // 1 tap by default
                    if (keyw == _warpconvol) addCode(0); //lambda = 0 by default
                }
                //TODO addDataSpaceAligned8 is also generating an opcode at the end of the table!
                if (keyw == _warpconvol) max++; //always add one extra sample in buffer when warped fir requested
                dspprintf3("%d impulses, max %d taps\n",numFilt,max);
                opcodePtr(base+1)->i32 = addDataSpaceAligned8(max);
                calcLength();
                break; }

            case _integrator: {
                dsp_INTEGRATOR(); 
                break; }
            case _cicus: {
                searchExpressionRangeError( &p, &delay, _tdelay );
                dsp_CIC_FixedMicroSec( delay );
                break; }
            case _cicn: {
                searchExpressionRangeError( &p, &delay, _tdelay );
                dsp_CIC_N( delay );
                break; }
            case _expma : {
                double value = 0;
                searchExpressionRangeError( &p, &value, _tpercent );
                dsp_EXPMA(value);
                break; }

            case _thdcomp : {
                double c2=0.0,c3=0.0;
                searchExpressionRangeError( &p, &c2, _tpercent );
                getDelimiterError( &p, ',',30);
                searchExpressionRangeError( &p, &c3, _tpercent );
                dsp_THDCOMP(c2,c3);
                break; }

            case _tpdf: {
                searchExpressionRangeError( &p, &tpdf, _ttpdf );
                dsp_TPDF(tpdf);
                break; }
            case _white: {
                dsp_WHITE();
                break; }

            case _sine:
            case _square:
            case _dirac: {
                getDelimiterError( &p, '(', 28 );
                searchExpressionRangeError( &p, &freq, _tfreq );
                getDelimiterError( &p, ',', 30);
                searchExpressionRangeError( &p, &gain, _tvalue32 );
                getDelimiterError( &p, ')', 29 );
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
                labelptr_t drcin = searchLabel( &p );
                if ((drcin == NULL) || (drcin->s.type != label_drcin)) fatalErrorNum(36);
                usedLabelInTile(drcin);
                if ((keyw == _limiterpeak)||(keyw == _limiterrms)||(keyw == _limiterpeakhard)||(keyw == _noisegate))
                    if (drcin->numValues < 4) fatalErrorNum(38);
                if (keyw == _compressor)
                    if (drcin->numValues < 5) fatalErrorNum(39);
                if (keyw == _expander)
                    if (drcin->numValues < 6) fatalErrorNum(39);

                getDelimiterError( &p, ',', 30 );
                labelptr_t drcout = searchLabel( &p );
                if ((drcout == NULL) || (drcout->s.type != label_drcout)) fatalErrorNum(37);
                usedLabelInTile(drcout);
                break; }

            case _memclr:   //fallthrough voluntary
            case _swapmem:
            case _memadd:
            case _memsub:
            case _mulmem:
            case _divmem:
            case _avgmem:
            case _memavg:
            case _memneg:
            case _memsave:
            case _loadmem: {
                int op = (keyw - _memclr) + DSP_MEMCLR;
                int addr = getLabelMemory( &p );
                dsp_FUNC_MEM(op, addr);
                break; }
            //TODO interpret parameters and generate opcodes
            case _addmem:
            case _submem:
            case _mixermem: {
                int ofs = 0;
                do {
                    int addr = getLabelMemory( &p );
                    if (ofs == 0) ofs = dsp_FUNC_MEM((keyw - _memclr) + DSP_MEMCLR, addr);
                    else addCodeOffset(addr,ofs);
                    res = searchDelimiter( &p, ",");
                } while(res);
                calcLength();
                break;
            }
            case _memvalue: 
            case _memgain: { 
                int addr = getLabelMemory(&p); 
                getDelimiterError(&p, ',', 30);
                double result;
                searchExpressionRangeError( &p, &result, _tvalue32);
                if (keyw == _memvalue) dsp_FUNC_MEM(DSP_MEMVALUE,addr);
                if (keyw == _memgain)  dsp_FUNC_MEM(DSP_MEMGAIN,addr);
                addGainCodeQNM(result);
                break; }
            case _meminput: { 
                int addr = getLabelMemory( &p ); 
                getDelimiterError( &p, ',', 30);
                searchExpressionRangeError( &p, &input, _tIO  );
                dsp_FUNC_MEM(DSP_MEMINPUT,addr);
                addCode(input);
                calcLength();
                break; }

            case _instructions: {
                int freq = dspConvertFrequencyFromIndex(dspMinSamplingFreq);
                int maxinst = 128000000/freq;
                res = testExpression( &p, &input);
                if (res == _empty) input = 0.0;
                else outOfRangeError(input,8,maxinst);
                dsp_FULL_LOAD(input);
                break; }

            case _priorityon :  { dsp_singleOpcode(DSP_PRIO_ON); break; }
            case _priorityoff : { dsp_singleOpcode(DSP_PRIO_OFF); break; }

//end of dsp keywords
            case -1: { //this is not a keyword so it must be a label then
                if (searchKeywords( &p, paramKeywords, paramKeywordsNumber )>=0) fatalErrorNum(49);
                if (searchKeywords( &p, filterNames, filterTypesNumber )>=0) fatalErrorNum(49);
                labelptr_t l = searchLabel( &p );
                if (l) {
                    //label found, make sure user wants to overload label definition (?)
                    //only for numerical labels
                    if (l->s.type>_valueint) fatalErrorNum(12);
                    getDelimiterError( &p, '?', 12 );
                    //yes overloading accepted
                } else {
                    //label unknown. add it to the list
                    l = appendNewLabel( p );
                    p += l->s.length;
                    //eventually accept ? for numerical lables
                    if (l->s.type <= _valueint) searchDelimiter( &p, "?" );   
                }
                int filterNum=0, filterType=0, filterkeyw = 0;
        filter_entry:
                filterType = searchKeywords( &p, filterNames, filterTypesNumber );
                if ( filterType >= 0 ) checkAndCreateParam();
        filter_retry:
                if (filterkeyw==0) { }
                if ( filterType >= 0 ) {
                    if ( filterNum == 0 ) {
                        //this new label is followed by a filter name for the first time
                        if ((l->s.type != _empty) 
                         && (l->s.type != label_filter)
                         && (l->s.type != label_filters)
                         && (l->s.type != label_filter8)) fatalErrorNum(12);
                        l->s.type = label_filter;
                        dspOutLabelName = l->s.name;
                        l->s.address = dspBiquad_Sections_Flexible();
                    }
                    filterNum++;
                    getDelimiterError( &p, '(', 19 );
                    //read filter cutoff frequency
                    res = testExpression( &p, &freq );
                    if (res == _empty) fatalErrorNum(14);
                    if (res == _valuedb) fatalErrorNum(25);
                    outOfRangeError( freq, valueMin[_tfreq], valueMax[_tfreq]);
                    //read filter Q only for generic second order filters
                    gain = 1.0;  //default value
                    filterQ = 1.0; //default value
                    if (filterType >= 45) { //as of BPQ0DB
                        getDelimiterError( &p, ',', 21 );
                        res = testExpression( &p, &filterQ );
                        if (res ==  _empty) fatalErrorNum(15);
                        outOfRangeError( filterQ, valueMin[_tfilterQ], valueMax[_tfilterQ] );
                        if (filterType == 55) { //FLT
                            getDelimiterError( &p, ',', 21 );
                            //read frequency fp & qp
                            res = testExpression( &p, &freqLT );
                            if (res == _empty) fatalErrorNum(14);
                            if (res == _valuedb) fatalErrorNum(25);
                            outOfRangeError( freqLT, valueMin[_tfreq], valueMax[_tfreq]);
                            //read filter Q
                            getDelimiterError( &p, ',', 21 );
                            res = testExpression( &p, &filterQLT );
                            if (res ==  _empty) fatalErrorNum(15);
                            outOfRangeError( filterQLT, valueMin[_tfilterQ], valueMax[_tfilterQ] );
                        }
                    }
                    //read filter gain
                    res = searchDelimiter( &p, "," );
                    if (res>0)
                        searchExpressionRangeError( &p, &gain, _tvalue32 );
                    getDelimiterError( &p, ')', 20 );
                    if (filterType == 55) {
                        //add filter characteristics in the dsp code (param section)
                        dspprintf2("filter %s LT with F0=%f, Q0=%f, Fp=%f, Qp=%f, G=%f\n",l->s.name,freq,filterQ,freqLT,filterQLT,gain);
                        dsp_FilterLT( freq, filterQ, freqLT, filterQLT, gain );
                        dspoutFilters5(filterTypes[filterType],filterOrders[filterType],freq,filterQ,freqLT,filterQLT,gain,filterNames[filterType]);
                    } else
                    if (filterType == 54) {
                        dspprintf2("filter %s HILBERT with xx=%f, xx=%f, G=%f\n",l->s.name,freq,filterQ,gain);
                        //TODO dsp_hilbert
                    } else {
                        //add filter characteristics in the dsp code (param section)
                        dspprintf2("filter %s type %s created with F=%f, Q=%f, G=%f\n",l->s.name,filterNames[filterType],freq,filterQ,gain);
                        dsp_filter( filterTypes[filterType], freq, filterQ, gain );
                        dspoutFilters3(filterTypes[filterType],filterOrders[filterType],freq,filterQ,gain,filterNames[filterType]);
                    }
                    //accept potentially other filters on the same line, separated with comma
                    filterType = searchKeywords( &p, filterNames, filterTypesNumber );
                    if (filterType >= 0) goto filter_retry;
                    break; //leave case -1 and pursue with ; or EOL
                } 
    //not a filter
                res = searchKeywords( &p, paramKeywords, paramKeywordsNumber );
                switch (res) {
                case _MEMORY : {
                    // MEMORY keyword recognized
                    if (l->s.type != _empty) fatalErrorNum(12);
                    l->s.type = label_memory;
                    //accept a potential MEMORY size parameter as an integer
                    double memSize = 1.0;
                    searchExpressionRangeError( &p, &memSize, _tmem );
                    l->value = memSize;
                    int memSizeInt = memSize;
                    //allocate space in the dsp code
                    checkAndCreateParam();
                    l->s.address = dspMem_LocationMultiple(memSizeInt);
                    break; }
                case _TAPS: { //TAPS
                    checkAndCreateParam();
                    if (l->s.type != _empty) fatalErrorNum(12);
                    l->s.type = label_taps;
                    l->s.address = dspMem_LocationMultiple(0);
                    if (testDelimiter( &p, "\"")) {
                        tapsinclude = 1;    //to come back here
                        goto gotoinclude;   //will consider this string as a new file name
                    } 
                labeltaps:
                    tapsinclude = 0;
                    int numTaps = 0;
                    do {
                        if (testDelimiter( &p, "#\r\n\01" )) {
                            //special case : autorise taps across lines without needing "\"
                            if (fgetLine() == 0) fatalErrorNum(1);
                            p = line; errPtr = line;
                            continue;
                        }
                        double tap = 0.0;
                        searchExpressionRangeError( &p, &tap, _tq31 );
                        l->value = 0;
                        addDoubleCodeQ31(tap);
                        numTaps++;
                        res = searchDelimiter( &p, "," );
                    } while (res);
                    if (numTaps & 1) addCode(0);//always round up to even number
                    //dspprintf2("*** %d TAPS ***\n",numTaps);
                    l->numValues = numTaps;
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
                        double value = 0.0;
                        searchExpressionRangeError( &p, &value, (res == _VALUE) ? _tvalue32 : _tint32 );
                        l->values[numValues] = value;
                        if (numValues == 0) l->value = value;
                        if (res == _VALUE) addGainCodeQNM(value); else addCode(value);
                        numValues++;
                        res2 = searchDelimiter( &p, "," );
                        if (res2 && ((numValues >= label_max_values) ) ) fatalErrorNum(35);
                    } while (res2);
                    l->numValues = numValues;
                    break; }
                case _DRCIN : {
                    checkAndCreateParam();
                    int numValues = 0;
                    if (l->s.type != _empty) fatalErrorNum(12);
                    l->s.type = label_drcin;
                    l->s.address = opcodeIndex();
                    double value = 0.0;
                    searchExpressionRangeError( &p, &value, _tdrc_attack );
                    l->value = value;       //attack
                    l->values[0] = value;   //attack
                    addGainCodeQNM(value);
                    numValues++;
                    getDelimiterError( &p, ',', 30 );
                    searchExpressionRangeError( &p, &value, _tdrc_attack );
                    l->values[1] = value;   //release
                    addGainCodeQNM(value);
                    addCode(0); //alpha computed upon fs change
                    addCode(0); //alpha computed upon fs change
                    numValues++;
                    res = searchDelimiter( &p, "," );
                    if (res) {
                        searchExpressionRangeError( &p, &value, _tvalue32 );
                        l->values[2] = value;   //threshold
                        addGainCodeQNM(value);
                        numValues++;
                        getDelimiterError( &p, ',', 30 );
                        searchExpressionRangeError( &p, &value, _tvalue32 );
                        l->values[3] = value;   //release
                        addGainCodeQNM(value);
                        numValues++;
                        res = searchDelimiter( &p, "," );
                        if (res) {
                            searchExpressionRangeError( &p, &value, _tvalue32 );
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
                    if (keyw == _FILTER) { 
                        l->s.type = label_filters;
                        dspprintf3("_FILTER");
                    }
                    else l->s.type = label_filter8;
                    checkAndCreateParam();
                    l->s.address = opcodeIndex();
                    filterkeyw = keyw;
                    goto filter_entry;
                    break; }

                case -1 :  { // then should be an numerical expression
                    
                    res = searchDelimiter( &p, "=" );
                    enum label_type_e old = l->s.type;
                    double temp;
                    if (res) {
                        //full expression authorized after an "="
                        res = searchExpression( &p, &temp );
                        old = _empty;  //clear previous definition if any
                    } else {
                        //only direct value after a label
                        res = searchNumerical( &p, &temp, acceptDB );  // only numerical, eventually db
                        if (res == _empty)  {
                            lastLabelName = l->s.name;
                            fatalErrorNum(7); }
                        if (testDelimiter( &p, "+-*/")) fatalErrorNum(31);
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
                    } //else dspprintf2("label type %d %s already set with value %f\n",l->s.type,l->s.name, l->value);
                    break; } // default
                } //switch (keyword)
                break; }
            default: {
                    fatalErrorNum(6);
                break; }
            } //end of switch keyw
            //dspprintf1("looking next instruction\n");
            //an instruction has been processed now go for next
            if (lineNum[0] > 0) {
                getSeparatorEOLError( &p );
            }
        } // while(p[0])
    } //while (1) = read next line
finished:

    fclose(*dspInput);
nextfile:
    dspprintf1("\n");
    } // while (nextname) = read next file if any

    size = dsp_END_OF_CODE();

    createSymbolTable();
    freeAllLabels();
    //report cores mips
    return size;
}

static void fatalErrorNum(int num){
    errNum = num;
    fatalError();
}
static void fatalErrorNumIf(int num, int cond) {
    if (cond) fatalErrorNum(num);
}


void fatalError(){
    dspPrintPending();
    if (errNum > 0) errNum = - errNum;
    switch (errNum) {
    case -1:  fprintf(stderr,"Error: numerical value expected\n"); break;
    case -2:  fprintf(stderr,"Error: numerical value or label expected\n"); break;
    case -3:  fprintf(stderr,"Error: filter label expected (%s)\n",lastLabelName); break;
    case -4:  fprintf(stderr,"Error: filter label expected, this one has a numerical value (%s)\n",lastLabelName); break;
    case -5:  fprintf(stderr,"Error: value out of range (%f ... %f)\n",errMin, errMax); break;
    case -6:  fprintf(stderr,"Error: dsp keyword or user label starting with a..z letter exptected, found char \"%c\" <0x%X> : \n",(*errPtr>=' ')?*errPtr:' ',*errPtr); break;
    case -7:  fprintf(stderr,"Error: filter or numerical value exptected after a label definition (%s)\n",lastLabelName); break;
    case -8:  fprintf(stderr,"Error: this dsp instruction requires saturation before execution\n"); break;
    case -9:  fprintf(stderr,"Error: dsp accumulator already saturated by previous instruction\n"); break;
    case -10: fprintf(stderr,"Error: special label defined with MEMORY is expected (%s)\n",lastLabelName); break;
    case -11: fprintf(stderr,"Error: integer value expected\n"); break;
    case -12: fprintf(stderr,"Error: label already exists (%s)\n",lastLabelName); break;
    case -13: fprintf(stderr,"Error: end of line or \";\" separator expected, found character \"%c\" <0x%x>\n",(*errPtr>=' ')?*errPtr:' ',*errPtr); break;
    case -14: fprintf(stderr,"Error: filter frequency (or label) expected\n"); break;
    case -15: fprintf(stderr,"Error: filter Q (or label) expected\n"); break;
    case -16: fprintf(stderr,"Error: filter gain (or label) expected\n"); break;
    case -17: fprintf(stderr,"Error: label cannot be created outside of a \"param\" section\n"); break;
    case -18: fprintf(stderr,"Error: missing numerical value to finish expression \n"); break;
    case -19: fprintf(stderr,"Error: after filter type, a bracket \"(\" is expected\n"); break;
    case -20: fprintf(stderr,"Error: missing closing bracket, \")\" is expected\n"); break;
    case -21: fprintf(stderr,"Error: filter parameters separated with a coma \",\" is expected\n"); break;
    case -22: fprintf(stderr,"Error: special label defined with MEMORY is expected\n"); break;
    case -23: fprintf(stderr,"Error: a gain value is expected\n"); break;
    case -24: fprintf(stderr,"Error: problem while evaluating expression\n"); break;
    case -25: fprintf(stderr,"Error: problem with decibel while evaluating expression\n"); break;
    case -26: fprintf(stderr,"Error: missing closing bracket : \"]\" is expected\n"); break;
    case -27: fprintf(stderr,"Error: a label with numerical value is expected (%s)\n",lastLabelName); break;
    case -28: fprintf(stderr,"Error: opening bracket \"(\" is expected\n"); break;
    case -29: fprintf(stderr,"Error: closing bracket \")\" is expected\n"); break;
    case -30: fprintf(stderr,"Error: comma \",\" is expected\n"); break;
    case -31: fprintf(stderr,"Error: expression with operator requires equal \"=\" operator\n"); break;
    case -32: fprintf(stderr,"Error: end of line expected, found character \"%c\" <0x%x>\n",(*errPtr>=' ')?*errPtr:' ',*errPtr); break;
    case -33: fprintf(stderr,"Error: TAPS label expected\n"); break;
    case -34: fprintf(stderr,"Error: string expected\n"); break;
    case -35: fprintf(stderr,"Error: too much values\n"); break;
    case -36: fprintf(stderr,"Error: DRCIN label expected\n"); break;
    case -37: fprintf(stderr,"Error: DRCOUT label expected\n"); break;
    case -38: fprintf(stderr,"Error: DRCIN requires 4 parameter\n"); break;
    case -39: fprintf(stderr,"Error: DRCIN requires 5 parameter\n"); break;
    case -40: fprintf(stderr,"Error: VALUEINT label expected\n"); break;
    case -41: fprintf(stderr,"Error: MEMORY label expected\n"); break;
    case -42: fprintf(stderr,"Error: string expected with quote \" delimiters\n"); break;
    case -43: fprintf(stderr,"Error: too much included files, max %d\n",maxIncludedFiles); break;
    case -44: fprintf(stderr,"Error: frequency value not valid\n"); break;
    case -45: fprintf(stderr,"Error: cannot change this dsp parameter after code generation\n"); break;
    case -46: fprintf(stderr,"Error: mantissa value not valid\n"); break;
    case -47: fprintf(stderr,"Error: IO max must be multiple of 8\n"); break;
    case -48: fprintf(stderr,"Error: too much \"(\"\n"); break;
    case -49: fprintf(stderr,"Error: cannot be used without a label name upfront\n"); break;
    case -50: fprintf(stderr,"Error: CLOCK must be multiple of 4\n"); break;
    case -51: fprintf(stderr,"Error: opening bracket \"(\" or end-of-line expected\n"); break;

    default: break;
    }
    fprintf(stderr,"l%d: %s",lineNum[0]-1,line);
    fprintf(stderr,"l%d: ",lineNum[0]-1);
    char *q = line;
    while (isSpaceOrTab(*errPtr)) errPtr++;
    while (q != errPtr) { fprintf(stderr,"%c",(((*q)==9) ? *q : ' ')); q++; }
    fprintf(stderr,"^\n");
    freeAllLabels();
    fclose(*dspInput);
    exit(errNum);
}
