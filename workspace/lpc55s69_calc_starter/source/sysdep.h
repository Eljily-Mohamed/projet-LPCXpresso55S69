/****************************************************************
 * calc
 *  (C) 1993-2023 R. Grothmann
 *  (C) 2021-2024 E. Bouchare
 *
 * sysdep.h
 *
 ****************************************************************/
#ifndef SYSDEP_H
#define SYSDEP_H

typedef struct _Calc	Calc;
typedef struct _header  header;

#define TERMWIDTH		80
#define TERMHEIGHT		24

#define HELPFILE "2:/help.txt"

#include <math.h>
#include <float.h>


#define LABEL_LEN_MAX		14	/* Maximum length of an identifier */

#define NESTED_CTRL_MAX		20	/* Maximum number of nested control structures */

#define LONG	long
#define ULONG	unsigned long

#ifdef FLOAT32
typedef float			real;

#define fmod			fmodf
#define floor			floorf
#define ceil			ceilf
#define	fabs			fabsf
#define sqrt			sqrtf
#define pow				powf
#define log				logf
#define log10			log10f
#define exp				expf
#define cos				cosf
#define sin				sinf
#define tan				tanf
#define acos			acosf
#define asin			asinf
#define atan			atanf
#define atan2			atan2f
#define erf				erff
#define erfc			erfcf

#define EPSILON			FLT_EPSILON

#define UDF_LEVEL_MAX	12

#define ALIGNMENT		4

#else

typedef double			real;

#define fmod			fmod
#define floor			floor
#define ceil			ceil
#define fabs			fabs
#define sqrt			sqrt
#define pow				pow
#define log				log
#define log10			log10
#define exp				exp
#define cos				cos
#define sin				sin
#define tan				tan
#define acos			acos
#define asin			asin
#define atan			atan
#define atan2			atan2
#define erf				erf
#define erfc			erfc

#define EPSILON			DBL_EPSILON

#define UDF_LEVEL_MAX	40

#define ALIGNMENT		8

#endif

#define ALIGN(sz)		((sz)!=0 ? ((((sz)-1)/ALIGNMENT)+1)*ALIGNMENT : 0)

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

/* Filesystem interaction */
#ifdef _WIN32
#define PATH_DELIM_CHAR '\\'
#define PATH_DELIM_STR "\\"
#else
#define PATH_DELIM_CHAR '/'
#define PATH_DELIM_STR "/"
#endif

#define MAX_PATH			5
#define MAX_COLORS			16

extern char* path[MAX_PATH];
extern int npath;

char* fs_cd (char *dir);
int   fs_dir(char *dir_name, char *pat, char ** files[], int *files_count);
int   fs_rm(char *filename);
int   fs_mkdir(char* dirname);
int   fs_exec(char *name, char *args);

#define EXTENSION ".e"

typedef enum {
	key_normal, cursor_up, cursor_down, cursor_left, cursor_right,
	escape, delete, backspace, clear_home, switch_screen, enter,
	space, line_end, line_start, fk1, fk2, fk3, fk4, fk5, fk6, fk7,
	fk8, fk9, fk10, fk11, fk12, word_left, word_right, help, sel_insert,
	page_up, page_down, eot
} scan_t;

/* time */
real sys_clock (void);
void sys_wait (real delay, scan_t *scan);

/* text IO */
/*** output modes ***
   CC_OUTPUT:	standard result output mode
   CC_EDIT:		edited character echo mode
   CC_FEDIT:	edited function echo message
   CC_WARN:		warning message mode
   CC_ERROR:	error message mode
 */
#define	CC_OUTPUT		1
#define	CC_WARN			2
#define	CC_ERROR		3
#define CC_EDIT			4
#define CC_FEDIT		5

void sys_out_mode(int mode);
int sys_wait_key (scan_t *scan);
int sys_test_key (void);

/* output */
void sys_clear (void);		/* clear the output peripheral */
void sys_print (char *s);	/* print an output text (no newline) */

void text_mode (void);

void graphic_mode (void);
void gflush (void);		/* flush out graphics */
void gclear (void);		/* clear the graphical screen */

void edit_on_cb (void);
void edit_off_cb (void);
void cursor_off_cb (void); 
void cursor_on_cb (void);
void move_cr_cb (void);
void move_cl_cb (void);
void page_up_cb(void);
void page_down_cb(void);

void clear_eol (void);

/* graphics support */

/* graph options */
#define	G_FRAME			(1U<<0)		/* draw a frame around the graph */
#define	G_AXIS			(1U<<1)		/* draw X and Y axes */
#define	G_XLOG			(1U<<2)		/* X axis is log scales */
#define G_XGRID			(1U<<3)		/* draw the X grid */
#define	G_XTICKS		(1U<<4)		/* draw ticks on the X axis */
#define G_XAUTOTICKS	(1U<<5)		/* ticks on the X axis are generated automatically according to xmin and xmax */
#define G_YLOG			(1U<<6)		/* Y axis is log scales */
#define	G_YGRID			(1U<<7)		/* draw the Y grid */
#define G_YTICKS		(1U<<8)		/* draw ticks on the Y axis */
#define G_YAUTOTICKS	(1U<<9)		/* ticks on the Y axis are generated automatically according to ymin and ymax */
#define G_WORLDUNSET	(1U<<10)	/* no scale set yet */
#define G_AUTOSCALE		(1U<<11)	/* xmin, xmax, ymin, ymax updated according to graph extent */
#define G_AUTOCOLOR		(1U<<12)	/* plot color autoincremented */
#define G_AUTOMARK		(1U<<13)	/* mark type autoincremented */
#define G_AXISUNSET		(1U<<14)	/* axis type (linear/log) is unset */

#define G_LTYPE_MSK		(0xFU<<16)
#define G_MTYPE_MSK		(0xFU<<20)
#define G_COLOR_MSK		(0xFU<<24)
#define G_LWIDTH_MSK	(0xFU<<28)

typedef enum {
	L_SOLID, L_DOTTED, L_DASHED, L_COMB, L_ARROW, L_BAR, L_FBAR, 
	L_STEP, L_FSTEP, L_NONE
} line_t;

typedef enum {
	M_CROSS, M_PLUS, M_DOT, M_STAR, M_CIRCLE, M_FCIRCLE,
	M_SQUARE, M_FSQUARE, M_DIAMOND, M_FDIAMOND, M_ARROW,
	M_TRIANGLE, M_FTRIANGLE, M_NONE
} marker_t;

#define	G_ALIGN_NE				(1U<<0)
#define	G_ALIGN_N				(1U<<1)
#define	G_ALIGN_NW				(1U<<2)
#define	G_ALIGN_W				(1U<<3)
#define	G_ALIGN_SW				(1U<<4)
#define	G_ALIGN_S				(1U<<5)
#define	G_ALIGN_SE				(1U<<6)
#define	G_ALIGN_E				(1U<<7)
#define	G_ALIGN_CENTER			(1U<<8)

#define G_TITLE					(1U<<0)
#define G_XLABEL				(1U<<1)
#define G_YLABEL				(1U<<2)

void graphic_mode (void);
void gsubplot(int r, int c, int index);
void gsetplot(real xmin, real xmax, real ymin, real ymax, unsigned long flags, unsigned long mask);
void ggetplot(real *xmin, real *xmax, real *ymin, real *ymax, unsigned long *flags);
void gplot(Calc *cc, header *hdx, header *hdy);
void gsetxgrid(header *ticks, real factor, unsigned int color);
void gsetygrid(header *ticks, real factor, unsigned int color);
void gtext(real x, real y, char *text, unsigned int align, int angle, unsigned int color);
void glabel(char *text, unsigned int type);

void gclear (void);
void mouse (int *, int *);
void getpixel (real *x, real *y);
void scale (real s);

#endif
