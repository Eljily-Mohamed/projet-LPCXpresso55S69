/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * calc.h
 *
 ****************************************************************/
#ifndef CALC_H
#define CALC_H

#include <stdio.h>
#include <setjmp.h>

#include "sysdep.h"
#include "stack.h"

extern Calc *calc;

/* commands */
typedef enum {
	c_none=-1, c_allv, c_quit, c_cmd,
	c_return, c_endfunction,
	c_for, c_end, c_break, c_to, c_do, c_in, c_step, c_loop, 
	c_repeat, c_while, c_until,
	c_if, c_then, c_else, c_elseif, c_endif,
	c_global, c_const
} cmdtyp;

typedef real cplx[2];

#define ISDIGIT(x) ((x)>='0' && (x)<='9')
#define ISALPHA(x) (((x)>='A' && (x)<='Z') || ((x)>='a' && (x)<='z'))

typedef enum {
	T_NONE=0,
	T_ADD,				/* '+' */
	T_SUB,				/* '-' */
	T_MUL,				/* '*' */
	T_DIV,				/* '/' */
	T_POW,				/* '^' */
	T_DOT,				/* '.' */
	T_SOLVE,			/* '\\' */
	T_LPAR,				/* '(' */
	T_RPAR,				/* ')' */
	T_NEG,				/* unary minus '-' */
	T_LBRACKET,			/* '[' */
	T_RBRACKET,			/* ']' */
	T_LBRACE,			/* '{' */
	T_RBRACE,			/* '}' */
	T_EQ,				/* '==' */
	T_ABOUTEQ,			/* '~=' */
	T_NE,				/* '!=' */
	T_GT,				/* '>' */
	T_GE,				/* '>=' */
	T_LT,				/* '<' */
	T_LE,				/* '<=' */
	T_AND,				/* '&&' */
	T_OR,				/* '||' */
	T_NOT,				/* '!' */
	T_HCONCAT,			/* '|' */
	T_VCONCAT,			/* '_' */
	T_TRANSPOSE,		/* '\'' */
	T_COL,				/* ':' */
	T_ASSIGN,			/* '=' */
	T_EOS,				/* '\n' ou \0 */
	T_SEMICOL,			/* ';' */
	T_COMMA,			/* ',' */
	T_THEN,				/* then */
	T_ELSE,				/* else */
	T_ELSEIF,			/* elseif */
	T_UNTIL,			/* until */
	T_TO,				/* to */
	T_IN,				/* in */
	T_DO,				/* do */
	T_END,				/* end */
	T_STEP,				/* step */
	T_HASH,				/* '#' */
	T_DQUOTE,			/* '"' */
	T_REAL,				/* a real number */
	T_IMAG,				/* an imaginary part number*/
	T_LABEL,			/* a label */
	T_FUNCREF,			/* a function ref: 'name(' */
	T_MATREF,			/* a matrix ref: 'name[' */
	T_MATREF1,			/* a matrix ref: 'name{' */
} token_t;

#define LINEMAX	256		/* Maximum input line length */

struct _Calc {
	char *			line;			/* pointer to the input line */
	int				linenb;			/* line number */
	/* scanner */
	char *			next;			/* pointer to the next char */
	real			val;			/* real value scanned */
	char			str[LABEL_LEN_MAX+1];	/* string scanned */
	
#if 0
	/* output mode (EDIT_ECHO/OUTPUT/WARNING/ERROR) */
	int				omode;
#endif
	
	/* number output formatting */
	int				disp_mode;
	int				disp_digits;
	int				disp_fieldw;
	int				disp_eng_sym;
	real			maxexpo;
	real			minexpo;
	char			expoformat[16];
	char			fixedformat[16];

	/* stack handling */
	char *			ramstart;		/* tobal start of RAM for the stack */
	char *			ramend;			/* total end of RAM for the Euler stack */
	char *			newram;			/* start of new ram for new variables on stack */
	char *			udfstart;		/* start of user defined functions */
	char *			udfend;			/* end of user defined functions */
	char *			globalstart;	/* start of the global context */
	char *			globalend;		/* end of the global context */
	char *			startlocal;		/* start of current local variables */
	char *			endlocal;		/* end of current local variables */
	
	/* IO */
	FILE *			infile;			/* input file */
	FILE *			outfile;		/* output file */

	header *		stack;			/* where the arguments are stacked,
									   where the results are to be stacked */
	char *			xstart;			/* extra parameter start address */
	char *			xend;			/* extra parameter */
	header *		result;			/* last result */
	int				nresults;		/* number of results returned */
	
	/* user defined functions handling */
	header *		running;		/* running udf */
	int				actargn;		/* actual number of arguments */
	int				trace;			/* tracing feature activated? */
	long			loopindex;		/* index used by loop statement */
	int				level;			/* udf call level */
	
	/* epsilon used by ~= operator to say two values are equal */
	real			epsilon;

	/* state values */
	int				quit;			/* if 1, exit the interpreter, default 0 */
	unsigned int	flags;			/* parser flags */
	
	/* terminal window handling */
	int				termwidth;		/* terminal window width [nb chars] */
	int				encoding;
	
	/* error handling */
	jmp_buf *		env;
};

/*** calc flags ***
   CC_OUTPUTING: allow outputing results
   CC_NOSUBMREF:        0 : do not expand submatrix references values can
                            be affected to (before '='),
                        1 : expand submatrices references early so that
                            actual value can be used in expression calculus,
                            after '='.
   CC_PARSE_INDEX:      when 1 allow parsing of special index ':', else 0.
   CC_PARSE_UDF:        set to 1 while parsing a user defined function(udf)
   CC_PARSE_PARAM_LIST: set to 1 while parsing the parameter list when defining
                        a udf and when calling a function (builtin or udf) -- to
                        allow correct last parenthesis of the parameter list
   CC_EXEC_UDF:         set to 1 when in a user defined function (udf),
                        default 0
   CC_EXEC_RETURN:		'return' is to be executed in the function
   CC_EXEC_STRING:      set to 1 when interpreting a single code line, default
                        0. No assignment, no nextline allowed!
                        see input(), interpret() and errorlevel() builtin
                        functions
   CC_SEARCH_GLOBALS:   when 1 allow to look for variables in the global scope
                        if they don't exist in the local scope.
   CC_VERBOSE:          when 1 allow having extra information on errors
 */
#define	CC_OUTPUTING		1<<0
#define	CC_NOSUBMREF		1<<1
#define CC_PARSE_INDEX		1<<2
#define CC_PARSE_PARAM_LIST	1<<3
#define CC_PARSE_UDF		1<<4
#define CC_EXEC_UDF			1<<5
#define CC_EXEC_RETURN		1<<6
#define CC_EXEC_STRING		1<<7
#define CC_TRACE_UDF		1<<8
#define CC_SEARCH_GLOBALS	1<<9
#define CC_VERBOSE			1<<10
#define CC_USE_UTF8			1<<16

#define CC_SET(cc,prop)		((cc)->flags |= (prop))
#define CC_UNSET(cc,prop)	((cc)->flags &= ~(prop))
#define CC_ISSET(cc,prop)	((cc)->flags & (prop))
#define CC_TOGGLE(cc,PROP)	((cc)->flags ^= (prop))

void output (Calc *cc, char *s);
void outputf (Calc *cc, char *fmt, ...);

token_t scan(Calc *cc);
token_t scan_path(Calc *cc);
int parse(Calc *cc);
token_t parse_expr(Calc *cc);
void cc_warn(Calc *cc, char *s, ...);
void cc_error(Calc *cc, char *s, ...);
void trace_udfline (Calc* cc, char *next);
void main_loop (Calc *cc, int argc, char *argv[]);
token_t cmd2tok(int cmd);

#endif
