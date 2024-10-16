/* graphics.c */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "calc.h"

/***
 * 2D Graphics
 *
 * Functions
 *   [r,c,id]=subplot(211)
 *   [xm,xM,ym,yM]=plotarea(x,y)
 *   [xm,xM,ym,yM]=setplot([xm,xM,ym,yM])
 *   [xm,xM,ym,yM]=setplot()
 *   [xm,xM,ym,yM]=plot(x,y,style)
 *   xgrid({ticks}) // custom xticks (linear mode)
 *   ygrid({ticks}) // custom yticks (linear mode)
 *   text(s,[y,y],angle,indent)
 *   legend([x,y],indent)
 *   
 * Graph state and style
 * - type: 2d, complex plane
 * - style: none, axes, frame
 * - xorg,yorg
 * - linear/log scales in x, y
 * - text objs
 *   * size
 * Data
 * - lines: l{...}
 *   * color
 *   * width
 *   * style n,-,.,--,! (comb),->,h,
 * - marks: m{#}{...}
 *   * color
 *   * style! {#}[],<>,o,x,+,^,v
 *
 *************/
 
header* msubplot (Calc *cc, header *hd);
header* msetplot (Calc *cc, header *hd);
header* msetplot0 (Calc *cc, header *hd);
header* mplotarea (Calc *cc, header *hd);
header* mplot (Calc *cc, header *hd);
header* mplot1 (Calc *cc, header *hd);
header* mplotstyle (Calc *cc, header *hd);
header* mxgrid (Calc *cc, header* hd);
header* mygrid (Calc *cc, header* hd);
header* mtext (Calc *cc, header *hd);
header* mtitle(Calc *cc, header *hd);
header* mxlabel(Calc *cc, header *hd);
header* mylabel(Calc *cc, header *hd);

#if 0
//header* mcontour (Calc *cc, header *hd);
//header* mwire (Calc *cc, header *hd);
//header* msolid (Calc *cc, header *hd);
//header* msolid1 (Calc *cc, header *hd);
//header* mview (Calc *cc, header *hd);
//header* mmesh (header *);
header* mplot1 (Calc *cc, header *hd);
void ghold (Calc *cc);
void show_graphics (Calc *cc);
//header* mctext (Calc *cc, header *hd);
header* mtextsize (Calc *cc, header *hd);
header* mstyle (Calc *cc, header *hd);
header* mcolor (Calc *cc, header *hd);
header* mfcolor (Calc *cc, header *hd);
header* mwcolor (Calc *cc, header *hd);
header* mtcolor (Calc *cc, header *hd);
header* mwindow (Calc *cc, header *hd);
header* mwindow0 (Calc *cc, header *hd);
header* mscale (Calc *cc, header *hd);
header* mmouse (Calc *cc, header *hd);
//header* mproject (Calc *cc, header *hd);
header* mview0 (Calc *cc, header *hd);
header* mholding (Calc *cc, header *hd);
header* mholding0 (Calc *cc, header *hd);
header* mscaling (Calc *cc, header *hd);
#endif

#endif
