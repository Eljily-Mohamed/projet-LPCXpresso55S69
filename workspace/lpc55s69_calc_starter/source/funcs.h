/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * funcs.h
 *
 ****************************************************************/
#ifndef FUNCS_H
#define FUNCS_H

#include "calc.h"

header* minput (Calc *cc, header *hd);

/* light user functions */
header* interpret_luf (Calc *cc, header *var, header *hd, int argn, int epos);

/* user defined functions */
void make_xors (void);
header* interpret_udf (Calc *cc, header *var, header *args, int argn, int epos);
header* margn (Calc *cc, header *hd);
header* margs (Calc *cc, header *hd);
header* mxargs (Calc *cc, header *hd);
header* mindex (Calc *cc, header *hd);
header* merror (Calc *cc, header *hd);

/* type introspection */
header* mname (Calc *cc, header *hd);
header* miscomplex (Calc *cc, header *hd);
header* misreal (Calc *cc, header *hd);
header* misstring (Calc *cc, header *hd);
header* misvar (Calc *cc, header *hd);
header* misfunction (Calc *cc, header *hd);
header* misnan (Calc *cc, header *hd);
header* misinf (Calc *cc, header *hd);
header* misfinite (Calc *cc, header *hd);

/* complex utils */
void c_add (cplx x, cplx y, cplx z);
void c_sub (cplx x, cplx y, cplx z);
void c_mul (cplx x, cplx y, cplx z);
void c_div (cplx x, cplx y, cplx z);
void c_copy (cplx x, cplx y);
void make_complex (Calc *cc, header *hd);

/* time functions */
header* mtime (Calc *cc, header *hd);
header* mwait (Calc *cc, header *hd);

/* number and text formatting functions */
header* mformat (Calc *cc, header *hd);
header* mprintf (Calc *cc, header *hd);

/* const */
header* mpi (Calc *cc, header *hd);
header* mepsilon (Calc *cc, header *hd);
header* msetepsilon (Calc *cc, header *hd);

/* arithmetic operators */
header* add (Calc *cc, header *hd, header *hd1);
header* subtract (Calc *cc, header *hd, header *hd1);
header* dotmultiply (Calc *cc, header *hd, header *hd1);
header* dotdivide (Calc *cc, header *hd, header *hd1);
header* opposite (Calc *cc, header *hd);
header* multiply (Calc *cc, header *hd, header *hd1);
header* wmultiply (Calc *cc, header *hd);
header* smultiply (Calc *cc, header *hd);
header* mpower (Calc *cc, header *hd, header *hd1);
header* transpose (Calc *cc, header *hd);
header* mvconcat (Calc *cc, header *hd, header *hd1);
header* mhconcat (Calc *cc, header *hd, header *hd1);
header* vectorize (Calc *cc, header *init, header *step, header *end);
header* msolve (Calc *cc, header *hd, header* hd1);

/* compare operators */
header* mgreater (Calc *cc, header *hd, header *hd1);
header* mless (Calc *cc, header *hd, header *hd1);
header* mgreatereq (Calc *cc, header *hd, header *hd1);
header* mlesseq (Calc *cc, header *hd, header *hd1);
header* mequal (Calc *cc, header *hd, header *hd1);
header* munequal (Calc *cc, header *hd, header *hd1);
header* maboutequal (Calc *cc, header *hd, header *hd1);

/* boolean operators */
header* mnot (Calc *cc, header *hd);
header* mand (Calc *cc, header *hd, header *hd1);
header* mor (Calc *cc, header *hd, header *hd1);

/* complex operations */
header* mcomplex (Calc *cc, header *hd);
header* mconj (Calc *cc, header *hd);
header* mre (Calc *cc, header *hd);
header* mim (Calc *cc, header *hd);
header* marg (Calc *cc, header *hd);
header* mabs (Calc *cc, header *hd);

/* elementwise math func */
header* msin (Calc *cc, header *hd);
header* mcos (Calc *cc, header *hd);
header* mtan (Calc *cc, header *hd);
header* masin (Calc *cc, header *hd);
header* macos (Calc *cc, header *hd);
header* matan (Calc *cc, header *hd);
header* mexp (Calc *cc, header *hd);
header* mlog (Calc *cc, header *hd);
header* msqrt (Calc *cc, header *hd);
header* mmod (Calc *cc, header *hd);

header* msign (Calc *cc, header *hd);
header* mceil (Calc *cc, header *hd);
header* mfloor (Calc *cc, header *hd);
header* mround (Calc *cc, header *hd);

header* merf (Calc *cc, header *hd);
header* merfc (Calc *cc, header *hd);

/* number theory */
header* mfac (Calc *cc, header *hd);
header* mbin (Calc *cc, header *hd);
header* mlogfac (Calc *cc, header *hd);
header* mlogbin (Calc *cc, header *hd);

/* matrix operations */
/*   get information */
header* mcols (Calc *cc, header *hd);
header* mrows (Calc *cc, header *hd);
header* msize (Calc *cc, header *hd);

header* mnonzeros (Calc *cc, header *hd);
header* mall (Calc *cc, header *hd);
header* many (Calc *cc, header *hd);
header* mextrema (Calc *cc, header *hd);

/*   generate matrices */
header* mmatrix (Calc *cc, header *hd);
header* mzeros (Calc *cc, header *hd);
header* mones (Calc *cc, header *hd);
header* mdiag (Calc *cc, header *hd);
header* mdiag2 (Calc *cc, header *hd);
header* msetdiag (Calc *cc, header *hd);
header* mband (Calc* cc, header *hd);

/*	transform matrix */
header* mdup (Calc *cc, header *hd);
header* mredim (Calc *cc, header *hd);
header* msum (Calc *cc, header *hd);
header* mprod (Calc *cc, header *hd);
header* mcolsum (Calc *cc, header *hd);
header* mcumsum (Calc *cc, header *hd);
header* mcumprod (Calc *cc, header *hd);
header* mflipx (Calc *cc, header *hd);
header* mflipy (Calc *cc, header *hd);

header* mmax (Calc *cc, header *hd);
header* mmin (Calc *cc, header *hd);
header* mmax1 (Calc *cc, header *hd);
header* mmin1 (Calc *cc, header *hd);
header* msort (Calc *cc, header *hd);
header* mstatistics (Calc *cc, header *hd);

/* polynoms */
header* mpolyval (Calc *cc, header *hd);
header* mpolyadd (Calc *cc, header *hd);
header* mpolymult (Calc *cc, header *hd);
header* mpolydiv (Calc *cc, header *hd);
header* mpolytrunc (Calc *cc, header *hd);
header* mpolycons (Calc *cc, header *hd);
header* mpolysolve (Calc *cc, header *hd);
header* mpolyroot (Calc *cc, header *hd);

/* interpolation (divided difference, Lagrange) */
header* dd (Calc *cc, header *hd);
header* ddval (Calc *cc, header *hd);
header* polydd (Calc *cc, header *hd);
header* mlagr (Calc *cc, header *hd);

/* linear algebra */
header* mlu (Calc *cc, header *hd);
header* mlusolve (Calc *cc, header *hd);
header* mtridiag (Calc *cc, header *hd);
header* mcharpoly (Calc *cc, header *hd);

/* FFT (see dsp.h) */

/* random and statistics */
header* mseed (Calc *cc, header *hd);
header* mrandom (Calc *cc, header *hd);
header* mnormal (Calc *cc, header *hd);
header* mshuffle (Calc *cc, header *hd);
header* mcount (Calc *cc, header *hd);
header* mfind (Calc *cc, header *hd);

#endif
