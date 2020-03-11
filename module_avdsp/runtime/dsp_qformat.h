/*
 * dsp_qformat.h
 *
 *  Created on: 20 déc. 2019
 *      Author: Fabrice
 */


#ifndef DSP_QFORMAT_H_
#define DSP_QFORMAT_H_

/** These Macros can be used to parameterize the conversion macros.
 * E.g.
 * \code
 * #define BP 20  // location of the binary point
 * \endcode
 * Then use the macro like this:
 * \code
 * Q(BP)(1.234567)
 * \endcode
 */
#define F0(N) F ## N
#define F(N) F0(N)

#define Q0(N) Q ## N
#define Q(N) Q0(N)
#define QDP0(N) QDP ## N
#define QDP(N) QDP0(N)

// WARNING WARNING Q31 not compatible with exact value of 1.0 => generate -1
// Q31(1.0) => 0x80000000
// int x = Q31(1.0) = -2^32 = -2147483648
// Q31(-1.0) => 0x80000000
// int x = Q31(-1.0) = -2^32 = -2147483648
// F31(x) => -1.0 : conclusion 1.0 not supported!
// patch below force 1.0 to be converted in the larger positive number : 0x7FFFFFFF.
// F31(0x7FFFFFFF) => 1.0 ! fine

// Convert from floating point to fixed point Q format.
// The number indicates the fractional bits or the position of the binary point
#define Q31(f) ((f >= 1.0 )? (int)0x7FFFFFFF : (f <= -1.0) ? (int)(0x80000001) : (int)((signed long long)((f) * ((unsigned long long)1 << (31+20)) + (1<<19)) >> 20))
#define Q30(f) ((f >= 2.0 )? (int)0x3FFFFFFF : (int)((signed long long)((f) * ((unsigned long long)1 << (30+20)) + (1<<19)) >> 20))
#define Q29(f) (int)((signed long long)((f) * ((unsigned long long)1 << (29+20)) + (1<<19)) >> 20)
#define Q28(f) (int) ( (signed long long)( (f) * ( (unsigned long long)1 << (28+20)) + (1<<19)) >> 20)
#define Q27(f) (int)((signed long long)((f) * ((unsigned long long)1 << (27+20)) + (1<<19)) >> 20)
#define Q26(f) (int)((signed long long)((f) * ((unsigned long long)1 << (26+20)) + (1<<19)) >> 20)
#define Q25(f) (int)((signed long long)((f) * ((unsigned long long)1 << (25+20)) + (1<<19)) >> 20)
#define Q24(f) (int)((signed long long)((f) * ((unsigned long long)1 << (24+20)) + (1<<19)) >> 20)
#define Q23(f) (int)((signed long long)((f) * ((unsigned long long)1 << (23+20)) + (1<<19)) >> 20)
#define Q22(f) (int)((signed long long)((f) * ((unsigned long long)1 << (22+20)) + (1<<19)) >> 20)
#define Q21(f) (int)((signed long long)((f) * ((unsigned long long)1 << (21+20)) + (1<<19)) >> 20)
#define Q20(f) (int)((signed long long)((f) * ((unsigned long long)1 << (20+20)) + (1<<19)) >> 20)
#define Q19(f) (int)((signed long long)((f) * ((unsigned long long)1 << (19+20)) + (1<<19)) >> 20)
#define Q18(f) (int)((signed long long)((f) * ((unsigned long long)1 << (18+20)) + (1<<19)) >> 20)
#define Q17(f) (int)((signed long long)((f) * ((unsigned long long)1 << (17+20)) + (1<<19)) >> 20)
#define Q16(f) (int)((signed long long)((f) * ((unsigned long long)1 << (16+20)) + (1<<19)) >> 20)
#define Q15(f) (int)((signed long long)((f) * ((unsigned long long)1 << (15+20)) + (1<<19)) >> 20)
#define Q14(f) (int)((signed long long)((f) * ((unsigned long long)1 << (14+20)) + (1<<19)) >> 20)
#define Q13(f) (int)((signed long long)((f) * ((unsigned long long)1 << (13+20)) + (1<<19)) >> 20)
#define Q12(f) (int)((signed long long)((f) * ((unsigned long long)1 << (12+20)) + (1<<19)) >> 20)
#define Q11(f) (int)((signed long long)((f) * ((unsigned long long)1 << (11+20)) + (1<<19)) >> 20)
#define Q10(f) (int)((signed long long)((f) * ((unsigned long long)1 << (10+20)) + (1<<19)) >> 20)
#define Q9(f)  (int)((signed long long)((f) * ((unsigned long long)1 << (9+20)) + (1<<19)) >> 20)
#define Q8(f)  (int)((signed long long)((f) * ((unsigned long long)1 << (8+20)) + (1<<19)) >> 20)

// Convert from floating point to fixed point Q format.
// The number indicates the fractional bits or the position of the binary point
#define QDP30(f) ((signed long long)((f) * ((unsigned long long)1 << (60+3)) + (1<<2)) >> 3)
#define QDP29(f) ((signed long long)((f) * ((unsigned long long)1 << (58+3)) + (1<<2)) >> 3)
#define QDP28(f) ( (signed long long)( (f) * ( (unsigned long long)1 << (56+3)) + (1<<2)) >> 3)
#define QDP27(f) ((signed long long)((f) * ((unsigned long long)1 << (54+3)) + (1<<2)) >> 3)
#define QDP26(f) ((signed long long)((f) * ((unsigned long long)1 << (52+3)) + (1<<2)) >> 3)
#define QDP25(f) ((signed long long)((f) * ((unsigned long long)1 << (50+3)) + (1<<2)) >> 3)
#define QDP24(f) ((signed long long)((f) * ((unsigned long long)1 << (48+3)) + (1<<2)) >> 3)
#define QDP23(f) ((signed long long)((f) * ((unsigned long long)1 << (46+3)) + (1<<2)) >> 3)
#define QDP22(f) ((signed long long)((f) * ((unsigned long long)1 << (44+3)) + (1<<2)) >> 3)
#define QDP21(f) ((signed long long)((f) * ((unsigned long long)1 << (42+3)) + (1<<2)) >> 3)
#define QDP20(f) ((signed long long)((f) * ((unsigned long long)1 << (40+3)) + (1<<2)) >> 3)
#define QDP19(f) ((signed long long)((f) * ((unsigned long long)1 << (38+3)) + (1<<2)) >> 3)
#define QDP18(f) ((signed long long)((f) * ((unsigned long long)1 << (36+3)) + (1<<2)) >> 3)
#define QDP17(f) ((signed long long)((f) * ((unsigned long long)1 << (34+3)) + (1<<2)) >> 3)
#define QDP16(f) ((signed long long)((f) * ((unsigned long long)1 << (32+3)) + (1<<2)) >> 3)
#define QDP15(f) ((signed long long)((f) * ((unsigned long long)1 << (30+3)) + (1<<2)) >> 3)
#define QDP14(f) ((signed long long)((f) * ((unsigned long long)1 << (28+3)) + (1<<2)) >> 3)
#define QDP13(f) ((signed long long)((f) * ((unsigned long long)1 << (26+3)) + (1<<2)) >> 3)
#define QDP12(f) ((signed long long)((f) * ((unsigned long long)1 << (24+3)) + (1<<2)) >> 3)
#define QDP11(f) ((signed long long)((f) * ((unsigned long long)1 << (22+3)) + (1<<2)) >> 3)
#define QDP10(f) ((signed long long)((f) * ((unsigned long long)1 << (20+3)) + (1<<2)) >> 3)
#define QDP9(f)  ((signed long long)((f) * ((unsigned long long)1 << (18+3)) + (1<<2)) >> 3)
#define QDP8(f)  ((signed long long)((f) * ((unsigned long long)1 << (16+3)) + (1<<2)) >> 3)


// Convert from fixed point to double precision floating point
// The number indicates the fractional bits or the position of the binary point
#define F31(x) ((x == 0x7FFFFFFF) ? 1.0 : (x == 0xFFFFFFFF) ? -1.0 : (double)(x)/(double)(2147483648.0))
#define F30(x) ((double)(x)/(double)(1<<30))
#define F29(x) ((double)(x)/(double)(1<<29))
#define F28(x) ((double)(x)/(double)(1<<28))
#define F27(x) ((double)(x)/(double)(1<<27))
#define F26(x) ((double)(x)/(double)(1<<26))
#define F25(x) ((double)(x)/(double)(1<<25))
#define F24(x) ((double)(x)/(double)(1<<24))
#define F23(x) ((double)(x)/(double)(1<<23))
#define F22(x) ((double)(x)/(double)(1<<22))
#define F21(x) ((double)(x)/(double)(1<<21))
#define F20(x) ((double)(x)/(double)(1<<20))
#define F19(x) ((double)(x)/(double)(1<<19))
#define F18(x) ((double)(x)/(double)(1<<18))
#define F17(x) ((double)(x)/(double)(1<<17))
#define F16(x) ((double)(x)/(double)(1<<16))

#define FDP28(x) ((double)((x))/268435456.0/268435456.0)

// short
#define F15(x) ((double)(x)/(double)(1<<15))
#define F14(x) ((double)(x)/(double)(1<<14))
#define F13(x) ((double)(x)/(double)(1<<13))
#define F12(x) ((double)(x)/(double)(1<<12))
#define F11(x) ((double)(x)/(double)(1<<11))
#define F10(x) ((double)(x)/(double)(1<<10))
#define F9(x)  ((double)(x)/(double)(1<<9))
#define F8(x)  ((double)(x)/(double)(1<<8))

#endif /* DSP_QFORMAT_H_ */
