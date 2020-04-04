
// some optimized function dealing with float and double encoded IEEE754

#ifndef DSP_IEEE754_OPTIMISE
#define DSP_IEEE754_OPTIMISE 63  // default value enabling all, can be overloaded in the makefile or compiler parameters
// +1  : int dsps31Float0DB(float f)                            OK
// +2  : void dspTruncateFloat0DB(float * f, const int bit)     OK  // not good for negative numbers
// +4  : void dspSaturateFloat0db(float *f)                     OK
// +8  : float dspIntToFloatScaled(int X, const int shift)      OK
// +16 : void dspShiftFloat(float *f, int shift)                OK
// +32 : double dspMulFloatFloat(float a, float b)              OK
#endif


// static check that the embedded foating point library is compatible with our optimized and simplified functions.
// compiler will compute this at compile time as part of the optimization process
static inline int dspFloatIsIEE754(){
    dspALU32_t V = { .f = 0x812345 };
    return V.i == 0x4B012345;
}

static inline int dspDoubleIsIEE754(){
    dspALU64_t V = { .f = 0x123456 };
    return V.i == 0x4132345600000000;
}



#if 0// helpers during tests
void printFloatIEEE754Structure(float f){
    dspALU32_t X = { .f = f };
    printf("X = %f = %X, ",X.f,X.i);
    int exp  = (X.i >> 23) & 255; printf("exp=%d(%d) ",exp, exp-127);
    int mant = (X.i & ((1<<23)-1)) | (1<<23); printf("mant=%X = ",mant);
    for (int i=0; i<23;i++) {
        if (mant & 0x800000) printf("1"); else printf("0");
        mant <<=1; }
    printf("\n");
}
void printDoubleIEEE754Structure(double f){
    dspALU64_t X = { .f = f };
    printf("X = %lf = %llX, ",X.f,X.i);
    int exp  = (X.i >> 52) & 2047; printf("exp=%d(%d) ",exp,exp-1023);
    long long mant = (X.i & ((1ULL<<52)-1)) | (1ULL<<52); printf("mant=%llX = ",mant);
    for (int i=0; i<23;i++) {
        if (mant & 0x0008000000000000) printf("1"); else printf("0");
        mant <<=1; }
    printf("..\n");
}
#endif


// convert a float from the range -1.0..+1.0 to and signed 32 bits integer (s31), with saturation
static inline int dsps31Float0DB(float f){
    // sign + 8bits exponent (bias 127) + 23 bits mantissa | (implicit 1<<23)
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 1)
    if (dspFloatIsIEE754()){
        dspALU32_t V = { .f = f };
        int exp  = (V.i >> 23) & 255;
        if (exp == 0) return 0;
        unsigned int mant = (V.i & ((1<<23)-1)) | (1<<23);
        mant <<= 8;     // preventive shift left to normalize mantissa on bit 31 so that we only need some shift right
        int n = 127-exp;
        if (n > 0) mant >>= n;
        else mant = 0x7FFFFFFF; // saturation
        if (V.i <0) mant = -mant;
        return mant;
    } else
#endif
    {   f *= 1ULL<< 31;
        long long mant = f;
        long long max = (1ULL<<31)-1;
        if (mant > max) mant = max;
        else
            if (mant < (-max)) mant = -max;
        return mant; }
}

static inline int dsps31Double0DB(double d){
    // sign + 11bits exponent (bias 1023) + 52 bits mantissa | (implicit 1<<52)
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 1)
    if (dspDoubleIsIEE754()){
        dspALU64_t V = { .f = d };
        int exp  = (V.i >> 52) & 2047;
        if (exp == 0) return 0;
        long long mant = (V.i & ((1ULL<<52)-1)) | (1ULL<<52);
        int n = 1044-exp;
        if (n > 21) mant >>= n;
        else mant = 0x7FFFFFFF;
        if (V.i < 0) mant = -mant;
        return mant;
    } else
#endif
    {   d *= 1ULL << 31;
        long long mant = d;
        long long max = (1ULL<<31)-1;
        if (mant > max) mant = max;
        else
            if (mant < (-max)) mant = -max;
        return mant; }
}

// considering a float in the range -1.0..+1.0
// reduce the bit precision, performing a bitwise AND function
// with a mask made of "bit" 1s. used for dithering before storing sample
static inline void dspTruncateFloat0DB(float * f, const int bit){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 2)
    if ((dspFloatIsIEE754())) {
        dspALU32_t *p = (dspALU32_t *)f;
        int exp = (p->i >> 23) & 255;
        if (exp == 0) { *f = 0.0; return; }
        int n = 151-bit-exp;
        if (n > 0) {
            if (n >= 24)
                if (p->i >=0) p->i = 0;
                else p->i = (256+128-bit)<<23; //res = negative number close to 0 : 1.0/2^dither
            else  {
                int mask = -1 << n;
                if (p->i <0) {
                    int add = ~mask; p->i += add; }
                p->i &= mask; }
        }
    } else
#endif
    {   // simple but costly method, by converting to a scaled integer
        *f *= (1ULL << (bit-1+32) );
        long long intpart = *f;
        intpart &= 0xFFFFFFFF00000000;
        *f = intpart;
        *f /= (1ULL << (bit-1+32) ); }
}


static inline void dspTruncateDouble0DB(double * d, const int bit){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 2)
    if ((dspDoubleIsIEE754())) {
        dspALU64_t *p = (dspALU64_t *)d;
        int exp = (p->i >> 52) & 2047;
        if (exp == 0) { *d = 0.0; return; }
        int n = 1076-bit-exp;
        if (n > 0) {
            if (n >= 53)
                if (p->i >=0) p->i = 0;
                else {
                    int exp = (2048+1024-bit)<<20;
                    p->i = (long long)exp <<32; //res = negative number close to 0 : 1.0/2^dither
                }
            else  {
                long long mask = -1LL << n;
                if (p->i < 0) {
                    long long add = ~mask; p->i += add; }
                p->i &= mask; }
        }
    } else
#endif
    {   *d *= (1ULL << (bit-1+32) );
        long long intpart = *d;
        intpart &= 0xFFFFFFFF00000000;
        *d = intpart;
        *d /= (1ULL<< (bit-1+32) ); }
}


static inline void dspSaturateFloat0db(float *f){
    // sign + 8bits exponent (bias 127) + 23 bits mantissa | (implicit 1<<23)
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 4)
    if (dspFloatIsIEE754()){
        dspALU32_t *p = (dspALU32_t *)f;
        int exp  = (p->i >> 23); // keep sign, arithmetic shift right
        //printf("exp=%d\n",exp);
        if (exp >= 127) p->f = 1.0;
        else
            if ((exp<0) && (exp >=-129)) p->f =-1.0;
    } else
#endif
    {   if ((*f) >=  1.0)  *f =  1.0; else
        if ((*f) <= -1.0)  *f = -1.0; }
}


static inline void dspSaturateDouble0db(double *d){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 4)
    if (dspFloatIsIEE754()){
        dspALU64_t *p = (dspALU64_t*)d;
        int exp = (p->i >> 52); // keep sign, arithmetic shift right
        if (exp >= 1023) p->f = 1.0;
        else
            if ((exp<0) && (exp >= -1025)) p->f =-1.0;
    } else
#endif
    {   if ((*d) >=  1.0)  *d =  1.0; else
        if ((*d) <= -1.0)  *d = -1.0; }
}


// fast conversion from an integer (typically an audio sample)
// to a float IEEE format number, with scale reduction by 2^shift
static inline float dspIntToFloatScaled(int X, const int shift){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 8)
    if ((dspDoubleIsIEE754())) {
        if (X == 0) return 0.0;
        int exp = 0;
        if (X<0) { X = -X; exp = 256; } // keep sign information in exponent
        unsigned int acc = X;
    #ifdef DSP_XS1
        int count;
        asm("clz %0,%1":"=r"(count):"r"(acc));   // count number of leading 0
        if (count>=8) acc <<= (count-8);
        else acc >>= (8-count);
        exp += 127+31-count;
    #else
        // normalize to 24 bits mantissa, acc is 31 usefull bits only due to sign removal
        unsigned int mask = 0xFF000000; // potentially shift 7 times right from 31 to 24bits
        if (acc & mask) {  acc>>=1; exp++;
        if (acc & mask) {  acc>>=1; exp++;
        if (acc & mask) {  acc>>=1; exp++;
        if (acc & mask) {  acc>>=1; exp++;
        if (acc & mask) {  acc>>=1; exp++;
        if (acc & mask) {  acc>>=1; exp++;
        if (acc & mask) {  acc>>=1; exp++; } } } } } } }
        else {
            if ((acc & 0xFFFFFF00)==0) {  acc <<= 16; exp -= 16; } else
            if ((acc & 0xFFFF0000)==0) {  acc <<= 8;  exp -= 8; }
            mask = 0xFF800000;
            if ((acc & mask) == 0) { acc <<=1; exp--;
            if ((acc & mask) == 0) { acc <<=1; exp--;
            if ((acc & mask) == 0) { acc <<=1; exp--;
            if ((acc & mask) == 0) { acc <<=1; exp--;
            if ((acc & mask) == 0) { acc <<=1; exp--;
            if ((acc & mask) == 0) { acc <<=1; exp--;
            if ((acc & mask) == 0) { acc <<=1; exp--; } } } } } } }
        }
        exp += 127+23;
    #endif
        exp -= shift;
        acc &= 0x007FFFFF;  // removing highest bit (always 1)
        acc |= (exp<<23);
        dspALU32_t tmp;
        tmp.i = acc;
        return tmp.f;
    } else
#endif
        return (float)X / (float)(1ULL<<shift);
}


static inline double dspIntToDoubleScaled(int X, const int shift){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 8)
    if ((dspDoubleIsIEE754())) {
        if (X == 0) return 0.0;
        unsigned long long acc;
        int exp = 0;
        if (X<0) { X = -X; exp = 2048; }
    #ifdef DSP_XS1
        int count;
        asm("clz %0,%1":"=r"(count):"r"(X));   // count number of leading 0
        acc = (unsigned)X;
        count += 21;
        acc <<= count;
        exp += 1023+52-count;
    #else
        // normalize up to bit 31, using 32bits format to speed up
        if ((X & 0xFFFFFF00)==0) {  X <<= 24; exp -= 24; } else
        if ((X & 0xFFFF0000)==0) {  X <<= 16; exp -= 16; } else
        if ((X & 0xFF000000)==0) {  X <<= 8;  exp -= 8; }
        unsigned int mask = 0x80000000;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--;
        if ((X & mask) == 0) { X <<=1; exp--; } } } } } } } }
        acc = (unsigned)X;
        // normalize up to bit 53 as per IEEE format for mantissa
        acc <<= 21;
        exp += 1054;
    #endif
        exp -= shift;
        acc &= 0x000FFFFFFFFFFFFF;  // removing highest bit 53 (as it is always 1)
        acc |= ((long long)exp<<52);
        dspALU64_t tmp;
        tmp.i = acc;
        return tmp.f;
    } else
#endif
        return (double)X / (float)(1ULL<<shift);
}

static inline void dspShiftFloat(float *f, int shift){
    // sign + 8bits exponent (bias 127) + 23 bits mantissa | (implicit 1<<23)
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 16)
    if (dspFloatIsIEE754()){
        dspALU32_t *p = (dspALU32_t *)f;
        shift <<= 23;   // scale to the exponent position in the 32bits framework
        p->i += shift;  // no any check for overload/underload... TODO
    } else
#endif
    {   float mul;
        if (shift>=0) {
            mul = (1ULL<< shift);
            *f *= mul;
        } else {
            mul = (1ULL<< -shift);
            *f /= mul; }
    }
}

static inline void dspShiftDouble(double *f, int shift){
    // sign + 8bits exponent (bias 127) + 23 bits mantissa | (implicit 1<<23)
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 16)
    if (dspFloatIsIEE754()){
        dspALU64_t *p = (dspALU64_t *)f;
        long long shift64 = shift;
        shift64 <<= 52;   // scale to the exponent position in the 64bits framework
        p->i += shift64;  // no any check for overload... TODO
    } else
#endif
    {   float mul;
        if (shift>=0) {
            mul = (1ULL<< shift);
            *f *= mul;
        } else {
            mul = (1ULL<< -shift);
            *f /= mul; }
    }
}

static inline float dspMulFloatFloat(float a, float b){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 32)
    if (dspFloatIsIEE754()){
        dspALU32_t A = { .f=a };
        dspALU32_t B = { .f=b };
        dspALU32_t R;
        int ea = (A.i >> 23) & 255;
        if (ea == 0) return 0.0;
        int eb = (B.i >> 23) & 255;
        if (eb == 0) return 0.0;
        int exp = ea+eb-127;
        if (exp<1) return 0.0;  // monitor smallest number
        if ((A.i ^ B.i) & 0x80000000) exp |= (1<<8);   // activate sign bit
        unsigned int ma = (A.i & 0x7FFFFF) | (1<<23);
        unsigned int mb = (B.i & 0x7FFFFF) | (1<<23);
        ma <<= 5; mb <<= 5;
        // 29x29 bits = 58 bits or 57
        //- 32bits = 26 or 25, need 24, will shilt right 1 or 2 depending on bit 25
        unsigned reshi;
    #ifdef DSP_XS1
        int z = 0;
        asm("lmul %0,%1,%2,%3,%4,%4":"=r"(reshi),"=r"(z):"r"(ma),"r"(mb),"r"(z));
    #else
        unsigned long long res;
        res = (unsigned long long)ma * mb;
        res >>= 32; // keep msb only
        reshi = res;
    #endif
        if (reshi & (1<<25)) {
               exp++;
               reshi >>= 2;
        } else reshi >>= 1;
        reshi &= (1ULL<<23)-1;  // keep only 23 usefull bits
        reshi |= exp << 23;     // put exponent (and sign) in place
        R.i = reshi;
        return R.f;
    } else
#endif
    {  return a * b; }
}

static inline double dspMulFloatDouble(float a, float b){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 32)
    if (dspFloatIsIEE754()){
        dspALU32_t A = { .f=a };
        dspALU32_t B = { .f=b };
        dspALU64_t R;
        int ea = (A.i >> 23) & 255;    // remove sign
        if (ea == 0) return 0.0;
        int eb = (B.i >> 23) & 255;
        if (eb == 0) return 0.0;
        unsigned int ma = (A.i & 0x7FFFFF) | (1<<23);
        unsigned int mb = (B.i & 0x7FFFFF) | (1<<23);
        unsigned long long res;
        int exp = 1023+ea+eb-127-127;
        if (exp<1) return 0.0;
        if ((A.i ^ B.i) & 0x80000000) exp |= (1<<11);   // activate sign bit
    #ifdef DSP_XS1
        int z = 0; int ah; unsigned al;
        asm("lmul %0,%1,%2,%3,%4,%4":"=r"(ah),"=r"(al):"r"(ma),"r"(mb),"r"(z));
        res = ((unsigned long long)ah << 32) | al;
    #else
        res = (unsigned long long)ma * mb;
    #endif
        if (res & 0x800000000000) { exp++; res <<= 5; }  else res <<= 6;
        res &= (1ULL<<52)-1; // kep only 52bits
        res |= ((long long)exp << 52);
        R.i = res;
        return R.f;
    } else
#endif
    {   double res = a;
        res *= b;
        return res; }
}

static inline void dspMaccFloatFloat(dspALU_t *acc, dspALU_SP_t a, dspALU_SP_t b){
#if defined(DSP_IEEE754_OPTIMISE) && (DSP_IEEE754_OPTIMISE & 32)
    if (sizeof(dspALU_t)==sizeof(dspALU_SP_t)){
        if (dspFloatIsIEE754()){
            *acc += dspMulFloatFloat(a, b);
            return; }
    } else
        if (dspDoubleIsIEE754()) {    // target is double
            *acc += dspMulFloatDouble(a, b);
            return; }
#endif
        *acc += (dspALU_t)a * b;
}

