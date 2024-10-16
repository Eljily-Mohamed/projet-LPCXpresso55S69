/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * stack.c
 *
 ****************************************************************/
#include <stdlib.h>
#include <string.h>

#include "calc.h"
#include "funcs.h"

/* Data stack organization

	Original scheme
	===============

	----------------------------- <-- ramend      |local scope |global scope
	                                              |   active   |  active	   
	            free                              |            |
	                                              |            |
	----------------------------- <-- newram      |            |
	   transient calculus stack                   |            |
	----------------------------- <--             | [endlocal] |
	running function local scope                  |            |
	----------------------------- <-- globalend   |[startlocal]| [endlocal]          
	      global variables                        |            |
	----------------------------- <-- udfend      |            |[startlocal]
    udf (user defined functions)
	----------------------------- <-- ramstart
	
	
	Alternate scheme 1
	==================
	
	----------------------------- <-- ramend      |local scope |global scope
	udf (user defined functions)                  |   active   |  active
	----------------------------- <-- udfstart    |            | [endlocal]          
	global variables                              |            |
	----------------------------- <-- globalstart | [endlocal] |[startlocal]
	running function local scope                  |            |
	----------------------------- <--             [[startlocal]|
	   
	   
	            free
	   
	----------------------------- <-- newram
	   transient calculus stack
	----------------------------- <-- ramstart


	Alternate scheme 2 : current scheme
	==================
	
	----------------------------- <-- ramend      |local scope |global scope
	 udf (user defined functions)                 |   active   |  active	   
	----------------------------- <-- udfstart    |            |
	                                              |            |
	            free                              |            |
	                                              |            |
	----------------------------- <-- newram      |            |
	   transient calculus stack                   |            |
	----------------------------- <--             | [endlocal] |
	running function local scope                  |            |
	----------------------------- <-- globalend   |[startlocal]| [endlocal]          
	      global variables                        |            |
	----------------------------- <-- ramstart    |            |[startlocal]
    

   The stack is used for transient calculus and to store variables and user
   defined functions.
   
   the running function local variables are transient and exist only while
   the function is executed.
   
   startlocal and endlocal relates either to the current running local scope 
   or to the global scope when an expression is evaluated at the global level 
   of the interpreter.
   
 */


int stack_init(Calc *cc, unsigned long stacksize)
/***** stack_init
   allocate stack and initialize global variables
 *****/
{
	cc->ramstart = (char*)malloc(stacksize);
	if (cc->ramstart) {
		cc->globalstart=cc->globalend=cc->ramstart;
		cc->startlocal=cc->endlocal=cc->ramstart;
		cc->newram=cc->ramstart;
		cc->udfstart=cc->udfend=cc->ramend=cc->ramstart+stacksize;
		
		return 1;
	}
	
	return 0;
}

void stack_rewind(Calc *cc)
{
	cc->startlocal=cc->globalstart;
	cc->newram=cc->globalend=cc->endlocal;
}

int xor (char *n)
/***** xor
	compute a hashcode for the name n.
 *****/
{	int r=0;
	while (*n) r^=*n++;
	return r;
}

void* stack_alloc (Calc *cc, stacktyp type, int size, char *name)
/***** make_header
	push a new element on the stack.
	- type: data type
	- size: data size requested (without the header)
	- name: optionnal name (NULL if none) 
	return the position after the header.
******/
{	header* hd;
	void* erg;
	/* align size to a multiple of the ALIGNMENT parameter */
	size=ALIGN(size);
	
	hd=(header *)(cc->newram);
	if (cc->newram+size+sizeof(header)>cc->udfstart) cc_error(cc,"Stack overflow!");
	hd=(header *)cc->newram;
	hd->size=size+sizeof(header);
	hd->type=type;
	hd->flags=0;
	if (name) {
		strncpy(hd->name,name,LABEL_LEN_MAX);
		hd->name[LABEL_LEN_MAX]=0;
		hd->xor=xor(name);
	} else {
		*(hd->name)=0;
		hd->xor=0;
	}
	erg=cc->newram+sizeof(header);
	cc->newram+=size+sizeof(header);
	return erg;
}

/***** stack_realloc
	expand allocation on the stack
 *****/
void* stack_realloc(Calc *cc, header* hd, int size)
{
	int sz=sizeof(header);
	char *p=(char*)(hd+1), *res;
	
	switch (hd->type) {
	case s_matrix:
	case s_cmatrix:
		sz+=sizeof(dims);
		p+=sizeof(dims);
		break;
	default:
		break;
	}
	
	res=p;
	
	size=ALIGN(size);
	
	if ((char*)(p+size)<=cc->udfstart) {
		hd->size=sz+size;
		cc->newram=(char*)hd+hd->size;
	} else cc_error(cc, "Stack overflow!");

	return res;
}

header *new_real (Calc *cc, real x, char *name)
/***** new real
	push a real on stack.
*****/
{
	header* hd=(header *)cc->newram;
	real* d=(real *)stack_alloc(cc,s_real,sizeof(real),name);
	*d=x;
	return hd;
}

header *new_complex (Calc *cc, real re, real im, char *name)
/***** new real
	push a complex on stack.
*****/
{
	header *hd=(header *)cc->newram;
	real* d=(real *)stack_alloc(cc,s_complex,2*sizeof(real),name);
	*d=re; *(d+1)=im;
	return hd;
}

header* new_string(Calc *cc)
{
	int len=0;
	header *hd=(header*)cc->newram;
	char* d=(char*)stack_alloc(cc,s_string,0,"");	/* destination */
	char *next = cc->next, *s=cc->next;	/* s beginning of the string */
	char c;

	while (1) {
		if ((c=*next++)=='\\' && *(next)=='\"') next++;	/* count escaped character \" */
		else if (c=='\"' || c==0)  break;
		else if (CC_ISSET(cc,CC_USE_UTF8)) {
			if ((c & 0x80)==0) ;
			else if ((c & 0xE0)==0xC0 && ((*next & 0xC0)==0x80)) next++;
			else if ((c & 0xF0)==0xE0) {
				int n=2;
				while (n && (c=*next)!=0 && (c & 0xC0)==0x80) {n--; next++;}
				if (n) goto err;
			} else if ((c & 0xF8)==0xF0) {
				int n=3;
				while (n && (c=*next)!=0 && (c & 0xC0)==0x80) {n--; next++;}
				if (n) goto err;
			} else if ((c & 0xFC)==0xF8) {
				int n=4;
				while (n && (c=*next)!=0 && (c & 0xC0)==0x80) {n--; next++;}
				if (n) goto err;
			} else {
				goto err;
			}
		}
	}
	if (c!='\"') cc_error(cc,"\" missing");
	len=ALIGN((int)(next-s));
	if (d+len<=cc->udfstart) {			/* adjust object size */
		hd->size+=len;
	} else cc_error(cc,"Stack overflow!");
	/* de-escape \" */
	for (char *x=s; x!=next-1; d++,x++) {
		if (*x=='\\' && *(x+1)=='\"') {
			x++;
			len--;
		}
		*d=*x;
	}
	*d=0;
	cc->newram=(char*)hd+hd->size;
	cc->next=next;
	return hd;
	
err:
	cc->newram=(char*)hd;
	cc->next=next;
	cc_error(cc,"Invalid UTF-8 character");
	return NULL;
}

header *new_cstring (Calc *cc, char *s, int length, char *name)
/***** new real
	push a string on stack.
*****/
{
	header *hd=(header *)cc->newram;
	char* d=(char *)stack_alloc(cc,s_string,length+1,name);
	strncpy(d,s,length); d[length]=0;
	return hd;
}

header *new_matrix (Calc *cc, int rows, int columns, char *name)
/***** new_matrix
	push a new matrix on the stack.
*****/
{
	header *hd=(header *)cc->newram;
	dims* d=(dims *)stack_alloc(cc,s_matrix,matrixsize(rows,columns),name);
	d->c=columns; d->r=rows;
	return hd;
}

header *new_cmatrix (Calc *cc, int rows, int columns, char *name)
/***** new_cmatrix
	push a new complex matrix on the stack.
*****/
{
	header *hd=(header *)cc->newram;
	dims* d=(dims *)stack_alloc(cc,s_cmatrix,matrixsize(rows,2*columns),name);
	d->c=columns; d->r=rows;
	return hd;
}

header *new_command (Calc *cc, int no)
/***** new_command
	push a command on stack.
*****/
{
	header *hd=(header *)cc->newram;
	int *d=(int *)stack_alloc(cc,s_command,sizeof(int),"");
	*d=no;
	return hd;
}

header *new_udf (Calc *cc, char *name)
/***** new real
	pops a udf on stack.
*****/
{
	header *hd=(header *)cc->newram;
	ULONG *d=(ULONG *)stack_alloc(cc,s_udf,sizeof(ULONG)+sizeof(int)+sizeof(unsigned int),name);
	*d=sizeof(header)+sizeof(ULONG)+sizeof(int)+sizeof(unsigned int);
	return hd;
}

header* new_reference (Calc *cc, header *ref, char *name)
/***** new_reference
	push a new reference (pointer) to a variable on the stack.
*****/
{
	header* hd=(header *)cc->newram;
	header** d=(header**)stack_alloc(cc,s_reference,sizeof(header *),name);
	*d=ref;
	return hd;
}

header* new_funcref (Calc *cc, header *ref, char *name)
/***** new_reference
	push a new reference (pointer) to a variable on the stack.
*****/
{
	header* hd=(header *)cc->newram;
	header** d=(header**)stack_alloc(cc,s_funcref,sizeof(header *),name);
	*d=ref;
	return hd;
}

header* new_binfuncref (Calc *cc, binfunc_t *ref, char *name)
/***** new_reference
	push a new reference (pointer) to a variable on the stack.
*****/
{
	header* hd=(header *)cc->newram;
	binfunc_t** d=(binfunc_t**)stack_alloc(cc,s_funcref,sizeof(binfunc_t *),name);
	*d=ref;
	hd->flags=FLAG_BINFUNC;
	return hd;
}

header *new_subm (Calc *cc, header *var, ULONG l, char *name)
/* make a new submatrix reference, which structure is
     header  : name, s_submatrixref
     header* : pointer to the matrix
     dims    : dims of the submatrix (special case r=1, c=1)
     int[2]  : row and col indexes in the original matrix
 */
{
	header **d,*hd=(header *)cc->newram;
	dims *dim;
	int *n,r,c;
	ULONG size=sizeof(header *)+sizeof(dims)+2*sizeof(int);
	d=(header **)stack_alloc(cc,s_submatrixref,size,name);
	*d=var;
	dim=(dims *)(d+1);
	dim->r=1; dim->c=1;
	n=(int *)(dim+1);
	c=dimsof(var)->c;
	if (c==0 || dimsof(var)->r==0) cc_error(cc,"Matrix is empty!");
	r=(int)(l/c);
	*n++=r;
	*n=(int)(l-(ULONG)r*c-1);
	return hd;
}

header *new_csubm (Calc *cc, header *var, ULONG l, char *name)
/* make a new submatrix reference, which structure is
     header  : name, s_submatrixref
     header* : pointer to the matrix
     dims    : dims of the submatrix (special case r=1, c=1)
     int[2]  : row and col indexes in the original matrix
 */
{
	header **d,*hd=(header *)cc->newram;
	dims *dim;
	int *n,r,c;
	ULONG size=sizeof(header *)+sizeof(dims)+2*sizeof(int);
	d=(header **)stack_alloc(cc,s_csubmatrixref,size,name);
	*d=var;
	dim=(dims *)(d+1);
	dim->r=1; dim->c=1;
	n=(int *)(dim+1);
	c=dimsof(var)->c;
	if (c==0 || dimsof(var)->r==0) cc_error(cc,"Matrix is empty!");
	r=(int)(l/c);
	*n++=r;
	*n=(int)(l-r*c-1);
	return hd;
}

static header *new_submatrixref (Calc *cc, header *var, header *rows, header *cols, 
	char *name, int type)
/* make a new submatrix reference (general case), which structure is
     header  : name, s_submatrixref
     header* : pointer to the matrix
     dims    : dims of the submatrix (nb of row indexes, nb of col indexes)
     int[]   : indexes of rows, followed by indexes of cols in the original
               matrix.
 */
{	ULONG size;
	header **d;
	real *mr=NULL,*mc=NULL,x,*mvar;
	dims *dim;
	int c=0,r=0,*n,i,c0,r0,cvar,rvar,allc=0,allr=0;
	header *hd=(header *)cc->newram;
	getmatrix(var,&rvar,&cvar,&mvar);
	/* analyze row indexes */
	if (rows->type==s_matrix) {
		if (dimsof(rows)->r==1) r=dimsof(rows)->c;
		else if (dimsof(rows)->c==1) r=dimsof(rows)->r;
		else cc_error(cc,"Illegal index!");
		mr=matrixof(rows);
	} else if (rows->type==s_real) {
		r=1; mr=realof(rows);
	} else if (rows->type==s_command && *commandof(rows)==c_allv) {
		allr=1; r=rvar;
	} else {
		cc_error(cc,"Illegal index!");
	}
	/* analyze col indexes */
	if (cols->type==s_matrix) {
		if (dimsof(cols)->r==1) c=dimsof(cols)->c;
		else if (dimsof(cols)->c==1) c=dimsof(cols)->r;
		else cc_error(cc,"Illegal index!");
		mc=matrixof(cols);
	} else if (cols->type==s_real) {
		c=1; mc=realof(cols);
	} else if (cols->type==s_command && *commandof(cols)==c_allv) {
		allc=1; c=cvar;
	} else {
		cc_error(cc,"Illegal index!");
	}
	
	size=sizeof(header *)+sizeof(dims)+((ULONG)r+c)*sizeof(int);
	d=(header **)stack_alloc(cc,type,size,name);		/* pointer to header* field */
	dim = (dims *)(d+1);							/* pointer to dims field */
	n=(int *)(dim+1);								/* pointer to index field */
	/* set pointer to the matrix header */
	*d=var;
	/* push row indexes (index field), check for index validity */
	r0=0;
	if (allr) {
		hd->flags|=FLAG_SUBMALLR;
		for (i=0; i<rvar; i++) *n++=i;
		r0=rvar;
	} else for (i=0; i<r; i++) {
		x=(*mr++)-1;
		if (!((x<0.0) || (x>=rvar)) ) {
			*n++=(int)x; r0++;
		}
	}
	/* push col indexes (index field), check for index validity */
	c0=0;
	if (allc) {
		hd->flags|=FLAG_SUBMALLC;
		for (i=0; i<cvar; i++) *n++=i;
		c0=cvar;
	} else for (i=0; i<c; i++) {
		x=(*mc++)-1;
		if (!((x<0.0) || (x>=cvar))) {
			*n++=(int)x; c0++;
		}
	}
	/* set the size of the submatrix: nb rows, nb cols */
	dim->r=r0; dim->c=c0;
	/* adjust size (some indexes may have been rejected), and newram accordingly */
	size=(char *)n-(char *)hd;
	size=ALIGN(size);
	cc->newram=(char *)hd+size;
	hd->size=size;
	
	return hd;
}

static header *build_csmatrix (Calc *cc, header *var, header *rows, header *cols)
/***** built_csmatrix
	built a complex submatrix from the matrix hd on the stack.
*****/
{	real *mr=NULL,*mc=NULL,*mvar,*mh,*m;
	int n,c=0,r=0,c0,r0,i,j,cvar,rvar,allc=0,allr=0,*pc,*pr,*nc,*nr;
	header *hd;
	char *ram;
	getmatrix(var,&rvar,&cvar,&mvar);
	/* analyze row indexes */
	if (rows->type==s_matrix) {
		if (dimsof(rows)->r==1) r=dimsof(rows)->c;
		else if (dimsof(rows)->c==1) r=dimsof(rows)->r;
		else cc_error(cc,"Illegal index!");
		mr=matrixof(rows);
	} else if (rows->type==s_real) {
		r=1; mr=realof(rows);
	} else if (rows->type==s_command && *commandof(rows)==c_allv) {
		allr=1; r=rvar;
	} else {
		cc_error(cc,"Illegal index!");
	}
	/* analyze col indexes */
	if (cols->type==s_matrix) {
		if (dimsof(cols)->r==1) c=dimsof(cols)->c;
		else if (dimsof(cols)->c==1) c=dimsof(cols)->r;
		else cc_error(cc,"Illegal index!");
		mc=matrixof(cols);
	} else if (cols->type==s_real) {
		c=1; mc=realof(cols);
	} else if (cols->type==s_command && *commandof(cols)==c_allv) {
		allc=1; c=cvar;
	} else {
		cc_error(cc,"Illegal index!");
	}
	
	ram=cc->newram;
	if (ram+((ULONG)(c)+(ULONG)(r))*sizeof(int)>cc->udfstart) {
		cc_error(cc,"Out of memory!");
	}
	nr=pr=(int *)ram; nc=pc=pr+r; cc->newram=(char *)(pc+c);
	c0=0; r0=0;
	if (allc) { for (i=0; i<c; i++) pc[i]=i; c0=c; }
	else for (i=0; i<c; i++)
	{	n=(int)(*mc++)-1;
		if (n>=0 && n<cvar) { *nc++=n; c0++; }
	}
	if (allr) { for (i=0; i<r; i++) pr[i]=i; r0=r; }
	else for (i=0; i<r; i++) 
	{	n=(int)(*mr++)-1;
		if (n>=0 && n<rvar) { *nr++=n; r0++; }
	}
	if (c0==1 && r0==1)
	{	m=cmat(mvar,cvar,pr[0],pc[0]);
		return new_complex(cc,*m,*(m+1),"");
	}
	hd=new_cmatrix(cc,r0,c0,"");
	m=matrixof(hd);
	for (i=0; i<r0; i++)
		for (j=0; j<c0; j++)
		{	mh=cmat(mvar,cvar,pr[i],pc[j]);
			*m++=*mh++;
			*m++=*mh;
		}
	return hd;
}

static header *build_smatrix (Calc *cc, header *var, header *rows, header *cols)
/***** built_smatrix
	built a submatrix from the matrix hd on the stack.
*****/
{	real *mr=NULL,*mc=NULL,*mvar,*m;
	int n,c=0,r=0,c0,r0,i,j,cvar,rvar,allc=0,allr=0,*pr,*pc,*nc,*nr;
	header *hd;
	char *ram;
	getmatrix(var,&rvar,&cvar,&mvar);
	if (rows->type==s_matrix) {
		if (dimsof(rows)->r==1) r=dimsof(rows)->c;
		else if (dimsof(rows)->c==1) r=dimsof(rows)->r;
		else cc_error(cc,"Illegal index!");
		mr=matrixof(rows);
	} else if (rows->type==s_real) {
		r=1; mr=realof(rows);
	} else if (rows->type==s_command && *commandof(rows)==c_allv) {
		allr=1; r=rvar;
	} else {
		cc_error(cc,"Illegal index!");
	}
	if (cols->type==s_matrix) {
		if (dimsof(cols)->r==1) c=dimsof(cols)->c;
		else if (dimsof(cols)->c==1) c=dimsof(cols)->r;
		else cc_error(cc,"Illegal index!");
		mc=matrixof(cols);
	} else if (cols->type==s_real) {
		c=1; mc=realof(cols);
	} else if (cols->type==s_command && *commandof(cols)==c_allv) {
		allc=1; c=cvar;
	} else {
		cc_error(cc,"Illegal index!");
	}
	ram=cc->newram;
	if (ram+((ULONG)(c)+(ULONG)(r))*sizeof(int)>cc->udfstart) {
		cc_error(cc,"Out of memory!");
	}
	nr=pr=(int *)ram; nc=pc=pr+r; cc->newram=(char *)(pc+c);
	c0=0; r0=0;
	if (allc) { for (i=0; i<c; i++) pc[i]=i; c0=c; }
	else for (i=0; i<c; i++) {
		n=(int)(*mc++)-1;
		if (n>=0 && n<cvar) { *nc++=n; c0++; }
	}
	if (allr) { for (i=0; i<r; i++) pr[i]=i; r0=r; }
	else for (i=0; i<r; i++) {
		n=(int)(*mr++)-1;
		if (n>=0 && n<rvar) { *nr++=n; r0++; }
	}
	if (c0==1 && r0==1)	{
		return new_real(cc,*mat(mvar,cvar,pr[0],pc[0]),"");
	}
	hd=new_matrix(cc,r0,c0,"");
	m=matrixof(hd);
	for (i=0; i<r0; i++)
		for (j=0; j<c0; j++)
			*m++=*mat(mvar,cvar,pr[i],pc[j]);
	return hd;
}

header *new_submatrix (Calc *cc, header *hd, header *rows, header *cols, 
	char *name)
{	if (CC_ISSET(cc,CC_NOSUBMREF)) return build_smatrix(cc,hd,rows,cols);
	return new_submatrixref(cc,hd,rows,cols,name,s_submatrixref);
}

header *new_csubmatrix (Calc *cc, header *hd, header *rows, header *cols, 
	char *name)
{	if (CC_ISSET(cc,CC_NOSUBMREF)) return build_csmatrix(cc,hd,rows,cols);
	return new_submatrixref(cc,hd,rows,cols,name,s_csubmatrixref);
}

/* defined in parse.c: do we allow to search a name in global 
   scope when we are in a user defined function.
 */
void getmatrix (header *hd, int *r, int *c, real **m)
/***** getmatrix
	get rows and columns from a matrix.
*****/
{	dims *d;
	if (hd->type==s_real || hd->type==s_complex) {
		*r=*c=1;
		*m=realof(hd);
	} else {
		d=dimsof(hd);
		*m=matrixof(hd);
		*r=d->r; *c=d->c;
	}
}

header *searchvar (Calc *cc, char *name)
/***** searchvar
	search a local variable, named "name".
	return 0, if not found.
*****/
{	int r;
	if (name[0]!='$') {
		header *hd=(header *)cc->startlocal;
		r=xor(name);
		while ((char *)hd<cc->endlocal) {
			if (r==hd->xor && !strcmp(hd->name,name)) return hd;
			hd=nextof(hd);
		}
		if (cc->globalstart!=cc->startlocal && CC_ISSET(cc,CC_SEARCH_GLOBALS)) {
			hd=(header *)cc->globalstart;
			while ((char *)hd<cc->globalend) {
				if (r==hd->xor && !strcmp(hd->name,name)) return hd;
				hd=nextof(hd);
			}
		}
	} else {
		name=name+1;
		r=xor(name);
		header *hd=(header *)cc->globalstart;
		while ((char *)hd<cc->globalend) {
			if (r==hd->xor && !strcmp(hd->name,name)) return hd;
			hd=nextof(hd);
		}
	}
	return NULL;
}

header *searchudf (Calc *cc, char *name)
/***** searchudf
	search a udf, named "name".
	return 0, if not found.
*****/
{	char r=xor(name);
	
	header* hd;
	if ((hd=searchvar(cc,name))!=NULL) {
#ifndef PRIO_TO_UDF
		/* allow light user functions to have precedence over 
		   user defines functions and binary functions (useful to
		   be sure that light user functions passed as parameters
		   will be called even if there exists functions withe the
		   same name
		 */
		if (hd->type==s_string) return hd;	/* light user function */
#endif
		if (hd->flags & FLAG_BINFUNC) {		/* bin_func_ref */
			return hd;
		} else if (hd->type==s_reference){	/* may be a ref on a bin_func_ref? */
			while (hd && hd->type==s_reference) hd=referenceof(hd);
			if (hd->flags & FLAG_BINFUNC) return hd;
#ifndef PRIO_TO_UDF
			if (hd->type==s_string) return hd;	/* light user function */
#endif
		}
		/* may be a reference to a udf? */
		while (hd && hd->type==s_funcref) hd=referenceof(hd);
		if (hd->type==s_udf) return hd;
	}
	/* standard udf? */
	hd=(header *)cc->udfstart;
	while ((char *)hd<cc->udfend)
	{	if (hd->type==s_udf && r==hd->xor && !strcmp(hd->name,name)) return hd;
		hd=nextof(hd);
	}
	/* none found */
	return NULL;
}

int kill_local (Calc *cc, char *name)
/***** kill_local
	kill a local variable name, if there is one. returns operation done
*****/
{	LONG size,rest;
	header *hd=(header *)cc->startlocal;
	while ((char *)hd<cc->endlocal) {
		char r=xor(name);
		if (r==hd->xor && !strcmp(hd->name,name) && !(hd->flags & FLAG_CONST)) {
			/* found! */
			size=hd->size;
			rest=cc->newram-(char *)hd-size;
			if (size) {
				memmove((char *)hd,(char *)hd+size,rest);
				cc->endlocal-=size; cc->newram-=size;
			}
			return 1;
		}
		hd=(header *)((char *)hd+hd->size);
	}
	return 0;
}

int kill_udf (Calc *cc, char *name)
/***** kill_udf
	kill a user function named 'name', if any. returns operation done
*****/
{	LONG size,rest;
	header *hd=(header *)cc->udfstart;
	while ((char *)hd<cc->udfend) {
		char r=xor(name);
		if (r==hd->xor && !strcmp(hd->name,name)) {
			/* found! */
			size=hd->size;
			rest=(char *)hd-cc->udfstart;
			/* remove references to the killed function */
			header *h1=(header*)cc->startlocal;
			while (h1!=(header*)cc->endlocal) {
				if (h1->type==s_funcref && referenceof(h1)==hd) {
					/* this variable references the function, remove the variable */
					LONG sz=h1->size, rem=cc->newram-(char *)h1-sz;
					if (rem) memmove((char *)h1,(char *)h1+sz,rem);
					cc->endlocal-=sz; cc->newram-=sz;
				} else h1=nextof(h1);
			}
			if (size && rest) {
				for (header*h=(header*)cc->udfstart; h!=hd; h=nextof(h)) {
					for (header *h1=(header*)cc->startlocal;h1!=(header*)cc->endlocal;h1=nextof(h1)) {
						if (h1->type==s_funcref && referenceof(h1)==h) {
							referenceof(h1)=(header*)((char*)h+size);
						}
					}
				}
				memmove(cc->udfstart+size,cc->udfstart,rest);
			}
			cc->udfstart+=size;
			return 1;
		}
		hd=(header *)((char *)hd+hd->size);
	}
	return 0;
}

#if 0
static int sametype (header *hd1, header *hd2)
/***** sametype
	returns true, if hd1 and hd2 have the same type and dimensions.
*****/
{	dims *d1,*d2;
	if (hd1->type==s_string && hd2->type==s_string)
    	return hd1->size>=hd2->size;
	if (hd1->type!=hd2->type || hd1->size!=hd2->size) return 0;
	if (hd1->type==s_matrix || hd1->type==s_cmatrix) {
		d1=dimsof(hd1); d2=dimsof(hd2);
		if (d1->r!=d2->r) return 0;
	}
	return 1;
}
#endif

header *assign (Calc *cc, header *var, header *value)
/***** assign
	assign the value to the variable.
*****/
{	char name[LABEL_LEN_MAX+1],*nextvar;
	LONG size,dif;
	real *m,*mv,*m1,*m2;
	int i,j,k,c,r,cv,rv,*rind,*cind;
	dims *d;
	header *help,*orig;
	value=getvalue(cc, value);
	size=value->size;
	if(var->name[0]=='$' && CC_ISSET(cc,CC_EXEC_UDF)) cc_error(cc, "assignment of globals forbidden in functions");
	if (var->type==s_reference && !referenceof(var)) {
		/* may be a new variable or udf (udf are always cleared before being redefined) */
		strcpy(name,var->name);
		if (value->type==s_udf) {
			/* assign a new udf with the value (body of the udf) */
			strcpy(value->name,name);
			value->xor=xor(name);
			if (cc->newram+size>cc->udfstart) {
				cc_error(cc,"Memory overflow while assigning user function %s.",var->name);
			}
			cc->udfstart-=size;
			memmove(cc->udfstart,(char *)value,size);
			return (header *)cc->udfstart;
		}
		/* else, assign a new variable with the value */
		if (cc->newram+size>cc->udfstart) cc_error(cc,"Memory overflow while assigning variable %s.", var->name);
		/* shift the transient memory by size to make room for the new variable
		   (necessary to deal with multiple assignment) */
		memmove(cc->endlocal+size,cc->endlocal,cc->newram-cc->endlocal);
		/* update address if an intermediate result, else do nothing 
		   (value is referencing an existing variable) */
		var=(header*)((char*)var+size);
		if ((char*)value>cc->endlocal) {
			value=(header *)((char *)value+size);
			cc->newram+=size;
		}
		memmove(cc->endlocal,(char *)value,size);
		value=(header *)cc->endlocal;
		cc->endlocal+=size;
		value->flags &= ~FLAG_CONST;
		strcpy(value->name,name);value->xor=xor(value->name);
		return value;
	} else {
		/* the variable already exists: may be
		   - a udf reference: TO COMMENT
		   - a submatrix reference: we can only change elements, but not the size
		   - a whole variable: if inside local scope, the variable can be changed,
		     even resized, if outside local scope, content can be changed if 
		     current size is compatible with new data size.
		 */
		while (var) {
			if (var->flags & FLAG_CONST) cc_error(cc,"variable has const status (can't write)!");
			if (var->type==s_reference && !(var->flags & FLAG_COPYONWRITE))
				var=referenceof(var);
			else break;
		}
//		if (var->flags & FLAG_CONST) cc_error(cc,"can't write a constant!");
		
//		while (var && var->type==s_reference) var=referenceof(var);
		if (!var) cc_error(cc,"Internal variable error!");
//		if (var->type!=s_udf && value->type==s_udf)
//			cc_error(cc,"Cannot assign a UDF to a variable!\n");
		
		/* make assignment of submatrix */
		if (var->type==s_submatrixref) {
			if (submrefof(var)->flags & FLAG_CONST) cc_error(cc,"variable has const status (can't write)!");
//			if ((char *)submrefof(var)<cc->startlocal || (char *)submrefof(var)>cc->endlocal) {
			/* ???? maybe, the second part should be (char *)submrefof(var) > cc->udfstart ???? */
			if ((char *)submrefof(var)<cc->startlocal) {
				/* not a local variable! */
				cc_error(cc,"Cannot change variable %s not in current context; use return!",
					var->name);
			}
			d=submdimsof(var);
			if (value->type==s_complex || value->type==s_cmatrix) {
				orig=submrefof(var);
				/* check const flag of var */
				help=new_reference(cc,orig,""); 
				mcomplex(cc,help);
				var->type=s_csubmatrixref;
				submrefof(var)=help;
				assign(cc,var,value);
				submrefof(var)=orig;
				assign(cc,orig,help); 
				return orig;
			} else if (value->type!=s_real && value->type!=s_matrix) {
				cc_error(cc,"Illegal assignment!");
			}
			getmatrix(value,&rv,&cv,&mv);
			getmatrix(submrefof(var),&r,&c,&m);
			if (d->r!=rv || d->c!=cv) {
				if (rv==1 && cv==1) {
					rind=rowsof(var); cind=colsof(var);
					for (i=0; i<d->r; i++) {
						m1=mat(m,c,rind[i],0);
						for (j=0; j<d->c; j++) {
							m1[cind[j]]=*mv;
						}
					}
					return submrefof(var);
				} else if (rv*cv==0) {		/* assign [] to matrix elements : remove them */
					if (r==1 && d->r==1) {
						cind=colsof(var);
						j=cind[0];m1=m+j;j++;
						for (i=1; i<d->c; i++) {
							while (j<cind[i]) *m1++=m[j++];
							j++;
						}
						for (;j<c;j++) {
							*m1++=m[j];
						}
						dimsof(submrefof(var))->c=c-d->c;
						return submrefof(var);
					}
					if (c==1 && d->c==1) {
						rind=rowsof(var);
						j=rind[0];m1=m+j;j++;
						for (i=1; i<d->r; i++) {
							while (j<rind[i]) *m1++=m[j++];
							j++;
						}
						for (;j<r;j++) {
							*m1++=m[j];
						}
						dimsof(submrefof(var))->r=r-d->r;
						return submrefof(var);
					}
					if (var->flags & FLAG_SUBMALLC) {
						rind=rowsof(var);
						j=rind[0]+1;m1=mat(m,c,rind[0],0);
						for (i=1; i<d->r; i++) {
							while (j<rind[i]) {
								/* copy a whole row */
								m2=mat(m,c,j,0);
								for (int k=0; k<c; k++) {
									*m1++=*m2++;
								}
								j++;
							}
							j++;
						}
						for (;j<r;j++) {
							m2=mat(m,c,j,0);
							for (int k=0; k<c; k++) {
								*m1++=*m2++;
							}
						}
						dimsof(submrefof(var))->r=r-d->r;
						return submrefof(var);
					}
					if (var->flags & FLAG_SUBMALLR) {
						cind=colsof(var);
						m1=m;
						for (i=0; i<d->r; i++) {
							m2=mat(m,c,i,0);k=0;
							for (j=0;j<d->c; j++) {
								while (k<cind[j]) {
									*m1++=m2[k];k++;
								}
								k++;
							}
							for (; k<c; k++) {
								*m1++=m2[k];
							}
						}
						dimsof(submrefof(var))->c=c-d->c;
						return submrefof(var);
					}
				}
				cc_error(cc,"Illegal assignment!\nrow or column do not agree!");
			}
			rind=rowsof(var); cind=colsof(var);
			for (i=0; i<d->r; i++) {
				m1=mat(m,c,rind[i],0);
				m2=mat(mv,cv,i,0);
				for (j=0; j<d->c; j++) {
					m1[cind[j]]=*m2++;
				}
			}
			return submrefof(var);
		} else if (var->type==s_csubmatrixref) {
			if (submrefof(var)->flags & FLAG_CONST) cc_error(cc,"variable has const status (can't write)!");
//			if ((char *)submrefof(var)<cc->startlocal || (char *)submrefof(var)>cc->endlocal) {
			if ((char *)submrefof(var)<cc->startlocal) {
				/* not a local variable! */
				cc_error(cc,"Cannot change variable %s not in current context; use return!",
					var->name);
			}
			d=submdimsof(var);
			if (value->type==s_real || value->type==s_matrix) {
				help=new_reference(cc,value,"");
				mcomplex(cc,help);
				assign(cc,var,help);
				return submrefof(var);
			}
			if (value->type!=s_complex && value->type!=s_cmatrix) {
				cc_error(cc,"Illegal assignment!");
			}
			getmatrix(value,&rv,&cv,&mv);
			getmatrix(submrefof(var),&r,&c,&m);
			if (d->r!=rv || d->c!=cv) {
				if (rv==1 && cv==1) {
					rind=rowsof(var); cind=colsof(var);
					for (i=0; i<d->r; i++) {
						m1=cmat(m,c,rind[i],0);
						for (j=0; j<d->c; j++) {
							c_copy(mv,m1+(long)2*cind[j]);
						}
					}
					return submrefof(var);
				} else if (rv*cv==0) {		/* assign [] to matrix elements : remove them */
					if (r==1 && d->r==1) {
						cind=colsof(var);
						j=cind[0];m1=m+2*j;j++;
						for (i=1; i<d->c; i++) {
							while (j<cind[i]) {*m1++=m[2*j];*m1++=m[2*j+1];j++;}
							j++;
						}
						for (;j<c;j++) {
							*m1++=m[2*j];*m1++=m[2*j+1];
						}
						dimsof(submrefof(var))->c=c-d->c;
						return submrefof(var);
					}
					if (c==1 && d->c==1) {
						rind=rowsof(var);
						j=rind[0];m1=m+2*j;j++;
						for (i=1; i<d->r; i++) {
							while (j<rind[i]) {*m1++=m[2*j];*m1++=m[2*j+1];j++;}
							j++;
						}
						for (;j<r;j++) {
							*m1++=m[2*j];*m1++=m[2*j+1];
						}
						dimsof(submrefof(var))->r=r-d->r;
						return submrefof(var);
					}
					if (var->flags & FLAG_SUBMALLC) {
						rind=rowsof(var);
						j=rind[0]+1;m1=cmat(m,c,rind[0],0);
						for (i=1; i<d->r; i++) {
							while (j<rind[i]) {
								/* copy a whole row */
								m2=cmat(m,c,j,0);
								for (int k=0; k<c; k++) {
									*m1++=*m2++;*m1++=*m2++;
								}
								j++;
							}
							j++;
						}
						for (;j<r;j++) {
							m2=cmat(m,c,j,0);
							for (int k=0; k<c; k++) {
								*m1++=*m2++;*m1++=*m2++;
							}
						}
						dimsof(submrefof(var))->r=r-d->r;
						return submrefof(var);
					}
					if (var->flags & FLAG_SUBMALLR) {
						cind=colsof(var);
						m1=m;
						for (i=0; i<d->r; i++) {
							m2=cmat(m,c,i,0);k=0;
							for (j=0;j<d->c; j++) {
								while (k<cind[j]) {
									*m1++=m2[2*k];*m1++=m2[2*k+1];k++;
								}
								k++;
							}
							for (; k<c; k++) {
								*m1++=m2[2*k];*m1++=m2[2*k+1];
							}
						}
						dimsof(submrefof(var))->c=c-d->c;
						return submrefof(var);
					}
				}
				cc_error(cc,"Illegal assignment!\nrow or column do not agree!");
			}
			rind=rowsof(var); cind=colsof(var);
			for (i=0; i<d->r; i++) {
				m1=cmat(m,c,rind[i],0);
				m2=cmat(mv,cv,i,0);
				for (j=0; j<d->c; j++) {
					c_copy(m2,m1+(ULONG)2*cind[j]); m2+=2;
				}
			}
			return submrefof(var);
		} else if (var->type==s_reference && (var->flags & FLAG_COPYONWRITE)) {
			strcpy(name,var->name);
			/* var will be no more accessible by name */
			var->name[0]=0;
			/* COPYONWRITE policy (for user function parameters) */
			if (cc->newram+size>cc->udfstart) cc_error(cc,"Memory overflow while assigning variable %s.", var->name);
			/* shift the transient memory by size to make room for the new variable
			   (necessary to deal with multiple assignment) */
			memmove(cc->endlocal+size,cc->endlocal,cc->newram-cc->endlocal);
			/* update address if an intermediate result, else do nothing 
			   (value is referencing an existing variable) */
			var=(header*)((char*)var+size);
			if ((char*)value>cc->endlocal) {
				value=(header *)((char *)value+size);
				cc->newram+=size;
			}
			memmove(cc->endlocal,(char *)value,size);
			value=(header *)cc->endlocal;
			cc->endlocal+=size;
			value->flags &= ~FLAG_CONST;
			strcpy(value->name,name);value->xor=xor(value->name);
			return value;
		} else {
			/* the whole variable */
//			if ((char *)var<cc->startlocal || (char *)var>cc->endlocal) {
			if ((char *)var<cc->startlocal) {
				/* not a local variable! */
				cc_error(cc,"Cannot change variable %s not in current context; use return!",
					var->name);
			}
#if 0
			if ((char *)var<cc->startlocal || (char *)var>cc->endlocal) {
				/* not a local variable! */
//				if (!sametype(var,value)) {
					cc_error(cc,"Cannot change variable %s not in current context; use return!",
						var->name);
//				}
				memcpy((char *)(var+1),(char *)(value+1),
					value->size-sizeof(header));
				return var;
			}
#endif
			/* expand the size (in + or -), and move all the data */
			dif=value->size-var->size;
			if (cc->newram+dif>cc->udfstart) {
				cc_error(cc,"Memory overflow");
			}
			nextvar=(char *)var+var->size;
			if (dif!=0)
				memmove(nextvar+dif,nextvar,cc->newram-nextvar);
			cc->newram+=dif; cc->endlocal+=dif;
			if (value>var) {
				/* the value address was shifted by dif */
				value=(header *)((char *)value+dif);
			}
			var->type = value->type;
			var->size = value->size;
			var->flags = value->flags & ~FLAG_CONST;
			memmove((char *)var+sizeof(header),(char *)value+sizeof(header),value->size-sizeof(header));
		}
	}
	return var;
}

header *getvalue (Calc *cc, header *hd)
/***** getvalue
	get an actual value of a reference.
    references to functions with no arguments (e.g. pi) should be
    executed
    submatrix references are resolved to matrices
 *****/
{
	header *old=hd,*mhd,*result;
	dims *d;
	real *m,*mr,*m1,*m2,*m3;
	int r,c,*rind,*cind,*cind1,i,j;
	
	while (hd && hd->type==s_reference)
		hd=referenceof(hd);
		
	if (!hd) {
#if 0
		/* points nowhere, try to see if it's a function
		   (binfunc or udf) without parameter */
		mhd=(header *)newram;
		if (exec_binfunc(old->name,0,mhd)) return mhd;
		hd=searchudf(old->name);
		if (hd) {
			interpret_udf(hd,mhd,0);
			return mhd;
		}
#endif
		/* no variable, no function, so error */
		cc_error(cc,"Variable %s not defined!",old->name);
	}
	/* there is a variable
	   resolve submatrix references to matrices */
	/* dereference a new submatrix reference, which structure is
	     header  : name, s_submatrix
	     header* : pointer to the matrix
	     dims    : dims of the submatrix
	     int     : nb of the row in the original matrix
	     int     : nb of the col in the original matrix
	 */
	if (hd->type==s_submatrixref) {
		mhd=submrefof(hd); d=submdimsof(hd);
		rind=rowsof(hd); cind=colsof(hd);
		getmatrix(mhd,&r,&c,&m);
		if (d->r==1 && d->c==1)
			return new_real(cc,*mat(m,c,*rind,*cind),"");
		result=new_matrix(cc,d->r,d->c,"");
		mr=matrixof(result);
		for (i=0; i<d->r; i++) {
			cind1=cind;
			m1=mat(mr,d->c,i,0);
			m2=mat(m,c,*rind,0);
			for (j=0; j<d->c; j++) {
				m1[j]=m2[*cind1];
				cind1++;
			}
			rind++;
		}
		return result;
	}
	if (hd->type==s_csubmatrixref)
	{	mhd=submrefof(hd); d=submdimsof(hd);
		rind=rowsof(hd); cind=colsof(hd);
		getmatrix(mhd,&r,&c,&m);
		if (d->r==1 && d->c==1) {
			m=cmat(m,c,*rind,*cind);
			return new_complex(cc,*m,*(m+1),"");
		}
		result=new_cmatrix(cc,d->r,d->c,"");
		mr=matrixof(result);
		for (i=0; i<d->r; i++) {
			cind1=cind;
			m1=cmat(mr,d->c,i,0);
			m2=cmat(m,c,*rind,0);
			for (j=0; j<d->c; j++) {
				m3=m2+(ULONG)2*(*cind1);
				*m1++=*m3++; *m1++=*m3;
				cind1++;
			}
			rind++;
		}
		return result;
	}
	/* resolve 1x1 matrices to scalars */
	if (hd->type==s_matrix && dimsof(hd)->c==1 && dimsof(hd)->r==1) {
		return new_real(cc,*matrixof(hd),"");
	}
	if (hd->type==s_cmatrix && dimsof(hd)->c==1 && dimsof(hd)->r==1) {
		return new_complex(cc,*matrixof(hd),*(matrixof(hd)+1),"");
	}
	/* just return the variable */
	return hd;
}

header *next_param (Calc *cc, header *hd)
/***** next_param
	get the next value on stack, if there is one
*****/
{	hd=(header *)((char *)hd+hd->size);
	if ((char *)hd>=cc->newram) cc_error(cc,"Not enough argument on the stack!");
	return hd;
}

header* moveresult (Calc *cc, header *stack, header *result)
/***** moveresult
	move the result to the start of stack.
*****/
{	if (stack!=result) {
		memmove((char *)stack,(char *)result,result->size);
		cc->newram=(char *)stack+stack->size;
	}
	return stack;
}

header* moveresults (Calc *cc, header *stack, header *result)
/***** moveresults
	move several results to the start of stack.
*****/
{	int size;
	if (stack!=result) {
		size=cc->newram-(char *)result;
		memmove((char *)stack,(char *)result,size);
		cc->newram=(char *)stack+size;
	}
	return stack;
}

header* pushresults(Calc *cc, header *result)
{
	if (!result) return result;
	
	if (cc->stack!=result) {
		int size=calc->newram-(char *)result;
		memmove((char *)cc->stack,(char *)result,size);
		calc->newram=(char *)cc->stack+size;
	}
	return cc->stack;
}
