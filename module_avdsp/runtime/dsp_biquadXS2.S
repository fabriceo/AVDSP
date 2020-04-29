// Copyright (c) 2017-2019, XMOS Ltd, All rights reserved

#if defined(__XS2A__)

#define SKIP dspBiquadFreqSkip	/* global parameter from dsp_runtime.c to add a constatnt to coefptr*/
#define DSP_MANTBQ  28
#define NSTACKWORDS 8
//.syntax default
//.syntax architectural

	.text
    .issue_mode  dual
    .align 4
	.globl	dsp_biquads_xs2
	.type	dsp_biquads_xs2,@function
	.cc_top dsp_biquads_xs2.function,dsp_biquads_xs2

	/*
extern long long dsp_biquads_xs2( dspSample_t   xn,
							      dspParam_t *  coefPtr,
							      dspSample_t * dataPtr,
							      int           num);
	*/
	#define s0 r0
	#define filter_coeffs r1		/* pointer on the coeficients */
	#define state_data r2			/* pointer on state data */
	#define num_sections r3			/* number of section */
	#define h r4
	#define l r5
	#define s1 r6
	#define s2 r7
	#define c0 r8
	#define c1 r9

dsp_biquads_xs2:
	{dualentsp NSTACKWORDS; zext num_sections,8 } // keep num_sections lsb only
	{bt num_sections, dsp_biquads_xs2_entry; sub num_sections, num_sections, 1  }
	retsp NSTACKWORDS

dsp_biquads_xs2_entry:
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]

	ldd h,l,state_data[2]			// get "remainder" from last calculation (otherwise 0)
	ldd c1, c0, filter_coeffs[0] 	// c0=b0 and c1=b1
	ldd s2, s1, state_data[0]	 	// s1=x[n-1] and s2=x[n-2]
	std s1, s0, state_data[0]		// put x[n-1] into x[n-2] and x[n] into x[n-1]
	maccs h, l, s0, c0 				// x[n] * b0
	maccs h, l, s1, c1				// x[n-1] * b1
	ldd c0, c1, filter_coeffs[1] 	// c1=b2 and c0=a1
	maccs h, l, s2, c1 				// x[n-2] * b2
	ldd s2, s1, state_data[1]	 	// s1=y[n-1] and s2=y[n-2]
	maccs h, l, s1, c0				// y[n-1] * a1	this coef is reduced by 1.0 due to remainder integration
	{ldw c1, filter_coeffs[4]; ldc c0,(DSP_MANTBQ) } 	// c1=a2
	maccs h, l, s2, c1				// y[n-2] * a2
	lsats h, l, c0					// saturate
	std h,l,state_data[2]			// save accumulator for reminder re-integration at next cycle
	lextract s0, h, l, c0, 32		// remove BQ coefs precision and get back to sample precision 4.28
	std s1, s0, state_data[1]		// store into y[n-1] and y[n-2]

	{bf num_sections, dsp_biquads_xs2_end; sub num_sections,num_sections,1  }	// leave if finished

#if defined( SKIP )
	ldw r11, dp[SKIP]
dsp_biquads_xs2_loop:
	ldaw filter_coeffs, filter_coeffs[r11]
#else
dsp_biquads_xs2_loop:
	ldaw filter_coeffs, filter_coeffs[6]
#endif
	ldaw state_data, state_data[6]

	ldd h,l,state_data[2]			// get "remainder" from last calculation (otherwise 0)
	ldd c1, c0, filter_coeffs[0] 	// c0=b0 and c1=b1
	ldd s2, s1, state_data[0]	 	// s1=x[n-1] and s2=x[n-2]
	std s1, s0, state_data[0]		// put x[n-1] into x[n-2] and x[n] into x[n-1]
	maccs h, l, s0, c0 				// x[n] * b0
	maccs h, l, s1, c1				// x[n-1] * b1
	ldd c0, c1, filter_coeffs[1] 	// c1=b2 and c0=a1
	maccs h, l, s2, c1 				// x[n-2] * b2
	ldd s2, s1, state_data[1]	 	// s1=y[n-1] and s2=y[n-2]
	maccs h, l, s1, c0				// y[n-1] * a1	this coef is reduced by 1.0 due to remainder integration
	{ldw c1, filter_coeffs[4]; ldc c0,(DSP_MANTBQ)  } 	// c1=a2
	maccs h, l, s2, c1				// y[n-2] * a2
	lsats h, l, c0					// saturate
	std h,l,state_data[2]			// save accumulator for reminder re-integration at next cycle
	lextract s0, h, l, c0, 32		// remove BQ coefs precision and get back to sample precision 4.28
	std s1, s0, state_data[1]		// store into y[n-1] and y[n-2]

	{bt num_sections, dsp_biquads_xs2_loop; sub num_sections, num_sections, 1  }	// leave if finished

dsp_biquads_xs2_end:
	{ mov r0,l; mov r1,h }			// return 8.56
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
	retsp NSTACKWORDS


	// RETURN_REG_HOLDER
	.cc_bottom dsp_biquads_xs2.function
	.set	dsp_biquads_xs2.nstackwords,NSTACKWORDS
	.globl	dsp_biquads_xs2.nstackwords
	.set	dsp_biquads_xs2.maxcores,1
	.globl	dsp_biquads_xs2.maxcores
	.set	dsp_biquads_xs2.maxtimers,0
	.globl	dsp_biquads_xs2.maxtimers
	.set	dsp_biquads_xs2.maxchanends,0
	.globl	dsp_biquads_xs2.maxchanends
.Ltmp0:
	.size	dsp_biquads_xs2, .Ltmp0-dsp_biquads_xs2


#endif
