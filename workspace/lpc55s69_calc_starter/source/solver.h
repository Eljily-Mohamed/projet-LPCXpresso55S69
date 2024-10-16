/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * solver.h
 *
 ****************************************************************/
#ifndef SOLVER_H
#define SOLVER_H

#include "calc.h"

void make_lu (Calc *cc, real *a, int n, int m,
	int **rows, int **cols, int *rankp, real *detp);
void cmake_lu (Calc *cc, real *a, int n, int m,
	int **rows, int **cols, int *rankp, real *detp, real *detip);

void lu_solve (Calc *cc, real *a, int n, real *rs, int m, real *res);
void clu_solve (Calc *cc, real *a, int n, real *rs, int m, real *res);

void solvesim (Calc *cc, real *a, int n, real *rs, int m, real *res);
void c_solvesim (Calc *cc, real *a, int n, real *rs, int m, real *res);

void tridiag (Calc *cc, real *a, int n, int **rows);
void ctridiag (Calc *cc, real *ca, int n, int **rows);
void charpoly (Calc *cc, real *m1, int n, real *p);
void ccharpoly (Calc *cc, real *m1, int n, real *p);

#endif
