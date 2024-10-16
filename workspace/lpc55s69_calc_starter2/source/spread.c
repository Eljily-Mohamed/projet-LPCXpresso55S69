/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * spread.c -- spread function elementwise
 *
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

#include "spread.h"

#define isreal(hd) (((hd)->type==s_real || (hd)->type==s_matrix))
#define iscomplex(hd) (((hd)->type==s_complex || (hd)->type==s_cmatrix))

header* map1 (Calc *cc, 
	void f(real *, real *),
	void fc(cplx, cplx),
	header *hd)
/***** map
	do the function elementwise to the value.
	the value may be real or complex!
******/
{	real x;
	dims *d;
	header *hd1=NULL;
	real *m,*m1;
	long i,n;
	if (hd->type==s_real) {
		f(realof(hd),&x);
		hd1=new_real(cc,x,"");
	} else if (hd->type==s_matrix) {
		d=dimsof(hd);
		hd1=new_matrix(cc,d->r,d->c,"");
		m=matrixof(hd);
		m1=matrixof(hd1);
		n=d->c*d->r;
		for (i=0; i<n; i++) {
			f(m,m1); m++; m1++;
		}
	} else if (fc && hd->type==s_complex) {
		cplx z;
		fc(cplxof(hd),z);
		hd1=new_complex(cc,z[0],z[1],"");
	} else if (fc && hd->type==s_cmatrix) {
		d=dimsof(hd);
		hd1=new_cmatrix(cc,d->r,d->c,"");
		m=matrixof(hd);
		m1=matrixof(hd1);
		n=d->c*d->r;
		for (i=0; i<n; i++) {
			fc(m,m1); m+=2; m1+=2;
		}
	} else {
		cc_error(cc,"Illegal operation");
	}
	return hd1;
}

header* map1r (Calc *cc, 
	void f(real *, real *),
	void fc(cplx, real *),
	header *hd)
/***** map
	do the function elementwise to the value.
	the value may be real or complex! the result is always real.
******/
{	real x;
	dims *d;
	header *hd1=NULL;
	real *m,*m1;
	int i,n;
	if (hd->type==s_real) {
		f(realof(hd),&x);
		hd1=new_real(cc,x,"");
	} else if (hd->type==s_matrix) {
		d=dimsof(hd);
		hd1=new_matrix(cc,d->r,d->c,"");
		m=matrixof(hd);
		m1=matrixof(hd1);
		n=d->c*d->r;
		for (i=0; i<n; i++) {
			f(m,m1); m++; m1++;
		}
	} else if (fc && hd->type==s_complex) {
		fc(cplxof(hd),&x);
		hd1=new_real(cc,x,"");
	} else if (fc && hd->type==s_cmatrix) {
		d=dimsof(hd);
		hd1=new_matrix(cc,d->r,d->c,"");
		m=matrixof(hd);
		m1=matrixof(hd1);
		n=d->c*d->r;
		for (i=0; i<n; i++) {
			fc(m,m1); m+=2; m1++;
		}
	} else {
		cc_error(cc,"Illegal operation");
	}
	return hd1;
}

header* map2 (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, cplx),
	header *hd1, header *hd2)
/**** map2
    calculate the result of a binary operator applied to hd1 and
    hd2, selecting the right operator (in R or C). If hd1 or hd2 
    are matrices, the operator is applied elementwise
 ****/
{	int t1=0,t2=0,t,r1,c1,r2,c2,rr,cr,r,c; /* means real */
	real *m1,*m2,*m,x,*l1,*l2;
	cplx z;
	header *result;
	if (isreal(hd1)) t1=0;
	else if (iscomplex(hd1)) t1=1;
	else {
		cc_error(cc,"Can't operate on non numerical value");
	}
	if (isreal(hd2)) t2=0;
	else if (iscomplex(hd2)) t2=1;
	else {
		cc_error(cc,"Can't operate on non numerical value");
	}
	if ( (t1==0 && t2==0 && !f) || (!fc && (t1==1 || t2==1)) ) {
		cc_error(cc,"Cannot evaluate this operation.");
	}
	getmatrix(hd1,&r1,&c1,&m1); l1=m1;
	getmatrix(hd2,&r2,&c2,&m2); l2=m2;
//	if (r1==0 || r2==0 || c1==0 || c2==0) cc_error(cc, "can't eval operation with null matrix");
	if (r1==0 || r2==0 || c1==0 || c2==0) cc_warn(cc, "empty matrix involved in calculation");
	if ((r1>1 && r2>1 && (r1!=r2)) || (c1>1 && c2>1 && (c1!=c2))) {
	    cc_error(cc,"Cannot combine these matrices!");
	}
	if (r1 && r2) {
		rr = (r1>r2) ? r1 : r2;
	} else {
		rr = 0;
	}
	if (c1 && c2) {
		cr = (c1>c2) ? c1 : c2;
	} else {
		cr = 0;
	}
	t=t1;
	if (t2!=0) t=t2;
	switch (t) {
		case 0 :
			if (rr==1 && cr==1) {
				f(m1,m2,&x);
				return new_real(cc,x,"");
			}
			result=new_matrix(cc,rr,cr,"");
			m=matrixof(result);
			for (r=0; r<rr; r++) {
				for (c=0; c<cr; c++) {
					f(m1,m2,m);
					if (c1>1) m1++;
					if (c2>1) m2++;
					m++;
				}
				if (r1==1) m1=l1;
				else if (c1==1) m1++;
				if (r2==1) m2=l2;
				else if (c2==1) m2++;
			}
			return result;
		case 1 :
			if (rr==1 && cr==1) {
				if (t1==0) {
					cplx a={*m1,0.0};
					fc(a,m2,z);
				} else if (t2==0) {
					cplx a={*m2,0.0};
					fc(m1,a,z);
				} else fc(m1,m2,z);
				return new_complex(cc,z[0],z[1],"");
			}
			result=new_cmatrix(cc,rr,cr,"");
			m=matrixof(result);
			for (r=0; r<rr; r++) {
				for (c=0; c<cr; c++) {
					if (t1==0) {
						cplx a={*m1,0.0};
						fc(a,m2,m);
						if (c1>1) m1++;
						if (c2>1) m2+=2;
					} else if (t2==0) {
						cplx a={*m2,0.0};
						fc(m1,a,m);
						if (c1>1) m1+=2;
						if (c2>1) m2++;
					} else {
						fc(m1,m2,m);
						if (c1>1) m1+=2;
						if (c2>1) m2+=2;
					}
					m+=2;
				}
				if (r1==1) {
					m1=l1;
				} else if (c1==1) {
					if (t1==0) m1++;
					else m1+=2;
				}
				if (r2==1) {
					m2=l2;
				} else if (c2==1) {
					if (t2==0) m2++;
					else m2+=2;
				}
			}
			return result;
	}
	return NULL;
}

header* map2r (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, real *),
	header *hd1, header *hd2)
{	int t1=0,t2=0,t,r1,c1,r2,c2,r,c,rr,cr; /* means real */
	real *m1,*m2,*m,x,*l1,*l2;
	header *result;
	if (isreal(hd1)) t1=0;
	else if (iscomplex(hd1)) t1=1;
	else {
		cc_error(cc,"Can't operate on non numerical value");
	}
	if (isreal(hd2)) t2=0;
	else if (iscomplex(hd2)) t2=1;
	else {
		cc_error(cc,"Can't operate on non numerical value");
	}
	if ( (t1==0 && t2==0 && !f) || (!fc && (t1==1 || t2==1)) ) {
		cc_error(cc,"Cannot evaluate this operation.");
	}
	getmatrix(hd1,&r1,&c1,&m1); l1=m1;
	getmatrix(hd2,&r2,&c2,&m2); l2=m2;
//	if (r1==0 || r2==0 || c1==0 || c2==0) cc_error(cc, "can't eval operation with null matrix");
	if ((r1>1 && r2>1 && (r1!=r2)) ||
	 (c1>1 && c2>1 && (c1!=c2))) {
	    cc_error(cc,"Cannot combine these matrices!");
	}
	if (r1 && r2) {
		rr = (r1>r2) ? r1 : r2;
	} else {
		rr = 0;
	}
	if (c1 && c2) {
		cr = (c1>c2) ? c1 : c2;
	} else {
		cr = 0;
	}
	t=t1; if (t2!=0) t=t2;
	switch (t) {
		case 0 :
			if (rr==1 && cr==1) {
				f(m1,m2,&x);
				return new_real(cc,x,"");
			}
			result=new_matrix(cc,rr,cr,"");
			m=matrixof(result);
			for (r=0; r<rr; r++) {
				for (c=0; c<cr; c++) {
					f(m1,m2,m);
					if (c1>1) m1++;
					if (c2>1) m2++;
					m++;
				}
				if (r1==1) m1=l1;
				else if (c1==1) m1++;
				if (r2==1) m2=l2;
				else if (c2==1) m2++;
			}
			return result;
		case 1 :
			if (rr==1 && cr==1) {
				if (t1==0) {
					cplx a={*m1,0.0};
					fc(a,m2,&x);
				} else if (t2==0) {
					cplx a={*m2,0.0};
					fc(m1,a,&x);
				} else fc(m1,m2,&x);
				return new_real(cc,x,"");
			}
			result=new_matrix(cc,rr,cr,"");
			m=matrixof(result);
			for (r=0; r<rr; r++) {
				for (c=0; c<cr; c++) {
					if (t1==0) {
						cplx a={*m1,0.0};
						fc(a,m2,m);
						if (c1>1) m1++;
						if (c2>1) m2+=2;
					} else if (t2==0) {
						cplx a={*m2,0.0};
						fc(m1,a,m);
						if (c1>1) m1+=2;
						if (c2>1) m2++;
					} else {
						fc(m1,m2,m);
						if (c1>1) m1+=2;
						if (c2>1) m2+=2;
					}
					m++;
				}
				if (r1==1) m1=l1;
				else if (c1==1) {
					if (t1==0) m1++;
					else m1+=2;
				}
				if (r2==1) m2=l2;
				else if (c2==1) {
					if (t2==0) m2++;
					else m2+=2;
				}
			}
			return result;
	}
	return 0;
}


header* spread2 (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, cplx),
	header *hd)
{	header *result,*hd1;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	result=map2(cc,f,fc,hd,hd1);
	return pushresults(cc,result);
}

header* spread2r (Calc *cc, 
	void f (real *, real *, real *),
	void fc (cplx, cplx, real *),
	header *hd)
{	header *result,*hd1;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd);	hd1=getvalue(cc,hd1);
	result=map2r(cc,f,fc,hd,hd1);
	return pushresults(cc,result);
}

static real (*func) (real);

static void funceval (real *x, real *y)
/* evaluates the function stored in func with pointers. */
{	*y=func(*x);
}

header* spread1 (Calc *cc, 
	real f (real),
	void fc (cplx, cplx),
	header *hd)
{	header *result;
	hd=getvalue(cc,hd);
	func=f;
	result=map1(cc,funceval,fc,hd);
	return pushresults(cc,result);
}

header* spread1r (Calc *cc, 
	real f (real),
	void fc (cplx, real *),
	header *hd)
{	header *result;
	hd=getvalue(cc,hd);
	func=f;
	result=map1r(cc,funceval,fc,hd);
	return pushresults(cc,result);
}
