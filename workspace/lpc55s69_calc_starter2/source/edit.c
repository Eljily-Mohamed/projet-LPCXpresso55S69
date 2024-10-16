/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * edit.c
 *
 ****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "edit.h"
#include "sysdep.h"
//#include "graphics.h"

char fktext [12][32];

static char history[HIST_MAX][LINEMAX];
static int act_history=0,	/* number of actual entries in history */
           hist=0;			/* current line */

void clear_fktext (void)
{	int i;
	for (i=0; i<12; i++) fktext[i][0]=0;
}

static void cpy (char *dest, char *source)
{	memmove(dest,source,strlen(source)+1);
}

static void push_in_history (char *s)
{	int i;

	/* do not push empty lines */
	char *p=s;
	while (*p==' ' || *p=='\t') p++;
	if (!strlen(p)) return;
	
	/* do not push a line in history, if it is already the last
	   one pushed in */
	if (act_history && strcmp(s,history[act_history-1])==0) return;
	
	/* make room if necessary*/
	if (act_history>=HIST_MAX) {
		for (i=0; i<HIST_MAX-1; i++) strcpy(history[i],history[i+1]);
		act_history=HIST_MAX-1;
		hist--; if (hist<0) hist=0;
	}
	
	/* push the line in history */
	strcpy(history[act_history],s);
	if (hist==act_history) hist++;
	act_history++;
}

static void left (int n)
{	int i;
	for (i=0; i<n; i++) move_cl_cb();
}
/*
static void right (int n)
{	int i;
	for (i=0; i<n; i++) move_cr_cb();
}
*/
static void fkinsert (int i, int *pos, char *s)
{	char *p;
	p=fktext[i];
	if (strlen(s)+strlen(p)>=LINEMAX-2) return;
	cpy(s+*pos+strlen(p),s+*pos); memmove(s+*pos,p,strlen(p));
	sys_print(p); *pos+=strlen(p);
}

static char helpstart[LABEL_LEN_MAX+1];
static char helpextend[16][LABEL_LEN_MAX+1];
static int helpn=0,helpnext=0,helphist=-1;
int extend(Calc *cc, char* start, char extend[16][LABEL_LEN_MAX+1]);

static void edithelp (Calc *cc, char *s, int *pos, int *shorter)
/* extend the command at cursor position */
{	char *start,*end,*p;
	int i,l;
	/* search history */
	l=strlen(s);
	helphist=-1;
	for (i=act_history-1; i>=0; i--)
		if (strncmp(history[i],s,l)==0)
		{	helphist=i;
			break;
		}
	/* not the beginning of a command in history,
	   maybe a command, builtin or user defined function name,
	   so find the extent of the name */
	helpn=0;
	start=end=p=s+(*pos);
	while (start>s && (ISALPHA(*(start-1)) || ISDIGIT(*(start-1))))
		start--;
	while (ISDIGIT(*start)) start++;
	while (ISALPHA(*end) || ISDIGIT(*end)) end++;
	if (start>s+(*pos) || start>=end || (end-start)>LABEL_LEN_MAX) return;
	/* we have an extent */
	while (p<end) { move_cr_cb(); (*pos)++; p++; }
	memmove(helpstart,start,end-start);
	helpstart[end-start]=0;
	helpn=extend(cc,helpstart,helpextend);
	helpnext=0;
}

void output (Calc *cc, char *s);

static void prompt (Calc *cc)
{	if (!CC_ISSET(cc,CC_OUTPUTING)) {
		sys_out_mode(CC_EDIT);
	} else if (!CC_ISSET(cc,CC_PARSE_UDF)) {
		sys_out_mode(CC_EDIT);
	} else {
		sys_out_mode(CC_FEDIT);
	}
}

#ifdef WITHGRAPHICS
void do_show_graphics (Calc *cc);
#endif

scan_t edit (Calc *cc, char *s)
{	int pos, shorter;
	scan_t scan;
	char ch,chs[2]="a",*p=0;
	s[0]=0; pos=0;
	edit_on_cb();
	prompt(cc);
	helpn=0; helphist=-1; //hist=act_history;
	while (1){
//		cursor_on_cb();
		ch=sys_wait_key(&scan);
//		cursor_off_cb();
		if (scan!=help && helpnext<helpn) helpn=0;
		if (scan==enter || scan==eot) break;
		shorter=0;
		switch (scan) {
			case key_normal :	/* standard characters */
				if (isprint(ch) && strlen(s)<LINEMAX-2) {
					cpy(s+pos+1,s+pos);
					s[pos]=ch; pos++;
					chs[0]=ch;
					sys_print(chs);
				}
				break;
			case cursor_left :
				if (pos) { pos--; move_cl_cb(); }; continue;
		    case cursor_right : /* cursor right */
		    	if (s[pos]) { pos++; move_cr_cb(); }; continue;
			case word_right : /* a word to the right */
			{	while (s[pos] && s[pos]!=' ')
				{	pos++; move_cr_cb(); }
				while (s[pos]==' ')
				{	pos++; move_cr_cb(); }
				continue;
			}
			case word_left : /* a word to the right */
			{	if (pos) { pos--; move_cl_cb(); }
				while (pos && s[pos]==' ')
				{	pos--; move_cl_cb(); }
				while (pos && s[pos]!=' ')
				{	pos--; move_cl_cb(); }
				if (pos) { pos++; move_cr_cb(); }
				continue;
			}
		    case backspace :
		    	if (pos) {
		    		pos--;
		    		cpy(s+pos,s+pos+1); 
		    		move_cl_cb(); shorter=1;
		    	}
		    	break;
		    case delete :
		    	if (s[pos]) {
		    		cpy(s+pos,s+pos+1); shorter=1;
		    	}
		    	break;
		    case cursor_up :
		    	if (hist) {
		    		hist--; strcpy(s,history[hist]); shorter=1;
		    	}
		    	left(pos); pos=0;
		    	break;
		    case cursor_down :
		    	hist++;
		    	if (hist<act_history) {
		    		strcpy(s,history[hist]); shorter=1;
		    	} else {
		    		hist=act_history; s[0]=0; shorter=1;
		    	}
		    	left(pos); pos=0;
		    	break;
		    case clear_home :
		    case escape :
		    	left(pos); pos=0; s[0]=0;
		    	shorter=1; hist=act_history;
		    	break;
		    case line_end :
		    	while (s[pos]) { pos++; move_cr_cb(); }
		    	continue;
		    case line_start :
		    	left(pos); pos=0;
		    	continue;
		    case page_up:
		    	page_up_cb();
		    	continue;
		    case page_down:
		    	page_down_cb();
		    	continue;
		    case switch_screen :
#ifdef WITHGRAPHICS
				edit_off_cb(); do_show_graphics(cc); edit_on_cb();
#endif
		    	break;
		    case help :
		    	if (helpnext>=helpn && helphist<0) {
		    		edithelp(cc,s,&pos,&shorter);
		    		p=0;
		    	}
		    	if (helphist>=0) {	/* completion found in history */
		    		strcpy(s,history[helphist]);
		    		hist=helphist;
		    		p=s+pos;
		    		sys_print(p);
		    		pos+=strlen(p);
		    		shorter=1;
		    		helphist=-1;
		    	} else if (helpnext<helpn) {	/* completion is a builtin/command/udf */
		    		if (p) {					/* erase preceding extension (not the first time) */
		    			left(strlen(p));
		    			pos-=strlen(p);
		    			cpy(s+pos,s+pos+strlen(p));
		    		}
		    		p=helpextend[helpnext++];
		    		if (strlen(s)+strlen(p)>=LINEMAX-2) {
		    			p=0; helpn=0; break;
		    		}
					cpy(s+pos+strlen(p),s+pos);	/* get the following extension in the list */
					memmove(s+pos,p,strlen(p));
					sys_print(p);
					pos+=strlen(p);
					shorter=1;
		    	}
		    	break;
		    case fk1 : fkinsert(0,&pos,s); break;
		    case fk2 : fkinsert(1,&pos,s); break;
		    case fk3 : fkinsert(2,&pos,s); break;
		    case fk4 : fkinsert(3,&pos,s); break;
		    case fk5 : fkinsert(4,&pos,s); break;
		    case fk6 : fkinsert(5,&pos,s); break;
		    case fk7 : fkinsert(6,&pos,s); break;
		    case fk8 : fkinsert(7,&pos,s); break;
		    case fk9 : fkinsert(8,&pos,s); break;
		    case fk10 : fkinsert(9,&pos,s); break;
		    case fk11 : fkinsert(10,&pos,s); break;
		    case fk12 : fkinsert(11,&pos,s); break;
		    default:
		    	continue;
		}
		sys_print(s+pos);
		if (shorter) clear_eol();
		left(strlen(s+pos));
	}
	push_in_history(s);
	if (cc->outfile) {
		fprintf(cc->outfile,"%s\n",s);
	}
	edit_off_cb();
	sys_print("\n");
	return scan;
}

void next_line (Calc *cc)
/**** next_line
	read a line from keyboard or file.
 ****/
{
	if (CC_ISSET(cc,CC_EXEC_UDF)) {	/* executing a udf */
		while (*cc->next==0) cc->next++;
		cc->line=cc->next;
		if (cc->trace>0) trace_udfline(cc,cc->next);
	} else {
		if (cc->trace==-1) cc->trace=1;		/* ?? */
		if (CC_ISSET(cc,CC_EXEC_STRING)) {
			/* interpreting a single string. So, no nextline! */
			cc_error(cc,"Input ended in string!\n");
		}
		if (!cc->infile) {		/* get input from keyboard*/
			scan_t r=edit(cc,cc->line);
//				fprintf(stderr,"NEW LINE!\n");
			if (r==eot) {
//				fprintf(stderr,"EOT!\n");
				strcpy(cc->line,"quit");
			}
		} else {				/* get input from infile */
			int count=0,input;
			char *p=cc->line;
			cc->linenb++;
			while(1) {
				input=fgetc(cc->infile);
				if (input==EOF) {
					/* treat file end as an exception */
					*p=0;
					if (CC_ISSET(cc,CC_PARSE_UDF)) cc_error(cc,"End of file reached in function definition!\n");
					longjmp(*cc->env,4);
				}
				if (input=='\n') break;
				if (count>=LINEMAX-2) {
					 *cc->line=0; cc_error(cc,"Line too long!\n");
				}
//				if ((char)input>=' ' || (signed char)input<0 || (char)input=='\t') {
					*p++=(char)input; count++;
//				}
			}
			*p=0;
		}
		cc->next=cc->line;
	}
}
