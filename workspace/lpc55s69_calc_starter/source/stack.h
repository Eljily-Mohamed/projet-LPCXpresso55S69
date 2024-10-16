/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * stack.h
 *
 ****************************************************************/
#ifndef STACK_H
#define STACK_H

#ifdef HEADER32
/* Header 32 bytes */
typedef enum {
	s_real,
	s_complex,
	s_matrix,
	s_cmatrix,
	s_string,
	s_udf,
	s_reference,
	s_submatrixref,
	s_csubmatrixref,
	s_funcref,
	s_command,
} stacktyp;

struct _header {
	char			name[LABEL_LEN_MAX+1];
	char			xor;
	size_t			size;
	stacktyp		type;
	unsigned int	flags;
};
#else
/* Header 24 bytes */
typedef unsigned short	stacktyp;

#define s_real				0
#define s_complex			1
#define s_matrix			2
#define s_cmatrix			3
#define s_string			4
#define s_udf				5
#define s_reference			6
#define s_submatrixref		7
#define s_csubmatrixref		8
#define s_funcref			9
#define s_command			10

struct _header {
	char			name[LABEL_LEN_MAX+1];
	char			xor;
	int				size;
	stacktyp		type;
	unsigned short	flags;
};
#endif

/* flag defines */
#define FLAG_UDFTRACE		0x01		/* trace the UDF function */
#define FLAG_COPYONWRITE	0x10		/* enforce copy on write policy for user function reference parameters */
#define FLAG_NAMEDPAR		0x20
#define FLAG_CONST			0x40		/* variable has const status */
#define FLAG_BINFUNC		0x80
#define FLAG_SUBMALLR		0x100		/* submatrix gets all rows from original matrix */
#define FLAG_SUBMALLC		0x200		/* submatrix gets all cols from original matrix */

/* matrix dimensions */
typedef struct {
	int c,r;
} dims;

typedef struct { header hd; double val; } realtyp;

/* binary function */
typedef struct _binfunc_t {
	char 	*name;
	int		nargs;
	header* (*f) (Calc *cc, header *);
} binfunc_t;

/* user defined functions */
#define	MAXARGS			10

typedef struct {
	char	name[LABEL_LEN_MAX+1];
	char	xor;
} udf_arg;

#define realof(hd) ((real *)((hd)+1))
#define imagof(hd) ((real *)((hd)+1)+1)
#define cplxof(hd) ((real *)((hd)+1))
#define matrixof(hd) ((real *)((char *)((hd)+1)+sizeof(dims)))
#define dimsof(hd) ((dims *)((hd)+1))
#define commandof(hd) ((int *)((hd)+1))
#define referenceof(hd) (*((header **)((hd)+1)))
#define rowsof(hd) ((int *)((dims *)((header **)((hd)+1)+1)+1))
#define colsof(hd) ((int *)((dims *)((header **)((hd)+1)+1)+1)+submdimsof((hd))->r)
#define submrefof(hd) (*((header **)((hd)+1)))
#define submdimsof(hd) ((dims *)((header **)((hd)+1)+1))
#define stringof(hd) ((char *)((hd)+1))
/* get the binfunc address from a binfuncref */
#define binfuncof(hd) (*(binfunc_t**)((hd)+1))
/* get the starting address of the udf code */
#define udfof(hd) ((char *)(hd)+(*((ULONG *)((hd)+1))))
/* get the address of the offset to jump to the start of udf code */
#define udfstartof(hd) ((ULONG*)((hd)+1))
/* get the next arg, */
#define udfnextarg(p, hasval)  ( (hasval) ? (char *)(p)+((header*)p)->size : (char*)(p)+sizeof(udf_arg) )
/* get the address of the parameter section of the udf */
#define udfargsof(hd) ((char *)(hd)+sizeof(header)+sizeof(ULONG))
/* get the address of the next object on the stack */
#define nextof(hd) ((header *)((char *)(hd)+(hd)->size))


#define mat(m,c,i,j) (m+((LONG)(c)*(i)+(j)))
#define cmat(m,c,i,j) (m+(2*((LONG)(c)*(i)+(j))))

#define matrixsize(c,r) (sizeof(dims)+(c)*(r)*sizeof(real))
#define cmatrixsize(c,r) (sizeof(dims)+2l*(c)*(r)*sizeof(real))

#define isreal(hd) (((hd)->type==s_real || (hd)->type==s_matrix))
#define iscplx(hd) (((hd)->type==s_complex || (hd)->type==s_cmatrix))
#define isrealorcplx(hd) (((hd)->type==s_complex || (hd)->type==s_cmatrix || (hd)->type==s_real || (hd)->type==s_matrix))

/* utils */
int xor (char *n);

/* allocate stack and initialize stack environment */
int stack_init(Calc *cc, unsigned long stacksize);
void stack_rewind(Calc *cc);

/* stack allocation */
void* stack_alloc (Calc *cc, stacktyp type, int size, char *name);
void* stack_realloc(Calc *cc, header* hd, int size);

/* push new element on the stack */
header* new_real (Calc *cc, real x, char *name);
header* new_complex (Calc *cc, real x, real y, char *name);
header* new_string(Calc *cc);
header* new_cstring (Calc *cc, char *s, int size, char *name);
header* new_matrix (Calc *cc, int rows, int cols, char *name);
header* new_cmatrix (Calc *cc, int rows, int cols, char *name);
header* new_reference (Calc *cc, header *hd, char *name);
header* new_submatrix (Calc *cc, header *hd, header *cols, header *rows,
	char *name);
header* new_csubmatrix (Calc *cc, header *hd, header *cols, header *rows,
	char *name);
header* new_command (Calc *cc, int no);
header* new_udf (Calc *cc, char *name);
header* new_funcref (Calc *cc, header *hd, char *name);
header* new_binfuncref (Calc *cc, binfunc_t *ref, char *name);
header* new_subm (Calc *cc, header *var, ULONG l, char *name);
header* new_csubm (Calc *cc, header *var, ULONG l, char *name);

void getmatrix (header *hd, int *r, int *c, real **m);

header *getvalue (Calc *cc, header *hd);
header *assign (Calc *cc, header *var, header *value);

header *searchvar (Calc *cc, char *name);
header *searchudf (Calc *cc, char *name);

int kill_local (Calc *cc, char *name);
int kill_udf (Calc *cc, char *name);

header *next_param (Calc *cc, header *hd);

header* moveresult (Calc *cc, header *stack, header *result);
header* moveresults (Calc *cc, header *stack, header *result);
header* pushresults(Calc *cc, header *result);

#endif
