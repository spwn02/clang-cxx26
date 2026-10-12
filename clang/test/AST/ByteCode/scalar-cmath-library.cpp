// RUN: %clang_cc1 -std=c++20 -verify=expected,old-classic %s
// RUN: %clang_cc1 -std=c++20 -fexperimental-new-constant-interpreter -verify=expected %s
// RUN: %clang_cc1 -std=c++23 -verify=expected,pre26-classic %s
// RUN: %clang_cc1 -std=c++23 -fexperimental-new-constant-interpreter -verify=expected %s
// RUN: %clang_cc1 -std=c++26 -verify=expected,cxx26 %s
// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -verify=expected,cxx26 %s
// cxx26-no-diagnostics

extern "C" {
float fmaf(float,float,float); // old-classic-note {{declared here}}
long double fmal(long double,long double,long double);
double ldexp(double,int);
float ldexpf(float,int);
long double ldexpl(long double,int);
double scalbn(double,int);
float scalbnf(float,int);
long double scalbnl(long double,int);
double scalbln(double,long);
float scalblnf(float,long);
long double scalblnl(long double,long);
double nextafter(double,double);
float nextafterf(float,float);
long double nextafterl(long double,long double);
double nexttoward(double,long double);
float nexttowardf(float,long double);
long double nexttowardl(long double,long double);
double frexp(double,int*);
float frexpf(float,int*);
long double frexpl(long double,int*);
double fmin(double,double);
float fminf(float,float);
long double fminl(long double,long double);
double fmax(double,double);
float fmaxf(float,float);
long double fmaxl(long double,long double);
double nextup(double); // pre26-classic-note {{declared here}} old-classic-note {{declared here}}
float nextupf(float);
long double nextupl(long double);
double nextdown(double);
float nextdownf(float);
long double nextdownl(long double);
double fmaximum(double,double);
float fmaximumf(float,float);
long double fmaximuml(long double,long double);
double fminimum(double,double);
float fminimumf(float,float);
long double fminimuml(long double,long double);
double fmaximum_num(double,double);
float fmaximum_numf(float,float);
long double fmaximum_numl(long double,long double);
double fminimum_num(double,double);
float fminimum_numf(float,float);
long double fminimum_numl(long double,long double);
}
#if __cplusplus >= 202302L
static_assert(fmaf(2,3,1)==7);
static_assert(fmal(2,3,1)==7);
static_assert(ldexp(1.5,4)==24 && ldexpf(1.5f,4)==24 && ldexpl(1.5L,4)==24);
static_assert(scalbn(1,10)==1024 && scalbnf(1,10)==1024 && scalbnl(1,10)==1024);
static_assert(scalbln(1,10)==1024 && scalblnf(1,10)==1024 && scalblnl(1,10)==1024);
static_assert(nextafter(1,2)>1 && nextafterf(1,2)>1 && nextafterl(1,2)>1);
static_assert(nexttoward(1,2)>1 && nexttowardf(1,1.0L+1e-10L)>1 && nexttowardl(1,2)>1);
constexpr bool decompose() { int e; return frexp(48,&e)==0.75 && e==6 && frexpf(48,&e)==0.75 && e==6 && frexpl(48,&e)==0.75 && e==6; }
static_assert(decompose());
static_assert(fmin(1,2)==1 && fminf(1,2)==1 && fminl(1,2)==1);
static_assert(fmax(1,2)==2 && fmaxf(1,2)==2 && fmaxl(1,2)==2);
#else
constexpr float old_fma = fmaf(2,3,1); // expected-error {{must be initialized by a constant expression}} old-classic-note {{non-constexpr function 'fmaf' cannot be used in a constant expression}}
#endif
#if __cplusplus > 202302L
static_assert(nextup(1)>1 && nextupf(1)>1 && nextupl(1)>1);
static_assert(nextdown(1)<1 && nextdownf(1)<1 && nextdownl(1)<1);
static_assert(fmaximum(1,2)==2 && fmaximumf(1,2)==2 && fmaximuml(1,2)==2);
static_assert(fminimum(1,2)==1 && fminimumf(1,2)==1 && fminimuml(1,2)==1);
static_assert(fmaximum_num(1,2)==2 && fmaximum_numf(1,2)==2 && fmaximum_numl(1,2)==2);
static_assert(fminimum_num(1,2)==1 && fminimum_numf(1,2)==1 && fminimum_numl(1,2)==1);
#else
constexpr double old_nextup = nextup(1); // expected-error {{must be initialized by a constant expression}} pre26-classic-note {{non-constexpr function 'nextup' cannot be used in a constant expression}} old-classic-note {{non-constexpr function 'nextup' cannot be used in a constant expression}}
#endif
