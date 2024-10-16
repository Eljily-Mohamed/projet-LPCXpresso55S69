/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * scan.c
 *
 ****************************************************************/
#include <string.h>
#include "calc.h"
#include "edit.h"

token_t scan(Calc *cc)
{
	token_t tok;
	char* in;
	char c;
	int dexp;
	real val;
	
restart:
	tok = T_NONE;
	in = cc->next;
	
	/* skip white space */
	while ((c=*in++)==' ' || c=='\t') ;
	
	if (c==0x02) {
		/* compiled number */
		memcpy(&cc->val,in,sizeof(real));
		in+=sizeof(real);
		tok=T_REAL;
		if ((c=*in)=='i' || c=='j') {
			tok=T_IMAG;
			in++;
		}
		/*add for complex i */
	} else if (c==0x03) {
		/* compiled command */
		int cmd=*in++;
		tok=cmd2tok(cmd);
	} else if (c=='0' && !(*in>='1' && *in<='9')) {
		/* try to catch 0 */
		if((c=*in++)=='.') {
			val=0.0;
			goto do_dot;
		} else if (c=='b') {
			int v=0;
			while ((c=*in++)=='0' || c=='1') {
				v=2*v+c-'0';
			}
			in--;
			cc->val=(real)v;
			tok=T_REAL;
		} else if (c=='x') {
			int v=0;
			while(1) {
				if ((c=*in++)>='0' && c<='9') {
					v=v*16+c-'0';
				} else if (c>='A'  && c<='F') {
					v=v*16+c-'A'+10;
				} else if (c>='a'  && c<='f') {
					v=v*16+c-'a'+10;
				} else {
					break;
				}
			}
			in--;
			cc->val=(real)v;
			tok=T_REAL;
		} else {
			if (c=='i' || c=='j') {
				tok=T_IMAG;
			} else {
				tok=T_REAL;
				in--;
			}
			cc->val=0.0;
		}
	} else if (c>='1' && c<='9') {
		/* try to catch a number */
		/* integer part */
		val=c-'0';
		c=*in++;
		while (c>='0' && c<='9') {
			val=val*10+(c-'0');
			c=*in++;
		}
		if (c!='.') goto do_exp;
do_dot:	/* fractionnal part */
		if (!(*in>='0' && *in<='9')) { /* the dot is for matrix multiply ... */
			in--;
			tok=T_REAL;
			cc->val=val;
			goto scan_end;
		}
		dexp=0;
		while ((c=*in++)>='0' && c<='9') {
			val=val*10.0 + (real)(c-'0');
			dexp++;
		}
		while (dexp--) val /= 10.0;
do_exp: /* exponent */
		if (c=='e' || c=='E') {
			int neg=0;
			dexp=0;
			if ((c=*in++)=='-') neg=1;
			else if (c!='+') in--;
			if ((c=*in++)<'0' || c>'9') {
				cc_error(cc,"Missing exponent");
				return T_NONE;
			}
			while (c>='0' && c<='9') {
				dexp=dexp*10 + (c-'0');
				c=*in++;
			}
			in--;
			if (neg) {
				while (dexp--) val /= 10.0;
			} else {
				while (dexp--) val *= 10.0;
			}
		} else {
			switch (c) {
			case 'y': val*=1e-24; break;
			case 'z': val*=1e-21; break;
			case 'a': val*=1e-18; break;
			case 'f': val*=1e-15; break;
			case 'p': val*=1e-12; break;
			case 'n': val*=1e-9; break;
			case 'u': val*=1e-6; break;
			case 'm': val*=1e-3; break;
			case 'k':
			case 'K': val*=1e3; break;
			case 'M': val*=1e6; break;
			case 'G': val*=1e9; break;
			case 'T': val*=1e12; break;
			case 'P': val*=1e15; break;
			case 'E': val*=1e18; break;
			case 'Z': val*=1e21; break;
			case 'Y': val*=1e24; break;
			default: in--; break;
			}
//			while (((c=*in++)>='A' && c<'Z') || (c>='a' && c<='z') || (c>='2' && c<='9')) ;
		}
		if ((c=*in++)=='i' || c=='j') {
			tok=T_IMAG;
		} else {
			tok=T_REAL;
			in--;
		}
		cc->val=val;
		
	} else if ((c>='A' && c<='Z') || (c>='a' && c<='z') || (c=='$')) {
		/* try to catch a label */
		int i=0;
		cc->str[i++]=c;
		c=*in++;
		for ( ; 
		     i<LABEL_LEN_MAX && ((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9')) ; 
		     i++, c=*in++) {
			cc->str[i]=c;
		}
		cc->str[i]='\0';
		if (i==LABEL_LEN_MAX) cc_error(cc, "identifier too long");
		while(c==' ' || c=='\t') {c=*in++;}
		if (c=='(') {
			tok=T_FUNCREF;
		} else if (c=='[') {
			tok=T_MATREF;
		} else if (c=='{') {
			tok=T_MATREF1;
		} else if (strcmp(cc->str,"then")==0) {
			in--; tok=T_THEN;
		} else if (strcmp(cc->str,"else")==0) {
			in--; tok=T_ELSE;
		} else if (strcmp(cc->str,"elseif")==0) {
			in--; tok=T_ELSEIF;
		} else if (strcmp(cc->str,"do")==0) {
			in--; tok=T_DO;
		} else if (strcmp(cc->str,"until")==0) {
			in--; tok=T_UNTIL;
		} else if (strcmp(cc->str,"to")==0) {
			in--; tok=T_TO;
		} else if (strcmp(cc->str,"in")==0) {
			in--; tok=T_IN;
		} else if (strcmp(cc->str,"end")==0) {
			in--; tok=T_END;
		} else {
			in--;
			tok=T_LABEL;
		}
	} else {
		switch (c) {
		case '+':
			tok=T_ADD;
			break;
		case '-':
			tok=T_SUB;
			break;
		case '*':
			tok=T_MUL;
			break;
		case '/':
			tok=T_DIV;
			break;
		case '.':
			if (*in=='.') {
				next_line(cc);
				goto restart;
			}
			tok=T_DOT;
			break;
		case '\\':
			tok=T_SOLVE;
			break;
		case '^':
			tok=T_POW;
			break;
		case '(':
			tok=T_LPAR;
			break;
		case ')':
			tok=T_RPAR;
			break;
		case '[':
			tok=T_LBRACKET;
			break;
		case ']':
			tok=T_RBRACKET;
			break;
		case '{':
			tok=T_LBRACE;
			break;
		case '}':
			tok=T_RBRACE;
			break;
		case ':':
			tok=T_COL;
			break;
		case ';':
			tok=T_SEMICOL;
			break;
		case ',':
			tok=T_COMMA;
			break;
		case '#':
			if (*in=='#') {
				in--; *in=0;
				tok=T_EOS;
				break;
			}
			tok=T_HASH;
			break;
		case 0:
		case '\n':
			tok=T_EOS;
			in--;
			break;
		case '\"':
			tok=T_DQUOTE;
			break;
		case '=':
			if (*in++=='=') {
				tok=T_EQ;
			} else {
				tok=T_ASSIGN;
				in--;
			}
			break;
		case '!':
			if (*in++=='=') {
				tok=T_NE;
			} else {
				tok=T_NOT;
				in--;
			}
			break;
		case '>':
			if (*in++=='=') {
				tok=T_GE;
			} else {
				tok=T_GT;
				in--;
			}
			break;
		case '<':
			if (*in++=='=') {
				tok=T_LE;
			} else {
				tok=T_LT;
				in--;
			}
			break;
		case '&':
			if (*in++=='&') {
				tok=T_AND;
			} else {
				in-=2;
			}
			break;
		case '|':
			if (*in++=='|') {
				tok=T_OR;
			} else {
				tok=T_HCONCAT;
				in--;
			}
			break;
		case '_':
			tok=T_VCONCAT;
			break;
		case '~':
			if (*in++=='=') {
				tok=T_ABOUTEQ;
			} else {
				in-=2;
			}
			break;
		case '\'':
			tok=T_TRANSPOSE;
			break;
		}
	}

scan_end:
	cc->next=in;
	return tok;
}

token_t scan_path(Calc *cc)
{
	token_t tok=T_NONE;
	int len=0;
	header *hd=(header*)cc->newram;
	char* d=(char*)stack_alloc(cc,s_string,0,"");	/* destination */
	char *next = cc->next, *s,*e;	/* s beginning of the string, e end of string */
	char c;

	cc->result=NULL;
	while ((c=*next)==' ' || c=='\t') next++;
	e=s=next;
	if (c==0) {
		goto end;
	} else if (c==';' || c==',') {
		next++;
		goto end;
	} else if (!( (c>='A' && c <='Z') || (c>='a' && c<='z') 
	       || c=='.' || c=='/' || c=='_') ) goto err;
	       
	while (1) {
		if ((c=*next++)==0 || c=='\n') {
			if (e==s) e=next-1;
			next--;
			tok=T_EOS; break;
		} else if (c==' ' || c=='\t') {
			e=next-1;
			while ((c=*next)==' ' || c=='\t') next++;
		} else if (c==',') {
			if (e==s) e=next-1;
			tok=T_COMMA; break;
		} else if (c==';') {
			if (e==s) e=next-1;
			tok=T_SEMICOL; break;
		} else if (c=='"' || c=='\'') {
			goto err;
		} else if ((c & 0x80)==0) {
		} else if ((c & 0xE0)==0xC0 && ((*next & 0xC0)==0x80)) {
			next++;
		} else if ((c & 0xF0)==0xE0) {
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
end:
	len=(int)(e-s);
	if (d+len<=cc->udfstart) {			/* adjust object size */
		hd->size+=ALIGN(len+1);
	} else cc_error(cc,"Stack overflow!");
	memcpy(d,s,len);
	d[len]=0;
	cc->newram=(char*)hd+hd->size;
	cc->next=next;
	cc->result=hd;
	return tok;
	
err:
	cc->newram=(char*)hd;
	cc->next=next;
	cc_error(cc,"Invalid path!");
	return T_NONE;
}

#if 0
/* test scan function */
int parse(Calc *cc)
{
	token_t tok;
		
	if (setjmp(cc->err)) {
		return NULL;		/* error */
	}
	
	while ((tok=scan(cc))!=T_EOS) {
		switch(tok) {
		case T_ADD: printf("T_ADD\n"); break;
		case T_SUB: printf("T_SUB\n"); break;
		case T_MUL: printf("T_MUL\n"); break;
		case T_DIV: printf("T_DIV\n"); break;
		case T_POW: printf("T_POW\n"); break;
		
		case T_EQ: printf("T_EQ\n"); break;
		case T_NE: printf("T_NE\n"); break;
		case T_GT: printf("T_GT\n"); break;
		case T_GE: printf("T_GE\n"); break;
		case T_LT: printf("T_LT\n"); break;
		case T_LE: printf("T_LE\n"); break;
		case T_AND: printf("T_AND\n"); break;
		case T_OR: printf("T_OR\n"); break;
		
		case T_HCONCAT: printf("T_HCONCAT\n"); break;
		case T_VCONCAT: printf("T_VCONCAT\n"); break;
		
		case T_LPAR: printf("T_LPAR\n"); break;
		case T_RPAR: printf("T_RPAR\n"); break;
		case T_LBRACKET: printf("T_LBRACKET\n"); break;
		case T_RBRACKET: printf("T_RBRACKET\n"); break;
		case T_LBRACE: printf("T_LBRACE\n"); break;
		case T_RBRACE: printf("T_RBRACE\n"); break;
		
		case T_REAL: printf("T_REAL (%g)\n", cc->val); break;
		case T_IMAG: printf("T_IMAG (%gi)\n", cc->val); break;
		case T_LABEL: printf("T_LABEL (%s)\n", cc->str); break;
		case T_NONE:
		default:
			printf("unknown token\n");
			break;
		}
	}
	return NULL;
}

int main()
{
	char input[80];
	Calc _calc;
	Calc *calc=&_calc;

	for (;;) {
		printf("> ");
		if (fgets(input, sizeof(input), stdin) == NULL) {
			return EXIT_SUCCESS;
		}
		
		/* appel de la fonction parse() */
		
		calc->line = calc->next = input;
		parse(calc);
	}
	
	return EXIT_SUCCESS;
}
#endif
