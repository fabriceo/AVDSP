/*
 * HilbertDesign.c
 *
 *  Created on: 23 déc. 2022
 *      Author: Fabrice
 */

#include <stdio.h>
#include <math.h>

#define assert(x) {}
#define double float

static double   ipowp (double x, long n)
{
    assert (n >= 0);

    double z = 1.0;
    while (n != 0) {
        if ((n & 1) != 0) z *= x;
        n >>= 1;
        x *= x; }
    return z;
}


static double  compute_acc_num (double q, int order, int c)
{
    assert (c >= 1);
    assert (c < order * 2);

    int            i   = 0;
    int            j   = 1;
    double         acc = 0;
    double         q_ii1;
    do
    {
        q_ii1  = ipowp (q, i * (i + 1));
        q_ii1 *= sin ((i * 2 + 1) * c * M_PI / order) * j;
        acc   += q_ii1;

        j = -j;
        ++i;
    }
    while (fabs (q_ii1) > 1e-100);

    return acc;
}



static double  compute_acc_den (double q, int order, int c)
{
    assert (c >= 1);
    assert (c < order * 2);

    int            i   =  1;
    int            j   = -1;
    double         acc =  0;
    double         q_i2;
    do
    {
        q_i2  = ipowp (q, i * i);
        q_i2 *= cos (i * 2 * c * M_PI / order) * j;
        acc  += q_i2;

        j = -j;
        ++i;
    }
    while (fabs (q_i2) > 1e-100);

    return acc;
}


static void    compute_transition_param (double *k, double *q, double transition)
{
    assert (transition > 0);
    assert (transition < 0.5);

    *k  = tan ((1 - transition * 2) * M_PI / 4);
    *k *= *k;
    assert (*k < 1);
    assert (*k > 0);
    double         kksqrt = pow (1 - *k * *k, 0.25);
    const double   e = 0.5 * (1 - kksqrt) / (1 + kksqrt);
    const double   e2 = e * e;
    const double   e4 = e2 * e2;
    *q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
    assert (q > 0);
}


static double  compute_coef (int index, double k, double q, int order)
{
    assert (index >= 0);
    assert (index * 2 < order);

    const int      c    = index + 1;
    const double   num  = compute_acc_num (q, order, c) * pow (q, 0.25);
    const double   den  = compute_acc_den (q, order, c) + 0.5;
    const double   ww   = num / den;
    const double   wwsq = ww * ww;

    const double   x    = sqrt ((1 - wwsq * k) * (1 - wwsq / k)) / (1 + wwsq);
    const double   coef = (1 - x) / (1 + x);

    return coef;
}

extern void    compute_coefs_spec_order_tbw (double *coef_arr, int nbr_coefs, double transition);
void    compute_coefs_spec_order_tbw (double *coef_arr, int nbr_coefs, double transition)
{
    assert (nbr_coefs > 0);
    assert (transition > 0);
    assert (transition < 0.5);

    double         k;
    double         q;
    compute_transition_param (&k, &q, transition);
    const int      order = nbr_coefs * 2 + 1;

    // Coefficient calculation
    for (int index = 0; index < nbr_coefs; ++index)
    {
        coef_arr [index] = compute_coef (index, k, q, order);
    }
}


int HilbertDesignTest() {

const int numCoefs = 8; // Number of coefficients, must be even
double transition = 2*20.0/96000; // Sampling frequency is 44.1 kHz. Approx. 90 deg phase difference band is from 20 Hz to 22050 Hz - 20 Hz. The transition bandwidth is twice 20 Hz.
double coefs[numCoefs];
/*
Phase reference path c coefficients:
0.54331142587629277507,0.91245076641442879328,0.98635050978014349177,0.99871842272710165123,
+90 deg path c coefficients:
0.19346824887735125653,0.78995292705629904795,0.96502852355058565959,0.99493573378216082492,
 */
  compute_coefs_spec_order_tbw (&coefs[0], numCoefs, transition);
  printf("Phase reference path c coefficients:\n");
  for (int i = 1; i < numCoefs; i += 2) {
    printf("%.20f,", coefs[i]);
  }
  printf("\n+90 deg path c coefficients:\n");
  for (int i = 0; i < numCoefs; i += 2) {
    printf("%.20f,", coefs[i]);
  }
  printf("\n");
  return 0;
}


