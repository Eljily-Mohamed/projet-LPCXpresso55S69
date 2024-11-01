#include "sysdep.h"
#include "calc.h"
#include "dsp.h"
#include "funcs.h"
#include <stdio.h>

header* test_pqcos_cos (Calc *cc, header *hd)
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
