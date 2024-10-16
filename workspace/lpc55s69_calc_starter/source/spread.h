/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * spread.c -- spread function elementwise
 *
 ****************************************************************/
#ifndef SPREAD_H
#define SPREAD_H

#include "calc.h"

header* map1 (Calc *cc, 
	void f(real *, real *),
	void fc(cplx, cplx),
	header *hd);
header* map1r (Calc *cc, 
	void f(real *, real *),
	void fc(cplx, real *),
	header *hd);

header* spread1 (Calc *cc, 
	real f (real),
	void fc (cplx, cplx),
	header *hd);
header* spread1r (Calc *cc, 
	real f (real),
	void fc (cplx, real *),
	header *hd);

header* map2 (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, cplx),
	header *hd1, header *hd2);
header* map2r (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, real *),
	header *hd1, header *hd2);

header* spread2 (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, cplx),
	header *hd);
header* spread2r (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, real *),
	header *hd);

#endif
