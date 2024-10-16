/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * solver.h
 *
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "calc.h"
#include "funcs.h"

int *perm,*col,signdet,luflag=0;
real **lumat,*c,det;
cplx **c_lumat,*c_c,c_det;
int rank;

#define outofram() { cc_error(cc,"Out of Memory!"); }

/***************** real linear systems *******************/

void lu (Calc* cc, real *a, int n, int m)
/***** lu
	lu decomposition of a
*****/
{	int i,j,k,mm,j0,kh;
	real *d,piv,temp,*temp1,zmax,help;
	char *ram=cc->newram;
	
	if (!luflag){
		/* get place for result c and move a to c */
		c=(real *)ram;
		ram+=(LONG)n*m*sizeof(real);
		if (ram>cc->udfstart) outofram();
		memmove((char *)c,(char *)a,(LONG)n*m*sizeof(real));
	} else c=a;
	
	/* inititalize lumat */
	lumat=(real **)ram;
	ram+=(LONG)n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	d=c; 
	for (i=0; i<n; i++) { lumat[i]=d; d+=m; }
	
	/* get place for perm */
	perm=(int *)ram;
	ram+=(LONG)n*sizeof(int);
	if (ram>cc->udfstart) outofram();
	
	/* get place for col */
	col=(int *)ram;
	ram+=(LONG)m*sizeof(int);
	if (ram>cc->udfstart) outofram();
	
	/* d is a vector needed by the algorithm */
	d=(real *)ram;
	ram+=(LONG)n*sizeof(real);
	if (ram>cc->udfstart) outofram();
	
	/* gauss algorithm */
	if (!luflag) {
		for (k=0; k<n; k++) {
			perm[k]=k;
			for (zmax=0.0, j=0; j<m; j++)
				if ( (help=fabs(lumat[k][j])) >zmax) zmax=help;
			if (zmax==0.0) { cc_error(cc,"error in LU decomposition"); }
			d[k]=zmax;
		}
	} else {
		for (k=0; k<n; k++) perm[k]=k;
	}
	signdet=1; rank=0; det=1.0; k=0;
	for (kh=0; kh<m; kh++) {
		piv=luflag?fabs(lumat[k][kh]):(fabs(lumat[k][kh])/d[k]);
		j0=k;
		for (j=k+1; j<n; j++) {
			temp=luflag?fabs(lumat[j][kh]):(fabs(lumat[j][kh])/d[j]);
			if (piv<temp) {
				piv=temp; j0=j;
			}
		}
		if (piv<cc->epsilon) {
			signdet=0;
			if (luflag) {
				col[kh]=0;
				continue;
			} else {
				cc_error(cc,"Determinant zero!"); 
			}
		} else {
			col[kh]=1; rank++;
			det*=lumat[j0][kh];
		}
		if (j0!=k) {
			signdet=-signdet;
			mm=perm[j0]; perm[j0]=perm[k]; perm[k]=mm;
			if (!luflag) {
			 temp=d[j0]; d[j0]=d[k]; d[k]=temp; }
			temp1=lumat[j0]; lumat[j0]=lumat[k]; lumat[k]=temp1;
		}
		for (j=k+1; j<n; j++)
			if (lumat[j][kh] != 0.0) {
				lumat[j][kh] /= lumat[k][kh];
				for (temp=lumat[j][kh], mm=kh+1; mm<m; mm++)
					lumat[j][mm]-=temp*lumat[k][mm];
			}
		k++;
		if (k>=n) { kh++; break; }
	}
	if (k<n || kh<m) {
		signdet=0;
		if (!luflag) {
			cc_error(cc,"Determinant zero!"); 
		}
	}
	for (j=kh; j<m; j++) col[j]=0;
	det=det*signdet;
	cc->newram=ram;
}

void make_lu (Calc *cc, real *a, int n, int m, 
	int **rows, int **cols, int *rankp,
	real *detp)
{
	luflag=1; lu(cc,a,n,m);
	*rows=perm; *cols=col; *rankp=rank; *detp=det;
}

void lu_solve (Calc *cc, real *a, int n, real *rs, int m, real *res)
{	real **x,**b,*h,sum,*d;
	int i,k,l,j;
	char *ram=cc->newram;
	
	/* initialize x and b */
	x=(real **)ram;
	ram+=(LONG)n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	h=res; for (i=0; i<n; i++) { x[i]=h; h+=m; }

	b=(real **)ram;
	ram+=(LONG)n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	h=rs; for (i=0; i<n; i++) { b[i]=h; h+=m; }
	
	/* inititalize lumat */
	lumat=(real **)ram;
	ram+=(LONG)n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	d=a; 
	for (i=0; i<n; i++) { lumat[i]=d; d+=n; }
	
	for (l=0; l<m; l++)
	{	x[0][l]=b[0][l];
		for (k=1; k<n; k++)
		{	x[k][l]=b[k][l];
			for (j=0; j<k; j++)
				x[k][l] -= lumat[k][j]*x[j][l];
		}
		x[n-1][l] /= lumat[n-1][n-1];
		for (k=n-2; k>=0; k--)
		{	for (sum=0.0, j=k+1; j<n; j++)
				sum+=lumat[k][j]*x[j][l];
			x[k][l] = (x[k][l]-sum)/lumat[k][k];
		}
	}
}

void solvesim (Calc *cc, real *a, int n, real *rs, int m, real *res)
/**** solvesim
	solve simultanuously a linear system.
****/
{	real **x,**b,*h,sum;
	int i,k,l,j;
	char *ram0=cc->newram, *ram;
	luflag=0;
	lu(cc,a,n,n);
	ram=cc->newram;
	/* initialize x and b */
	x=(real **)ram;
	ram+=(LONG)n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	h=res; for (i=0; i<n; i++) { x[i]=h; h+=m; }
	b=(real **)ram;
	ram+=(LONG)n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	h=rs; for (i=0; i<n; i++) { b[i]=h; h+=m; }
	
	for (l=0; l<m; l++) {
		x[0][l]=b[perm[0]][l];
		for (k=1; k<n; k++) {
			x[k][l]=b[perm[k]][l];
			for (j=0; j<k; j++)
				x[k][l] -= lumat[k][j]*x[j][l];
		}
		x[n-1][l] /= lumat[n-1][n-1];
		for (k=n-2; k>=0; k--) {
			for (sum=0.0, j=k+1; j<n; j++)
				sum+=lumat[k][j]*x[j][l];
			x[k][l] = (x[k][l]-sum)/lumat[k][k];
		}
	}
	cc->newram=ram0;
}
/******************* complex linear systems **************/
static real c_abs (cplx x)
{	return sqrt(x[0]*x[0]+x[1]*x[1]);
}

void c_lu (Calc *cc, real *a, int n, int m)
/***** clu
	lu decomposition of a
*****/
{	int i,j,k,mm,j0,kh;
	real *d,piv,temp,zmax,help;
	cplx t,*h,*temp1;
	char *ram=cc->newram;
	if (!luflag)
	{	/* get place for result c and move a to c */
		c_c=(cplx *)ram;
		ram+=(LONG)n*m*sizeof(cplx);
		if (ram>cc->udfstart) outofram();
		memmove((char *)c_c,(char *)a,(LONG)n*m*sizeof(cplx));
	}
	else c_c=(cplx *)a;
	
	/* inititalize c_lumat */
	c_lumat=(cplx **)ram;
	ram+=(LONG)n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	h=c_c; 
	for (i=0; i<n; i++) { c_lumat[i]=h; h+=m; }
	
	/* get place for perm */
	perm=(int *)ram;
	ram+=(LONG)n*sizeof(int);
	if (ram>cc->udfstart) outofram();
	
	/* get place for col */
	col=(int *)ram;
	ram+=(LONG)m*sizeof(int);
	if (ram>cc->udfstart) outofram();
	
	/* d is a vector needed by the algorithm */
	d=(real *)ram;
	ram+=(LONG)n*sizeof(real);
	if (ram>cc->udfstart) outofram();
	
	/* gauss algorithm */
	if (!luflag) {
		for (k=0; k<n; k++) {
			perm[k]=k;
			for (zmax=0.0, j=0; j<m; j++)
				if ( (help=c_abs(c_lumat[k][j])) >zmax) zmax=help;
			if (zmax==0.0) { cc_error(cc,"error in LU decomposition"); }
			d[k]=zmax;
		}
	} else {
		for (k=0; k<n; k++) perm[k]=k;
	}
	signdet=1; rank=0; c_det[0]=1.0; c_det[1]=0.0; k=0;
	for (kh=0; kh<m; kh++) {
		piv=luflag?c_abs(c_lumat[k][kh]):(c_abs(c_lumat[k][kh])/d[k]);
		j0=k;
		for (j=k+1; j<n; j++) {
			temp=luflag?c_abs(c_lumat[j][kh]):(c_abs(c_lumat[j][kh])/d[j]);
			if (piv<temp) {
				piv=temp; j0=j;
			}
		}
		if (piv<cc->epsilon) {
			signdet=0;
			if (luflag) {
				col[kh]=0;
				continue;
			} else {
				cc_error(cc,"Determinant zero!"); 
			}
		} else {
			col[kh]=1; rank++;
			c_mul(c_det,c_lumat[j0][kh],c_det);
		}
		if (j0!=k) {
			signdet=-signdet;
			mm=perm[j0]; perm[j0]=perm[k]; perm[k]=mm;
			if (!luflag) {
				temp=d[j0]; d[j0]=d[k]; d[k]=temp;
			}
			temp1=c_lumat[j0]; c_lumat[j0]=c_lumat[k]; 
				c_lumat[k]=temp1;
		}
		for (j=k+1; j<n; j++)
			if (c_lumat[j][kh][0] != 0.0 || c_lumat[j][kh][1]!=0.0) {
				c_div(c_lumat[j][kh],c_lumat[k][kh],c_lumat[j][kh]);
				for (mm=kh+1; mm<m; mm++) {
					c_mul(c_lumat[j][kh],c_lumat[k][mm],t);
					c_sub(c_lumat[j][mm],t,c_lumat[j][mm]);
				}
			}
		k++;
		if (k>=n) { kh++; break; }
	}
	if (k<n || kh<m) {
		signdet=0;
		if (!luflag) {
			cc_error(cc,"Determinant zero!"); 
		}
	}
	for (j=kh; j<m; j++) col[j]=0;
	c_det[0]=c_det[0]*signdet; c_det[1]=c_det[1]*signdet;
	cc->newram=ram;
}

void cmake_lu (Calc *cc, real *a, int n, int m,
	int **rows, int **cols, int *rankp,
	real *detp, real *detip)
{
	luflag=1; c_lu(cc,a,n,m);
	*rows=perm; *cols=col; *rankp=rank; 
	*detp=c_det[0]; *detip=c_det[1];
}

void clu_solve (Calc *cc, real *a, int n, real *rs, int m, real *res)
/**** solvesim
	solve simultanuously a linear system.
****/
{	cplx **x,**b,*h;
	cplx sum,t;
	int i,k,l,j;
	char *ram=cc->newram;
	
	/* initialize x and b */
	x=(cplx **)ram;
	ram+=(LONG)n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	h=(cplx *)res; for (i=0; i<n; i++) { x[i]=h; h+=m; }
	
	b=(cplx **)ram;
	ram+=(LONG)n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	h=(cplx *)rs; for (i=0; i<n; i++) { b[i]=h; h+=m; }
	
	/* inititalize c_lumat */
	c_lumat=(cplx **)ram;
	ram+=(LONG)n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	h=(cplx *)a; 
	for (i=0; i<n; i++) { c_lumat[i]=h; h+=n; }
	
	for (l=0; l<m; l++) {
		c_copy(b[0][l],x[0][l]);
		for (k=1; k<n; k++) {
			c_copy(b[k][l],x[k][l]);
			for (j=0; j<k; j++) {
				c_mul(c_lumat[k][j],x[j][l],t);
				c_sub(x[k][l],t,x[k][l]);
			}
		}
		c_div(x[n-1][l],c_lumat[n-1][n-1],x[n-1][l]);
		for (k=n-2; k>=0; k--) {
			sum[0]=0; sum[1]=0.0;
			for (j=k+1; j<n; j++) {
				c_mul(c_lumat[k][j],x[j][l],t);
				c_add(sum,t,sum);
			}
			c_sub(x[k][l],sum,t);
			c_div(t,c_lumat[k][k],x[k][l]);
		}
	}
}

void c_solvesim (Calc *cc, real *a, int n, real *rs, int m, real *res)
/**** solvesim
	solve simultanuously a linear system.
****/
{	cplx **x,**b,*h;
	cplx sum,t;
	int i,k,l,j;
	char *ram0=cc->newram, *ram;
	luflag=0; c_lu(cc,a,n,n);
	ram=cc->newram;
	
	/* initialize x and b */
	x=(cplx **)ram;
	ram+=(LONG)n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	h=(cplx *)res; for (i=0; i<n; i++) { x[i]=h; h+=m; }
	
	b=(cplx **)ram;
	ram+=(LONG)n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	h=(cplx *)rs; for (i=0; i<n; i++) { b[i]=h; h+=m; }
	
	for (l=0; l<m; l++) {
		c_copy(b[perm[0]][l],x[0][l]);
		for (k=1; k<n; k++) {
			c_copy(b[perm[k]][l],x[k][l]);
			for (j=0; j<k; j++) {
				c_mul(c_lumat[k][j],x[j][l],t);
				c_sub(x[k][l],t,x[k][l]);
			}
		}
		c_div(x[n-1][l],c_lumat[n-1][n-1],x[n-1][l]);
		for (k=n-2; k>=0; k--) {
			sum[0]=0; sum[1]=0.0;
			for (j=k+1; j<n; j++) {
				c_mul(c_lumat[k][j],x[j][l],t);
				c_add(sum,t,sum);
			}
			c_sub(x[k][l],sum,t);
			c_div(t,c_lumat[k][k],x[k][l]);
		}
	}
	cc->newram=ram0;
}

/**************** tridiagonalization *********************/

real **mg;

void tridiag (Calc *cc, real *a, int n, int **rows)
/***** tridiag
	tridiag. a with n rows and columns.
	r[] contains the new indices of the rows.
*****/
{	char *ram=cc->newram,rh;
	real **m,maxi,*mh,lambda,h;
	int i,j,ipiv,ik,jk,k,*r;
	
	/* make a pointer array to the rows of m : */
	m=(real **)ram; ram+=n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	for (i=0; i<n; i++) { m[i]=a; a+=n; }
	r=(int *)ram; ram+=n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	for (i=0; i<n; i++) r[i]=i;
	
	/* start algorithm : */
	for (j=0; j<n-2; j++) /* need only go the (n-2)-th column */
	{	/* determine pivot */
		jk=r[j]; maxi=fabs(m[j+1][jk]); ipiv=j+1;
		for (i=j+2; i<n; i++)
		{	h=fabs(m[i][jk]);
			if (h>maxi) { maxi=h; ipiv=i; }
		}
		if (maxi<cc->epsilon) continue;
		/* exchange with pivot : */
		if (ipiv!=j+1)
		{	mh=m[j+1]; m[j+1]=m[ipiv]; m[ipiv]=mh;
			rh=r[j+1]; r[j+1]=r[ipiv]; r[ipiv]=rh;
		}
		/* zero elements */
		for (i=j+2; i<n; i++)
		{	jk=r[j]; m[i][jk]=lambda=-m[i][jk]/m[j+1][jk];
			for (k=j+1; k<n; k++) 
			{	ik=r[k]; m[i][ik]+=lambda*m[j+1][ik];
			}
			/* same for columns */
			jk=r[j+1]; ik=r[i];
			for (k=0; k<n; k++) m[k][jk]-=lambda*m[k][ik];
		}
	}
	*rows=r; mg=m;
}

cplx **cmg;

void ctridiag (Calc *cc, real *ca, int n, int **rows)
/***** tridiag
	tridiag. a with n rows and columns.
	r[] contains the new indices of the rows.
*****/
{	char *ram=cc->newram,rh;
	cplx **m,*mh,lambda,*a=(cplx *)ca,help;
	real maxi,h;
	int i,j,ipiv,ik,jk,k,*r;
	
	/* make a pointer array to the rows of m : */
	m=(cplx **)ram; ram+=n*sizeof(real *);
	if (ram>cc->udfstart) outofram();
	for (i=0; i<n; i++) { m[i]=a; a+=n; }
	r=(int *)ram; ram+=n*sizeof(cplx *);
	if (ram>cc->udfstart) outofram();
	for (i=0; i<n; i++) r[i]=i;
	
	/* start algorithm : */
	for (j=0; j<n-2; j++) /* need only go the (n-2)-th column */
	{	/* determine pivot */
		jk=r[j]; maxi=c_abs(m[j+1][jk]); ipiv=j+1;
		for (i=j+2; i<n; i++)
		{	h=c_abs(m[i][jk]);
			if (h>maxi) { maxi=h; ipiv=i; }
		}
		if (maxi<cc->epsilon) continue;
		/* exchange with pivot : */
		if (ipiv!=j+1)
		{	mh=m[j+1]; m[j+1]=m[ipiv]; m[ipiv]=mh;
			rh=r[j+1]; r[j+1]=r[ipiv]; r[ipiv]=rh;
		}
		/* zero elements */
		for (i=j+2; i<n; i++)
		{	jk=r[j];
			c_div(m[i][jk],m[j+1][jk],lambda);
			lambda[0]=-lambda[0]; lambda[1]=-lambda[1];
			c_copy(lambda,m[i][jk]);
			for (k=j+1; k<n; k++) 
			{	ik=r[k];
				c_mul(lambda,m[j+1][ik],help);
				c_add(m[i][ik],help,m[i][ik]);
			}
			/* same for columns */
			jk=r[j+1]; ik=r[i];
			for (k=0; k<n; k++)
			{	c_mul(lambda,m[k][ik],help);
				c_sub(m[k][jk],help,m[k][jk]);
			}
		}
	}
	*rows=r; cmg=m;
}

void charpoly (Calc *cc, real *m1, int n, real *p)
/***** charpoly
	compute the chracteristic polynomial of m.
*****/
{	int i,j,k,*r;
	real **m,h1,h2;
	tridiag(cc,m1,n,&r); m=mg; /* unusual global variable handling */
	/* compute the p_n rekursively : */
	m[0][r[0]]=-m[0][r[0]]; /* first one is x-a(0,0). */
	for (j=1; j<n; j++)
	{	m[0][r[j]]=-m[0][r[j]];
		for (k=1; k<=j; k++)
		{	h1=-m[k][r[j]]; h2=m[k][r[k-1]]; 
			for (i=0; i<k; i++) 
				m[i][r[j]]=m[i][r[j]]*h2+m[i][r[k-1]]*h1;
			m[k][r[j]]=h1;
		}
		for (i=0; i<j; i++) m[i+1][r[j]]+=m[i][r[j-1]];
	}
	for (i=0; i<n; i++) p[i]=m[i][r[n-1]];
	p[n]=1.0;
}

void ccharpoly (Calc *cc, real *m1, int n, real *p)
/***** charpoly
	compute the chracteristic polynomial of m.
*****/
{	int *r,i,j,k;
	cplx **m,h1,h2,g1,g2,*pc=(cplx *)p;
	ctridiag(cc,m1,n,&r); m=cmg; /* unusual global variable handling */
	/* compute the p_n rekursively : */
	m[0][r[0]][0]=-m[0][r[0]][0];
	m[0][r[0]][1]=-m[0][r[0]][1]; /* first one is x-a(0,0). */
	for (j=1; j<n; j++)
	{	m[0][r[j]][0]=-m[0][r[j]][0];
		m[0][r[j]][1]=-m[0][r[j]][1];
		for (k=1; k<=j; k++)
		{	h1[0]=-m[k][r[j]][0]; h1[1]=-m[k][r[j]][1]; 
			c_copy(m[k][r[k-1]],h2); 
			for (i=0; i<k; i++) 
			{	c_mul(h2,m[i][r[j]],g1);
				c_mul(h1,m[i][r[k-1]],g2);
				c_add(g1,g2,m[i][r[j]]);
			}
			c_copy(h1,m[k][r[j]]);
		}
		for (i=0; i<j; i++) 
		{	c_add(m[i+1][r[j]],m[i][r[j-1]],m[i+1][r[j]]);
		}
	}
	for (i=0; i<n; i++) c_copy(m[i][r[n-1]],pc[i]);
	pc[n][0]=1.0; pc[n][1]=0.0;
}

