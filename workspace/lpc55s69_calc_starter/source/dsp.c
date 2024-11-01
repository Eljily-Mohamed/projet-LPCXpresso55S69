#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arm_math.h"
#include <stdlib.h>

#include "fsl_powerquad.h"
#include "fsl_i2c.h"

#include "dsp.h"
#include "funcs.h"
#include "sysdep_pcm.h"


/****************************************************************
 *	filter(b,a,x,yini)
 ****************************************************************/
header* mfilter (Calc *cc, header *hd)
// filter as in Matlab (almost)
{
	header *hdb=hd, *hda, *hdx, *hdy, *result;
	real *ma,*mb,*mx,*my;
	int ra,ca,rb,cb,rx,cx,ry,cy;

	hda=next_param(cc,hdb); hdx=next_param(cc,hda);
	hdy=next_param(cc,hdx);
	hdb=getvalue(cc,hdb); hda=getvalue(cc,hda);
	hdx=getvalue(cc,hdx); hdy=getvalue(cc,hdy);
	getmatrix(hdb,&rb,&cb,&mb); getmatrix(hda,&ra,&ca,&ma);
	getmatrix(hdx,&rx,&cx,&mx); getmatrix(hdy,&ry,&cy,&my);

	if (ra!=1 || rb!=1 || rx!=1 || ry!=1) cc_error(cc,"Filter can only work with row vectors");
	if (cx<=cb) cc_error(cc,"data must be longer then filter");
	if (cy<ca-1) cc_error(cc,"not enough start values for feedback filter.\n");
	if (ca<1 || cb<1) cc_error(cc,"filter coefficients must not be empty");

	if (isreal(hda) && isreal(hdb) && isreal(hdx) && isreal(hdy)) {
		if (ma[0]==0) cc_error(cc,"first denominator coeff must be non-zero");

		result=new_matrix(cc,1,cx-cb+1,"");
		real *mres=matrixof(result);
		for (int i=0; i<cx-cb+1; i++) {
			real s=0.0;
			for (int j=0; j<cb; j++) s+=mb[j]*mx[cb-1+i-j];
			for (int j=1; j<ca; j++) {
				if (i-j>=0) {
					s-=mres[i-j]*ma[j];
				} else {
					s-=my[i-j+cy]*ma[j];
				}
			}
			s=s/ma[0];
			mres[i]=s;
		}
	} else if (isrealorcplx(hda) && isrealorcplx(hdb) &&
		isrealorcplx(hdx) && isrealorcplx(hdy)) {
		real r=ma[0]*ma[0]+ma[1]*ma[1];
		if (r==0) cc_error(cc,"first denominator coeff must be non-zero");
		result=new_cmatrix(cc,1,cx-cb+1,"");
/*		real *mres=matrixof(result);
		for (int i=0; i<cx-cb+1; i++) {
			real sre=0,sim=0;
			for (int j=0; j<cb; j++) {
				sre+=getvre(hdb,j)*getvre(hdx,cb-1+i-j)-getvim(hdb,j)*getvim(hdx,cb-1+i-j);
				sim+=getvim(hdb,j)*getvre(hdx,cb-1+i-j)+getvre(hdb,j)*getvim(hdx,cb-1+i-j);
			}
			for (int j=1; j<ca; j++) {
				if (i-j>=0) {
					sre-=getvre(result,i-j)*getvre(hda,j)-getvim(result,i-j)*getvim(hda,j);
					sim-=getvim(result,i-j)*getvre(hda,j)+getvre(result,i-j)*getvim(hda,j);
				} else {
					sre-=getvre(hdy,i-j+cy)*getvre(hda,j)-getvim(hdy,i-j+cy)*getvim(hda,j);
					sim-=getvim(hdy,i-j+cy)*getvre(hda,j)+getvre(hdy,i-j+cy)*getvim(hda,j);
				}
			}
			sre=(sre*getvre(hda,0)+sim*getvim(hda,0))/r;
			sim=(sim*getvre(hda,0)-sre*getvim(hda,0))/r;
			mres[2*i]=sre; mres[2*i+1]=sim;
		}
*/
	} else cc_error(cc,"bad parameters in filter(b,a,x,yini)");

	return moveresult(cc,hd,result);
}

/****************************************************************
 *	FFT
 ****************************************************************/
int nn;
cplx *ff,*zz,*fh;

static void rfft (LONG m0, LONG p0, LONG q0, LONG n)
/***** rfft
	make a fft on x[m],x[m+q0],...,x[m+(p0-1)*q0] (p points).
	one has xi_p0 = xi_n^n = zz[n] ; i.e., p0*n=nn.
*****/
{	LONG p,q,m,l;
	LONG mh,ml;
	int found=0;
	cplx sum,h;
	if (p0==1) return;
//	if (test_key()==escape) cc_error("fft interrupted")
	if (p0%2==0) {
		p=p0/2; q=2;
	} else {
		q=3;
		while (q*q<=p0) {
			if (p0%q==0) {
				found=1; break;
			}
			q+=2;
		}
		if (found) p=p0/q;
		else { q=p0; p=1; }
	}
	if (p>1) for (m=0; m<q; m++)
		rfft((m0+m*q0)%nn,p,q*q0,nn/p);
	mh=m0;
	for (l=0; l<p0; l++) {
		ml=l%p;
		c_copy(ff[(m0+ml*q*q0)%nn],sum);
		for (m=1; m<q; m++) {
			c_mul(ff[(m0+(m+ml*q)*q0)%nn],zz[(n*l*m)%nn],h);
			c_add(sum,h,sum);
		}
		sum[0]/=q; sum[1]/=q;
		c_copy(sum,fh[mh]);
		mh+=q0; if (mh>=nn) mh-=nn;
	}
	for (l=0; l<p0; l++) {
		c_copy(fh[m0],ff[m0]);
		m0+=q0; if (m0>=nn) mh-=nn;
	}
}

static void fft (Calc *cc, real *a, int n, int signum)
{	cplx z;
	real h;
	int i;

	if (n==0) return;
	nn=n;

	char *ram=cc->newram;
	ff=(cplx *)a;
	zz=(cplx *)ram;
	ram+=n*sizeof(cplx);
	fh=(cplx *)ram;
	ram+=n*sizeof(cplx);
	if (ram>cc->udfstart)  cc_error(cc,"Memory overflow!");

	/* compute zz[k]=e^{-2*pi*i*k/n}, k=0,...,n-1 */
	h=2*M_PI/n; z[0]=cos(h); z[1]=signum*sin(h);
	zz[0][0]=1; zz[0][1]=0;
	for (i=1; i<n; i++) {
		if (i%16) { zz[i][0]=cos(i*h); zz[i][1]=signum*sin(i*h); }
		else c_mul(zz[i-1],z,zz[i]);
	}
	rfft(0,n,1,1);
	if (signum==-1)
		for (i=0; i<n; i++) {
			ff[i][0]*=n; ff[i][1]*=n;
		}
}

header* mfft (Calc *cc, header *hd)
{	header *st=hd,*result;
	real *m,*mr;
	int r,c;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix)	{
		make_complex(cc,st); hd=st;
	}
	getmatrix(hd,&r,&c,&m);
	if (r!=1) cc_error(cc,"row vector expected");
	result=new_cmatrix(cc,1,c,"");
	mr=matrixof(result);
    memmove((char *)mr,(char *)m,(ULONG)2*c*sizeof(real));
	fft(cc,mr,c,-1);
	return pushresults(cc,result);
}

header* mifft (Calc *cc, header *hd)
{	header *st=hd,*result;
	real *m,*mr;
	int r,c;
	hd=getvalue(cc,hd);
	if (hd->type==s_real || hd->type==s_matrix) {
		make_complex(cc,st); hd=st;
	}
	getmatrix(hd,&r,&c,&m);
	if (r!=1) cc_error(cc,"row vector expected");
	result=new_cmatrix(cc,1,c,"");
	mr=matrixof(result);
    memmove((char *)mr,(char *)m,(ULONG)2*c*sizeof(real));
	fft(cc,mr,c,1);
	return pushresults(cc,result);
}

/* accelerometer */
header* maccel(Calc *cc, header *hd)
{
    // matrix of 1 row and 3 columns for x, y, and z
    int rows = 1;
    int cols = 3;
    header *result = new_matrix(cc, rows, cols, "");
    // Fill the matrix with random values for x, y, and z
    real *m = matrixof(result);
	int32_t data[3];
	mma8652_read_xyz(data);
	m[0] = (real)data[0]/1000.0;
	m[1] = (real)data[1]/1000.0;
	m[2] = (real)data[2]/1000.0;
    return pushresults(cc, result);
}

/* pqcos */

header* mpqcos(Calc* cc, header* hd) {
    header *result;
    int r, c;
    real *m, *mr; // pointers to input and output matrices
    hd = getvalue(cc, hd);

    // Check input type
    if (hd->type != s_real && hd->type != s_matrix) {
        cc_error(cc, "Invalid input type for mpqcos");
        return NULL;
    }

    // Retrieves the input matrix
    getmatrix(hd, &r, &c, &m);


    if (hd->type == s_real) {
        result = new_real(cc, 0, "");
        mr = realof(result);
        PQ_CosF32(&m[0], &mr[0]);
    } else {
        result = new_matrix(cc, r, c, "");
        mr = matrixof(result);
        PQ_VectorCosF32(m, mr, r * c);
    }

    return pushresults(cc, result);
}

// Function that tests the difference in execution time between cos and pqcos
header* bench_cos_pqcos(Calc *cc, header *hd) {
	    header *result;
	    header *pqcos_result;
	    // The array size used to calculate cos and pqcos
	    int size;
	    int r, c; // rows and columns
	    real *m, *mr; // pointers to input and output matrices

	    double tpqcos,tcos,t0,t1;

	    hd = getvalue(cc, hd);

	    // Check input type
	    if (hd->type != s_real) {
	        cc_error(cc, "Invalid input type for mpqcos");
	        return NULL;
	    }

	    getmatrix(hd, &r, &c, &m);
	    size = m[0];

	    result = new_matrix(cc, 1,size, "");
	    m = matrixof(result);
	    for (int i = 0; i < size; i++) {
	    	m[i] = (i/100*3.14);
	    }

	    //pqcos
	    result = new_matrix(cc, 1,size, "");
	    mr = matrixof(result);
	    t0 = sys_clock();
	    PQ_VectorCosF32(m,mr,size);
	    tpqcos = sys_clock() - t0; // Measure time

	    //cos
	    t1 = sys_clock();
	    for (int i = 0; i < size; i++) {
	    	  double result_cod = cos(i+1/i);
	    }
	    tcos = sys_clock() - t1; // Measure time


		// matrix of 1 row and 3 columns for x, y, and z
		int rows = 1;
		int cols = 2;

		result = new_matrix(cc, rows, cols, "");
		mr = matrixof(result);
		mr[0] = tpqcos;
		mr[1] = tcos;
		return pushresults(cc, result);
}


static q15_t *fft_in = NULL;  // dynamically allocated memory
static q15_t *fft_out = NULL; // dynamically allocated memory

header* mpqfft(Calc *cc, header *hd) {
    header *st=hd,*result;
    real *m,*mr;
    int r,c;
    hd=getvalue(cc,hd);

    // Convert to complex if the input is real or matrix
    if (hd->type == s_real || hd->type == s_matrix) {
        make_complex(cc, st); hd = st;
    }

    getmatrix(hd, &r, &c, &m);
    if (r != 1) cc_error(cc, "row vector expected");

    result = new_cmatrix(cc, 1, c, "");
    mr = matrixof(result);

    // Allocate memory for fft_in and fft_out based on the size of input
    fft_in = (q15_t *)malloc(2 * c * sizeof(q15_t));   // Complex data requires 2x space
    fft_out = (q15_t *)malloc(2 * c * sizeof(q15_t));  // Complex data requires 2x space

    if (!fft_in || !fft_out) {
        cc_error(cc, "Memory allocation failed");
        return NULL;
    }

    // Calculates the prescaler based on the input size of the FFT

    int Prescale = (int)round(log2(c));

    // Case 1: fft real
    if (hd->type == s_matrix) {
        for (int i = 0; i < c; i++) {
            fft_in[i] = (int32_t)(m[i] * (1 <<(Prescale - 1)));  // Real values
        }
    }
    //case 2: fft complexe
    else if (hd->type == s_complex || hd->type == s_cmatrix) {
        for (int i = 0; i < c; i++) {
            fft_in[2 * i] = (int32_t)(m[2 * i] * (1<<(Prescale - 1)));      // Real part
            fft_in[2 * i + 1] = (int32_t)(m[2 * i + 1] * (1<<(Prescale - 1))); // Imaginary part
        }
    }

    memset(fft_out, 0, 2 * c * sizeof(q15_t));

    // Initialize the PowerQuad hardware
    PQ_Init(POWERQUAD);

    // Configure PowerQuad for FFT processing
    pq_config_t pq_cfg;
    pq_cfg.inputAFormat = kPQ_16Bit;
    pq_cfg.inputAPrescale = Prescale;
    pq_cfg.inputBFormat = kPQ_16Bit;
    pq_cfg.inputBPrescale = 0;
    pq_cfg.tmpFormat = kPQ_16Bit;
    pq_cfg.tmpPrescale = 0;
    pq_cfg.outputFormat = kPQ_16Bit;
    pq_cfg.outputPrescale = 0;
    pq_cfg.tmpBase = (uint32_t *)0xE0000000;
    pq_cfg.machineFormat = kPQ_32Bit;

    PQ_SetConfig(POWERQUAD, &pq_cfg);

    // Perform FFT based on the matrix type
    if (hd->type == s_matrix) {
        PQ_TransformRFFT(POWERQUAD, c, fft_in, fft_out);
    } else {
        PQ_TransformCFFT(POWERQUAD, c, fft_in, fft_out);
    }
    PQ_WaitDone(POWERQUAD);

    // Copy the FFT results back into the result matrix
    for (int i = 0; i < c; i++) {
        mr[2 * i] = (float)(fft_out[2*i]) / (1<<(Prescale - 1));       // Real part
        mr[2 * i + 1] = (float)(fft_out[2 * i + 1]) / (1<<(Prescale-1)); // Imaginary part
    }

    // Free allocated memory
    free(fft_in);
    free(fft_out);

    return pushresults(cc, result);
}


header* mpqifft(Calc* cc, header* hd) {
	header *st=hd,*result;
	    real *m,*mr;
	    int r,c;
	    hd=getvalue(cc,hd);

	    // Convert to complex if the input is real or matrix
	    if (hd->type == s_real || hd->type == s_matrix) {
	        make_complex(cc, st); hd = st;
	    }

	    getmatrix(hd, &r, &c, &m);
	    if (r != 1) cc_error(cc, "row vector expected");

	    result = new_cmatrix(cc, 1, c, "");
	    mr = matrixof(result);

	    // Allocate memory for fft_in and fft_out based on the size of input
	    fft_in = (q15_t *)malloc(2 * c * sizeof(q15_t));   // Complex data requires 2x space
	    fft_out = (q15_t *)malloc(2 * c * sizeof(q15_t));  // Complex data requires 2x space

	    if (!fft_in || !fft_out) {
	        cc_error(cc, "Memory allocation failed");
	        return NULL;
	    }

	    int Prescale = (int)round(log2(c));

	    // Populate the FFT input array
	    if (hd->type == s_matrix) {
	        for (int i = 0; i < c; i++) {
	            fft_in[i] = (int16_t)(m[i] * (Prescale));  // Real values
	        }
	    } else if (hd->type == s_complex || hd->type == s_cmatrix) {
	        for (int i = 0; i < c; i++) {

	            fft_in[2 * i] = (int16_t)(m[2 * i] * (Prescale)); // Real part
	            fft_in[2 * i + 1] = (int16_t)(m[2 * i + 1] * (Prescale)); // Imaginary part
	        }
	    }

	    memset(fft_out, 0, 2 * c * sizeof(q15_t));

	    // Initialize the PowerQuad hardware
	    PQ_Init(POWERQUAD);

	    // Configure PowerQuad for FFT processing
	    pq_config_t pq_cfg;
	    pq_cfg.inputAFormat = kPQ_16Bit;
	    pq_cfg.inputAPrescale = Prescale;
	    pq_cfg.inputBFormat = kPQ_16Bit;
	    pq_cfg.inputBPrescale = 0;
	    pq_cfg.tmpFormat = kPQ_16Bit;
	    pq_cfg.tmpPrescale = 0;
	    pq_cfg.outputFormat = kPQ_16Bit;
	    pq_cfg.outputPrescale = 0;
	    pq_cfg.tmpBase = (uint32_t *)0xE0000000;
	    pq_cfg.machineFormat = kPQ_32Bit;

	    PQ_SetConfig(POWERQUAD, &pq_cfg);

	    PQ_TransformIFFT(POWERQUAD, c, fft_in, fft_out);
	    PQ_WaitDone(POWERQUAD);

	    // Copy the FFT results back into the result matrix
	    for (int i = 0; i < c; i++) {
	        mr[2 * i] = (fft_out[2*i]/(1<<Prescale)) / (Prescale - 1) ;
	        mr[2 * i + 1] = (fft_out[2 * i + 1]/(1<<Prescale)) / (Prescale - 1);
	    }

	    // Free allocated memory
	    free(fft_in);
	    free(fft_out);

	    return pushresults(cc, result);
}


/****************************************************************
 *	Audio play
 ****************************************************************/
/* pcmfreq: get the CODEC sampling frequency
 *   fs=pcmfreq()
 *****/
header* mpcmfreq0 (Calc *cc, header *hd)
{
	return new_real(cc,pcm_get_freq(),"");
}

/* pcmfreq: set the CODEC sampling frequency, return the updated freq
 *   fs=pcmfreq()
 *****/
header* mpcmfreq (Calc *cc, header *hd)
{
	header *result;
	hd=getvalue(cc, hd);
	if (hd->type!=s_real) cc_error(cc,"real sample freq expected!");
	result=new_real(cc,pcm_set_freq(*realof(hd)),"");
	return pushresults(cc,result);
}

/* pcmvol: set the left and right output CODEC levels
 *   ok=pcmvol([left,right]) left and right output volume levels
 *****/
header* mpcmvol (Calc *cc, header *hd)
{
	header *result;
	real *m;

	hd=getvalue(cc, hd);

	if (hd->type!=s_real && !(hd->type==s_matrix && dimsof(hd)->r==1 && dimsof(hd)->c==2)) cc_error(cc,"bad parameter!");
	if (hd->type==s_real) {
		result=new_matrix(cc,1,2,"");
		m=matrixof(result); m[0]=*realof(hd);m[1]=*realof(hd);
	} else {
		result=hd;
		m=matrixof(hd);
	}
	if (m[0]<0.0 || m[0]>100.0 || m[1]<0.0 || m[1]>100.0) cc_error(cc,"volume is not in the range 0..100");
	result=new_real(cc,(real)pcm_vol(m),"");
	return pushresults(cc,result);
}

/* pcmplay: play samples in [1xn] or [2xn] vector or a file
 *   pcmplay(vector) | pcmplay(filename)
 *****/
header* mpcmplay (Calc *cc, header *hd)
{	header *result;
	real *m;
	int r,c;

	hd=getvalue(cc,hd);
	if (hd->type!=s_matrix && dimsof(hd)->r<1) cc_error(cc,"[1xn] or [2xn] real matrix expected!");
	getmatrix(hd,&r,&c,&m);

	result = new_real(cc,(real)pcm_play(m,r,c),"");

	return pushresults(cc,result);
}

/* mpcmrec: record n samples
 *  [2xn]=pcmrec(n)
 *****/
header* mpcmrec(Calc *cc, header *hd)
{
	header *result;
	hd=getvalue(cc,hd);
	if (hd->type!=s_real) cc_error(cc,"real value expected!");
	result=new_matrix(cc,2,*realof(hd),"");
	if (!pcm_rec(matrixof(result),*realof(hd))) cc_error(cc,"PCM Io error!");

	return pushresults(cc,result);
}

/* pcmloop: sample, modify, output the signal
 *   fs=pcmloop()
 *****/
header* mpcmloop (Calc *cc, header *hd)
{
	return new_real(cc,(real)pcm_loop(),"");
}




