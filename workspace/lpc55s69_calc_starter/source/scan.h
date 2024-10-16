/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * scan.h
 *
 ****************************************************************/
#ifndef SCAN_H
#define SCAN_H

#include "calc.h"

typedef enum {
	T_NONE=0,
	T_EOS,				/* '\n' ou \0 */
	T_ADD,				/* '+' */
	T_SUB,				/* '-' */
	T_MUL,				/* '*' */
	T_DIV,				/* '/' */
	T_POW,				/* '^' */
	T_LPAR,				/* '(' */
	T_RPAR,				/* ')' */
	T_NEG,				/* unary minus '-' */
	T_LBRACKET,			/* '[' */
	T_RBRACKET,			/* ']' */
	T_LBRACE,			/* '{' */
	T_RBRACE,			/* '}' */
	T_ASSIGN,			/* '=' */
	T_EQ,				/* '==' */
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
	T_COMMA,			/* ',' */
	T_COL,				/* ':' */
	T_SEMICOL,			/* ';' */
	T_DOT,				/* '.' */
	T_HASH,				/* '#' */
	T_REAL,				/* a real number */
	T_IMAG,				/* an imaginary part number*/
	T_LABEL,			/* a label */
	T_FUNCREF,			/* a function ref: 'name(' */
	T_MATREF			/* a matrix ref: 'name[' */
} token_t;

token_t scan(Calc *cc);

#endif
