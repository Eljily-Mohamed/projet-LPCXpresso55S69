/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * funcs.c
 *
 ****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "funcs.h"
#include "spread.h"
#include "edit.h"
#include "sysdep.h"
#include "solver.h"
#include "fsl_powerquad.h"


/**************** inline input handling ****************/
header* minput (Calc *cc, header *hd)
{	header *st=hd;
	jmp_buf *oldenv, env;
	unsigned int oldflags;
	char input[LINEMAX],*oldnext, *oldline;
	hd=getvalue(cc,hd);
	if (hd->type!=s_string) cc_error(cc,"string expected");

	output(cc,stringof(hd)); output(cc,"? ");
	edit(cc,input);
	oldenv=cc->env;
	oldflags=cc->flags;
	oldline=cc->line; oldnext=cc->next;
	CC_SET(cc,CC_EXEC_STRING);
	cc->line=cc->next=input;
	cc->env=&env;
	switch (setjmp(env)) {
	case 0:
		break;
	default:
		cc->next=oldnext;
		cc->line=oldline;
		cc->flags=oldflags;
		cc->env=oldenv;
		return NULL;
	}
	parse(cc);
	cc->next=oldnext;
	cc->line=oldline;
	cc->env=oldenv;
	CC_UNSET(cc,CC_EXEC_STRING);
	
	return moveresult(cc,st,cc->result);
}

/************** light user function (luf) handling **************/
/* interpret_luf
 *   interpret a one line function defined as a string
 *   used as ephemeral functions
 */
header* interpret_luf (Calc *cc, header *var, header *args, int argn, int epos)
{	header *st=args, *hd, *oldrunning, *results=(header*)cc->newram;
	jmp_buf *oldenv, env;
	unsigned int oldflags;
	char *oldnext, *oldline, *oldstartlocal, *oldendlocal;
	int oldargn;
	token_t tok;
	
	if (var==cc->running) cc_error(cc,"recursion not allowed in light user functions");

	oldenv=cc->env;
	oldflags=cc->flags;
	oldline=cc->line;
	oldnext=cc->next;
	oldargn=cc->actargn;
	oldstartlocal=cc->startlocal;
	oldendlocal=cc->endlocal;
	oldrunning=cc->running; 
	
	/* setup the new scope */
	cc->env=&env;
	cc->startlocal=(char *)args; cc->endlocal=cc->newram;
	cc->running=var;
	cc->actargn=argn;
	CC_SET(cc,CC_EXEC_STRING|CC_SEARCH_GLOBALS|CC_EXEC_UDF);
	cc->line=cc->next=stringof(var);
	cc->env=&env;
	cc->newram=cc->endlocal;

	switch (setjmp(env)) {
	case 0:
		break;
	default:
		cc->next=oldnext;
		cc->line=oldline;
		cc->flags=oldflags;
		cc->startlocal=oldstartlocal;
		cc->endlocal=oldendlocal;
		cc->running=oldrunning;
		cc->actargn=oldargn;
		cc->env=oldenv;
		outputf(cc,"error in light user function '%s'\n",var->name);
		longjmp(*cc->env,2);	/* back to enclosing error handler */
	}
	
	hd=args;
	if (strncmp(cc->next,"@(",2)!=0) {	/* function is just a string: */
		int count=0;
		char c[]="x";
		while (hd!=(header*)cc->endlocal) {	
			if (count<argn) {		/* rename actual parameters with x, y, z, a, b */
				strcpy(hd->name,c); hd->xor=xor(c);
				c[0]++;
				if (c[0]>'z') c[0]='a';
			} else break;
			hd=nextof(hd);
			count++;
		}
	} else {						/* anonymous function @(a,d,) ... */
		int count=0;
		cc->next+=2;
		while (hd!=(header*)cc->endlocal) {	
			if (count<argn) {
				tok=scan(cc);
				if (tok!=T_LABEL) cc_error(cc,"bad format in light user function parameter list");
				strcpy(hd->name,cc->str); hd->xor=xor(cc->str);
				count++;
				tok=scan(cc);
				if (tok==T_RPAR) break;
				if (tok==T_COMMA) hd=nextof(hd);
				else cc_error(cc,"bad format in light user function parameter list");
			}
		}
	}
	
	do {
		tok=parse_expr(cc);
		/* eliminate all references when returning multiple values
		   in user functions
		 */
		if (tok==T_ASSIGN || !cc->result) cc_error(cc,"bad expression");
		header *hd=getvalue(cc,cc->result);
		moveresult(cc,cc->result,hd);
	} while (tok==T_COMMA);

	cc->next=oldnext;
	cc->line=oldline;
	cc->flags=oldflags;
	cc->startlocal=oldstartlocal;
	cc->endlocal=oldendlocal;
	cc->running=oldrunning;
	cc->actargn=oldargn;
	cc->env=oldenv;
	
	return moveresults(cc,st,results);
}

/**************** user defined function handling ****************/
static const char *argname[] = {
	"arg1","arg2","arg3","arg4","arg5",
	"arg6","arg7","arg8","arg9","arg10"
};

static const char xors[MAXARGS+1]="EFG@ABCLMu";

#if 0
void make_xors (void)
{	int i;
	for (i=0; i<MAXARGS; i++) xors[i]=xor(argname[i]);
}
#endif

char *type_udfline (Calc *cc, char *start);

header* interpret_udf (Calc *cc, header *var, header *args, int argn, int epos)
/**** interpret_udf
	interpret a user defined function.
	context:
	- line
	- udfon status: 0=in global frame
	                1=in a udf
	                2=in the return statement
	- searchglobal: search variables in the global frame as well?
	- actargn     : actual number of arguments pushed on the stack
	- startlocal  : beginning of the function frame used to access
	                its arguments and local variables, on the stack.
	- endlocal    : end of the function frame.
	- running     : the current running function (header of the 
	                function on the stack).
	Parameters:
	
	Parameters are passed as references, but they sould not be 
	writable.
	
	TODO: add a 'copy on write' policy scheme for parameters.
	
	- formal parameters are defined from address udfargsof(var)
	  the number of formal parameters is given by
	  int nargu = *(int *)udfargsof(var)
	  
	- the 'argn' actual parameters are pushed on the stack by the
	  calling env and are available from address 'args'
****/
{
	/* saved context */
	jmp_buf *oldenv;
	unsigned int oldflags;
	int oldargn,oldtrace;
	char *oldnext=cc->next,*oldstartlocal,*oldendlocal,*oldline;
	real oldepsilon;
	char *oldxstart = cc->xstart, *oldxend=cc->xend;
	header *oldrunning;
	/* locals */
	jmp_buf env;
	char *p;
	header *st=args,*hd=args,*hd1;
	unsigned int arg_bitmap=0;
	int nargu,i,k,undef=0;

	/* set p to point to the start of the formal parameter block */
	p=udfargsof(var);
	nargu=*((int *)p); p+=sizeof(int);
	unsigned int def_bitmap = *(unsigned int*)p; p+=sizeof(unsigned int);
	
	/* name actual parameters according to the formal ones defined
	   in the function parameter list */
	for (i=0; i<argn; i++) {
		if (i<nargu) {	/* standard parameters */
			if (hd->type!=s_reference || (hd->type==s_reference && referenceof(hd))) {
				udf_arg* arg=(udf_arg*)p;
				strcpy(hd->name,arg->name); hd->xor=arg->xor;
				if (hd->type==s_reference) hd->flags|=FLAG_COPYONWRITE;
				arg_bitmap |= 1<<i;
			} else {
				undef++;	/* some parameter undefined */
			}
			p=udfnextarg(p, def_bitmap & (1<<i));
		} else {		/* extra parameters */
			/* valid parameter: rename it 'arg#' with # the
			   position in the actual parameter list */
			if (hd->type!=s_reference || (hd->type==s_reference && referenceof(hd))) {
				strcpy(hd->name,argname[i]); hd->xor=xors[i];
				if (hd->type==s_reference) hd->flags|=FLAG_COPYONWRITE;
			} else cc_error(cc,"undefined extra parameter...");
		}
		hd=nextof(hd);
	}
	if (argn<nargu)	{
		undef+=nargu-argn;
		epos +=nargu-argn;
	}
	while (hd!=(header*)cc->newram && i<epos) {		// replace by cc->extra_start
		/* try to see if named parameters set on the stack correspond
		   to unset parameters. Alert on duplicate parameter setups */
		p=udfargsof(var)+sizeof(int)+sizeof(unsigned int);
		for (k=0,hd1=args; k<nargu && hd1!=(header*)cc->newram; k++,hd1=nextof(hd1)) {
			udf_arg* arg=(udf_arg*)p;
			if ( (hd->xor==arg->xor) && (strcmp(hd->name,arg->name)==0) ) {
				if (arg_bitmap & (1<<k)) {	/* error! parameter set twice */
					cc_error(cc,"parameter '%s' already set by standard parameter",hd->name);
				} else if (k<argn) {	/* ,, -> there was an empty reference */
//				} else if (hd1->type==s_reference && !referenceof(hd1)) {					/* named parameter used */
					/* move the defined variable to the right place */
					long dif=hd->size-hd1->size;
					memmove((char*)hd1+hd->size,nextof(hd1),cc->newram-(char*)nextof(hd1));
					hd=(header*)((char*)hd+dif);
					if (cc->newram+dif>cc->udfstart) cc_error(cc,"Memory overflow!");
					cc->newram+=dif;
					memcpy(hd1,hd,hd->size);
					memmove(hd,nextof(hd),cc->newram-(char*)nextof(hd));
					/* update pointers */
					cc->newram-=hd1->size;
					arg_bitmap|=1<<k;
					undef--;
					epos--;
					break;
				} else {				/* no reference at all */
					arg_bitmap|=1<<k;
					undef--;
					epos--;
					break;
				}
			}
			/* next arg in the function formal parameter list */
			p=udfnextarg(p, def_bitmap & (1<<k));
		}	
		hd=nextof(hd);
		i++;
	}
	
	/* check if all required parameters have values, try to use
	   the default value, if any, for unset parameters. */
	if (undef) {
		hd=args;
		p=udfargsof(var)+sizeof(int)+sizeof(unsigned int);
		for (k=0; k<nargu; k++) {
			if ((arg_bitmap & (1<<k))==0) {
				/* this has no value, try to put the default value */
				if (def_bitmap & (1<<k)) {
					header *def=(header*)p;
					long dif;
					if (k<argn) {	/* ,, -> there was an empty reference */
						dif=def->size-hd->size;
						if (cc->newram+dif>cc->udfstart) cc_error(cc,"Memory overflow!");
						memmove((char*)hd+def->size,nextof(hd),cc->newram-(char*)nextof(hd));
						
					} else {		/* no reference at all */
						dif=def->size;
						if (cc->newram+dif>cc->udfstart) cc_error(cc,"Memory overflow!");
						memmove((char*)hd+def->size,hd,cc->newram-(char*)hd);
					}
					cc->newram+=dif;
					memcpy(hd,def,def->size);
					arg_bitmap |= 1<<k;
					undef--;
				} else {
					/* required parameter not defined and no default value */
					udf_arg* arg=(udf_arg*)p;
					cc_error(cc,"Argument '%s' undefined.", arg->name);
				}
			}	/* else, this parameter has a value set, skip it */
			p=udfnextarg(p, def_bitmap & (1<<k));
			hd=nextof(hd);
			if (!undef) break;
		}
	}
	
	/* unname extra parameters */
	for (i=0,hd=args ; (char*)hd!=cc->newram && i<epos ; i++,hd=nextof(hd)) ;
	cc->xstart=(char*)hd;
	cc->xend=cc->newram;
	for ( ; (char*)hd!=cc->newram ; hd=nextof(hd)) {
		hd->name[0]=0; hd->xor=0;
	}
//	new_real(cc, epos, "epos");
	/* Save context of the caller */
	oldenv=cc->env;
	oldnext=cc->next;
	oldline=cc->line;
	oldflags=cc->flags;
	oldtrace=cc->trace;
	oldargn=cc->actargn;
	oldstartlocal=cc->startlocal;
	oldendlocal=cc->endlocal;
	oldepsilon=cc->epsilon;
	CC_UNSET(cc,CC_SEARCH_GLOBALS);	/* by default, allow on searching in local scope */
	oldrunning=cc->running; 
	
	/* setup the new scope */
	cc->env=&env;
	cc->startlocal=(char *)args; cc->endlocal=cc->newram; cc->running=var;
	cc->actargn=argn;
	cc->line=cc->next=udfof(var);
	CC_UNSET(cc,CC_NOSUBMREF|CC_EXEC_RETURN);
	CC_SET(cc,CC_EXEC_UDF);

	/* set the synchronisation point to deal with errors */
	switch (setjmp(env)) {
	case 0:
		break;
	case 1:
		/* restore the calling context and jump to relevant error handler */
		cc->epsilon=oldepsilon;
		cc->xstart=oldxstart;
		cc->xend=oldxend;
		cc->endlocal=oldendlocal;
		cc->startlocal=oldstartlocal;
		cc->running=oldrunning;
		if (cc->trace>=0) cc->trace=oldtrace;
		cc->next=oldnext;
		cc->line=oldline;
		cc->actargn=oldargn;
		cc->flags=oldflags;
		cc->env=oldenv;
		outputf(cc,"error in function '%s'\n",var->name);
		longjmp(*cc->env,2);	/* back to enclosing error handler */
		break;
	case 2:
		if (CC_ISSET(cc,CC_VERBOSE)) {output(cc,"  ");type_udfline(cc,cc->line);}
		/* restore the calling context and jump to relevant error handler */
		cc->endlocal=oldendlocal;
		cc->startlocal=oldstartlocal;
		cc->running=oldrunning;
		if (cc->trace>=0) cc->trace=oldtrace;
		cc->epsilon=oldepsilon;
		cc->xstart=oldxstart;
		cc->xend=oldxend;
		cc->next=oldnext;
		cc->line=oldline;
		cc->actargn=oldargn;
		cc->flags=oldflags;
		cc->env=oldenv;
		outputf(cc,"error in function '%s'\n",var->name);
		longjmp(*cc->env,2);	/* back to enclosing error handler */
		break;
	}
	
	if ((oldtrace=cc->trace)>0) {
		if (cc->trace==2) cc->trace=0;
		if (cc->trace>0) trace_udfline(cc,cc->next);
	} else if (var->flags & FLAG_UDFTRACE) {
		cc->trace=1;
		if (cc->trace>0) trace_udfline(cc,cc->next);
	}
	
	cc->level++;
	if (cc->level>UDF_LEVEL_MAX) cc_error(cc,"Deepest UDF call level reached or too many recursions!");
	/* interpret the udf code */
	while (CC_ISSET(cc,CC_EXEC_UDF)) {
		cmdtyp cmd=parse(cc);
		if (cmd==c_return) {
			break;
		}
		if (sys_test_key()==escape) cc_error(cc,"User interrupted!");
	}
	cc->level--;
	/* function finished, restore the context of the caller */
	cc->endlocal=oldendlocal; cc->startlocal=oldstartlocal;
	cc->running=oldrunning;
	if (cc->trace>=0) cc->trace=oldtrace;
	cc->epsilon=oldepsilon;
	cc->xstart=oldxstart;
	cc->xend=oldxend;
	cc->next=oldnext;
	cc->line=oldline;
	cc->actargn=oldargn;
	cc->flags=oldflags;
	cc->env=oldenv;
	
	if (cc->result) {
		if (cc->nresults) {		/* multiple results */
			return moveresults(cc,st,cc->result);
		} else {				/* single result */
//			if (cc->result->type==s_reference) cc->newram=cc->endlocal;
			cc->result=getvalue(cc,cc->result);
			cc->nresults=1;
			return moveresult(cc,st,cc->result);
		}
	}
	return NULL;
}

header* margn (Calc *cc, header *hd)
{	return new_real(cc,cc->actargn,"");
}

header* margs (Calc *cc, header *hd)
/* return all args from realof(hd)-st argument on */
{	header *st=hd,*hd1,*result;
	int i,n;
	long size;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"real value expected");
	n=(int)*realof(hd);
	if (n<1) cc_error(cc,"arg1 must be >= 1");
	if (n>cc->actargn) {
		cc->newram=(char *)st; return st;
	}
	result=(header *)cc->startlocal; i=1;
	while (i<n && result<(header *)cc->endlocal) {
		result=nextof(result); i++;
	}
	hd1=result;
	while (i<cc->actargn+1 && hd1<(header *)cc->endlocal) {
		hd1=nextof(hd1); i++;
	}
	size=(char *)hd1-(char *)result;
	if (size<=0) {
		cc_error(cc,"Error in args!");
	}
	memmove((char *)st,(char *)result,size);
	cc->newram=(char *)st+size;
	return st;
}

header* mxargs (Calc *cc, header *hd)
{
	if (cc->xstart) {
		long sz=cc->xend-cc->xstart;
		memcpy(hd,cc->xstart,sz);
		cc->newram+=sz;
	}
	return hd;
}

header* mindex (Calc *cc, header *hd)
{	return new_real(cc,(real)cc->loopindex,"");
}

header* merror (Calc *cc, header *hd)
{	hd=getvalue(cc, hd);
	if (hd->type!=s_string) cc_error(cc,"string expected");
	cc_error(cc,"Error: %s",stringof(hd));
	return NULL;
}

/****************************************************************
 *  type introspection
 ****************************************************************/
header* mname (Calc *cc, header *hd)
{	header *result;
	hd=getvalue(cc,hd);
	result=new_cstring(cc,hd->name,strlen(hd->name),"");
	return pushresults(cc,result);
}

header* miscomplex (Calc *cc, header *hd)
{	header *result;
	hd=getvalue(cc, hd);
	if (hd->type==s_complex || hd->type==s_cmatrix) {
		result=new_real(cc,1.0,"");
	} else {
		result=new_real(cc,0.0,"");
	}
	return pushresults(cc,result);
}

header* misreal (Calc *cc, header *hd)
{	header *result;
	hd=getvalue(cc, hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		result=new_real(cc,1.0,"");
	} else {
		result=new_real(cc,0.0,"");
	}
	return pushresults(cc,result);
}

header* misstring (Calc *cc, header *hd)
{	header *result;
	hd=getvalue(cc,hd);
	if (hd->type==s_string) {
		result=new_real(cc,1.0,"");
	} else {
		result=new_real(cc,0.0,"");
	}
	return pushresults(cc,result);
}

header* misvar (Calc *cc, header *hd)
{
	header *result;
	if (hd->type==s_reference && searchvar(cc,hd->name)!=0) {
		result=new_real(cc,1.0,"");
	} else {
		result=new_real(cc,0.0,"");
	}
	return pushresults(cc,result);
}

/* needs to test function variables directly ... */
binfunc_t *binfunc_find (char *name);

header* misfunction (Calc *cc, header *hd)
{	header *result;
	hd=getvalue(cc,hd);
	if (hd->type==s_funcref) {
		result=new_real(cc,1.0,"");
	} else if (hd->type==s_string
		&& (searchudf(cc,stringof(hd))!=NULL
		|| binfunc_find(stringof(hd))!=NULL)) {
		result=new_real(cc,1.0,"");
	} else {
		result=new_real(cc,0.0,"");
	}
	return pushresults(cc,result);
}

/****************************************************************
 *	number inspection
 ****************************************************************/
real risnan(real val)
{
	return (real)isnan(val);
}

header* misnan (Calc *cc, header *hd)
{	return spread1(cc,risnan,NULL,hd);
}

real risinf(real val)
{
	return (real)isinf(val);
}

header* misinf (Calc *cc, header *hd)
{	return spread1(cc,risinf,NULL,hd);
}

real risfinite(real val)
{
	return (real)isfinite(val);
}

header* misfinite (Calc *cc, header *hd)
{	return spread1(cc,risfinite,NULL,hd);
}

/****************************************************************
 *	time functions
 ****************************************************************/
header* mtime (Calc *cc, header *hd)
{	return new_real(cc,sys_clock(),"");
}

header* mwait (Calc *cc, header *hd)
{	header *result;
	real now;
	scan_t h;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"real value expected");
	now=sys_clock();
	sys_wait(*realof(hd),&h);
	if (h==escape) cc_error(cc,"user interrupt");
	result=new_real(cc,sys_clock()-now,"");
	return pushresults(cc,result);
}

/****************************************************************
 *	number and text formatting functions
 ****************************************************************/
/***************** number display mode handling *****************/
/* Display modes :
   - standard formats
     mode FRAC: display result as aproximated fraction (?), fallback to STD
                mode if the aproximation is too long to converge.
       params: eps, nb of digits to display a number
     mode STD: display smartly the result as standard or exponentiated number
       params: nb of significant digits, nb of digits to display a number
     mode SCI: display in loating point scientific notation m*1Eexp
     	params: nb of significant digits, nb of digits to display a number
     mode FIXED: display the result with a fixed point format (may be bad
                 with floating point data)
       params: number of dits after the decimal point, nb of digits to display a number
   - ingineering formats
     mode ENG1: use exponentiation format (incr/decr of the power of 10 by 3)
     mode ENG2: use suffixes
       params: nb of digits to display a number
 */

/***** mformat
   format(
     "STD"|"FIXED"|"ENG1"|"ENG2"|"SCI"|"FIXED"|"FRAC",
     [extend, digits]
   )
 *****/

header* mformat (Calc *cc, header *hd)
{	header *result, *hd1=next_param(cc,hd);
	int l,d;
	int oldl=cc->disp_fieldw,oldd=cc->disp_digits;
	int dmode=0;
	real *m;
	hd=getvalue(cc,hd);
	if (hd->type==s_string) {
		if (strcmp(stringof(hd),"STD")==0) {
			dmode=0;
		} else if (strcmp(stringof(hd),"ENG1")==0) {
			dmode=1;
		} else if (strcmp(stringof(hd),"ENG2")==0) {
			dmode=2;
		} else if (strcmp(stringof(hd),"SCI")==0) {
			dmode=3;
		} else if (strcmp(stringof(hd),"FIXED")==0) {
			dmode=4;
		} else if (strcmp(stringof(hd),"FRAC")==0) {
			dmode=5;
		} else {
			cc_error(cc,"Display mode expected");
		}
	}
	if (hd1) {
		hd1=getvalue(cc,hd1);
		if (hd1->type!=s_matrix || dimsof(hd1)->r!=1 || dimsof(hd1)->c!=2)
			cc_error(cc,"1x2 real vector expected");
		l=(int)*matrixof(hd1); d=(int)*(matrixof(hd1)+1);
		if (l<0 || l>2*DBL_DIG+18 || d<0 || d>DBL_DIG+1) cc_error(cc,"bad value");
		
		cc->disp_eng_sym=0;		/*  no multiple/submultiple symbols */
		
		switch (dmode) {
		case 0: /* smart STD */
			if (l<d+8) l=d+8;
			sprintf(cc->fixedformat,"%%0.%dG",d);
			sprintf(cc->expoformat,"%%0.%dE",d-1);
			cc->minexpo=pow(10,(-d+1>-4) ? -d+1 : -4);
			cc->maxexpo=pow(10,d-1)-0.1;
			break;
		case 1:		/* ENG1 */
			if (d<3) {
				d=3;
				cc_warn(cc,"warning: \"ENG1\" display mode can't be set to less than 3 digits (3 digits enforced)");
			}
			if (l<d+8) l=d+8; 
			cc->disp_eng_sym=0;
			break;
		case 2:		/* ENG2 */
			if (d<3) {
				d=3;
				cc_warn(cc,"warning: \"ENG2\" display mode can't be set to less than 3 digits (3 digits enforced)");
			}
			if (l<d+4) l=d+4;
			cc->disp_eng_sym=1;
			break;
		case 3:		/* SCI */
			if (l<d+8) l=d+8;
			sprintf(cc->expoformat,"%%0.%dE",d-1);
			break;
		case 4:		/* FIXED */
			if (l<DBL_DIG+3) l=DBL_DIG+3;
			sprintf(cc->fixedformat,"%%0.%dF",d);
			break;
		case 5:		/* FRAC */
		default:	/* never used */
			cc_error(cc,"mode not supported");
			break;
		}
		cc->disp_mode=dmode;
		cc->disp_digits=d;
		cc->disp_fieldw=l;
	} else cc_error(cc,"1x2 matrix expected");
	result=new_matrix(cc,1,2,"");
	m=matrixof(result);
	*m=(real)oldl;
	*(m+1)=(real)oldd;
	return pushresults(cc,result);
}

header* mprintf (Calc *cc, header *hd)
{	header *hd1,*result;
	char s[1024];
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd);
	hd1=getvalue(cc,hd1);
	if (hd->type!=s_string || hd1->type!=s_real)
		cc_error(cc,"printf(\"format string\",value)");
	snprintf(s,1024,stringof(hd),*realof(hd1));
	result=new_cstring(cc,s,strlen(s)+1,"");
	return pushresults(cc,result);
}

/****************************************************************
 *	const
 ****************************************************************/
header* mepsilon (Calc *cc, header *hd)
{	return new_real(cc,cc->epsilon,"");
}

header* msetepsilon (Calc *cc, header *hd)
{	header *result;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"real value expected");
	result=new_real(cc,cc->epsilon,"");
	cc->epsilon=*realof(hd);
	return pushresults(cc,result);
}	

/****************************************************************
 *	basic operators
 ****************************************************************/
static void r_add (real *x, real *y, real *z)
{	*z=*x+*y;
}

void c_add (cplx x, cplx y, cplx z)
{	z[0]=x[0]+y[0];
	z[1]=x[1]+y[1];
}

header* add (Calc *cc, header *hd, header *hd1)
/***** add
	add the values.
*****/
{	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	result=map2(cc,r_add,c_add,hd,hd1);
	return pushresults(cc,result);
}

static void r_sub (real *x, real *y, real *z)
{	*z=*x-*y;
}

void c_sub (cplx x, cplx y, cplx z)
{	z[0]=x[0]-y[0];
	z[1]=x[1]-y[1];
}

header* subtract (Calc *cc, header *hd, header *hd1)
/***** subtract
	subtract the values.
*****/
{	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	result=map2(cc,r_sub,c_sub,hd,hd1);
	return pushresults(cc,result);
}

static void r_mul (real *x, real *y, real *z)
{	*z=*x*(*y);
}


void c_mul (cplx x, cplx y, cplx z)
{	/* temp var h so that the same var can be used as src and dest operand */
	real h=x[0]*y[0] - x[1]*y[1];
	z[1]=x[0]*y[1] + x[1]*y[0];
	z[0]=h;
}

header* dotmultiply (Calc *cc, header *hd, header *hd1)
/***** dotmultiply
	multiply the values elementwise.
*****/
{	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	result=map2(cc,r_mul,c_mul,hd,hd1);
	return pushresults(cc,result);
}

static void r_div (real *x, real *y, real *z)
{	
	*z=*x/(*y);
}

void c_div (cplx x, cplx y, cplx z)
{	/* temp var h so that the same var can be used as src and dest operand */
	real r,h;
	r=y[0]*y[0] + y[1]*y[1];
	h=(x[0]*y[0] + x[1]*y[1])/r;
	z[1]=(x[1]*y[0] - x[0]*y[1])/r;
	z[0]=h;
}

header* dotdivide (Calc *cc, header *hd, header *hd1)
/***** adotdivide
	divide the values elementwise.
*****/
{	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	result=map2(cc,r_div,c_div,hd,hd1);
	return pushresults(cc,result);
}

/* cumulative sum of products: s += x*y */
static void c_scalp (cplx x, cplx y, cplx s)
{	s[0] += x[0]*y[0] - x[1]*y[1];
	s[1] += x[0]*y[1] + x[1]*y[0];
}

/* complex number copy: y=x */
void c_copy (cplx x, cplx y)
{	y[0]=x[0]; y[1]=x[1];
}

header* multiply (Calc *cc, header *hd, header *hd1)
/***** multiply
	matrix multiplication.
*****/
{	header *result=NULL,*st=hd;
	dims *d,*d1;
	real *m,*m1,*m2,*mm1,*mm2,x;
	int i,j,c,r,k;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_matrix && hd1->type==s_matrix) {
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_matrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
				mm1=mat(m1,d->c,i,0); mm2=m2+j;
				x=0.0;
				for (k=0; k<d->c; k++) {
					x+=(*mm1)*(*mm2);
					mm1++; mm2+=d1->c;
				}
				*mat(m,c,i,j)=x;
			}
		return moveresult(cc,st,result);
	} else if (hd->type==s_matrix && hd1->type==s_cmatrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
			   mm1=mat(m1,d->c,i,0); mm2=m2+2*j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					cplx a={*mm1,0.0};
					c_scalp(a,mm2,x);
					mm1++; mm2+=2*d1->c;
				}
				c_copy(x,cmat(m,c,i,j));
			}
		return pushresults(cc,result);
	} else if (hd->type==s_cmatrix && hd1->type==s_matrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
				mm1=cmat(m1,d->c,i,0); mm2=m2+j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					cplx a={*mm2,0.0};
					c_scalp(mm1,a,x);
					mm1+=2; mm2+=d1->c;
				}
				c_copy(x,cmat(m,c,i,j));
			}
		return pushresults(cc,result);
	} else if (hd->type==s_cmatrix && hd1->type==s_cmatrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
			   mm1=cmat(m1,d->c,i,0); mm2=m2+2*j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					c_scalp(mm1,mm2,x);
					mm1+=2; mm2+=2*d1->c;
				}
				c_copy(x,cmat(m,c,i,j));
			}
		return pushresults(cc,result);
	}
	return dotmultiply(cc,st,nextof(st));
}

header* wmultiply (Calc *cc, header *hd)
/***** multiply
	matrix multiplication for weakly nonzero matrices.
*****/
{	header *result=NULL,*st=hd, *hd1;
	dims *d,*d1;
	real *m,*m1,*m2,*mm1,*mm2,x;
	int i,j,c,r,k;
	hd=getvalue(cc,hd); hd1=getvalue(cc,nextof(st));
	if (hd->type==s_matrix && hd1->type==s_matrix) {
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_matrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
				mm1=mat(m1,d->c,i,0); mm2=m2+j;
				x=0.0;
				for (k=0; k<d->c; k++) {
					if ((*mm1!=0.0)&&(*mm2!=0.0)) x+=(*mm1)*(*mm2);
					mm1++; mm2+=d1->c;
				}
				*mat(m,c,i,j)=x;
			}
		return moveresult(cc,st,result);
	} else if (hd->type==s_matrix && hd1->type==s_cmatrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
			   mm1=mat(m1,d->c,i,0); mm2=m2+2*j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					cplx a={*mm1,0.0};
					if ((*mm2!=0.0 || *(mm2+1)!=0.0) &&
							(*mm1!=0.0)) c_scalp(a,mm2,x);
					mm1++; mm2+=2*d1->c;
				}
				c_copy(x,cmat(m,c,i,j));
			}
		return pushresults(cc,result);
	} else if (hd->type==s_cmatrix && hd1->type==s_matrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
				mm1=cmat(m1,d->c,i,0); mm2=m2+j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					cplx a={*mm2,0.0};
					if ((*mm2!=0.0) && (*mm1!=0.0 || *(mm1+1)!=0.0))
						c_scalp(mm1,a,x);
					mm1+=2; mm2+=d1->c;
				}
				c_copy(x,cmat(m,c,i,j));
			}
		return pushresults(cc,result);
	} else if (hd->type==s_cmatrix && hd1->type==s_cmatrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
			   mm1=cmat(m1,d->c,i,0); mm2=m2+2*j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					if ((*mm2!=0.0 || *(mm2+1)!=0.0) &&
							(*mm1!=0.0 || *(mm1+1)!=0.0))
						c_scalp(mm1,mm2,x);
					mm1+=2; mm2+=2*d1->c;
				}
				c_copy(x,cmat(m,c,i,j));
			}
		return pushresults(cc,result);
	} else cc_error(cc,"bad type");
	return NULL;
}

header* smultiply (Calc *cc, header *hd)
/***** multiply
	matrix multiplication for weakly nonzero symmetric matrices.
*****/
{	header *result=NULL,*st=hd, *hd1;
	dims *d,*d1;
	real *m,*m1,*m2,*mm1,*mm2,x;
	int i,j,c,r,k;
	hd=getvalue(cc,hd); hd1=getvalue(cc,nextof(st));
	if (hd->type==s_matrix && hd1->type==s_matrix) {
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_matrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=i; j<c; j++) {
				mm1=mat(m1,d->c,i,0); mm2=m2+j;
				x=0.0;
				for (k=0; k<d->c; k++) {
					if ((*mm1!=0.0)&&(*mm2!=0.0)) x+=(*mm1)*(*mm2);
					mm1++; mm2+=d1->c;
				}
				*mat(m,c,i,j)=x;
				*mat(m,c,j,i)=x;
			}
		return moveresult(cc,st,result);
	} else if (hd->type==s_matrix && hd1->type==s_cmatrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=i; j<c; j++) {
			   mm1=mat(m1,d->c,i,0); mm2=m2+2*j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					cplx a={*mm1,0.0};
					if ((*mm2!=0.0 || *(mm2+1)!=0.0) &&
							(*mm1!=0.0)) c_scalp(a,mm2,x);
					mm1++; mm2+=2*d1->c;
				}
				c_copy(x,cmat(m,c,i,j)); x[1]=-x[1];
				c_copy(x,cmat(m,c,j,i));
			}
		return pushresults(cc,result);
	} else if (hd->type==s_cmatrix && hd1->type==s_matrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=i; j<c; j++) {
				mm1=cmat(m1,d->c,i,0); mm2=m2+j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					cplx a={*mm2,0.0};
					if ((*mm2!=0.0) && (*mm1!=0.0 || *(mm1+1)!=0.0))
						c_scalp(mm1,a,x);
					mm1+=2; mm2+=d1->c;
				}
				c_copy(x,cmat(m,c,i,j)); x[1]=-x[1];
				c_copy(x,cmat(m,c,j,i));
			}
		return pushresults(cc,result);
	} else if (hd->type==s_cmatrix && hd1->type==s_cmatrix) {
		cplx x;
		d=dimsof(hd);
		d1=dimsof(hd1);
		if (d->c != d1->r) cc_error(cc,"Cannot multiply these!");
		r=d->r; c=d1->c;
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
		m1=matrixof(hd);
		m2=matrixof(hd1);
		for (i=0; i<r; i++)
			for (j=i; j<c; j++) {
			   mm1=cmat(m1,d->c,i,0); mm2=m2+2*j;
				x[0]=0.0; x[1]=0.0;
				for (k=0; k<d->c; k++) {
					if ((*mm2!=0.0 || *(mm2+1)!=0.0) &&
							(*mm1!=0.0 || *(mm1+1)!=0.0))
						c_scalp(mm1,mm2,x);
					mm1+=2; mm2+=2*d1->c;
				}
				c_copy(x,cmat(m,c,i,j)); x[1]=-x[1];
				c_copy(x,cmat(m,c,j,i));
			}
		return pushresults(cc,result);
	} else cc_error(cc,"bad type");
	return NULL;
}

static void r_opposite (real *x, real *y)
{	*y= -*x;
}

static void c_opposite (cplx x, cplx y)
{	y[0]= -x[0];
	y[1]= -x[1];
}

header* opposite (Calc *cc, header *hd)
/***** opposite
	compute -matrix.
*****/
{	header *result;
	hd=getvalue(cc,hd);
	result=map1(cc,r_opposite,c_opposite,hd);
	return pushresults(cc,result);
}

void make_complex (Calc *cc, header *hd)
/**** make_complex
	make a function argument complex.
****/
{	header *old=hd,*nextarg;
	int size;
	int r,c,i,j;
	real *m,*m1;
	hd=getvalue(cc,hd);
	if (hd->type==s_real) {
		size=sizeof(header)+2*sizeof(real);
		nextarg=nextof(old);
		if (cc->newram+(size-old->size)>cc->udfstart) cc_error(cc,"Memory overflow!");
		if (cc->newram>(char *)nextarg)
			memmove((char *)old+size,(char *)nextarg,
				cc->newram-(char *)nextarg);
		cc->newram+=size-old->size;
		*(old->name)=0; old->size=size;
		old->type=s_complex;
		*realof(old)=*realof(hd);
		*imagof(old)=0.0;
	} else if (hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		size=sizeof(header)+cmatrixsize(r,c);
		nextarg=nextof(old);
		if (cc->newram+(size-old->size)>cc->udfstart) cc_error(cc,"Memory overflow!");
		if (cc->newram>(char *)nextarg)
			memmove((char *)old+size,(char *)nextarg,
				cc->newram-(char *)nextarg);
		cc->newram+=size-old->size;
		*(old->name)=0; old->size=size; old->type=s_cmatrix;
		dimsof(old)->r=r; dimsof(old)->c=c;
		m1=matrixof(old);
		for (i=r-1; i>=0; i--)
			for (j=c-1; j>=0; j--) {
				*cmat(m1,c,i,j)=*mat(m,c,i,j);
				*(cmat(m1,c,i,j)+1)=0.0;
			}
	}
}

header* mvconcat (Calc *cc, header *hd, header *hd1)
{	header *st=hd,*result;
	real *m,*m1;
	int r,c,r1,c1;
	size_t size=0;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_real || hd->type==s_matrix) {
		if (hd1->type==s_real || hd1->type==s_matrix) {
			getmatrix(hd,&r,&c,&m);
			getmatrix(hd1,&r1,&c1,&m1);
			if (r!=0 && r1!=0 && (c!=c1 || r+r1<0)) cc_error(cc,"columns must agree");
			result=new_matrix(cc,r+r1,(c1!=0) ? c1 : c,"");
			if (r!=0) {
				size=r*c*sizeof(real);
				memmove((char *)matrixof(result),(char *)m,size);
			}
			memmove((char *)matrixof(result)+size,
				(char *)m1,r1*c1*sizeof(real));
		} else if (hd1->type==s_complex || hd1->type==s_cmatrix) {
			make_complex(cc,st);
			return mvconcat(cc,st,nextof(st));
		} else goto err;
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		if (hd1->type==s_complex || hd1->type==s_cmatrix) {
			getmatrix(hd,&r,&c,&m);
			getmatrix(hd1,&r1,&c1,&m1);
			if (r!=0 && r1!=0 && (c!=c1 || r+r1<0)) cc_error(cc,"columns must agree");
			result=new_cmatrix(cc,r+r1,(c1!=0) ? c1 : c,"");
			if (r!=0) {
				size=r*2*c*sizeof(real);
				memmove((char *)matrixof(result),(char *)m,size);
			}
			memmove((char *)matrixof(result)+size,
                (char *)m1,r1*2*c1*sizeof(real));
		} else if (hd1->type==s_real || hd1->type==s_matrix) {
			make_complex(cc,next_param(cc,st));
			return mvconcat(cc,st,nextof(st));
		} else goto err;
	} else goto err;
	
	return moveresult(cc,st,result);
err:
	cc_error(cc,"can't concat these");
	return NULL;
}

header* mhconcat (Calc *cc, header *hd, header *hd1)
{	header *st=hd,*result;
	real *m,*m1,*mr;
	int r,c,r1,c1,i;
	hd=getvalue(cc,hd);
	hd1=getvalue(cc,hd1);
	if (hd->type==s_string && hd1->type==s_string) {
		result=new_cstring(cc,stringof(hd),
			strlen(stringof(hd))+strlen(stringof(hd1))+1,"");
		strcat(stringof(result),stringof(hd1));
	} else if (hd->type==s_real || hd->type==s_matrix) {
		if (hd1->type==s_real || hd1->type==s_matrix) {
			getmatrix(hd,&r,&c,&m);
			getmatrix(hd1,&r1,&c1,&m1);
			if (c!=0 && c1!=0 && (r!=r1 || c+c1<0)) cc_error(cc,"rows must agree");
			if (c1==0) r1=r;
			result=new_matrix(cc,r1,c+c1,"");
			mr=matrixof(result);
			for (i=0; i<r1; i++) {
				if (c!=0) memmove((char *)mat(mr,c+c1,i,0),
					(char *)mat(m,c,i,0),c*sizeof(real));
				if (c1!=0) memmove((char *)mat(mr,c+c1,i,c),
					(char *)mat(m1,c1,i,0),c1*sizeof(real));
			}
		} else if (hd1->type==s_complex || hd1->type==s_cmatrix) {
			make_complex(cc,st);
			return mhconcat(cc,st,nextof(st));
		} else goto err;
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		if (hd1->type==s_complex || hd1->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
			getmatrix(hd1,&r1,&c1,&m1);
			if (c!=0 && c1!=0 && (r!=r1 || c+c1<0)) cc_error(cc,"rows must agree");
			if (c1==0) r1=r;
			result=new_cmatrix(cc,r1,c+c1,"");
			mr=matrixof(result);
			for (i=0; i<r1; i++) {
				if (c!=0) memmove((char *)cmat(mr,c+c1,i,0),
                    (char *)cmat(m,c,i,0),c*2*sizeof(real));
				if (c1!=0) memmove((char *)cmat(mr,c+c1,i,c),
                    (char *)cmat(m1,c1,i,0),c1*2*sizeof(real));
			}
		} else if (hd1->type==s_real || hd1->type==s_matrix) {
			make_complex(cc,next_param(cc,st));
			return mhconcat(cc,st,nextof(st));
		} else goto err;
	} else goto err;

	return moveresult(cc,st,result);
err:
	cc_error(cc,"can't concat these");
	return NULL;
}


header* transpose (Calc *cc, header *hd)
/***** transpose 
	transpose a matrix
*****/
{	header *hd1=NULL,*st=hd;
	real *m,*m1,*mh;
	int c,r,i,j;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		hd1=new_matrix(cc,c,r,"");
		m1=matrixof(hd1);
		for (i=0; i<r; i++) {
			mh=m1+i;
			for (j=0; j<c; j++) {
				*mh=*m++; mh+=r;
			}
		}
	} else if (hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		hd1=new_cmatrix(cc,c,r,"");
		m1=matrixof(hd1);
		for (i=0; i<r; i++) {
			mh=m1+2*i;
			for (j=0; j<c; j++) {
				*mh=*m++; *(mh+1)=-*m++;
                mh+=2*r;
			}
		}
	} else if (hd->type==s_real || hd->type==s_complex) {
		hd1=hd;
	} else {
		cc_error(cc,"cannot transpose this!");
	}
	return moveresult(cc,st,hd1);
}

header* vectorize (Calc *cc, header *init, header *step, header *end)
{	real vinit,vstep,vend,*m;
	int count;
	header *result;
	init=getvalue(cc,init); step=getvalue(cc,step); end=getvalue(cc,end);
	if (init->type!=s_real || step->type!=s_real || end->type!=s_real)
		cc_error(cc,"The ':' allows only real arguments!");
	vinit=*realof(init); vstep=*realof(step); vend=*realof(end);
	if (vstep==0) cc_error(cc,"A step size of 0 is not allowed in ':' ");
	if (1.0+fabs(vend-vinit)/fabs(vstep)*(1+cc->epsilon)>INT_MAX)
		cc_error(cc,"Too many elements");
	count=1+(int)(floor(fabs(vend-vinit)/fabs(vstep)*(1+cc->epsilon)));
	if ((vend>vinit && vstep<0) || (vend<vinit && vstep>0))
		count=0;
	result=new_matrix(cc,1,count,"");
	m=matrixof(result);
	while (count>0) {
		*m++=vinit;
		vinit+=vstep;
		count--;
	}
	return pushresults(cc,result);
}

header* msolve (Calc *cc, header *hd, header *hd1)
{	header *st=hd,*result=NULL;
	real *m,*m1;
	int r,c,r1,c1;
	hd=getvalue(cc,hd);
	hd1=getvalue(cc,hd1);
	if (hd->type==s_matrix || hd->type==s_real) {
		getmatrix(hd,&r,&c,&m);
		if (hd1->type==s_cmatrix) {
			make_complex(cc,st);
			return msolve(cc,st,nextof(st));
		}
		if (hd1->type!=s_matrix && hd1->type!=s_real)
			cc_error(cc,"real value or matrix expected");
		getmatrix(hd1,&r1,&c1,&m1);
		if (c!=r || c<1 || r!=r1) cc_error(cc,"bad size");
		result=new_matrix(cc,r,c1,"");
		solvesim(cc,m,r,m1,c1,matrixof(result));
	} else if (hd->type==s_cmatrix || hd->type==s_complex) {
		getmatrix(hd,&r,&c,&m);
		if (hd1->type==s_matrix || hd1->type==s_real) {
			make_complex(cc,next_param(cc,st));
			return msolve(cc,st,nextof(st));
		}
		if (hd1->type!=s_cmatrix && hd1->type!=s_complex) cc_error(cc,"complex value or matrix expected");
		getmatrix(hd1,&r1,&c1,&m1);
		if (c!=r || c<1 || r!=r1) cc_error(cc,"bad size");
		result=new_cmatrix(cc,r,c1,"");
		c_solvesim(cc,m,r,m1,c1,matrixof(result));
	} else cc_error(cc,"real or complex value or matrix expected");
	return pushresults(cc,result);
}

/****************************************************************
 *	compare operators
 ****************************************************************/
static void rgreater (real *x, real *y, real *z)
{	if (*x>*y) *z=1.0;
	else *z=0.0;
}

header* mgreater (Calc *cc, header *hd, header *hd1)
{	
	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string) {
		if (hd1->type!=s_string) cc_error(cc,"can only compare a string with another one");
		result=new_real(cc,strcmp(stringof(hd),stringof(hd1))>0,"");
	} else {
		result=map2(cc,rgreater,NULL,hd,hd1);
	}
	return pushresults(cc,result);
}

static void rless (real *x, real *y, real *z)
{	if (*x<*y) *z=1.0;
	else *z=0.0;
}

header* mless (Calc *cc, header *hd, header *hd1)
{
	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string) {
		if (hd1->type!=s_string) cc_error(cc,"can only compare a string with another one");
		result=new_real(cc,strcmp(stringof(hd),stringof(hd1))<0,"");
	} else {
		result=map2(cc,rless,NULL,hd,hd1);
	}
	return pushresults(cc,result);
}

static void rgreatereq (real *x, real *y, real *z)
{	if (*x>=*y) *z=1.0;
	else *z=0.0;
}

header* mgreatereq (Calc *cc, header *hd, header *hd1)
{
	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string) {
		if (hd1->type!=s_string) cc_error(cc,"can only compare a string with another one");
		result=new_real(cc,strcmp(stringof(hd),stringof(hd1))>=0,"");
	} else {
		result=map2(cc,rgreatereq,NULL,hd,hd1);
	}
	return pushresults(cc,result);
}

static void rlesseq (real *x, real *y, real *z)
{	if (*x<=*y) *z=1.0;
	else *z=0.0;
}

header* mlesseq (Calc *cc, header *hd, header *hd1)
{
	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string) {
		if (hd1->type!=s_string) cc_error(cc,"can only compare a string with another one");
		result=new_real(cc,strcmp(stringof(hd),stringof(hd1))<=0,"");
	} else {
		result=map2(cc,rlesseq,NULL,hd,hd1);
	}
	return pushresults(cc,result);
}

static void requal (real *x, real *y, real *z)
{
	*z = (*x==*y) ? 1.0 : 0.0;
}

static void cequal (cplx x, cplx y, real *z)
{
	*z = (x[0]==y[0] && x[1]==y[1]) ? 1.0 : 0.0;
}

header* mequal (Calc *cc, header *hd, header *hd1)
{
	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string) {
		if (hd1->type!=s_string) cc_error(cc,"can only compare a string with another one");
		result=new_real(cc,strcmp(stringof(hd),stringof(hd1))==0,"");
	} else if ( (hd->type==s_matrix || hd->type==s_cmatrix) && dimsof(hd)->r*dimsof(hd)->c==0 ) {
		/* empty matrix */
		if (hd1->type==s_matrix || hd1->type==s_cmatrix) {
			result=new_real(cc,(real)(dimsof(hd)->r*dimsof(hd)->c==dimsof(hd1)->r*dimsof(hd1)->c),"");
		} else {
			result=new_real(cc,0.0,"");
		}
	} else if ( (hd1->type==s_matrix || hd1->type==s_cmatrix) && dimsof(hd1)->r*dimsof(hd1)->c==0 ) {
		/* empty matrix */
		if (hd->type==s_matrix || hd->type==s_cmatrix) {
			result=new_real(cc,(real)(dimsof(hd)->r*dimsof(hd)->c==dimsof(hd1)->r*dimsof(hd1)->c),"");
		} else {
			result=new_real(cc,0.0,"");
		}
	} else {
		result=map2r(cc,requal,cequal,hd,hd1);
	}
	return pushresults(cc,result);
}

static void runequal (real *x, real *y, real *z)
{	if (*x!=*y) *z=1.0;
	else *z=0.0;
}

static void cunequal (cplx x, cplx y, real *z)
{
	*z = (x[0]!=y[0] || x[1]!=y[1]) ? 1.0 : 0.0;
}

header* munequal (Calc *cc, header *hd, header *hd1)
{
	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string) {
		if (hd1->type!=s_string) cc_error(cc,"can only compare a string with another one");
		result=new_real(cc,strcmp(stringof(hd),stringof(hd1))!=0,"");
	} else if ( (hd->type==s_matrix || hd->type==s_cmatrix) && dimsof(hd)->r*dimsof(hd)->c==0 ) {
		/* empty matrix */
		if (hd1->type==s_matrix || hd1->type==s_cmatrix) {
			result=new_real(cc,(real)(dimsof(hd)->r*dimsof(hd)->c!=dimsof(hd1)->r*dimsof(hd1)->c),"");
		} else {
			result=new_real(cc,1.0,"");
		}
	} else if ( (hd1->type==s_matrix || hd1->type==s_cmatrix) && dimsof(hd1)->r*dimsof(hd1)->c==0 ) {
		/* empty matrix */
		if (hd->type==s_matrix || hd->type==s_cmatrix) {
			result=new_real(cc,(real)(dimsof(hd)->r*dimsof(hd)->c!=dimsof(hd1)->r*dimsof(hd1)->c),"");
		} else {
			result=new_real(cc,1.0,"");
		}
	} else {
		result=map2r(cc,runequal,cunequal,hd,hd1);
	}
	return pushresults(cc,result);
}

static void raboutequal (real *x, real *y, real *z)
{	if (fabs(*x-*y)<calc->epsilon) *z=1.0;
	else *z=0.0;
}

static void caboutequal (cplx x, cplx y, real *z)
{
	*z = (fabs(x[0]-y[0])<calc->epsilon && fabs(x[1]-y[1])<calc->epsilon) ? 1.0 : 0.0;
}

header* maboutequal (Calc *cc, header *hd, header *hd1)
{	header *result=NULL;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type==s_string || hd1->type==s_string) cc_error(cc,"This operation cannot be evaluated on strings");
	result=map2r(cc,raboutequal,caboutequal,hd,hd1);
	return pushresults(cc,result);
}

/****************************************************************
 *	boolean operator
 ****************************************************************/
static void r_not (real *x, real *r)
{	if (*x!=0.0) *r=0.0;
	else *r=1.0;
}

static void c_not (cplx x, real *r)
{	if (x[0]==0.0 && x[1]==0.0) *r=1.0;
	else *r=0.0;
}

header* mnot (Calc *cc, header *hd)
{
	header *result;
	hd=getvalue(cc,hd);
	if ( (hd->type==s_matrix || hd->type==s_cmatrix) && dimsof(hd)->r*dimsof(hd)->c==0 ) {
		/* empty matrix */
		hd=new_real(cc,0.0,"");
	}
	result=map1r(cc,r_not,c_not,hd);
	return pushresults(cc,result);
}

static void r_or (real *x, real *y, real *z)
{	if (*x!=0.0 || *y!=0.0) *z=1.0;
	else *z=0.0;
}

header* mor (Calc *cc, header *hd, header *hd1)
{
	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if ( (hd->type==s_matrix || hd->type==s_cmatrix) && dimsof(hd)->r*dimsof(hd)->c==0 ) {
		/* empty matrix */
		hd=new_real(cc,0.0,"");
	} else if ( (hd1->type==s_matrix || hd1->type==s_cmatrix) && dimsof(hd1)->r*dimsof(hd1)->c==0 ) {
		/* empty matrix */
		hd1=new_real(cc,0.0,"");
	}
	result=map2(cc,r_or,NULL,hd,hd1);
	return pushresults(cc,result);
}

static void r_and (real *x, real *y, real *z)
{	if (*x!=0.0 && *y!=0.0) *z=1.0;
	else *z=0.0;
}

header* mand (Calc *cc, header *hd, header *hd1)
{
	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if ( (hd->type==s_matrix || hd->type==s_cmatrix) && dimsof(hd)->r*dimsof(hd)->c==0 ) {
		/* empty matrix */
		hd=new_real(cc,0.0,"");
	} else if ( (hd1->type==s_matrix || hd1->type==s_cmatrix) && dimsof(hd1)->r*dimsof(hd1)->c==0 ) {
		/* empty matrix */
		hd1=new_real(cc,0.0,"");
	}
	result=map2(cc,r_and,NULL,hd,hd1);
	return pushresults(cc,result);
}

/****************************************************************
 *	complex specific functions
 ****************************************************************/
header* mcomplex (Calc *cc, header *hd)
{	header *st=hd,*result=NULL;
	real *m,*mr;
	ULONG i,n;
	int c,r;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_cmatrix(cc,r,c,"");
		n=(ULONG)r*c;
        mr=matrixof(result)+(ULONG)2*(n-1);
		m+=n-1;
		for (i=0; i<n; i++) {
			*mr=*m--; *(mr+1)=0.0; mr-=2;
		}
		result=moveresult(cc,st,result);
	} else if (hd->type==s_real) {
		result=new_complex(cc,*realof(hd),0.0,"");
		result=moveresult(cc,st,result);
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		result=hd;
		result=moveresult(cc,st,result);
	} else cc_error(cc,"Can't make a complex of this!");
	return result;
}

static void c_conj (cplx x, cplx z)
{	z[0]=x[0]; z[1]=-x[1];
}

static real ident (real x)
{	return x;
}

header* mconj (Calc *cc, header *hd)
{
	return spread1(cc,ident,c_conj,hd);
}

static void c_realpart (cplx x, real *z)
{	*z=x[0];
}

header* mre (Calc *cc, header *hd)
{	return spread1r(cc,ident,c_realpart,hd);
}

static real zero (real x)
{	return 0.0;
}

static void c_imagpart (cplx x, real *z)
{	*z=x[1];
}

header* mim (Calc *cc, header *hd)
{	return spread1r(cc,zero,c_imagpart,hd);
}

static real rarg (real x)
{	if (x>=0) return 0.0;
	else return M_PI;
}

static void c_arg (cplx x, real *z)
{	
	*z = atan2(x[1],x[0]);
}

header* marg (Calc *cc, header *hd)
{	return spread1r(cc,rarg,c_arg,hd);
}

static void c_abs (cplx x, real *z)
{	*z=sqrt(x[0]*x[0] + x[1]*x[1]);
}

header* mabs (Calc *cc, header *hd)
{	return spread1r(cc,fabs,c_abs,hd);
}

/****************************************************************
 *	math funcs: exp, log, sqr, sin, cos, tan, asin, acos, atan,
 *              atan2, abs, mod, ^ (pow)
 ****************************************************************/
static void c_sin (cplx x, cplx z)
{	z[0]=cosh(x[1])*sin(x[0]);
	z[1]=sinh(x[1])*cos(x[0]);
}

header* msin (Calc *cc, header *hd) 
{	return spread1(cc,sin,c_sin,hd);
}

static void c_cos (cplx x, cplx z)
{	z[0]=cosh(x[1])*cos(x[0]);
	z[1]=-sinh(x[1])*sin(x[0]);
}

header* mcos (Calc *cc, header *hd)
{	return spread1(cc,cos,c_cos,hd);
}

static void c_tan (cplx x, cplx z)
{
	cplx s, c;
	c_sin(x,s); c_cos(x,c);
	c_div(s,c,z);
}

header* mtan (Calc *cc, header *hd)
{	return spread1(cc,tan,c_tan,hd);
}

void c_log (cplx x, cplx z)
{	
	z[0]=log(sqrt(x[0]*x[0] + x[1]*x[1]));
	c_arg(x,&z[1]);
}

static void c_atan (cplx x, cplx y)
{
	cplx h, g, t;
	h[0]=1-x[1]; h[1]=x[0]; g[0]=1+x[1]; g[1]=-x[0];
	c_div(h,g,t);
	c_log(t,h);
	y[0]=h[1]/2; y[1]=-h[0]/2;
}

header* matan (Calc *cc, header *hd)
{	return spread1(cc,atan,c_atan,hd);
}

static void c_sqrt (cplx x, cplx z)
{	real a,r;
	c_arg(x,&a); a=a/2.0;
	r=sqrt(sqrt(x[0]*x[0] + x[1]*x[1]));
	z[0]=r*cos(a);
	z[1]=r*sin(a);
}

static void c_asin (cplx x, cplx y)
{
	cplx h, g;
	c_mul(x,x,h);
	h[0]=1-h[0]; h[1]=-h[1];
	c_sqrt(h,g);
	h[0]=-x[1]+g[0]; h[1]=x[0]+g[1];
	c_log(h,g);
	y[0]=g[1]; y[1]=-g[0];
}

header* masin (Calc *cc, header *hd)
{	return spread1(cc,asin,c_asin,hd);
}

static void c_acos (cplx x, cplx y)
{
	cplx h, g;
	c_mul(x,x,h);
	h[0]=1-h[0]; h[1]=-h[1];
	c_sqrt(h,g);
	h[1]=x[1]+g[0]; h[0]=x[0]-g[1];
	c_log(h,g);
	y[0]=g[1]; y[1]=-g[0];	
}

header* macos (Calc *cc, header *hd)
{	return spread1(cc,acos,c_acos,hd);
}

static void c_exp (cplx x, cplx z)
{	real r=exp(x[0]);
	z[0]=cos(x[1])*r;
	z[1]=sin(x[1])*r;
}

header* mexp (Calc *cc, header *hd)
{	return spread1(cc,exp,c_exp,hd);
}

header* mlog (Calc *cc, header *hd)
{	return spread1(cc,log,c_log,hd);
}

header* msqrt (Calc *cc, header *hd)
{	return spread1(cc,sqrt,c_sqrt,hd);
}

static void rmod (real *x, real *n, real *y)
{	*y=fmod(*x,*n);
}

header* mmod (Calc *cc, header *hd)
{	return spread2(cc,rmod,0,hd);
}

static void c_pow (cplx x, cplx y, cplx z)
{
	cplx l,w;
	if (fabs(x[0])<calc->epsilon && fabs(x[1])<calc->epsilon) {
		z[0]=z[1]=0.0; return;
	}
	c_log(x,l);
	c_mul(y,l,w);
	c_exp(w,z);
}

static void r_pow (real *x, real *y, real *z)
{	int n;
	if (*x>0.0) *z=pow(*x,*y);
	else if (*x==0.0) if (*y==0.0) *z=1.0; else *z=0.0;
	else {
		n=(int)*y;
		if (n%2) *z=-pow(-*x,n);
		else *z=pow(-*x,n);
	}
}

header* mpower (Calc *cc, header *hd, header *hd1)
{
	header *result;
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	result=map2(cc,r_pow,c_pow,hd,hd1);
	return pushresults(cc,result);
}

header* merf (Calc *cc, header *hd)
{	return spread1(cc,erf,NULL,hd);
}

header* merfc (Calc *cc, header *hd)
{	return spread1(cc,erfc,NULL,hd);
}

/****************************************************************
 *	sign, ceil, floor, round functions
 ****************************************************************/
static real rsign (real x)
{	if (x<0) return -1;
	else if (x<=0) return 0;
	else return 1;
}

header* msign (Calc *cc, header *hd)
{	return spread1(cc,rsign,NULL,hd);
}

header* mceil (Calc *cc, header *hd)
{	return spread1(cc,ceil,NULL,hd);
}

header* mfloor (Calc *cc, header *hd)
{	return spread1(cc,floor,NULL,hd);
}

static real rounder;

static real r_round (real x)
{	x*=rounder;
	if (x>0) x=floor(x+0.5);
	else x=-floor(-x+0.5);
	return x/rounder;
}

static void c_round (cplx x, cplx z)
{	z[0]=r_round(x[0]);
	z[1]=r_round(x[1]);
}

static const real frounder[]={1.0,10.0,100.0,1000.0,10000.0,100000.0,1000000.0,
10000000.0,100000000.0,1000000000.0,10000000000.0};

header* mround (Calc *cc, header *hd)
{	header *hd1;
	int n;
	hd1=next_param(cc,hd); hd1=getvalue(cc,hd1);
	if (hd1->type!=s_real) cc_error(cc,"2nd arg: real value expected");
	n=(int)(*realof(hd1));
	if (n>0 && n<11) rounder=frounder[n];
	else rounder=pow(10.0,n);
	return spread1(cc,r_round,c_round,hd);
}

/****************************************************************
 *	matrix ops
 ****************************************************************/
header* msize (Calc *cc, header *hd)
{	header *result,*hd1=hd,*end=(header *)cc->newram, *old;
	int r=0,c=0,r0=0,c0=0;
	
	result=new_matrix(cc,1,2,"");
	while (end>hd) {
		old=hd1;
		while (hd1 && hd1->type==s_reference)
			hd1=referenceof(hd1);
		if (!hd1) cc_error(cc,"Variable %s not defined!",old->name);
	
		if (hd1->type==s_matrix || hd1->type==s_cmatrix) {
			r=dimsof(hd1)->r;
			c=dimsof(hd1)->c;
		} else if (hd1->type==s_real || hd1->type==s_complex) {
			r=c=1;
		} else if (hd1->type==s_submatrixref || hd1->type==s_csubmatrixref) {
			r=submdimsof(hd1)->r;
			c=submdimsof(hd1)->c;
		} else cc_error(cc,"bad type");
		
		if ((r>1 && r0>1 && r!=r0) || (c>1 && c0>1 && c!=c0)) {
			if (r0!=r && c0!=c) {
				cc_error(cc,"Matrix dimensions must agree!");
			}
		} else {
			if (r>r0) r0=r;
			if (c>c0) c0=c;
		}
        hd=nextof(hd);
	}
	*matrixof(result)=r0;
	*(matrixof(result)+1)=c0;
	return pushresults(cc,result);
}

header* mcols (Calc *cc, header *hd)
{	header *res;
	int n=0;
	hd=getvalue(cc,hd);
	switch (hd->type) {
		case s_matrix :
		case s_cmatrix : n=dimsof(hd)->c; break;
		case s_submatrixref :
		case s_csubmatrixref : n=submdimsof(hd)->c; break;
		case s_real :
		case s_complex : n=1; break;
		case s_string : n=(int)strlen(stringof(hd)); break;
		default : cc_error(cc,"bad type");
	}
	res=new_real(cc,n,"");
	return pushresults(cc,res);
}

header* mrows (Calc *cc, header *hd)
{	header *res;
	int n=0;
	hd=getvalue(cc,hd);
	switch (hd->type) {
		case s_matrix :
		case s_cmatrix : n=dimsof(hd)->r; break;
		case s_submatrixref :
		case s_csubmatrixref : n=submdimsof(hd)->r; break;
		case s_real :
		case s_complex : n=1; break;
		default : cc_error(cc,"bad type");
	}
	res=new_real(cc,n,"");
	return pushresults(cc,res);
}

header* mnonzeros (Calc *cc, header *hd)
{	header *st=hd,*result;
	real *m,*mr;
	int r,c,i,k;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real && hd->type!=s_matrix) cc_error(cc,"real value or matrix expected");
	getmatrix(hd,&r,&c,&m);
	if (r!=1 && c!=1) cc_error(cc,"row or colum vector expected");
	if (c==1) c=r;
	result=new_matrix(cc,1,c,"");
	k=0; mr=matrixof(result);
	for (i=0; i<c; i++) {
		if (*m++!=0.0) {
			*mr++=i+1; k++;
		}
	}
	/* update vector size */
	dimsof(result)->c=k;
	result->size=sizeof(header)+matrixsize(1,k);
	return moveresult(cc,st,result);
}

header* many (Calc *cc, header *hd)
{	header *result;
	int c,r,res=0;
	long i,n=0;
	real *m;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		n=(long)(c)*r;
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
        n=(long)2*(long)(c)*r;
	} else cc_error(cc,"wrong arg type");
	for (i=0; i<n; i++)
		if (*m++!=0.0) { res=1; break; }
	result=new_real(cc,res,"");
	return pushresults(cc,result);
}

header* mall (Calc *cc, header *hd)
{	header *result;
	int c,r,res=1;
	long i,n;
	real *m;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		n=(long)(c)*r;
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
        n=2L*(long)(c)*r;
	} else cc_error(cc,"wrong arg");
	
	for (i=0; i<n; i++)
		if (*m++==0.0) { res=0; break; }
	result=new_real(cc,res,"");
	return pushresults(cc,result);
}

header* mextrema (Calc *cc, header *hd)
{	header *result=NULL;
	real x,*m,*mr,min,max;
	int r,c,i,j,imin,imax;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,r,4,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			min=max=*m; imin=imax=0; m++;
			for (j=1; j<c; j++) {
				x=*m++;
				if (x<min) { min=x; imin=j; }
				if (x>max) { max=x; imax=j; }
			}
			*mr++=min; *mr++=imin+1; *mr++=max; *mr++=imax+1;
		}
	} else cc_error(cc,"real value or matrix expected");
	return pushresults(cc,result);
}

header* mmatrix (Calc *cc, header *hd)
{	header *hd1,*result=NULL;
	long i,n;
	real x,xi;
	real rows,cols,*mr;
	int r=0,c=0;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd);	hd1=getvalue(cc,hd1);

	if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2) {
		rows=*matrixof(hd); cols=*(matrixof(hd)+1);
		r=(rows>-1.0 && rows<(real)INT_MAX) ? (int)rows : 0;
		c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else if (hd->type==s_real) {
		cols=*realof(hd);
		r=1; c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else goto err;
//	if (r<0 || c<0) cc_error(cc,"1st arg: positive integer value or [r,c] vector expected!");
	
	if (hd1->type==s_real) {
		result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		x=*realof(hd1);
		n=(long)c*r;
		for (i=0; i<n; i++) *mr++=x;
	} else if (hd1->type==s_complex) {
		result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
		x=*realof(hd1); xi=*(realof(hd1)+1);
		n=(long)c*r;
		for (i=0; i<n; i++) {
			*mr++=x; *mr++=xi;
		}
	} else goto err;

	return pushresults(cc,result);
err:
	cc_error(cc,"matrix([n,m],v) or matrix(m,v)");
	return NULL;
}

header* mzeros (Calc *cc, header *hd)
{	header *result;
	real rows,cols,*m;
	int r=0,c=0;
	ULONG i,n;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2) {
		rows=*matrixof(hd); cols=*(matrixof(hd)+1);
		r=(rows>-1.0 && rows<(real)INT_MAX) ? (int)rows : 0;
		c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else if (hd->type==s_real) {
		cols=*realof(hd);
		r=1; c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else cc_error(cc,"zeros([n,m]) or zeros(m)");
//	if (r<1 || c<0) cc_error(cc,"positive integer value or [r,c] vector expected!");
	result=new_matrix(cc,r,c,"");
	m=matrixof(result);
	n=c*r;
	for (i=0; i<n; i++) *m++=0.0;
	return pushresults(cc,result);
}

header* mones (Calc *cc, header *hd)
{	header *result;
	real rows,cols,*m;
	int r=0,c=0;
	ULONG i,n;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2) {
		rows=*matrixof(hd); cols=*(matrixof(hd)+1);
		r=(rows>-1.0 && rows<(real)INT_MAX) ? (int)rows : 0;
		c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else if (hd->type==s_real) {
		cols=*realof(hd);
		r=1; c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else cc_error(cc,"ones([n,m]) or ones(m)");
//	if (r<1 || c<0) cc_error(cc,"positive integer value or [r,c] vector expected!");
	result=new_matrix(cc,r,c,"");
	m=matrixof(result);
	n=c*r;
	for (i=0; i<n; i++) *m++=1.0;
	return pushresults(cc,result);
}

header* mdiag (Calc *cc, header *hd)
{	header *st=hd,*result=NULL,*hd1,*hd2=0;
	real rows,cols,*m,*md;
	int r=0,c=0,i,ik=0,k,rd,cd;
	ULONG l,n;
	hd1=next_param(cc,st); hd2=next_param(cc,hd1);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1); hd2=getvalue(cc,hd2);
	if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2) {
		rows=*matrixof(hd); cols=*(matrixof(hd)+1);
		r=(rows>-1.0 && rows<(real)INT_MAX) ? (int)rows : 0;
		c=(cols>-1.0 && cols<(real)INT_MAX) ? (int)cols : 0;
	} else if (hd->type==s_real) {
		rows=*realof(hd);
		r=(rows>-1.0 && rows<(real)INT_MAX) ? (int)rows : 0;
		c=r;
	} else goto err;
//	if (r<1 || c<0)	cc_error(cc,"1st arg: integer [rxc] vector expected");
	if	(hd1->type!=s_real) goto err;
//	cc_error(cc,"2nd arg: real value expected");
	k=(int)*realof(hd1);
	if (hd2->type==s_matrix || hd2->type==s_real) {
		result=new_matrix(cc,r,c,"");
		m=matrixof(result);
		n=(ULONG)c*r;
		for (l=0; l<n; l++) *m++=0.0;
		getmatrix(hd2,&rd,&cd,&md);
		if (rd!=1 || cd<1) goto err;
//		cc_error(cc,"3rd arg: row vector or real value expected");
		m=matrixof(result);
		for (i=0; i<r; i++) {
			if (i+k>=0 && i+k<c) {
				*mat(m,c,i,i+k)=*md;
				ik++; if (ik<cd) md++;
			}
		}
	} else if (hd2->type==s_cmatrix || hd2->type==s_complex) {
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
        n=(ULONG)2*(ULONG)c*r;
		for (l=0; l<n; l++) *m++=0.0;
		getmatrix(hd2,&rd,&cd,&md);
		if (rd!=1 || cd<1) goto err;
		//cc_error(cc,"3rd arg: row vector or real value expected");
		m=matrixof(result);
		for (i=0; i<r; i++) {
			if (i+k>=0 && i+k<c) {
				*cmat(m,c,i,i+k)=*md;
				*(cmat(m,c,i,i+k)+1)=*(md+1);
				ik++; if (ik<cd) md+=2;
			}
		}
	} else goto err;
	return pushresults(cc,result);
err:
	cc_error(cc,"diag([nxm],k,v) or diag(n,k,v)");
	return NULL;
}

header* msetdiag (Calc *cc, header *hd)
{	header *result=NULL,*st=hd,*hd1,*hd2=0;
	real *m,*md,*mhd;
	int r,c,i,ik=0,k,rd,cd;
	hd=getvalue(cc,hd);
	if (hd->type!=s_matrix && hd->type!=s_cmatrix)
		cc_error(cc,"1st arg: matrix expected");
	getmatrix(hd,&c,&r,&mhd);
	hd1=next_param(cc,st); hd2=next_param(cc,hd1);
	hd1=getvalue(cc,hd1); hd2=getvalue(cc,hd2);
	if	(hd1->type!=s_real) cc_error(cc,"2nd arg: real value expected");
	k=(int)*realof(hd1);
	if (hd->type==s_matrix && 
			(hd2->type==s_complex || hd2->type==s_cmatrix)) {
		make_complex(cc,st);
		return msetdiag(cc,st);
	} else if (hd->type==s_cmatrix &&
			(hd2->type==s_real || hd2->type==s_matrix)) {
		make_complex(cc,nextof(nextof(st)));
		return msetdiag(cc,st);
	}
	
	if (hd->type==s_matrix) {
		result=new_matrix(cc,r,c,"");
		m=matrixof(result);
		memmove((char *)m,(char *)mhd,(ULONG)c*r*sizeof(real));
		getmatrix(hd2,&rd,&cd,&md);
		if (rd!=1 || cd<1) cc_error(cc,"3rd arg: row vector or real value expected");
		for (i=0; i<r; i++) {
			if (i+k>=0 && i+k<c) {
				*mat(m,c,i,i+k)=*md;
				ik++; if (ik<cd) md++;
			}
		}
	} else if (hd->type==s_cmatrix) {
		result=new_cmatrix(cc,r,c,"");
		m=matrixof(result);
        memmove((char *)m,(char *)mhd,(ULONG)c*r*(ULONG)2*sizeof(real));
		getmatrix(hd2,&rd,&cd,&md);
		if (rd!=1 || cd<1) cc_error(cc,"3rd arg: row vector or real value expected");
		m=matrixof(result);
		for (i=0; i<r; i++) {
			if (i+k>=0 && i+k<c) {
				*cmat(m,c,i,i+k)=*md;
				*(cmat(m,c,i,i+k)+1)=*(md+1);
				ik++; if (ik<cd) md+=2;
			}
		}
	} else cc_error(cc,"3rd arg: bad type");
	return pushresults(cc,result);
}

header* mdiag2 (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result=NULL;
	int c,r,i,n,l;
	real *m,*mh,*mr;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);

	if (hd1->type!=s_real) cc_error(cc,"2nd arg: real value expected");
	n=(int)*realof(hd1);
	if (hd->type==s_matrix || hd->type==s_real) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,1,r,"");
		mr=matrixof(result); l=0;
		for (i=0; i<r; i++) {
			if (i+n>=c) break;
			if (i+n>=0) { l++; *mr++=*mat(m,c,i,i+n); }
		}
		dimsof(result)->c=l;
		result->size=sizeof(header)+matrixsize(1,c);
	} else if (hd->type==s_cmatrix || hd->type==s_complex) {
		getmatrix(hd,&r,&c,&m);
		result=new_cmatrix(cc,1,r,"");
		mr=matrixof(result); l=0;
		for (i=0; i<r; i++) {
			if (i+n>=c) break;
			if (i+n>=0) {
				l++;
				mh=cmat(m,c,i,i+n);
				*mr++=*mh++;
				*mr++=*mh;
			}
		}
		dimsof(result)->c=l;
		result->size=sizeof(header)+cmatrixsize(1,c);
	}
	else cc_error(cc,"1st arg: real or complex matrix expected");
	return moveresult(cc,st,result);
}

header* mband (Calc* cc, header *hd)
{	header *hd1,*hd2,*result=NULL;
	int i,j,c,r,n1,n2;
	real *m,*mr;
	hd1=next_param(cc,hd); hd2=next_param(cc,hd1);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1); hd2=getvalue(cc,hd2);
	if (hd1->type!=s_real || hd2->type!=s_real) cc_error(cc,"2nd and 3rd args: real value expected");
	n1=(int)*realof(hd1); n2=(int)*realof(hd2);
	if (hd->type==s_matrix || hd->type==s_real) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
				if (j-i>=n1 && j-i<=n2) *mr++=*m++;
				else { *mr++=0.0; m++; }
			}
	} else if (hd->type==s_cmatrix || hd->type==s_complex) {
		getmatrix(hd,&r,&c,&m);
		result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++)
			for (j=0; j<c; j++) {
				if (j-i>=n1 && j-i<=n2) { *mr++=*m++; *mr++=*m++; }
				else { *mr++=0.0; *mr++=0.0; m+=2; }
			}
	}
	else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

header* mdup (Calc *cc, header *hd)
{	header *result=NULL,*st=hd,*hd1;
	real *m,*m1,*m2;
	int c,i,n,j,r;
	hd=getvalue(cc,hd);
	hd1=next_param(cc,st);
	hd1=getvalue(cc,hd1);
	if (hd1->type!=s_real) cc_error(cc,"2nd arg: real value expected");
	n=(int)*realof(hd1);
	if (n<1) cc_error(cc,"2nd arg: must be >= 1");
	if (hd->type==s_matrix && dimsof(hd)->r==1) {
		c=dimsof(hd)->c;
		result=new_matrix(cc,n,c,"");
		m1=matrixof(hd); m2=matrixof(result);
		for (i=0; i<n; i++) {
			m=mat(m2,c,i,0);
			memmove((char *)m,(char *)m1,c*sizeof(real));
		}
	} else if (hd->type==s_matrix && dimsof(hd)->c==1) {
		r=dimsof(hd)->r;
		result=new_matrix(cc,r,n,"");
		m1=matrixof(hd); m2=matrixof(result);
		for (i=0; i<r; i++) {
			for (j=0; j<n; j++)
				*mat(m2,n,i,j)=*mat(m1,1,i,0);
		}
	} else if (hd->type==s_real) {
		result=new_matrix(cc,n,1,"");
		m1=matrixof(result);
		for (i=0; i<n; i++) *m1++=*realof(hd);
	} else if (hd->type==s_cmatrix && dimsof(hd)->r==1) {
		c=dimsof(hd)->c;
		result=new_cmatrix(cc,n,c,"");
		m1=matrixof(hd); m2=matrixof(result);
		for (i=0; i<n; i++) {
			m=cmat(m2,c,i,0);
            memmove((char *)m,(char *)m1,(ULONG)2*c*sizeof(real));
		}
	} else if (hd->type==s_cmatrix && dimsof(hd)->c==1) {
		r=dimsof(hd)->r;
		result=new_cmatrix(cc,r,n,"");
		m1=matrixof(hd); m2=matrixof(result);
		for (i=0; i<r; i++)
		{
			for (j=0; j<n; j++) {
				*cmat(m2,n,i,j)=*cmat(m1,1,i,0);
				*(cmat(m2,n,i,j)+1)=*(cmat(m1,1,i,0)+1);
			}
		}
	} else if (hd->type==s_complex) {
		result=new_cmatrix(cc,n,1,"");
		m1=matrixof(result);
		for (i=0; i<n; i++) {
			*m1++=*realof(hd); *m1++=*imagof(hd);
		}
	} else cc_error(cc,"1st arg: value or vector expected");
	return pushresults(cc,result);
}

header* mredim (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result=NULL;
	int c1,r1;
	real *m;
	unsigned long i,n,size1,size;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd);
	hd1=getvalue(cc,hd1);
	if (hd1->type!=s_matrix || dimsof(hd1)->r!=1 || dimsof(hd1)->c!=2
		|| (hd->type!=s_matrix && hd->type!=s_cmatrix))
		cc_error(cc,"redim(M,[r,c])");
	m=matrixof(hd1);r1=(int)(*m++);c1=(int)(*m);
	if (r1<1 || c1<1) cc_error(cc,"2nd arg [r,c]: new dimensions must be>=1");
	size1=(long)c1*r1;
	size=(long)dimsof(hd)->c*dimsof(hd)->r;
	if (size<size1) n=size;
	else n=size1;
#ifdef ALWAYS_DUPLICATE_WHEN_REDIM
	if (hd->type==s_matrix) {
		result=new_matrix(cc,r1,c1,"");
		memmove((char *)matrixof(result),(char *)matrixof(hd),
			n*sizeof(real));
		if (n<size1) {
			m=matrixof(result)+n;
			for (i=n; i<size1; i++) *m++=0.0;
		}
	} else if (hd->type==s_cmatrix) {
		result=new_cmatrix(cc,r1,c1,"");
		memmove((char *)matrixof(result),(char *)matrixof(hd),
			2*n*sizeof(real));
		if (n<size1) {
			m=matrixof(result)+2*n;
			for (i=n; i<size1; i++) { *m++=0.0; *m++=0.0; }
		}
	} else cc_error(cc,"1st arg: vector or matrix expected");
#else
	if (size==size1) {
		dimsof(hd)->r=r1; dimsof(hd)->c=c1;
		result = (hd==st) ? hd : new_reference(cc,hd,"");
	} else if (hd->type==s_matrix) {
		result=new_matrix(cc,r1,c1,"");
		memmove((char *)matrixof(result),(char *)matrixof(hd),
			n*sizeof(real));
		if (n<size1) {
			m=matrixof(result)+n;
			for (i=n; i<size1; i++) *m++=0.0;
		}
	} else if (hd->type==s_cmatrix) {
		result=new_cmatrix(cc,r1,c1,"");
		memmove((char *)matrixof(result),(char *)matrixof(hd),
			2*n*sizeof(real));
		if (n<size1) {
			m=matrixof(result)+2*n;
			for (i=n; i<size1; i++) { *m++=0.0; *m++=0.0; }
		}
	} else cc_error(cc,"1st arg: vector or matrix expected");
#endif
	return pushresults(cc,result);
}

header* msum (Calc *cc, header *hd)
{	header *result=NULL;
	int c,r,i,j;
	real *m,*mr,s,si;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		if (r*c==0) {r=1; c=0;}				/* empty matrix */
		result=new_matrix(cc,r,1,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			s=0.0;
			for (j=0; j<c; j++) s+=*m++;
			*mr++=s;
		}
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		if (r*c==0) {r=1; c=0;}				/* empty matrix */
		result=new_cmatrix(cc,r,1,"");
		mr=matrixof(result);
		for (i=0; i<r; i++)  {
			s=0.0; si=0.0;
			for (j=0; j<c; j++) { s+=*m++; si+=*m++; }
			*mr++=s; *mr++=si;
		}
	} else cc_error(cc,"real or complex value or matrix expected");
	return pushresults(cc,result);
}

header* mcolsum (Calc *cc, header *hd)
{
	header *result=NULL;
	int c,r,i,j;
	real *m,*mr,*p,s,si;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,1,c,"");
		mr=matrixof(result);
		for (i=0; i<c; i++) {
			s=0.0; p=m+i;
			for (j=0; j<r; j++) {s+=*p;p+=c;}
			*mr++=s;
		}
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_cmatrix(cc,1,c,"");
		mr=matrixof(result);
		for (i=0; i<c; i++)  {
			s=0.0; si=0.0; p=m+2*i;
			for (j=0; j<r; j++) { s+=*p; si+=*(p+1); p+=2*c;}
			*mr++=s; *mr++=si;
		}
	} else cc_error(cc,"real or complex value or matrix expected");
	
	return pushresults(cc,result);
}

header* mprod (Calc *cc, header *hd)
{	header *result=NULL;
	int c,r,i,j;
	real *m,*mr;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		real s;
		getmatrix(hd,&r,&c,&m);
		if (r*c==0) {r=1; c=0;}				/* empty matrix */
		result=new_matrix(cc,r,1,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			s=1.0;
			for (j=0; j<c; j++) s*=*m++;
			*mr++=s;
		}
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		cplx s, h;
		getmatrix(hd,&r,&c,&m);
		if (r*c==0) {r=1; c=0;}				/* empty matrix */
		result=new_cmatrix(cc,r,1,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			s[0]=1.0; s[1]=0.0;
			for (j=0; j<c; j++) {
				c_mul(s,m,h);
				s[0]=h[0]; s[1]=h[1]; m+=2; 
			}
			*mr++=s[0]; *mr++=s[1];
		}
	} else cc_error(cc,"real or complex value or matrix expected");
	return pushresults(cc,result);
}

header* mcumsum (Calc *cc, header *hd)
{	header *result=NULL;
	real *m,*mr,sum=0,sumr=0,sumi=0;
	int r,c,i,j;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		if (c<1) result=new_matrix(cc,r,1,"");
		else result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			if (c>=1) sum=*m++;
			*mr++=sum;
			for (j=1; j<c; j++) {
				sum+=*m++;
				*mr++=sum;
			}
		}
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		if (c<1) result=new_cmatrix(cc,r,1,"");
		else result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			if (c>=1) { sumr=*m++; sumi=*m++; }
			*mr++=sumr; *mr++=sumi;
			for (j=1; j<c; j++) {
				sumr+=*m++; *mr++=sumr;
				sumi+=*m++; *mr++=sumi;
			}
		}
	}
	else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

header* mcumprod (Calc *cc, header *hd)
{	header *result=NULL;
	real *m,*mr,sum=1,sumi=1,sumr=0;
	int r,c,i,j;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		if (c<1) result=new_matrix(cc,r,1,"");
		else result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			if (c>=1) sum=*m++; 
			*mr++=sum;
			for (j=1; j<c; j++) {
				sum*=*m++;
				*mr++=sum;
			}
		}
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		if (c<1) result=new_cmatrix(cc,r,1,"");
		else result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			if (c>=1) { sumr=*m++; sumi=*m++; }
			*mr++=sumr; *mr++=sumi;
			for (j=1; j<c; j++) {
				sum=sumr*(*m)-sumi*(*(m+1));
				sumi=sumr*(*(m+1))+sumi*(*m);
				sumr=sum;
				m+=2;
				*mr++=sumr;
				*mr++=sumi;
			}
		}
	} else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

header* mflipx (Calc *cc, header *hd)
{	header *result=NULL;
	real *m,*mr,*mr1;
	int i,j,c,r;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_complex) {
		result=hd;
	} else if (hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			mr1=mr+(c-1);
			for (j=0; j<c; j++) *mr1--=*m++;
			mr+=c;
		}
	} else if (hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			mr1=mr+(2l*(c-1)+1);
			for (j=0; j<c; j++) {
				*mr1--=*m++; *mr1--=*m++;
			}
			mr+=2l*c;
		}
	} else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

header* mflipy (Calc *cc, header *hd)
{	header *result=NULL;
	real *m,*mr;
	int i,c,r;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_complex) {	
		result=hd;
	} else if (hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		mr+=(long)(r-1)*c;
		for (i=0; i<r; i++) {
			memmove((char *)mr,(char *)m,c*sizeof(real));
			m+=c; mr-=c;
		}
	} else if (hd->type==s_cmatrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
		mr+=2l*(long)(r-1)*c;
		for (i=0; i<r; i++) {
			memmove((char *)mr,(char *)m,2l*c*sizeof(real));
			m+=2l*c; mr-=2l*c;
		}
	} else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

static void r_max (real *x, real *y, real *z)
{	if (*x>*y) *z=*x;
	else *z=*y;
}

header* mmax (Calc *cc, header *hd)
{	return spread2(cc,r_max,0,hd);
}

static void r_min (real *x, real *y, real *z)
{	if (*x>*y) *z=*y;
	else *z=*x;
}

header* mmin (Calc *cc, header *hd)
{	return spread2(cc,r_min,0,hd);
}

typedef struct { real val; int ind; } sorttyp;

static int sorttyp_compare (const sorttyp *x, const sorttyp *y)
{	if (x->val>y->val) return 1;
	else if (x->val==y->val) return 0;
	else return -1;
}

header* msort (Calc *cc, header *hd)
{	header *result=NULL,*result1;
	real *m,*m1;
	sorttyp *t;
	int r,c,i;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real && hd->type!=s_matrix) cc_error(cc,"real value or matrix expected");
	getmatrix(hd,&r,&c,&m);
	if (c==1 || r==1) result=new_matrix(cc,r,c,"");
	else cc_error(cc,"row or colum vector expected");
	result1=new_matrix(cc,r,c,"");
	if (c==1) c=r;
	if (c==0) cc_error(cc,"can't sort a 0-sized vector");
	if (cc->newram+c*sizeof(sorttyp)>cc->udfstart) cc_error(cc,"Out of memory!");
	t=(sorttyp *)cc->newram;
	for (i=0; i<c; i++) {
		t->val=*m++; t->ind=i; t++;
	}
	qsort(cc->newram,c,sizeof(sorttyp),
		(int (*) (const void *, const void *))sorttyp_compare);
	m=matrixof(result); m1=matrixof(result1);
	t=(sorttyp *)cc->newram;
	for (i=0; i<c; i++) {
		*m++=t->val; *m1++=t->ind+1; t++;
	}
	return pushresults(cc,result); /* result and result1 */
}

header* mstatistics (Calc *cc, header *hd)
{	header *hd1,*result;
	int i,n,r,c,k;
	real *m,*mr;
	hd1=next_param(cc,hd); 
	hd=getvalue(cc,hd);	hd1=getvalue(cc,hd1);
	if (hd1->type!=s_real || hd->type!=s_matrix) cc_error(cc,"1st arg: real value or matrix expected");
	if (*realof(hd1)>INT_MAX || *realof(hd1)<2) cc_error(cc,"2nd arg >= 2");
	n=(int)*realof(hd1);
	getmatrix(hd,&r,&c,&m);
	if (r!=1 && c!=1) cc_error(cc,"1st arg: real row or column vector expected");
	if (c==1) c=r;
	result=new_matrix(cc,1,n,"");
	mr=matrixof(result); for (i=0; i<n; i++) *mr++=0.0;
	mr=matrixof(result);
	for (i=0; i<c; i++) {
		if (*m>=0 && *m<n) {
		k=floor(*m);
			mr[k]+=1.0;
		}
		m++;
	}
	return pushresults(cc,result);
}

header* mmax1 (Calc *cc, header *hd)
{	header *result=NULL;
	real x,*m,*mr,max;
	int r,c,i,j;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,r,1,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			max=*m; m++;
			for (j=1; j<c; j++) {
				x=*m++;
				if (x>max) max=x;
			}
			*mr++=max;
		}
	}
	else cc_error(cc,"real value or matrix expected");
	return pushresults(cc,result);
}

header* mmin1 (Calc *cc, header *hd)
{	header *result=NULL;
	real x,*m,*mr,max;
	int r,c,i,j;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&m);
		result=new_matrix(cc,r,1,"");
		mr=matrixof(result);
		for (i=0; i<r; i++) {
			max=*m; m++;
			for (j=1; j<c; j++) {
				x=*m++;
				if (x<max) max=x;
			}
			*mr++=max;
		}
	} else cc_error(cc,"real value or matrix expected");
	return pushresults(cc,result);
}

/****************************************************************
 *	polynom ops
 ****************************************************************/
#define max(x,y) ((x)>(y)?(x):(y))

static real *polynom;
static int degree, polreal;

static real peval (real x)
{	int i;
	real *p=polynom+degree,res;
	res=*p--;
	for (i=degree-1; i>=0; i--) res=res*x+(*p--);
	return res;
}

static void cpeval (cplx x, cplx z)
{	int i;
	real *p;
	cplx h;
	p=polynom+(polreal?degree:(2l*degree));
	*z=*p; z[1]=(polreal)?0.0:*(p+1);
	if (polreal) p--;
	else p-=2;
	for (i=degree-1; i>=0; i--) {
		c_mul(x,z,h);
		z[0]= h[0] + *p;
		if (!polreal) {
			z[1]=h[1]+*(p+1); p--;
		} else {
			z[1]=h[1];
		}
		p--;
	}
}

header* mpolyval (Calc *cc, header *hd)
{	header *hd1;
	int r,c;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&polynom);
		if (r!=1 || c<1) cc_error(cc,"non 0-sized row vector expected");
		degree=c-1;
		polreal=1;
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		make_complex(cc,hd1);
		getmatrix(hd,&r,&c,&polynom);
		if (r!=1 || c<1) cc_error(cc,"non 0-sized row vector expected");
		degree=c-1;
		polreal=0;
	} else cc_error(cc,"bad type");
	return spread1(cc,peval,cpeval,hd1);
}

static int testparams (Calc *cc, header **hd1, header **hd2)
{	header *h1=*hd1,*h2=*hd2;
	*hd1=getvalue(cc,h1);
	*hd2=getvalue(cc,h2);
	if ((*hd1)->type==s_complex || (*hd1)->type==s_cmatrix
		|| (*hd2)->type==s_complex || (*hd2)->type==s_cmatrix) {
			if ((*hd1)->type!=s_complex && (*hd1)->type!=s_cmatrix) {
			make_complex(cc,h1); *hd1=h1; *hd2=nextof(h1);
			*hd2=getvalue(cc,*hd2);
		} else if ((*hd2)->type!=s_complex && (*hd2)->type!=s_cmatrix) {
			make_complex(cc,h2); *hd2=h2;
		}
		return 1;
	}
	else return 0;
}

header* mpolyadd (Calc *cc, header *hd)
{	header *hd1,*result;
	int flag,c,c1,c2,i,r1,r2;
	real *m1,*m2,*m;
	
	hd1=next_param(cc,hd);
	flag=testparams(cc,&hd,&hd1);
	getmatrix(hd,&r1,&c1,&m1);
	getmatrix(hd1,&r2,&c2,&m2);
	if (r1!=1 || r2!=1) cc_error(cc,"row vector expected");
	
	c=max(c1,c2);
	if (flag) {/* complex values */
		result=new_cmatrix(cc,1,c,"");
		m=matrixof(result);
		for (i=0; i<c; i++) {
			if (i>=c1) {
				c_copy(m2,m); m+=2; m2+=2;
			} else if (i>=c2) {
				c_copy(m1,m); m+=2; m1+=2;
			} else {
				c_add(m1,m2,m); m1+=2; m2+=2; m+=2;
			}
		}
	} else {
		result=new_matrix(cc,1,c,"");
		m=matrixof(result);
		for (i=0; i<c; i++) {
			if (i>=c1) {
				*m++ = *m2++;
			} else if (i>=c2) {
				*m++ = *m1++;
			} else {
				*m++ = *m1++ + *m2++;
			}
		}	
	}
	return pushresults(cc,result);
}


header* mpolymult (Calc *cc, header *hd)
{	header *hd1,*result;
	int flag,c,c1,c2,i,r1,r2,j,k;
	real *m1,*m2,*mr,x;
	cplx *mc1,*mc2,*mcr,xc,hc;
	hd1=next_param(cc,hd);
	flag=testparams(cc,&hd,&hd1);
	getmatrix(hd,&r1,&c1,&m1);
	getmatrix(hd1,&r2,&c2,&m2);
	if ((r1!=1 && c1<1) || (r2!=1 && c2<1)) cc_error(cc,"row vector expected");
	
	if ((ULONG)c1+c2-1>INT_MAX) cc_error(cc,"can't handle those large vectors");
	c=c1+c2-1;
	if (flag) {
		mc1=(cplx*)m1; mc2=(cplx*)m2;
		result=new_cmatrix(cc,1,c,"");
		mcr=(cplx*)matrixof(result);
		c_copy(*mc1,xc); mc1++;
		for (i=0; i<c2; i++) c_mul(xc,mc2[i],mcr[i]);
		for (j=1; j<c1; j++) {
			c_copy(*mc1,xc); mc1++;
			for (k=j,i=0; i<c2-1; i++,k++) {
				c_mul(xc,mc2[i],hc);
				c_add(hc,mcr[k],mcr[k]);
			}
			c_mul(xc,mc2[i],mcr[k]);
		}
	} else {
		result=new_matrix(cc,1,c,"");
		mr=matrixof(result);
		x=*m1++;
		for (i=0; i<c2; i++) mr[i]=x*m2[i];
		for (j=1; j<c1; j++) {
			x=*m1++;
			for (k=j,i=0; i<c2-1; i++,k++) mr[k]+=x*m2[i];
			mr[k]=x*m2[i];
		}
	}
	return pushresults(cc,result);
}

header* mpolydiv (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result,*rest;
	int flag,c1,c2,i,r1,r2,j;
	real *m1,*m2,*mr,*mh,x,l;
	cplx *mc1,*mc2,*mcr,*mch,xc,lc,hc;
	hd1=next_param(cc,hd);
	flag=testparams(cc,&hd,&hd1);
	getmatrix(hd,&r1,&c1,&m1);
	getmatrix(hd1,&r2,&c2,&m2);
	if ((r1!=1 &&c1<1) || (r2!=1 && c2<1)) cc_error(cc,"row vector expected");
	if (c1<c2) {
		result=new_real(cc,0.0,"");
		rest=(header *)cc->newram;
		return moveresult(cc,rest,hd1);
	} else if (flag) {
		mc1=(cplx*)m1; mc2=(cplx*)m2;
		result=new_cmatrix(cc,1,c1-c2+1,"");
		mcr=(cplx*)matrixof(result);
		rest=new_cmatrix(cc,1,c2,"");
		mch=(cplx*)cc->newram;
		if (cc->newram+c1*sizeof(cplx)>cc->udfstart) cc_error(cc,"Out of memory!");
		memmove((char*)mch,(char*)mc1,c1*sizeof(cplx));
		c_copy(mc2[c2-1],lc);
		if (lc[0]==0.0 && lc[1]==0.0) cc_error(cc,"");
		for (i=c1-c2; i>=0; i--) {
			c_div(mch[c2+i-1],lc,xc); c_copy(xc,mcr[i]);
			for(j=0; j<c2; j++) {
				c_mul(mc2[j],xc,hc);
				c_sub(mch[i+j],hc,mch[i+j]);
			}
		}
		memmove((char*)matrixof(rest),(char*)mch,c2*sizeof(cplx));
	} else {
		result=new_matrix(cc,1,c1-c2+1,"");
		mr=matrixof(result);
		rest=new_matrix(cc,1,c2,"");
		mh=(real *)cc->newram;
		if (cc->newram+c1*sizeof(real)>cc->udfstart) cc_error(cc,"Out of memory!");
		memmove((char *)mh,(char *)m1,c1*sizeof(real));
		l=m2[c2-1];
		if (l==0.0) cc_error(cc,"");
		for (i=c1-c2; i>=0; i--) {
			x=mh[c2+i-1]/l; mr[i]=x;
			for(j=0; j<c2; j++) mh[i+j]-=m2[j]*x;
		}
		memmove((char*)matrixof(rest),(char*)mh,c2*sizeof(real));
	}
	moveresult(cc,st,result);
	moveresult(cc,nextof(st),rest);
	return st;
}

header* mpolycons (Calc *cc, header *hd)
{	header *result=NULL;
	int i,j,r,c;
	real *m,*mr,x;
	cplx *mc,*mcr,xc,hc;
	hd=getvalue(cc,hd);
	getmatrix(hd,&r,&c,&m);
	if (r!=1 && c<1) cc_error(cc,"row vector expected");
	if (hd->type==s_real || hd->type==s_matrix) {
		result=new_matrix(cc,1,c+1,"");
		mr=matrixof(result);
		mr[0]=-m[0]; mr[1]=1.0;
		for (i=1; i<c; i++) {
			x=-m[i]; mr[i+1]=1.0;
			for (j=i; j>=1; j--) mr[j]=mr[j-1]+x*mr[j];
			mr[0]*=x;
		}
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		mc=(cplx*)m;
		result=new_cmatrix(cc,1,c+1,"");
		mcr=(cplx*)matrixof(result);
		mcr[0][0]=-mc[0][0]; mcr[0][1]=-mc[0][1];
		mcr[1][0]=1.0; mcr[1][1]=0.0;
		for (i=1; i<c; i++) {
			xc[0]=-mc[i][0]; xc[1]=-mc[i][1];
			mcr[i+1][0]=1.0; mcr[i+1][1]=0.0;
			for (j=i; j>=1; j--) {
				c_mul(xc,mcr[j],hc);
				c_add(hc,mcr[j-1],mcr[j]);
			}
			c_mul(xc,mcr[0],mcr[0]);
		}
	} else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

header* mpolytrunc (Calc *cc, header *hd)
{	header *result=NULL;
	real *m;
	cplx *mc;
	int i;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix && dimsof(hd)->r==1) {
		m=matrixof(hd);
		for (i=dimsof(hd)->c-1; i>=0; i--) {
			if (fabs(m[i])>cc->epsilon) break;
		}
		if (i<0) {
			result=new_real(cc,0.0,"");
		} else {
			result=new_matrix(cc,1,i+1,"");
			memmove((char *)matrixof(result),(char *)matrixof(hd),
				(i+1)*sizeof(real));
		}
	} else if (hd->type==s_complex && dimsof(hd)->r==1) {
		mc=(cplx*)matrixof(hd);
		for (i=dimsof(hd)->c-1; i>=0; i--) {
			if (fabs(mc[i][0])>cc->epsilon && fabs(mc[i][1])>cc->epsilon) 
				break;
		}
		if (i<0) {
			result=new_complex(cc,0.0,0.0,"");
		} else {
			result=new_cmatrix(cc,1,i+1,"");
			memmove((char *)matrixof(result),(char *)matrixof(hd),
				(i+1)*sizeof(cplx));
		}
	}
	else cc_error(cc,"bad type");
	return pushresults(cc,result);
}

/*********** divided difference interpolation polynom ***********/
header* dd (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result;
	int flag,c1,c2,i,j,r;
	real *m1,*m2,*mr;
	cplx *mc1,*mc2,*mcr,hc1,hc2;
	hd1=next_param(cc,st);
	flag=testparams(cc,&hd,&hd1);
	getmatrix(hd,&r,&c1,&m1);
	if (r!=1) cc_error(cc,"row vector expected");
	getmatrix(hd1,&r,&c2,&m2);
	if (r!=1) cc_error(cc,"row vector expected");
	if (c1!=c2) cc_error(cc,"columns must agree");
	if (flag) {	/* complex values */
		mc1=(cplx*)m1; mc2=(cplx*)m2;
		result=new_cmatrix(cc,1,c1,"");
		mcr=(cplx*)matrixof(result);
		memmove((char *)mcr,(char *)mc2,c1*sizeof(cplx));
		for (i=1; i<c1; i++) {
			for (j=c1-1; j>=i; j--) {
				if (mc1[j][0]==mc1[j-i][0] &&
					mc1[j][1]==mc1[j-i][1]) cc_error(cc,"");
				c_sub(mcr[j],mcr[j-1],hc1);
				c_sub(mc1[j],mc1[j-i],hc2);
				c_div(hc1,hc2,mcr[j]);
			}
		}	
	} else {
		result=new_matrix(cc,1,c1,"");
		mr=matrixof(result);
		memmove((char *)mr,(char *)m2,c1*sizeof(real));
		for (i=1; i<c1; i++) {
			for (j=c1-1; j>=i; j--) {
				if (m1[j]==m1[j-i]) cc_error(cc,"");
				mr[j]=(mr[j]-mr[j-1])/(m1[j]-m1[j-i]);
			}
		}	
	}
	return pushresults(cc,result);
}

static real *divx,*divdif;
//static int degree,polreal;

static real rddeval (real x)
{	int i;
	real *p=divdif+degree,res;
	res=*p--;
	for (i=degree-1; i>=0; i--) res=res*(x-divx[i])+(*p--);
	return res;
}

static void cddeval (cplx x, cplx z)
{	int i;
	real *p,*dd;
	cplx h, xh;
	p=divdif+(polreal?degree:(2l*degree));
	dd=divx+(polreal?(degree-1):(2l*(degree-1)));
	z[0]=*p; z[1]=(polreal)?0.0:*(p+1);
	if (polreal) p--;
	else p-=2;
	for (i=degree-1; i>=0; i--) {
		xh[0]=x[0]-*dd;
		if (!polreal) { xh[1]=x[1]-*(dd+1); dd--; }
		else xh[1]=x[1];
		dd--;
		c_mul(xh,z,h);
		z[0]= h[0] + *p;
		if (!polreal) { z[1]=h[1]+*(p+1); p--; }
		else { z[1]=h[1]; }
		p--;
	}
}

header* ddval (Calc *cc, header *hd)
{	header *st=hd,*hdd,*hd1;
	int r,c,c1;
restart:
	hdd=next_param(cc,st);
	hd1=next_param(cc,hdd);
	hd=getvalue(cc,st);
	hdd=getvalue(cc,hdd);
	if (hd->type==s_real || hd->type==s_matrix) {
		getmatrix(hd,&r,&c,&divx);
		if (r!=1 || c<1) cc_error(cc,"non 0-sized row vector expected");
		degree=c-1;
		polreal=1;
		if (hdd->type!=s_real && hdd->type!=s_matrix) {
			if (hdd->type==s_complex || hdd->type==s_cmatrix) {
				make_complex(cc,st); goto restart;
			} else cc_error(cc,"bad type");
		}
		getmatrix(hdd,&r,&c1,&divdif);
		if (r!=1 || c1!=c) cc_error(cc,"");
	} else if (hd->type==s_complex || hd->type==s_cmatrix) {
		make_complex(cc,hd1);
		getmatrix(hd,&r,&c,&divx);
		if (r!=1 || c<1) cc_error(cc,"non 0-sized row vector expected");
		degree=c-1;
		polreal=0;
		if (hdd->type!=s_complex && hdd->type!=s_cmatrix) {
			if (hdd->type==s_real || hdd->type==s_matrix) {
				make_complex(cc,nextof(st)); goto restart;
			} else cc_error(cc,"bad type");
		}
		getmatrix(hdd,&r,&c1,&divdif);
		if (r!=1 || c1!=c) cc_error(cc,"");
	} else cc_error(cc,"bad type");
	return spread1(cc,rddeval,cddeval,hd1);
}

header* polydd (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result;
	int flag,c1,c2,i,j,r;
	real *m1,*m2,*mr,x;
	cplx *mc1,*mc2,*mcr,hc,xc;
	hd1=next_param(cc,st);
	flag=testparams(cc,&hd,&hd1);
	getmatrix(hd,&r,&c1,&m1);
	if (r!=1) cc_error(cc,"row vector expected");
	getmatrix(hd1,&r,&c2,&m2);
	if (r!=1) cc_error(cc,"row vector expected");
	if (c1!=c2) cc_error(cc,"columns must agree");
	if (flag) {	/* complex values */
		mc1=(cplx*)m1; mc2=(cplx*)m2;
		result=new_cmatrix(cc,1,c1,"");
		mcr=(cplx*)matrixof(result);
		c_copy(mc2[c1-1],mcr[c1-1]);
		for (i=c1-2; i>=0; i--) {
			c_copy(mc1[i],xc);
			c_mul(xc,mcr[i+1],hc);
			c_sub(mc2[i],hc,mcr[i]);
			for (j=i+1; j<c1-1; j++) {
				c_mul(xc,mcr[j+1],hc);
				c_sub(mcr[j],hc,mcr[j]);
			}
		}
	} else {
		result=new_matrix(cc,1,c1,"");
		mr=matrixof(result);
		mr[c1-1]=m2[c1-1];
		for (i=c1-2; i>=0; i--) {
			x=m1[i];
			mr[i]=m2[i]-x*mr[i+1];
			for (j=i+1; j<c1-1; j++) mr[j]=mr[j]-x*mr[j+1];
		}
	}
	return pushresults(cc,result);
}

/********* Lagrange interpolation (barycentric formula) *********/
/** {y,weights}=lagr(x,xi,yi)
 *    interpolate the Lagrange polynom at points (xi,yi), then
 *    calculates the values at abscissis x using the barycentric
 *    formula
 */
header* mlagr (Calc *cc, header *hd)
{	header *hd1,*hd2,*hw,*hy;
	int ri, ci, r, c;
	real *mxi, *myi, *mw, *mx, *my;
	hd1=next_param(cc,hd); hd2=next_param(cc,hd1);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1); hd2=getvalue(cc,hd2);
	if (!(hd->type==s_matrix && hd1->type==s_matrix && hd2->type==s_matrix
		&& dimsof(hd)->r==1 && dimsof(hd)->c>1
		&& dimsof(hd1)->r==1 && dimsof(hd1)->c>1
		&& dimsof(hd2)->r==1 && dimsof(hd2)->c==dimsof(hd1)->c)) cc_error(cc,"bad parameter in lagr(x,xi,yi)");
	/* create y matrix*/
	getmatrix(hd,&r,&c,&mx);
	hy=new_matrix(cc,r,c,"");
	/* calculate the weights */
	getmatrix(hd1,&ri,&ci,&mxi);
	hw=new_matrix(cc,1,ci,""); mw=matrixof(hw);
	for (int i=0; i<ci; i++) {
		real p=1.0;
		for (int k=0; k<ci; k++) {
			if (k==i) continue;
			p *= mxi[i]-mxi[k];
		}
		mw[i]=1/p;
	}
	/* evaluate the Lagrange polynom at x, using the barycentric formula */
	myi=matrixof(hd2);
	my=matrixof(hy);
	for (int k=0; k<c; k++) {
		real num=0.0, den=0.0;
		for (int i=0; i<ci; i++) {
			real ai;
			if (mx[k]==mxi[i]) { num=myi[i]; den=1.0; break;}
			ai=mw[i]/(mx[k]-mxi[i]);
			num+=ai*myi[i]; den+=ai;
		}
		my[k]=num/den;
	}
	return pushresults(cc,hy);
}
/************** bauhuber algorithm ***************/

#define ITERMAX 200
#define EPS (64*EPSILON)
#define QR 0.1
#define QI 0.8
#define EPSROOT (64*calc->epsilon)
#define BETA (2096*EPSROOT)

static void quadloes (real ar, real ai, real br, real bi,
	real cr, real ci, real *treal, real *timag)
{	real pr,pi,qr,qi,h;
	pr=br*br-bi*bi; pi=2*br*bi;
	qr=ar*cr-ai*ci; qi=ar*ci+ai*cr;
	pr=pr-4*qr; pi=pi-4*qi;
	h=sqrt(pr*pr+pi*pi);
	qr=h+pr; if (qr<0.0) qr=0; 
	qr=sqrt(qr/2);
	qi=h-pr; if (qi<0.0) qi=0; 
	qi=sqrt(qi/2);
	if (pi<0.0) qi=-qi;
	h=qr*br+qi*bi;
	if (h>0.0) { qr=-qr; qi=-qi; }
	pr=qr-br; pi=qi-bi;
	h=pr*pr+pi*pi;
	*treal=2*(cr*pr+ci*pi)/h;
	*timag=2*(ci*pr-cr*pi)/h;
}

static int cxdiv (real ar, real ai, real br, real bi,
	real *cr, real *ci)
{	real temp;
	if (br==0.0 && bi==0.0) return 1;
	if (fabs(br)>fabs(bi))
	{	temp=bi/br; br=temp*bi+br;
		*cr=(ar+temp*ai)/br;
		*ci=(ai-temp*ar)/br;
	}
	else
	{	temp=br/bi; bi=temp*br+bi;
		*cr=(temp*ar+ai)/bi;
		*ci=(temp*ai-ar)/bi;
	}
	return 0;
}

static real cxxabs (real ar, real ai)
{	if (ar==0.0) return fabs(ai);
	if (ai==0.0) return fabs(ar);
	return sqrt(ai*ai+ar*ar);
}

static void chorner (int n, int iu, real *ar, real *ai,
	real xr, real xi, real *pr, real *pi,
	real *p1r, real *p1i, real *p2r, real *p2i,
	real *rf1)
{	register int i,j;
	int i1;
	real temp,hh,tempr=0.0,tempi=0.0;
	*pr=ar[n]; *pi=ai[n];
	*p1r=*p2r=0.0; *p1i=*p2i=0.0;
	*rf1=cxxabs(*pr,*pi);
	i1=n-iu;
	for (j=n-iu,i=n-1; i>=iu; i--,j--)
	{	if (i<n-1)
		{	tempr=*p1r; tempi=*p1i;
			*p1r=*p1r * xr - *p1i * xi;
			*p1i=*p1i * xr + tempr * xi;
		}
		*p1r+=*pr; *p1i+=*pi;
		temp=*pr;
		*pr=*pr * xr - *pi * xi + ar[i];
		*pi=*pi * xr + temp * xi + ai[i];
		temp=cxxabs(*p1r,*p1i);
		hh=cxxabs(*pr,*pi); if (hh>temp) temp=hh;
		if (temp>*rf1)
		{	*rf1=temp; i1=j-1;
		}
		if (i<n-1)
		{	temp=*p2r;
			*p2r=*p2r * xr - *p2i * xi + tempr*2;
			*p2i=*p2i * xr + temp * xi + tempi*2;
		}
	}
	temp=cxxabs(xr,xi);
	if (temp!=0.0)
		*rf1=pow(temp,(real)i1)*(i1+1);
	else
		*rf1=cxxabs(*p1r,*p1i);
	*rf1*=EPS;
}

#if 0
static void scpoly (int n, real *ar, real *ai, real *scal)
{	real p,h;
	int i;
	*scal=0.0;
	p=cxxabs(ar[n],ai[n]);
	for (i=0; i<n; i++)
	{	ai[i]/=p; ar[i]/=p;
		h=pow(cxxabs(ar[i],ai[i]),1.0/(n-i));
		if (h>*scal) *scal=h;
	}
	ar[n]/=p; ai[n]/=p;
	if (*scal==0.0) *scal=1.0;
	for (p=1.0,i=n-1; i>=0; i--)
	{	p*= *scal;
		ar[i]/=p; ai[i]/=p;
	}
}
#endif

static void bauroot (int n, int iu, real *ar, real *ai, real *x0r,
	real *x0i)
{	int iter=0,i=0,aborted=0;
	real xoldr,xoldi,xnewr,xnewi,h,h1,h2,h3,h4,dzmax,dzmin,
		dxr=1,dxi=0,tempr,tempi,abs_pold,abs_pnew,abs_p1new,
		temp,ss,u,v,
		pr,pi,p1r,p1i,p2r,p2i,abs_pnoted=-1;
		
	dxr=dxi=xoldr=xoldi=0.0;
	if (n-iu==1)
	{	quadloes(0.0,0.0,ar[n],ai[n],
			ar[n-1],ai[n-1],x0r,x0i);
		goto stop;
	}
	if (n-iu==2)
	{	quadloes(ar[n],ai[n],ar[n-1],ai[n-1],
			ar[n-2],ai[n-2],x0r,x0i);
		goto stop;
	}
	xnewr=*x0r; xnewi=*x0i;
	chorner(n,iu,ar,ai,xnewr,xnewi,&pr,&pi,&p1r,&p1i,&p2r,&p2i,&ss);
	iter++;
	abs_pnew=cxxabs(pr,pi);
	if (abs_pnew==0) goto stop;
	abs_pold=abs_pnew;
	dzmin=BETA*(1+cxxabs(xnewr,xnewi));
	while (!aborted)
	{	abs_p1new=cxxabs(p1r,p1i);
		iter++;
		if (abs_pnew>abs_pold) /* Spiraling */
		{	i=0;
			temp=dxr;
			dxr=QR*dxr-QI*dxi;
			dxi=QR*dxi+QI*temp;
		}
		else /* Newton step */
		{	
			dzmax=1.0+cxxabs(xnewr,xnewi);
			h1=p1r*p1r-p1i*p1i-pr*p2r+pi*p2i;
			h2=2*p1r*p1i-pr*p2i-pi*p2r;
			if (abs_p1new>10*ss && cxxabs(h1,h2)>100*ss*ss)
				/* do a Newton step */
			{	i++;
				if (i>2) i=2;
				tempr=pr*p1r-pi*p1i;
				tempi=pr*p1i+pi*p1r;
				cxdiv(-tempr,-tempi,h1,h2,&dxr,&dxi);
				if (cxxabs(dxr,dxi)>dzmax)
				{	temp=dzmax/cxxabs(dxr,dxi);
					dxr*=temp; dxi*=temp;
					i=0;
				}
				if (i==2 && cxxabs(dxr,dxi)<dzmin/EPSROOT &&
					cxxabs(dxr,dxi)>0)
				{	i=0;
					cxdiv(xnewr-xoldr,xnewi-xoldi,dxr,dxi,&h3,&h4);
					h3+=1;
					h1=h3*h3-h4*h4;
					h2=2*h3*h4;
					cxdiv(dxr,dxi,h1,h2,&h3,&h4);
					if (cxxabs(h3,h4)<50*dzmin)
					{	dxr+=h3; dxi+=h4;
					}
				}
				xoldr=xnewr; xoldi=xnewi;
				abs_pold=abs_pnew;
			}
			else /* saddle point, minimize into direction pr+i*pi */
			{	i=0;
				h=dzmax/abs_pnew;
				dxr=h*pr; dxi=h*pi;
				xoldr=xnewr; xoldi=xnewi;
				abs_pold=abs_pnew;
				do
				{	chorner(n,iu,ar,ai,xnewr+dxr,xnewi+dxi,&u,&v,
						&h,&h1,&h2,&h3,&h4);
					dxr*=2; dxi*=2;
				}
				while (fabs(cxxabs(u,v)/abs_pnew-1)<EPSROOT);
			}
		} /* end of Newton step */
		xnewr=xoldr+dxr;
		xnewi=xoldi+dxi;
		dzmin=BETA*(1+cxxabs(xoldr,xoldi));
		chorner(n,iu,ar,ai,xnewr,xnewi,&pr,&pi,
			&p1r,&p1i,&p2r,&p2i,&ss);
		abs_pnew=cxxabs(pr,pi);
		if (abs_pnew==0.0) break;
		if (cxxabs(dxr,dxi)<dzmin && abs_pnew<1e-5
			&& iter>5) break;
		if (iter>ITERMAX)
		{	iter=0;
			if (abs_pnew<=abs_pnoted) break;
			abs_pnoted=abs_pnew;
			if (sys_test_key()==escape) cc_error(calc,"user interrupt!");
		}
	}
	*x0r=xnewr; *x0i=xnewi;
	stop: ;
/*
	chorner(n,iu,ar,ai,*x0r,*x0i,&pr,&pi,&p1r,&p1i,&p2r,&p2i,&ss);
	abs_pnew=cxxabs(pr,pi);
	printf("%20.5e +i* %20.5e, %20.5e\n",
		*x0r,*x0i,abs_pnew);
*/
}

static void polydiv (int n, int iu, real *ar, real *ai,
	real x0r, real x0i)
{	int i;
	for (i=n-1; i>iu; i--) {
		ar[i]+=ar[i+1]*x0r-ai[i+1]*x0i;
		ai[i]+=ai[i+1]*x0r+ar[i+1]*x0i;
	}
}

static void bauhuber (Calc *cc, real *p, int n, real *result, int all,
	real startr, real starti)
{	real *ar,*ai,scalefak=1.0;
	int i;
	real x0r,x0i;
	if (cc->newram+2*(n+1)*sizeof(real)>cc->udfstart) cc_error(cc,"Memory overflow!");
	ar=(real *)cc->newram;
	ai=ar+n+1;
	for (i=0; i<=n; i++) {
		ar[i]=p[2*i];
		ai[i]=p[2*i+1];
	}
/*	scpoly(n,ar,ai,&scalefak); */
	/* scalefak=1; */
	x0r=startr; x0i=starti;
	for (i=0; i<(all?n:1); i++) {
		bauroot(n,i,ar,ai,&x0r,&x0i);
		ar[i]=scalefak*x0r;
		ai[i]=scalefak*x0i;
		polydiv(n,i,ar,ai,x0r,x0i);
		x0i=-x0i;
	}
	for (i=0; i<n; i++) {
		result[2*i]=ar[i]; result[2*i+1]=ai[i];
	}
}

header* mpolysolve (Calc *cc, header *hd)
{	header *st=hd,*result;
	int r,c;
	real *m;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix) {
		make_complex(cc,st);
		hd=getvalue(cc,st);
	}
	if (hd->type!=s_cmatrix || dimsof(hd)->r!=1 || dimsof(hd)->c<2)
		cc_error(cc,"row vector expected");
	getmatrix(hd,&r,&c,&m);
	result=new_cmatrix(cc,1,c-1,""); 
	bauhuber(cc,m,c-1,matrixof(result),1,0,0);
	return pushresults(cc,result);
}

header* mpolyroot (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result;
	int r,c;
	real *m,xr=0.0,xi=0.0;
	hd1=nextof(hd);
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix) {
		make_complex(cc,st);
		hd=getvalue(cc,st);
	}
	hd1=nextof(st);
	hd1=getvalue(cc,hd1);
	if (hd1->type==s_real) {
		xr=*realof(hd1); xi=0;
	} else if (hd1->type==s_complex) {
		xr=*realof(hd1); xi=*(realof(hd1)+1);
	} else cc_error(cc,"Need a starting value!");

	if (hd->type!=s_cmatrix || dimsof(hd)->r!=1 || dimsof(hd)->c<2)
		 cc_error(cc,"row vector expected");
	getmatrix(hd,&r,&c,&m);
	result=new_complex(cc,0,0,"");
	bauhuber(cc,m,c-1,realof(result),0,xr,xi);
	return pushresults(cc,result);
}

/****************************************************************
 *	linear algebra
 ****************************************************************/
header* mlu (Calc *cc, header *hd)
{	header *st=hd,*result=NULL,*res1=NULL,*res2=NULL,*res3=NULL;
	real *m,*mr,*m1,*m2,det,deti;
	int r,c,*rows,*cols,rank,i;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix || hd->type==s_real) {
		getmatrix(hd,&r,&c,&m);
		if (r<1) cc_error(cc,"not a 0-sized matrix expected");
		result=new_matrix(cc,r,c,"");
		mr=matrixof(result);
		memmove((char *)mr,(char *)m,(ULONG)r*c*sizeof(real));
		make_lu(cc,mr,r,c,&rows,&cols,&rank,&det);
		res1=new_matrix(cc,1,rank,"");
		res2=new_matrix(cc,1,c,"");
		res3=new_real(cc,det,"");
		m1=matrixof(res1);
		for (i=0; i<rank; i++) {
			*m1++=*rows+1;
			rows++;
		}
		m2=matrixof(res2);
		for (i=0; i<c; i++) {
			*m2++=*cols++;
		}
	} else if (hd->type==s_cmatrix || hd->type==s_complex) {
		getmatrix(hd,&r,&c,&m);
		if (r<1) cc_error(cc,"not a 0-sized matrix expected");
		result=new_cmatrix(cc,r,c,"");
		mr=matrixof(result);
        memmove((char *)mr,(char *)m,(ULONG)r*c*(ULONG)2*sizeof(real));
		cmake_lu(cc,mr,r,c,&rows,&cols,&rank,&det,&deti); 
		res1=new_matrix(cc,1,rank,"");
		res2=new_matrix(cc,1,c,"");
		res3=new_complex(cc,det,deti,"");
		m1=matrixof(res1);
		for (i=0; i<rank; i++) {
			*m1++=*rows+1;
			rows++;
		}
		m2=matrixof(res2);
		for (i=0; i<c; i++) {
			*m2++=*cols++;
		}
	}
	else cc_error(cc,"bad type");
	hd=st;
	moveresult(cc,st,result); st=nextof(st);
	moveresult(cc,st,res1); st=nextof(st);
	moveresult(cc,st,res2); st=nextof(st);
	moveresult(cc,st,res3);
	return hd;
}

header* mlusolve (Calc *cc, header *hd)
{	header *st=hd,*hd1,*result=NULL;
	real *m,*m1;
	int r,c,r1,c1;
	hd=getvalue(cc,hd);
	hd1=next_param(cc,st);
	if (hd1) hd1=getvalue(cc,hd1);
	if (hd->type==s_matrix || hd->type==s_real) {
		getmatrix(hd,&r,&c,&m);
		if (hd1->type==s_cmatrix) {
			make_complex(cc,st);
			return mlusolve(cc,st);
		}
		if (hd1->type!=s_matrix && hd1->type!=s_real) cc_error(cc,"real value or matrix expected");
		getmatrix(hd1,&r1,&c1,&m1);
		if (c!=r || c<1 || r!=r1) cc_error(cc,"bad size");
		result=new_matrix(cc,r,c1,"");
		lu_solve(cc,m,r,m1,c1,matrixof(result));
	} else if (hd->type==s_cmatrix || hd->type==s_complex) {
		getmatrix(hd,&r,&c,&m);
		if (hd1->type==s_matrix || hd1->type==s_real) {
			make_complex(cc,next_param(cc,st));
			return mlusolve(cc,st);
		}
		if (hd1->type!=s_cmatrix && hd1->type!=s_complex) cc_error(cc,"complex value or matrix expected");
		getmatrix(hd1,&r1,&c1,&m1);
		if (c!=r || c<1 || r!=r1) cc_error(cc,"bad size");
		result=new_cmatrix(cc,r,c1,"");
		clu_solve(cc,m,r,m1,c1,matrixof(result));
	} else cc_error(cc,"real or complex value or matrix expected");
	return pushresults(cc,result);
}

header* mtridiag (Calc *cc, header *hd)
{	header *result=NULL,*result1=NULL;
	real *m,*mr;
	int r,c,*rows,i;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix) {
		getmatrix(hd,&c,&r,&m);
		if (c!=r || c==0) cc_error(cc,"non 0-sized square matrix expected");
		result=new_matrix(cc,c,c,"");
		result1=new_matrix(cc,1,c,"");
		mr=matrixof(result);
		memmove(mr,m,(ULONG)c*c*sizeof(real));
		tridiag(cc,mr,c,&rows);
		mr=matrixof(result1);
		for (i=0; i<c; i++) *mr++=rows[i]+1;
	} else if (hd->type==s_cmatrix) {
		getmatrix(hd,&c,&r,&m);
		if (c!=r || c==0) cc_error(cc,"non 0-sized square matrix expected");
		result=new_cmatrix(cc,c,c,"");
		result1=new_matrix(cc,1,c,"");
		mr=matrixof(result);
        memmove(mr,m,(ULONG)c*c*(ULONG)2*sizeof(real));
		ctridiag(cc,mr,c,&rows);
		mr=matrixof(result1);
		for (i=0; i<c; i++) *mr++=rows[i]+1;
	}
	else cc_error(cc,"matrix expected");
	return pushresults(cc,result);
//	moveresult((header *)newram,result1);
}

header* mcharpoly (Calc *cc, header *hd)
{	header *result=NULL,*result1=NULL;
	real *m,*mr;
	int r,c;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix) {
		getmatrix(hd,&c,&r,&m);
		if (c!=r || c==0) cc_error(cc,"non 0-sized square matrix expected");
		result=new_matrix(cc,c,c,"");
		result1=new_matrix(cc,1,c+1,"");
		mr=matrixof(result);
		memmove(mr,m,(ULONG)c*c*sizeof(real));
		charpoly(cc,mr,c,matrixof(result1));
	} else if (hd->type==s_cmatrix) {
		getmatrix(hd,&c,&r,&m);
		if (c!=r || c==0) cc_error(cc,"non 0-sized square matrix expected");
		result=new_cmatrix(cc,c,c,"");
		result1=new_cmatrix(cc,1,c+1,"");
		mr=matrixof(result);
        memmove(mr,m,(ULONG)c*c*(ULONG)2*sizeof(real));
		ccharpoly(cc,mr,c,matrixof(result1));
	} else cc_error(cc,"matrix expected");
	return pushresults(cc,result1);
}

/****************************************************************
 *	number theory
 ****************************************************************/
static real rfac (real x)
{	int i,n;
	real res=1.0;
	if (x<2.0) return 1.0;
	n=(int)x;
	for (i=2; i<=n; i++) res=res*i;
	return res;
}

header* mfac (Calc *cc, header *hd)
{	return spread1(cc,rfac,NULL,hd);
}

static real rlogfac (real x)
{	int i,n;
	real res=0;
	if (x<2.0) return 0.0;
	n=(int)x;
	for (i=2; i<=n; i++) res=res+log(i);
	return res;
}

header* mlogfac (Calc *cc, header *hd)
{	return spread1(cc,rlogfac,NULL,hd);
}

static void rbin (real *x, real *y, real *z)
{   int i,n,m,k;
	real res;
	n=(int)*x; m=(int)*y;
	if (m<=0.0) {
		*z=1.0;
	} else {
		res=k=(n-m+1);
		for (i=2; i<=m; i++) { k++; res=(res*k)/i; }
		*z=res;
	}
}

header* mbin (Calc *cc, header *hd)
{	return spread2(cc,rbin,NULL,hd);
}

static void rlogbin (real *x, real *y, real *z)
{   int i,n,m,k;
	real res;
	n=(int)*x; m=(int)*y;
	if (m<=0) {
		*z=0.0;
	} else {
		k=n-m+1;
		res=log(n-m+1);
		for (i=2; i<=m; i++) { k++; res+=log(k)-log(i); }
		*z=res;
	}
}

header* mlogbin (Calc *cc, header *hd)
{	return spread2(cc,rlogbin,NULL,hd);
}

/****************************************************************
 *	random and statistics
 ****************************************************************/
#define IM1 2147483563
#define IM2 2147483399
#define AM (1.0/IM1)
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)
#define RNMX (1.0-EPS)

static long randseed=1234512345;
#define IDUM2 123456789
static long idum2=IDUM2;
static long iy=0;
static long iv[NTAB];

static real ran2 (void)
{
	int j;
	long k;
	real temp;

	if (randseed <= 0) {
		if (-(randseed) < 1) randseed=1;
		else randseed = -(randseed);
		idum2=(randseed);
		for (j=NTAB+7;j>=0;j--) {
			k=(randseed)/IQ1;
			randseed=IA1*(randseed-k*IQ1)-k*IR1;
			if (randseed < 0) randseed += IM1;
			if (j < NTAB) iv[j] = randseed;
		}
		iy=iv[0];
	}
	k=(randseed)/IQ1;
	randseed=IA1*(randseed-k*IQ1)-k*IR1;
	if (randseed < 0) randseed += IM1;
	k=idum2/IQ2;
	idum2=IA2*(idum2-k*IQ2)-k*IR2;
	if (idum2 < 0) idum2 += IM2;
	j=iy/NDIV;
	iy=iv[j]-idum2;
	iv[j] = randseed;
	if (iy < 1) iy += IMM1;
	if ((temp=AM*iy) > RNMX) return RNMX;
	else return temp;
}

header* mseed (Calc *cc, header *hd)
{   header *result;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"real value expected!");
	result=new_real(cc,*realof(hd),"");
	randseed=-labs((long)(*realof(hd)*LONG_MAX));
	return pushresults(cc,result);
}

header* mrandom (Calc *cc, header *hd)
{	header *result;
	real row, col, *m;
	int r=0,c=0;
	long k,n;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2) {
		row=*matrixof(hd); col=*(matrixof(hd)+1);
		r=(row>-1.0 && row<(real)INT_MAX) ? (int)row : 0;
		c=(col>-1.0 && col<(real)INT_MAX) ? (int)col : 0;
	} else if (hd->type==s_real) {
		col=*realof(hd);
		r=1; c=(col>-1.0 && col<(real)INT_MAX) ? (int)col : 0;
	} else cc_error(cc,"random([n,m]) or random(m)");
//	if (r<1 || c<0) cc_error(cc,"positive integer value or [r,c] vector expected!");
	result=new_matrix(cc,r,c,"");
	m=matrixof(result);
	n=(long)c*r;
	for (k=0; k<n; k++) *m++=(real)ran2();
	return pushresults(cc,result);
}

static real gasdev (void)
{	static int iset=0;
	static real gset;
	real fac,rsq,v1,v2;
	if  (iset == 0) {
		do {
			v1=2.0*ran2()-1.0;
			v2=2.0*ran2()-1.0;
			rsq=v1*v1+v2*v2;
		} while (rsq >= 1.0 || rsq == 0.0);
		fac=sqrt(-2.0*log(rsq)/rsq);
		gset=v1*fac;
		iset=1;
		return v2*fac;
	} else {
		iset=0;
		return gset;
	}
}

header* mnormal (Calc *cc, header *hd)
{	header *result;
	real row, col, *m;
	int r=0, c=0;
	long k,n;
	hd=getvalue(cc,hd);
	if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2) {
		row=*matrixof(hd); col=*(matrixof(hd)+1);
		r=(row>-1.0 && row<(real)INT_MAX) ? (int)row : 0;
		c=(col>-1.0 && col<(real)INT_MAX) ? (int)col : 0;
	} else if (hd->type==s_real) {
		col=*realof(hd);
		r=1; c=(col>-1.0 && col<(real)INT_MAX) ? (int)col : 0;
	} else cc_error(cc,"normal([n,m]) or normal(m)");
//	if (r<1 || c<0) cc_error(cc,"positive integer value or [r,c] vector expected!");
	result=new_matrix(cc,r,c,"");
	m=matrixof(result);
	n=(long)c*r;
	for (k=0; k<n; k++) *m++=(real)gasdev();
	return pushresults(cc,result);
}

header* mshuffle (Calc *cc, header *hd)
{	header *result;
	real *m,*mr,x;
	int i,j,n;
	hd=getvalue(cc,hd);
	if (hd->type!=s_matrix || dimsof(hd)->r!=1)
		cc_error(cc,"real vector expected!");
	n=dimsof(hd)->c;
	result=new_matrix(cc,1,n,"");
	m=matrixof(hd); mr=matrixof(result);
	for (i=0; i<n; i++) *mr++=*m++;
	mr=matrixof(result);
	for (i=n-1; i>0; i--) {
		j=(int)floor(ran2()*(i+1));
		if (i!=j) {
			x=*(mr+i); *(mr+i)=*(mr+j); *(mr+j)=x;
		}
	}
	return pushresults(cc,result);
}

header* mfind (Calc *cc, header *hd)
{	header *hd1,*result;
	real *m,*m1,*mr;
	int i,j,k,c,r,c1,r1;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if ((hd->type!=s_matrix && hd->type!=s_real) || 
	    (hd1->type!=s_matrix && hd1->type!=s_real)) cc_error(cc,"real matrices expected!");
	getmatrix(hd,&c,&r,&m);
	getmatrix(hd1,&c1,&r1,&m1);
	if (c!=1 && r!=1) cc_error(cc,"real vector expected!");
	if (r!=1) c=r;
	result=new_matrix(cc,c1,r1,"");
	mr=matrixof(result);
	for (i=0; i<r1; i++) {
		for (j=0; j<c1; j++) {
			k=0;
			while (k<c && m[k]<=*m1) k++;
			if (k==c && *m1<=m[c-1]) k=c-1;
			*mr++=k; m1++;
		}
	}
	return pushresults(cc,result);
}

