
#include <stdio.h>      // import printf functions

#define DSP_MAXPOS(b) ( ((b)>=64) ?   9223372036854775807LL      : ((1UL << (b-1))-1) )
#define DSP_MINNEG(b) ( ((b)>=64) ? (-9223372036854775807LL-1LL) :  (1UL << (b-1))    )
#define DSP_QMSCALE(x,m,b) ( ((b)>=33) ? (long long)((double)(x)*(1LL<<(m))) : (int)((double)(x)*(1L<<(m))) )
#define DSP_QMBMIN(x,m,b) ( ( (-(x)) >  ( 1ULL << ( (b)-(m)-1) ) ) ? DSP_MINNEG(b) : DSP_QMSCALE(x,m,b) )
#define DSP_QMBMAX(x,m,b) ( (   (x)  >= ( 1ULL << ( (b)-(m)-1) ) ) ? DSP_MAXPOS(b) : DSP_QMBMIN(x,m,b)  )
#define DSP_QMB(x,m,b) ( ( ((m)>=(b))||((b)>64)||((m)<1) ) ? (1/((m)-(m))) : DSP_QMBMAX(x,m,b) )

#define DSP_QNM(x,n,m) DSP_QMB(x,m,n+m) //convert to m bit mantissa and n bit integer part including sign bit
#define DSP_QM32(x,m)  DSP_QMB(x,m,32)  //convert to 32bits int with mantissa "m"
#define DSP_QM64(x,m)  DSP_QMB(x,m,64)  //convert to 64bit long long with mantissa "m"

const int mantissa = 28;    // gives 3 bits for integer part (-8..+7.999)
const float epsilonf = 2*3.1415926*1000.0/96000.0;
static const int epsilon = epsilonf * (1<< mantissa);       // format 4.28
static const long long fullScale = 0.5 * (1LL<< (32+mantissa)); //equal 0.5 in format 4.60
int sine() {
    static long long xn=0,yn=0;
    long long x,y;
    if (yn == 0) xn = fullScale;
    y = yn >> mantissa;     //convert to 32 bits
    xn += y * -epsilon;     //4.60 += (1.31 * 4.28)
    x = xn >> mantissa;     //convert to 32 bits (1.31 considering sign)
    yn += x * epsilon;      //4.60 += (1.31 * 4.28)
    return x;
}

int main(int argc, char **argv) {
    const double z = 1.0;
    if (z >= ( 1ULL << 0)) printf("good\n"); else printf("bad\n");
    long long y = z;
    printf("z = %f, y = %lld = %llx\n",z,y,y);
    const double val = 2.0;
    long long xx = DSP_QM32(val,30);
    int hi = xx >> 32;
    unsigned int lo = xx & 0xFFFFFFFF;
    printf("val = %f, conv = %x-%x\n",val,hi,lo);
}
//
