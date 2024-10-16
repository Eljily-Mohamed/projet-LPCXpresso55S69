/************* graphics.c ***************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "graphics.h"

/* [r,c,n]=subplot(rcn)
 *   sets the current subplot in the format
 *   rci --> nb plot rows | nb of plot cols | current id
 *   each value coded on 1 digit so that 428 919 199 339 are legal
 *   values
 */
header* msubplot (Calc *cc, header *hd)
{	header *result;
	int tmp, r=1, c=1, index=1;
	static int gr=1, gc=1, initialized=1;
	real* m;
	hd=getvalue(cc,hd);
	if (hd->type==s_real) {
		tmp=(unsigned int)(*realof(hd));
		if (tmp>999) cc_error(cc,"subplot: bad layout");
		index = tmp % 10;
		r = tmp/100;
		c = tmp/10-r*10;
		if (r==0 || c==0) {
			cc_error(cc,"subplot: needs at least one row and one column!\n");
		}
		if ((r!=gr || c!=gc) && index!=1) initialized=0;
		if (index==1) {
			gr=r;
			gc=c;
			initialized=1;
		}
		if (index==0 || index>r*c || !initialized) {
			cc_error(cc,"subplot: bad subplot index!");
		}
	} else cc_error(cc,"subplot(rci)!");
	gsubplot(r,c,index);		/* callback for UI */
	
	result=new_matrix(cc,1,3,"");	/* return [r c id] */
	m=matrixof(result);
	*m++=(real)r;
	*m++=(real)c;
	*m=(real)index;
	return pushresults(cc,result);
}

/***** minmax
	compute the total minimum and maximum of n real numbers.
*****/
static void minmax (real *x, ULONG n, real *min, real *max, 
	int *imin, int *imax)
{	ULONG i;
	if (n==0) {
		*min=0; *max=0; *imin=0; *imax=0; return;
	}
	*min=*x; *max=*x; *imin=0; *imax=0; x++;
	for (i=1; i<n; i++) {
		if (*x<*min) {
			*min=*x; *imin=(int)i;
		} else if (*x>*max) {
			*max=*x; *imax=(int)i;
		}
		x++;
	}
}

/* [xm,xM,ym,yM]=plotarea(x,y)
 *   gets the area needed to plot the graph y(x)
 *   parameters
 *   x        : real vector/matrix
 *   y        : real vector/matrix
 *   returns the 1x4 vector [xmin,xmax,ymin,ymax]
 */
header* mplotarea (Calc *cc, header *hd)
{	header *hd1,*result;
	real *x,*y, xmin, xmax, ymin, ymax;
	int cx,rx,cy,ry,ix,iy;
	unsigned long flags;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type!=s_matrix && hd->type!=s_real &&
		hd1->type!=s_matrix && hd1->type!=s_real)
		cc_error(cc,"Wrong args: real matrices expected!");
	getmatrix(hd,&rx,&cx,&x); getmatrix(hd1,&ry,&cy,&y);
	if (cx!=cy || (rx>1 && ry!=rx))
		cc_error(cc,"Plot columns must agree!");
	
	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	
	if (flags & G_AUTOSCALE) {
		minmax(x,(LONG)cx*rx,&xmin,&xmax,&ix,&iy);
		minmax(y,(LONG)cy*ry,&ymin,&ymax,&ix,&iy);
	}
	
	result=new_matrix(cc,1,4,"");
	x=matrixof(result);
	*x++=xmin; *x++=xmax; *x++=ymin; *x++=ymax;
	return pushresults(cc,result);
}

/* [xm,xM,ym,yM]=setplot([xm,xM,ym,yM])
 *   sets the limits of the graph given by a 1x4 vector [xmin,xmax,ymin,ymax].
 *   returns the current settings.
 */
header* msetplot (Calc *cc, header *hd)
{
	header *result=NULL;
	real *m, xmin, xmax, ymin, ymax;
	unsigned long flags;
	hd=getvalue(cc,hd);
	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	if (hd->type==s_matrix && dimsof(hd)->r*dimsof(hd)->c==0) {
		result=new_matrix(cc,1,4,"");
		m=matrixof(result);
		*m++=xmin; *m++=xmax; *m++=ymin; *m=ymax;
		gsetplot(xmin,xmax,ymin,ymax,flags|G_WORLDUNSET|G_AUTOSCALE,G_WORLDUNSET|G_AUTOSCALE);
	} else if (hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==4) {
		m=matrixof(hd);
		xmin=*m++; xmax=*m++; ymin=*m++; ymax=*m;
		gsetplot(xmin,xmax,ymin,ymax,flags & ~(G_WORLDUNSET|G_AUTOSCALE),G_WORLDUNSET|G_AUTOSCALE);
		result=hd;
	} else cc_error(cc,"Setplot needs a 1x4 vector!");
	return pushresults(cc,result);
}

/* [xm,xM,ym,yM]=setplot()
 *   sets the limits of the graph given by a 1x4 vector [xmin,xmax,ymin,ymax].
 *   returns the current settings.
 */
header* msetplot0 (Calc *cc, header *hd)
{
	header *result=NULL;
	real *m, xmin, xmax, ymin, ymax;
	unsigned long flags;
	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	*m++=xmin; *m++=xmax; *m++=ymin; *m=ymax;
	return pushresults(cc,result);
}

/* parsestyle:
 *   parse style string and returns an integer bit field
 *   
 *   style : string describing the style
 *           - g{x{y}}                          grid style
 *           - {LX|LY|LL}						scale style
 *           - l{n|-|.|--|->|c|b|b#}			line style
 *           - m{x|+|*|.|{#}o|{#}[]|{#}<>}      marker style
 *           - c{#rrggbb|number{+}}  color style
 */
static void parsestyle(Calc *cc, char *s,
				unsigned long *style,
			 	unsigned long *mask, int reset)
{
//	unsigned long	st, msk=G_FRAME|G_AXIS;
	unsigned long	st, msk;
	line_t			ltype=L_SOLID;
	marker_t		mtype=M_NONE;
	char* s1;
	
	if (reset) {
		st=G_WORLDUNSET|G_AUTOSCALE|G_AXISUNSET;
		msk=0;
	} else {
		st=*style;
		msk=*mask;
	}
	
	while (*s!=0) {
		switch (*s) {
		case 'F':
			s1=s++;
			st|=G_FRAME; msk|=G_FRAME;
			break;
		case 'A':
			s1=s++;
			st|=G_AXIS; msk|=G_AXIS;
			break;
		case 'L':			/* scale style */
			s1=s++;
			if (*s=='L') {
				s+=1;
				if (st & G_AXISUNSET) st|=G_XLOG|G_YLOG;
			} else  if (*s=='X') {
				s+=1;
				if (st & G_AXISUNSET) st|=G_XLOG;
			} else if (*s=='Y') {
				s+=1;
				if (st & G_AXISUNSET) st|=G_YLOG;
			} else {
				st &=~(G_XLOG|G_YLOG);
			}
			if (st & G_AXISUNSET) {
				msk|=G_XLOG|G_YLOG|G_AXISUNSET;
				st &=~G_AXISUNSET;
			}
			break;
		case 'c':			/* color */
			s1=s++;
			if (*s=='#') {	/* parse hexa color */
				int color[3]={0,0,0};
				s++;
				for (int i=0;i<3;i++) {
					for (int j=0;j<2;j++) {
						if (*s>='0' && *s<='9') {
							color[i]=color[i]*10+(*s)-'0';
						} else if (*s>='A' && *s<='F') {
							color[i]=color[i]*10+(*s)-'A'+10;
						} else if (*s>='a' && *s<='f') {
							color[i]=color[i]*10+(*s)-'a'+10;
						} else {
							cc_error(cc, "bad color format %s\n",s1);
						}
						s++;
					}
				}
				/*setcolor(color);*/
			} else {		/* parse color nb */
				unsigned int color=0;
				if (*s>'0' && *s<='9') {
					color=*s-'0';
					s++;
					while (*s>='0' && *s<='9') {
						color=10*color+(*s)-'0';
						s++;
					}
					if (color<=MAX_COLORS)
						st=(st & ~G_COLOR_MSK) | (color-1)<<24;
					msk|=G_COLOR_MSK;
				}
				if (*s=='+') {
					s++;
					st|=G_AUTOCOLOR;
					msk|=G_AUTOCOLOR;
				} else {
					st&=~G_AUTOCOLOR;
					msk|=G_AUTOCOLOR;
				}
			}
			break;
		case 'l':			/* line style */
			s1=s++;
			if (*s=='n') {
				ltype=L_NONE; s+=1;
			} else if (*s=='c') {
				ltype=L_COMB; s+=1;
			} else if (*s=='-' && *(s+1)=='-') {
				ltype=L_DASHED; s+=2;
			} else if (*s=='-' && *(s+1)=='>') {
				ltype=L_ARROW; s+=2;
			} else if (*s=='-') {
				ltype=L_SOLID; s+=1;
			} else if (*s=='.') {
				ltype=L_DOTTED; s+=1;
			} else if (*s=='b' && *(s+1)=='#') {
				ltype=L_FBAR; s+=2;
			} else if (*s=='b') {
				ltype=L_BAR; s+=1;
			} else if (*s=='s' && *(s+1)=='#') {
				ltype=L_FSTEP; s+=2;
			} else if (*s=='s') {
				ltype=L_STEP; s+=1;
			} else {
				cc_error(cc, "unknown line type %s\n",s1);
			}
			st=(st & ~G_LTYPE_MSK) | (ltype<<16);
			msk|=G_LTYPE_MSK;
			break;
		case 'm':			/* marker style */
			s1=s++;
			if (*s=='n') {
				mtype=M_NONE; s+=1;
			} else if (*s=='a') {
				mtype=M_ARROW; s+=1;
			} else if (*s=='x') {
				mtype=M_CROSS; s+=1;
			} else if (*s=='t') {
				mtype=M_TRIANGLE; s+=1;
			} else if (*s=='.') {
				mtype=M_DOT; s+=1;
			} else if (*s=='+') {
				mtype=M_PLUS; s+=1;
			} else if (*s=='*') {
				mtype=M_STAR; s+=1;
			} else if (*s=='o' && *(s+1)=='#') {
				mtype=M_FCIRCLE; s+=2;
			} else if (*s=='o') {
				mtype=M_CIRCLE; s+=1;
			} else if (*s=='[' && *(s+1)==']' && *(s+2)=='#') {
				mtype=M_FSQUARE; s+=3;
			} else if (*s=='[' && *(s+1)==']') {
				mtype=M_SQUARE; s+=2;
			} else if (*s=='<' && *(s+1)=='>' && *(s+2)=='#') {
				mtype=M_FDIAMOND; s+=3;
			} else if (*s=='<' && *(s+1)=='>') {
				mtype=M_DIAMOND; s+=2;
			} else {
				cc_error(cc, "unknown marker type %s\n",s1);
			}
			st=(st & ~G_MTYPE_MSK) | (mtype<<20);
			if (mtype!=M_NONE) {
				st|=G_AUTOMARK;
			} else {
				st&=~G_AUTOMARK;
			}
			msk|=G_MTYPE_MSK;
			break;
		case 'w':		/* parse linewidth */
			s1=s++;
			if (*s=='=') {
				s++;
				int lw=0;
				if (*s>'0' && *s<='9') {
					lw=*s-'0';
					s++;
					while (*s>='0' && *s<='9') {
						lw=10*lw+(*s)-'0';
						s++;
					}
				}
				if (lw==0) lw=1;
				if (lw<16) {
					st=(st & ~G_LWIDTH_MSK) | (lw<<28);
					msk|=G_LWIDTH_MSK;
				}
			} else {
				cc_error(cc, "error in parsing linewidth %s\n",s1);
			}
			break;
		default:
			cc_error(cc, "unknown style %s\n",s);
		}
		if (*s==',') s++;
//		if (*s==';') {s++; break;}
	}
	*style=st;
	*mask=msk;
}

/* [xm,xM,ym,yM]=plot(x,y,"style")
 *   that plots the graph y(x) with line and/or marks according to the
 *   third parameter
 *   parameters
 *   x     : real vector/matrix
 *   y     : real vector/matrix
 *   style : string describing the style
 *           - g{x{y}}                          grid style
 *           - s{lx|ly|ll}                      scale style
 *           - l{n|-|.|--|->|c|b}               line style
 *           - m{x|+|*|.|{#}o|{#}[]|{#}<>}      marker style
 *           - c{#rrggbb|number{+}}  color style
 *   returns the 1x4 vector [xmin,xmax,ymin,ymax]
 */
header* mplot (Calc *cc, header *hd)
{	header *hd1,*hd2,*result;
	real *m,*x,*y,xmin,xmax,ymin,ymax;
	int cx,rx,cy,ry,ix,iy;
	unsigned long flags, mask=0;
	hd1=next_param(cc,hd); hd2=next_param(cc,hd1);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1); hd2=getvalue(cc,hd2);
	if (hd->type!=s_matrix && hd->type!=s_real &&
		hd1->type!=s_matrix && hd1->type!=s_real)
		cc_error(cc,"Wrong args: real matrices expected!");
	if (hd2->type!=s_string) cc_error(cc,"plot(x,y,\"style\")");
	getmatrix(hd,&rx,&cx,&x); getmatrix(hd1,&ry,&cy,&y);
	if (cx!=cy || (rx>1 && ry!=rx))
		cc_error(cc,"Plot columns must agree!");
	
	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	parsestyle(cc,stringof(hd2),&flags,&mask,0);
	
	if (flags & G_AUTOSCALE) {
		minmax(x,(LONG)cx*rx,&xmin,&xmax,&ix,&iy);
		minmax(y,(LONG)cy*ry,&ymin,&ymax,&ix,&iy);
	}

	if (flags & G_AXISUNSET) flags&=~G_AXISUNSET;
	
	gsetplot(xmin,xmax,ymin,ymax,flags,mask);

	// deal with oneshot features (x/y log axis, frame/axis)
	gplot(cc,hd,hd1);
	
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	m[0]=xmin; m[1]=xmax; m[2]=ymin; m[3]=ymax;
	
	return pushresults(cc,result);
}

/* [xm,xM,ym,yM]=plot(x,y) builtin function
 */
header* mplot1 (Calc *cc, header *hd)
{	header *hd1,*result;
	real *m,*x,*y,xmin,xmax,ymin,ymax;
	int cx,rx,cy,ry,ix,iy;
	unsigned long flags, mask=0;
	hd1=next_param(cc,hd);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1);
	if (hd->type!=s_matrix && hd->type!=s_real &&
		hd1->type!=s_matrix && hd1->type!=s_real)
		cc_error(cc,"Wrong args: real matrices expected!");
	getmatrix(hd,&rx,&cx,&x); getmatrix(hd1,&ry,&cy,&y);
	if (cx!=cy || (rx>1 && ry!=rx))
		cc_error(cc,"Plot columns must agree!");
	
	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	
	if (flags & G_AUTOSCALE) {
		minmax(x,(LONG)cx*rx,&xmin,&xmax,&ix,&iy);
		minmax(y,(LONG)cy*ry,&ymin,&ymax,&ix,&iy);
	}

	if (flags & G_AXISUNSET) flags&=~G_AXISUNSET;
	
	gsetplot(xmin,xmax,ymin,ymax,flags,mask);

	gplot(cc,hd,hd1);
	
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	m[0]=xmin; m[1]=xmax; m[2]=ymin; m[3]=ymax;
	
	return pushresults(cc,result);
}

/* [xlog,ylog,linewidth]=plotstyle("style")
 * return a vector 1x6
 * [xlog,ylog,linewidth,color,ltype,mtype]
 */
header* mplotstyle (Calc *cc, header *hd)
{
	header *result;
	unsigned long flags, mask;
	//=G_XGRID|G_XTICKS|G_XAUTOTICKS|G_YGRID|G_YTICKS|G_YAUTOTICKS|(M_NONE<<20)|(1<<28);
	real *m,xmin,xmax,ymin,ymax;

	hd=getvalue(cc,hd);
	if (hd->type!=s_string) cc_error(cc,"String expected!");

	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	parsestyle(cc,stringof(hd),&flags,&mask,0);

//	if ((mask & (G_XLOG|G_YLOG)) && (flags & G_AXISUNSET)) flags&=~G_AXISUNSET;

	gsetplot(xmin,xmax,ymin,ymax,flags,mask);
	
	result=new_matrix(cc,1,6,"");
	m=matrixof(result);
	*m++=(real)((flags & G_XLOG)!=0);
	*m++=(real)((flags & G_YLOG)!=0);
	*m++=(real)((flags & G_LWIDTH_MSK)>>28);
	*m++=(real)(((flags & G_COLOR_MSK)>>24)+1);
	*m++=(real)((flags & G_LTYPE_MSK)>>20);
	*m=(real)((flags & G_MTYPE_MSK)>>16);
	return pushresults(cc,result);
}

header* mxgrid (Calc *cc, header* hd)
{
	header *hd1,*hd2,*hd3,*hd4,*result;
	unsigned long flags, mask=G_XGRID|G_XTICKS|G_XAUTOTICKS;
	int r,c;
	real xmin, xmax, ymin, ymax, *m;
	
	hd1=next_param(cc,hd);  hd2=next_param(cc,hd1);
	hd3=next_param(cc,hd2); hd4=next_param(cc,hd3);
	hd=getvalue(cc,hd);		// ticks
	hd1=getvalue(cc,hd1);	// factor
	hd2=getvalue(cc,hd2);	// draw grid (bool)
	hd3=getvalue(cc,hd3);	// draw ticks (bool)
	hd4=getvalue(cc,hd4);	// grid color

	if (hd->type!=s_matrix || dimsof(hd)->r!=1)
		cc_error(cc,"xgrid needs a tick vector or []");
	if (hd1->type!=s_real) goto err;
	if (hd2->type!=s_real) goto err;
	if (hd3->type!=s_real) goto err;
	if (hd4->type!=s_real) goto err;

	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	
	getmatrix(hd,&r,&c,&m);
	if (c) {
		xmin=m[0]; xmax=m[0];
		for (int i=1; i<c; i++) {
			if (m[i]<xmin) xmin=m[i];
			if (m[i]>xmax) xmax=m[i];
		}
		if ((flags & G_XLOG) && (xmin<0.0 || xmax<0.0)) {
			cc_error(cc,"x log axis with negative or null boundary");
		}
		flags&=~(G_AUTOSCALE|G_XAUTOTICKS);
		mask|=G_AUTOSCALE;
	} else {
		flags|=G_XAUTOTICKS;
	}
	if ((int)(*realof(hd2))) {
		flags|=G_XGRID;
	} else {
		flags&=~G_XGRID;
	}
	if ((int)(*realof(hd3))) {
		flags|=G_XTICKS;
	} else {
		flags&=~G_XTICKS;
	}

	gsetplot(xmin,xmax,ymin,ymax,flags,mask);
	gsetxgrid(hd,*realof(hd1),(unsigned int)(*realof(hd4)) & 0xF);
	
	// return the new widow
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	*m++=xmin;*m++=xmax;*m++=ymin;*m=ymax;
	
	return pushresults(cc,result);
err:
	cc_error(cc,"Wrong arguments for xgrid!");
	return NULL;
}

header* mygrid (Calc *cc, header* hd)
{
	header *hd1,*hd2,*hd3,*hd4,*result;
	unsigned long flags, mask=G_YGRID|G_YTICKS|G_YAUTOTICKS;
	int r,c;
	real xmin, xmax, ymin, ymax, *m;
	
	hd1=next_param(cc,hd);  hd2=next_param(cc,hd1);
	hd3=next_param(cc,hd2); hd4=next_param(cc,hd3);
	hd=getvalue(cc,hd);		// ticks
	hd1=getvalue(cc,hd1);	// factor
	hd2=getvalue(cc,hd2);	// draw grid (bool)
	hd3=getvalue(cc,hd3);	// draw ticks (bool)
	hd4=getvalue(cc,hd4);	// grid color

	if (hd->type!=s_matrix || dimsof(hd)->r!=1)
		cc_error(cc,"ygrid needs a tick vector or []");
	if (hd1->type!=s_real) goto err;
	if (hd2->type!=s_real) goto err;
	if (hd3->type!=s_real) goto err;
	if (hd4->type!=s_real) goto err;

	ggetplot(&xmin,&xmax,&ymin,&ymax,&flags);
	
	getmatrix(hd,&r,&c,&m);
	if (c) {
		ymin=m[0]; ymax=m[0];
		for (int i=1; i<c; i++) {
			if (m[i]<ymin) ymin=m[i];
			if (m[i]>ymax) ymax=m[i];
		}
		if ((flags & G_YLOG) && (ymin<0.0 || ymax<0.0)) {
			cc_error(cc,"y log axis with negative or null boundary");
		}
		flags&=~(G_AUTOSCALE|G_YAUTOTICKS);
		mask|=G_AUTOSCALE;
	} else {
		flags|=G_YAUTOTICKS;
	}
	if ((int)(*realof(hd2))) {
		flags|=G_YGRID;
	} else {
		flags&=~G_YGRID;
	}
	if ((int)(*realof(hd3))) {
		flags|=G_YTICKS;
	} else {
		flags&=~G_YTICKS;
	}
	
	gsetplot(xmin,xmax,ymin,ymax,flags,mask);
	gsetygrid(hd,*realof(hd1),(unsigned int)(*realof(hd4)) & 0xF);
	
	// return the new widow
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	*m++=xmin;*m++=xmax;*m++=ymin;*m=ymax;
	
	return pushresults(cc,result);
err:
	cc_error(cc,"Wrong arguments for xgrid!");
	return NULL;
}

header* mtext (Calc *cc, header *hd)
{	header *st=hd, *hd1, *hd2, *hd3, *hd4;
	unsigned int indent, color;
	int angle;
	hd1=next_param(cc,hd);
	hd2=next_param(cc,hd1);
	hd3=next_param(cc,hd2);
	hd4=next_param(cc,hd3);
	hd=getvalue(cc,hd);			// [x,y]
	hd1=getvalue(cc,hd1);		// str
	hd2=getvalue(cc,hd2);		// indent
	hd3=getvalue(cc,hd3);		// angle
	hd4=getvalue(cc,hd4);		// color
	
	if (hd->type!=s_matrix || dimsof(hd)->r!=1 || dimsof(hd)->c!=2 
	 || hd1->type!=s_string || hd2->type!=s_string
	 || hd3->type!=s_real || hd4->type!=s_real)
		cc_error(cc,"text([x,y],str,ident,angle,color)");
	
	if      (strcmp(stringof(hd2),"N")==0)		indent=G_ALIGN_N;
	else if (strcmp(stringof(hd2),"E")==0)		indent=G_ALIGN_E;
	else if (strcmp(stringof(hd2),"S")==0)		indent=G_ALIGN_S;
	else if (strcmp(stringof(hd2),"W")==0)		indent=G_ALIGN_W;
	else if (strcmp(stringof(hd2),"NW")==0)		indent=G_ALIGN_NW;
	else if (strcmp(stringof(hd2),"SE")==0)		indent=G_ALIGN_SE;
	else if (strcmp(stringof(hd2),"SW")==0)		indent=G_ALIGN_SW;
	else if (strcmp(stringof(hd2),"CENTER")==0)	indent=G_ALIGN_CENTER;
	else										indent=G_ALIGN_NE;	/* default NE */
	
	angle=((int)*realof(hd3)) % 360;
	color=((int)*realof(hd4)<16) ? ((int)*realof(hd4))-1 : 0;
//	graphic_mode();
	gtext(*matrixof(hd),*(matrixof(hd)+1),
		stringof(hd1),indent,angle,color);
	return moveresult(cc,st,hd1);
}

header* mtitle (Calc *cc, header *hd)
{
	hd=getvalue(cc,hd);
	
	if (hd->type!=s_string) cc_error(cc,"string parameter expected");

	glabel(stringof(hd),G_TITLE);
	
	return pushresults(cc,hd);
}

header* mxlabel (Calc *cc, header *hd)
{
	hd=getvalue(cc,hd);
	
	if (hd->type!=s_string) cc_error(cc,"string parameter expected");

	glabel(stringof(hd),G_XLABEL);
	
	return pushresults(cc,hd);
}

header* mylabel (Calc *cc, header *hd)
{
	hd=getvalue(cc,hd);
	
	if (hd->type!=s_string) cc_error(cc,"string parameter expected");

	glabel(stringof(hd),G_YLABEL);
	
	return pushresults(cc,hd);
}


#if 0
/*****************************************************************/
static plot_t  plts[9]={
	[0] = {
		.upperc=10,
		.upperr=30,
		.lowerc=1010,
		.lowerr=1010,
		.x_min=-1.0,
		.x_max=1.0,
		.y_min=-1.0,
		.y_max=1.0,
		.style="",
		.newframe=1,
		.holding=0,
		.scaling=1,
		.xlog=0,
		.ylog=0,
		.xgrid=0,
		.xticks=0,
		.ygrid=0,
		.yticks=0						
	}
};
static plot_t* plt_cur=plts;
static int plt_rows=1,		/* number of subplot rows */
           plt_cols=1,		/* number of subplot cols */
           plt_id=0;		/* id of the current subplot <plt_rows*plt_index */

int framecolor=3,			/* color of the frame around the plot */
	linecolor=1,			/* color of curves */
	textcolor=1,			/* color of labels */
	gridcolor=3,			/* color of the grid */
	wirecolor=2,
	densitycolor=1;

line_t   linetype   = line_solid;
marker_t markertype = marker_cross;

int	linewidth  = 1;
int markersize = 10;

/* variables used by 3d plots */
real meshfactor=1,dgrid=0;	
real distance=5,tele=1.5,a_left=0.5,a_up=0.5;
int connected[4]={1,1,1,1};
int tconnected[3]={1,0,0};
int	twosides=1,
	triangles=0;

/*
typedef void (*markfn)(plot_t* p, real* x, real* y);
markfn markfuncs[12];

typedef void (*linefn)(plot_t* p, real* x, real* y);
linefn linefuncs[5];
*/

char* parsestyle(Calc *cc, plot_t* p, char*s)
{
	char* s1;

	linetype=line_solid;
	markertype=marker_none;
	
	while (*s!=0) {
		switch (*s) {
		case 'L':			/* scale style */
			s1=s++;
			if (*s=='L') {
				s+=1;
				p->xlog=1;
				p->ylog=1;
			} else  if (*s=='X') {
				s+=1;
				p->xlog=1;
			} else if (*s=='Y') {
				s+=1;
				p->ylog=1;
			} else {
				cc_error(cc,"unknown graph style %s\n",s1);
			}
			break;
		case 'c':			/* color */
			s1=s++;
			if (*s=='#') {	/* parse hexa color */
				int color[3]={0,0,0};
				s++;
				for (int i=0;i<3;i++) {
					for (int j=0;j<2;j++) {
						if (*s>='0' && *s<='9') {
							color[i]=color[i]*10+(*s)-'0';
						} else if (*s>='A' && *s<='F') {
							color[i]=color[i]*10+(*s)-'A'+10;
						} else if (*s>='a' && *s<='f') {
							color[i]=color[i]*10+(*s)-'a'+10;
						} else {
							cc_error(cc, "bad color format %s\n",s1); return s;
						}
						s++;
					}
				}
				/*setcolor(color);*/
			} else {		/* parse color nb */
				int color=0;
				if (*s>'0' && *s<='9') {
					color=*s-'0';
					s++;
					while (*s>='0' && *s<='9') {
						color=10*color+(*s)-'0';
						s++;
					}
				}
				if (*s=='+') {
					s++;
					plt_cur->autocolor=1;
				}
				if (color==0) color=1;
				linecolor=color;
				/* setcolor(index2rgb(color)); */
			}
			break;
		case 'l':			/* line style */
			s1=s++;
			if (*s=='n') {
				linetype=line_none; s+=1;
			} else if (*s=='c') {
				linetype=line_comb; s+=1;
			} else if (*s=='-' && *(s+1)=='-') {
				linetype=line_dashed; s+=2;
			} else if (*s=='-') {
				linetype=line_solid; s+=1;
			} else if (*s=='.') {
				linetype=line_dotted; s+=1;
			} else {
				cc_error(cc, "unknown line type %s\n",s1);
			}
			break;
		case 'm': {			/* marker style */
			int filled=0;
			s1=s++;
			if (*s=='#') {
				filled=1; s++;
			}
			if (*s=='a') {
				markertype=marker_arrow; s+=1;
			} else if (*s=='x') {
				markertype=marker_cross; s+=1;
			} else if (*s=='.') {
				markertype=marker_dot; s+=1;
			} else if (*s=='+') {
				markertype=marker_plus; s+=1;
			} else if (*s=='*') {
				markertype=marker_star; s+=1;
			} else if (*s=='o') {
				if (filled)
					markertype=marker_fcircle;
				else
					markertype=marker_circle;
				s+=1;
			} else if (*s=='[' && *(s+1)==']') {
				if (filled)
					markertype=marker_fsquare;
				else
					markertype=marker_square;
				s+=2;
			} else if (*s=='<' && *(s+1)=='>') {
				if (filled)
					markertype=marker_fdiamond;
				else
					markertype=marker_diamond;
				s+=2;
			} else {
				cc_error(cc, "unknown marker type %s\n",s1);
			}
			break;
		}
		case 'w':		/* parse linewidth */
			s1=s++;
			if (*s=='=') {
				s++;
				int lw=0;
				if (*s>'0' && *s<='9') {
					lw=*s-'0';
					s++;
					while (*s>='0' && *s<='9') {
						lw=10*lw+(*s)-'0';
						s++;
					}
				}
				if (lw==0) lw=1;
				linewidth=lw;
			} else {
				cc_error(cc, "error in parsing linewidth %s\n",s1);
			}
			break;
		default:
			cc_error(cc, "unknown style %s\n",s);
			return s;
		}
		if (*s==',') s++;
		if (*s==';') {s++; break;}
	}
	return s;
}

#define scrcol(x) ((p->upperc+((x)-p->x_min)/(p->x_max-p->x_min)*(p->lowerc-p->upperc)))
#define scrrow(y) ((p->lowerr-((y)-p->y_min)/(p->y_max-p->y_min)*(p->lowerr-p->upperr)))


void frame(plot_t* p)
{
	gframe(p);
	p->newframe=0;
}

/* mwindow
 *   builtin function to set the drawing window onto the graphical
 *   screen with size 1024x1024
 */
header* mwindow (Calc *cc, header *hd)
{	real *m;
	hd=getvalue(cc,hd);
	if (hd->type!=s_matrix || dimsof(hd)->r!=1 || dimsof(hd)->c!=4)
		cc_error(cc,"Arguments for window are [c0 r0 c1 r1]!\n");
	m=matrixof(hd);
	plt_cur->upperc=(int)(*m++);
	plt_cur->upperr=(int)(*m++);
	plt_cur->lowerc=(int)(*m++);
	plt_cur->lowerr=(int)(*m++);
	if (plt_cur->lowerr<plt_cur->upperr) plt_cur->lowerr=plt_cur->upperr+1;
	if (plt_cur->lowerc<plt_cur->upperc) plt_cur->lowerc=plt_cur->upperc+1;
	plt_cur->newframe=1;
	plt_cur->scaling=1;
	return hd;
}

/* mwindow0
 *   builtin function to the current drawing window of the 
 *   graphical screen
 */
header* mwindow0 (Calc *cc, header *hd)
{	real *m;
	hd=new_matrix(cc,1,4,"");
	m=matrixof(hd);
	*m++=plt_cur->upperc;
	*m++=plt_cur->upperr;
	*m++=plt_cur->lowerc;
	*m++=plt_cur->lowerr;
	return hd;
}

/* mframe: builtin function
 *   draws the frame around the graph
 */
void mframe (Calc *cc, header *hd)
{	graphic_mode();
	frame(plt_cur);
	new_real(cc,0.0,"");
}

/* msubplot: builtin function 
 *   sets the current subplot in the format
 *   rci --> nb plot rows | nb of plot cols | current id
 *   each value coded on 1 digit so that 428 919 199 339 are legal
 *   values
 */
header* msubplot (Calc *cc, header *hd)
{	header *st=hd,*result;
	int tmp;
	int r, c, index;
	real* m;
	hd=getvalue(cc,hd);
	if (hd->type==s_real) {
		tmp=(unsigned int)(*realof(hd));
		if (tmp>999) cc_error(cc,"subplot: bad layout");
		index = tmp % 10;
		r = tmp/100;
		c = tmp/10-r*10;
		if (r==0 || c==0) {
			cc_error(cc,"subplot: needs at least one row and one column!\n");
		}
		if (index==0 || index>r*c) {
			cc_error(cc,"subplot: bad subplot index!");
		}
	} else{
		cc_error(cc,"suplot: needs 1 real parameter!");
	}
/*	if (index==1) {
		gclear();
	}*/
	plt_rows=r;					/* update globals */
	plt_cols=c;
	plt_id=index-1;
	plt_cur=plts+plt_id;
	plt_cur->upperc=8*wchar+((index-1)%c)*1024/c;
	plt_cur->upperr=1.5*hchar+((index-1)/c)*1024/r;
	plt_cur->lowerc=(1+(index-1)%c)*1024/c-2*wchar;
	plt_cur->lowerr=(1+(index-1)/c)*1024/r-3*hchar;
#ifdef DEBUG
	fprintf(stderr,"uc=%d, ur=%d, lc=%d, lr=%d\n",plt_cur->upperc,plt_cur->upperr,plt_cur->lowerc,plt_cur->lowerr);
#endif
/*	plt_cur->upperc=10;
	plt_cur->upperr=30;
	plt_cur->lowerc=1010;
	plt_cur->lowerr=1010;*/
	plt_cur->x_min=-1.0;
	plt_cur->x_max=1.0;
	plt_cur->y_min=-1.0;
	plt_cur->y_max=1.0;
	plt_cur->style[0]='\0';
	plt_cur->newframe=1;
	plt_cur->holding= (index==1) ? 0 : 1;
	plt_cur->scaling=1;
	plt_cur->xlog=0;
	plt_cur->ylog=0;
	plt_cur->xgrid=0;
	plt_cur->xticks=0;
	plt_cur->ygrid=0;
	plt_cur->yticks=0;
	plt_cur->autocolor=0;
	gsubplot(r,c,index);		/* callback for UI */
	
	result=new_matrix(cc,1,3,"");	/* return [r c id] */
	m=matrixof(result);
	*m++=(real)r;
	*m++=(real)c;
	*m=(real)index;
	return moveresult(cc,st,result);
}

static void minmax (real *x, ULONG n, real *min, real *max, 
	int *imin, int *imax)
/***** minmax
	compute the total minimum and maximum of n real numbers.
*****/
{	ULONG i;
	if (n==0) {
		*min=0; *max=0; *imin=0; *imax=0; return;
	}
	*min=*x; *max=*x; *imin=0; *imax=0; x++;
	for (i=1; i<n; i++) {
		if (*x<*min) {
			*min=*x; *imin=(int)i;
		} else if (*x>*max) {
			*max=*x; *imax=(int)i;
		}
		x++;
	}
}


void do_plot (Calc *cc, plot_t* p, header *hdx, header *hdy)
{	int cx,rx,cy,ry,i,ix,iy;
	real *x,*y;
	char* style = p->style;
	getmatrix(hdx,&rx,&cx,&x); getmatrix(hdy,&ry,&cy,&y);
	if (cx!=cy || (rx>1 && ry!=rx))
		cc_error(cc,"Plot columns must agree!");
	if (p->scaling)	{
		minmax(x,(LONG)cx*rx,&p->x_min,&p->x_max,&ix,&iy);
		minmax(y,(LONG)cy*ry,&p->y_min,&p->y_max,&ix,&iy);
	}
	if (p->x_min==p->x_max) p->x_max=p->x_min+1;
	if (p->y_min==p->y_max) p->y_max=p->y_min+1;
	graphic_mode();
	if (!p->holding) gclear();
	if (!p->holding || p->newframe) frame(p);
	gclip(p);
	for (i=0; i<ry; i++) {
		style=parsestyle(cc, p, style);
		if (p->xlog && p->x_min<=0.0)
			cc_error(cc,"x log axis with negative or null boundary\n"); goto out;
		if (p->ylog && p->y_min<=0.0)
			cc_error(cc,"y log axis with negative or null boundary\n"); goto out;
		gpath(p,mat(x,cx,(i>=rx)?rx-1:i,0),mat(y,cy,i,0),cx);
		if (p->autocolor) {
			linecolor++;
			if (linecolor==16) linecolor=1;
		}
		if (sys_test_key()==27) break;
	}
	p->holding=1;
out:
	gunclip(p);
	gflush();
}

/* mplot: builtin funtion
 *   that plots the graph y(x) with line and/or marks according to the
 *   third parameter
 *   parameters
 *   x     : real vector/matrix
 *   y     : real vector/matrix
 *   style : string describing the style
 *           - s{lx|ly|ll}                      scale style
 *           - l{n|-|.|--}                      line style
 *           - m{x|+|*|.|{#}o|{#}[]|{#}<>}      marker style
 *           - c{#rrggbb|number{+}}  color style
 *   returns the 1x4 vector [xmin,xmax,ymin,ymax]
 */
header* mplot (Calc *cc, header *hd)
{	header *hd1,*st=hd,*result;
	real *m;
	hd=getvalue(cc,hd);
	if (hd) { /* parameters given */
		if (hd->type!=s_matrix && hd->type!=s_real)
			cc_error(cc, "Plot needs a real vector or matrix!");
		hd1=next_param(cc,st);
		if (hd1) {
//			hd2=next_param(cc,hd1);
			hd1=getvalue(cc,hd1);
			if (hd1->type!=s_matrix && hd1->type!=s_real)
				cc_error(cc,"Wrong arguments for plot!");
/*			if (hd2) {
				hd2=getvalue(hd2); if (error) return;
				if (hd2->type!=s_string)
				{	error=11001; output("Wrong arguments for plot!\n");
					return;
				}
				plt_cur->style = hd2 ? stringof(hd2) : "";
			}*/
		}
	}
	
	do_plot(cc,plt_cur,hd,hd1);
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	*m++=plt_cur->x_min; *m++=plt_cur->x_max; *m++=plt_cur->y_min; *m++=plt_cur->y_max;
	return moveresult(cc,st,result);
}


/* mplotarea: builtin function
 *   gets the area needed to plot the graph y(x)
 *   parameters
 *   x        : real vector/matrix
 *   y        : real vector/matrix
 *   returns the 1x4 vector [xmin,xmax,ymin,ymax]
 */
header* mplotarea (Calc *cc, header *hd)
{	header *hd1=0,*st=hd,*result;
	real *x,*y;
	int cx,rx,cy,ry,ix,iy;
	hd=getvalue(cc,hd);
	if (hd) { /* parameters given */
		if (hd->type!=s_matrix && hd->type!=s_real)
			cc_error(cc,"Plot needs a real vector or matrix!");
		hd1=next_param(cc, st);
		if (hd1) hd1=getvalue(cc,hd1);
		if (hd1->type!=s_matrix && hd1->type!=s_real)
			cc_error(cc,"Wrong arguments for plotarea!");
	}
	getmatrix(hd,&rx,&cx,&x); getmatrix(hd1,&ry,&cy,&y);
	if (cx!=cy || (rx>1 && ry!=rx))
		cc_error(cc,"Plot columns must agree!");
	if (plt_cur->scaling) {
		minmax(x,(LONG)cx*rx,&plt_cur->x_min,&plt_cur->x_max,&ix,&iy);
		minmax(y,(LONG)cy*ry,&plt_cur->y_min,&plt_cur->y_max,&ix,&iy);
		if (plt_cur->x_min==plt_cur->x_max) plt_cur->x_max=plt_cur->x_min+1;
		if (plt_cur->y_min==plt_cur->y_max) plt_cur->y_max=plt_cur->y_min+1;
		plt_cur->scaling=0;
	}
	result=new_matrix(cc,1,4,"");
	x=matrixof(result);
	*x++=plt_cur->x_min; *x++=plt_cur->x_max; *x++=plt_cur->y_min; *x++=plt_cur->y_max;
	return moveresult(cc,st,result);
}

/* msetplot: builtin function
 *   sets the limits of the graph given by a 1x4 vector [xmin,xmax,ymin,ymax].
 *   returns the old settings.
 */
header* msetplot (Calc *cc, header *hd)
{	header *st=hd,*result;
	real *m;
	hd=getvalue(cc,hd);
	if (hd->type!=s_matrix || dimsof(hd)->r!=1 || dimsof(hd)->c!=4)
		cc_error(cc,"Setplot needs a 1x4 vector!");
	result=new_matrix(cc,1,4,"");
	m=matrixof(result);
	*m++=plt_cur->x_min; *m++=plt_cur->x_max; *m++=plt_cur->y_min; *m++=plt_cur->y_max;
	m=matrixof(hd);
	plt_cur->x_min=*m++; plt_cur->x_max=*m++; plt_cur->y_min=*m++; plt_cur->y_max=*m++;
	plt_cur->scaling=0;
	return moveresult(cc,st,result);
}

/* mplot1: builtin function
 *   returns the current graph limits as a 1x4 vector [xmin,xmax,ymin,ymax].
 */
header* mplot1 (Calc *cc, header *hd)
{	header *st=hd, *result;
	real *x;
	result=new_matrix(cc,1,4,"");
	x=matrixof(result);
	*x++=plt_cur->x_min; *x++=plt_cur->x_max; *x++=plt_cur->y_min; *x++=plt_cur->y_max;
	return moveresult(cc,st,result);
}

/* mholding: builtin function
 *   sets the value of holding (same as hold on/off)
 *   returns the last setting
 */
header* mholding (Calc *cc, header *hd)
{	header *st=hd, *result;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"Holding needs a 1 or 0!");
	result=new_real(cc,plt_cur->holding,"");
	plt_cur->holding=(*realof(hd)!=0.0); plt_cur->scaling=!plt_cur->holding;
	return moveresult(cc,st,result);
}

/* mholding0: builtin function
 *   returns the current holding value.
 */
header* mholding0 (Calc *cc, header *hd)
{	return new_real(cc,plt_cur->holding,"");
}

/* mlogscale: builtin function
 *   sets the value of xlog and ylog flags
 *   returns the last setting
 */
header* mlogscale (Calc *cc, header *hd)
{	header *st=hd,*result;
	real *m;
	hd=getvalue(cc,hd);
	if (hd->type!=s_matrix || dimsof(hd)->r!=1 || dimsof(hd)->c!=2)
		cc_error(cc,"logscale needs a 1x2 vector!");
	result=new_matrix(cc,1,2,"");
	m=matrixof(result);
	*m++=plt_cur->xlog; *m=plt_cur->ylog;
	m=matrixof(hd);
	plt_cur->xlog=*m++; plt_cur->ylog=*m;
	plt_cur->scaling=0;
	return moveresult(cc,st,result);
}

/* mlogscale0: builtin function
 *   returns the current [xlog,ylog] logscale value.
 */
header* mlogscale0 (Calc *cc, header *hd)
{	header *st=hd, *result;
	real *m;
	result=new_matrix(cc,1,2,"");
	m=matrixof(result);
	*m++=plt_cur->xlog;*m=plt_cur->ylog;
	return moveresult(cc,st,result);
}

/* mscaling: builtin function
 *   sets the value of scaling (defines if scales need to be searched for)
 *   returns the last setting
 */
header* mscaling (Calc *cc, header *hd)
{	header *st=hd,*result;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real)
		cc_error(cc,"Scaling needs a 1 or 0!");
	result=new_real(cc,plt_cur->scaling,"");
	plt_cur->scaling=(*realof(hd)!=0.0);
	return moveresult(cc,st,result);
}

header* mxgrid (Calc *cc, header* hd)
{
	header *hd1,*hd2,*hd3,*hd4,*st=hd,*result;
	int r,c;
	real *ticks;
	
	if (plt_cur->xlog && plt_cur->x_min<=0.0)
		cc_error(cc,"x log axis with negative or null boundary");
	
	hd=getvalue(cc,hd);								// ticks
	if (hd->type!=s_matrix || dimsof(hd)->r!=1)
		cc_error(cc,"xgrid needs the ticks vector!");

	hd1=next_param(cc,st);
	if (hd1) {
		hd2=next_param(cc,hd1);
		hd1=getvalue(cc,hd1);						// factor
		if (hd1->type!=s_real) cc_error(cc,"Wrong arguments for xgrid!");
	}
	if (hd2) {
		hd3=next_param(cc,hd2);
		hd2=getvalue(cc,hd2);						// draw grid (bool)
		if (hd2->type!=s_real) cc_error(cc,"Wrong arguments for xgrid!");
	}
	if (hd3) {
		hd4=next_param(cc,hd3);
		hd3=getvalue(cc,hd3);						// draw ticks (bool)
		if (hd3->type!=s_real) cc_error(cc,"Wrong arguments for xgrid!");
	}
	if (hd4) {
		hd4=getvalue(cc,hd4);						// grid color
		if (hd4->type!=s_real) cc_error(cc,"Wrong arguments for xgrid!");
	}
	getmatrix(hd,&r,&c,&ticks);
	plt_cur->xgrid = (*realof(hd2)==1.0);
	plt_cur->xticks = (*realof(hd3)==1.0);
	gridcolor = *realof(hd4);
	gxgrid(plt_cur, *realof(hd1), ticks, c);
	result=new_real(cc,0.0,"");
	return moveresult(cc,st,result);
}

header* mygrid (Calc *cc, header* hd)
{
	header *hd1,*hd2,*hd3,*hd4,*st=hd,*result;
	int r,c;
	real *ticks;
	
	if (plt_cur->ylog && plt_cur->y_min<=0.0)
		cc_error(cc,"y log axis with negative or null boundary");

	hd=getvalue(cc,hd);							// ticks
	if (hd->type!=s_matrix || dimsof(hd)->r!=1)
		cc_error(cc,"ygrid needs the ticks vector!");
	hd1=next_param(cc,st);
	if (hd1) {
		hd2=next_param(cc,hd1);
		hd1=getvalue(cc,hd1);					// factor
		if (hd1->type!=s_real) cc_error(cc,"Wrong arguments for ygrid!");
	}
	if (hd2) {
		hd3=next_param(cc,hd2);
		hd2=getvalue(cc,hd2);					// draw grid (bool)
		if (hd2->type!=s_real) cc_error(cc,"Wrong arguments for ygrid!");
	}
	if (hd3) {
		hd4=next_param(cc,hd3);
		hd3=getvalue(cc,hd3);					// draw ticks (bool)
		if (hd3->type!=s_real) cc_error(cc,"Wrong arguments for ygrid!");
	}
	if (hd4) {
		hd4=getvalue(cc,hd4);					// grid color
		if (hd4->type!=s_real) cc_error(cc,"Wrong arguments for ygrid!");
	}
	getmatrix(hd,&r,&c,&ticks);
	plt_cur->ygrid = (*realof(hd2)==1.0);
	plt_cur->yticks = (*realof(hd3)==1.0);
	gridcolor = *realof(hd4);
	gygrid(plt_cur, *realof(hd1), ticks, c);
	result=new_real(cc,0.0,"");
	return moveresult(cc,st,result);
}

/* mouse: builtin function
 *   returns the mouse coordinates in the current basis
 *   WARNING! gives log(x) value when logscale are used
 */
header* mmouse (Calc *cc, header *hd)
{	header *st=hd, *result;
	int c,r;
	real *m;
	result=new_matrix(cc,1,2,"");
	graphic_mode();
	mouse(&c,&r);
	m=matrixof(result);
	*m++=plt_cur->x_min+(c-plt_cur->upperc)/(real)(plt_cur->lowerc-plt_cur->upperc)*(plt_cur->x_max-plt_cur->x_min);
	*m++=plt_cur->y_max-(r-plt_cur->upperr)/(real)(plt_cur->lowerr-plt_cur->upperr)*(plt_cur->y_max-plt_cur->y_min);
	return moveresult(cc,st,result);
}

/* mpixel: builtin function
 *   returns the mouse coordinates in the current basis
 *   WARNING! gives log(x) value when logscale are used
 */
header* mpixel (Calc *cc, header *hd)
{	real x,y;
	hd=new_matrix(cc,1,2,""); 
	getpixel(&x,&y);
	x*=(plt_cur->x_max-plt_cur->x_min)/(plt_cur->lowerc-plt_cur->upperc);
	y*=(plt_cur->y_max-plt_cur->y_min)/(plt_cur->lowerr-plt_cur->upperr);
	*(matrixof(hd))=x; *(matrixof(hd)+1)=y;
	return hd;
}

void ghold (Calc *cc)
/**** hold
	toggles holding of the current plot.
****/
{	static int oldhold=-1;
//	scan_space();
	if (!strncmp(cc->next,"off",3))
	{	oldhold=-1; plt_cur->holding=0; cc->next+=3;
	}
	else if (!strncmp(cc->next,"on",2))
	{	oldhold=-1; plt_cur->holding=1; cc->next+=2;
	}
	else
	{	if (oldhold!=-1) {	plt_cur->holding=oldhold; oldhold=-1; }
		else { oldhold=plt_cur->holding; plt_cur->holding=1; }
	}
	plt_cur->scaling=!plt_cur->holding;
}

void show_graphics (Calc *cc)
{	scan_t scan;
	graphic_mode(); sys_wait_key(&scan); text_mode();
}


header* mscale (Calc *cc, header *hd)
{	hd=getvalue(cc, hd);
	if (hd->type!=s_real) cc_error(cc,"Scale needs a real!");
	scale(*realof(hd));
	return hd;
}

header* mcolor (Calc *cc, header *hd)
{	int old=linecolor;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"Argument for color must be real!");
	linecolor=(int)*realof(hd);
	*realof(hd)=(real)old;
	return hd;
}

header* mtcolor (Calc *cc, header *hd)
{	int old=textcolor;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real)cc_error(cc,"Argument for color must be real!");
	textcolor=(int)*realof(hd);
	*realof(hd)=(real)old;
	return hd;
}

header* mstyle0 (Calc *cc, header*hd)
{
	header *result = new_cstring(cc,"",strlen(plt_cur->style)+1,"");
	int i;
	char *dest=stringof(result);
	for (i = 0; i < 31 && plt_cur->style[i]!='\0'; i++)
		dest[i] = plt_cur->style[i];
	dest[i]='\0';
	return result;
}

header* mstyle (Calc *cc, header *hd)
{
	header *st=hd, *result;
	char *s;
	int i;
	hd=getvalue(cc,hd);
	if (hd->type!=s_string) cc_error(cc,"Argument style must be a string!");
	result=new_cstring(cc,"",strlen(plt_cur->style)+1,"");
	s=stringof(result);
	for (i = 0; i < 31 && plt_cur->style[i] != '\0'; i++)
		s[i] = plt_cur->style[i];
	s[i]='\0';

	s=stringof(hd);
	for (i = 0; i < 31 && s[i]!='\0'; i++) plt_cur->style[i]=s[i];
	plt_cur->style[i]='\0';
	parsestyle(cc, plt_cur, s);
	return moveresult(cc,st,result);
	
/*	
	if (!strcmp(stringof(hd),"i")) linetype=line_none;
	else if (!strcmp(stringof(hd),"-")) linetype=line_solid;
	else if (!strcmp(stringof(hd),".")) linetype=line_dotted;
	else if (!strcmp(stringof(hd),"--")) linetype=line_dashed;
	else if (!strcmp(stringof(hd),"mx")) markertype=marker_cross;
	else if (!strcmp(stringof(hd),"mo")) markertype=marker_circle;
	else if (!strcmp(stringof(hd),"m<>")) markertype=marker_diamond;
	else if (!strcmp(stringof(hd),"m.")) markertype=marker_dot;
	else if (!strcmp(stringof(hd),"m+")) markertype=marker_plus;
	else if (!strcmp(stringof(hd),"m[]")) markertype=marker_square;
	else if (!strcmp(stringof(hd),"m*")) markertype=marker_star;
	else { markertype=marker_cross; linetype=line_solid; }
*/
}

header* mlinew (Calc *cc, header *hd)
{	header *st=hd,*result;
	int h,old=linewidth;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"Argument for linewidth must be a real!");
	if ((h=(int)*realof(hd))!=0) linewidth=h;
	result=new_real(cc,old,"");
	return moveresult(cc,st,result);
}

header* mtext (Calc *cc, header *hd)
{	header *st=hd, *hd1, *hd2;
	hd1=next_param(cc,hd);
	hd2=next_param(cc,hd1);
	hd=getvalue(cc,hd);
	if (hd1) hd1=getvalue(cc,hd1);
	if (hd1) hd2=getvalue(cc,hd2);
	if (hd->type!=s_string || hd1->type!=s_matrix || 
		dimsof(hd1)->r!=1 || dimsof(hd1)->c!=2 || hd2->type!=s_real)
		cc_error(cc,"Text needs a string and a vector [x y]!");
	graphic_mode();
	gtext((int)*matrixof(hd1),(int)*(matrixof(hd1)+1),
		stringof(hd),textcolor,*realof(hd2));
	gflush();
	return moveresult(cc,st,hd);
}

header* mtextsize (Calc *cc, header *hd)
{	header *st=hd, *result;
	result=new_matrix(cc,1,2,"");
	*matrixof(result)=wchar;
	*(matrixof(result)+1)=hchar;
	return moveresult(cc,st,hd);
}
#endif
