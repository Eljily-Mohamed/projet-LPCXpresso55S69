#ifndef _PCM_H_
#define _PCM_H_

#include "sysdep.h"
#include "calc.h"
#include "stack.h"

#include "fsl_wm8904.h"
#include "fsl_sysctl.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"

void codec_set_master(wm8904_handle_t *handle, int master);

real pcm_get_freq(void);
real pcm_set_freq(real fs);

int pcm_init(void);
int pcm_play(real *data, int ch, int n);
int pcm_rec(real *data, int n);
int pcm_loop();

#endif
