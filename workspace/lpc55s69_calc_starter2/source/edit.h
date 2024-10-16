/****************************************************************
 * calc
 *  (C) 1993-2021 R. Grothmann
 *  (C) 2021-2022 E. Bouchare
 *
 * edit.h
 *
 ****************************************************************/
#ifndef EDIT_H
#define EDIT_H

#include "sysdep.h"
#include "calc.h"

#define	HIST_MAX	16		/* Maximum entries in editing history */

extern char fktext[12][32];

scan_t edit (Calc *cc, char *s);
void next_line (Calc *cc);
void clear_fktext (void);

#endif
