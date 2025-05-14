/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* ieee style elementary functions */
extern double __ieee754_acos(double);			
extern double __ieee754_acosh(double);			
extern double __ieee754_asin(double);			
extern double __ieee754_atan2(double,double);			
extern double __ieee754_atanh(double);			
extern double __ieee754_cosh(double);
extern double __ieee754_exp(double);
extern double __ieee754_fmod(double,double);
extern double __ieee754_hypot(double,double);
extern double __ieee754_log(double);
extern double __ieee754_log10(double);
extern double __ieee754_pow(double,double);
extern double __ieee754_sinh(double);
extern double __ieee754_trunc(double);

/* standard functions */
extern double s_asinh(double x);
extern double s_atan(double x);
extern double s_cbrt(double x);
extern double s_ceil(double x);
extern double s_cos(double x);
extern double s_expm1(double x);
extern double s_log1p(double x);
extern double s_scalbn(double x, int n);
extern double s_sin(double x);
extern double s_tan(double x);
extern double s_tanh(double x);
extern double s_trunc(double x);

#define c_acos __ieee754_acos
#define c_acosh __ieee754_acosh
#define c_asin __ieee754_asin
#define c_asinh s_asinh
#define c_atan s_atan
#define c_atan2 __ieee754_atan2
#define c_atanh __ieee754_atanh
#define c_cbrt s_cbrt
#define c_ceil s_ceil
#define c_cos s_cos
#define c_cosh __ieee754_cosh
#define c_exp __ieee754_exp
#define c_expm1 s_expm1
#define c_fmod __ieee754_fmod
#define c_hypot __ieee754_hypot
#define c_log __ieee754_log
#define c_log10 __ieee754_log10
#define c_log1p s_log1p
#define c_pow __ieee754_pow
#define c_sin s_sin
#define c_sinh __ieee754_sinh
#define c_tan s_tan
#define c_tanh s_tanh
#define c_trunc s_trunc


