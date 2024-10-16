#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "calc.h"
#include "io.h"

/* mwritematrix
 *	stack: filename matrix flag -- matrix
 *
 *  write a real or complex matrix to filename each matrix column 
 *  on a line (transposed matrix) according to flag:
 *  - 0: store real or complex matrix as text file
 *       format: "CCTX" c r mtype data
 *  - 2: store complex matrix as text file, split real and imag parts
 *       format: "CCTX" c r mtype real and imag data pairs
 *  - 1: store real or complex matrix in binary format
 *       format: "CCBI" c r mtype data
 */
header* mwritematrix (Calc *cc, header *hd)
{
	header *hd1, *hd2, *result=NULL;
	real *m;
	int r, c;
	unsigned int flags, mtype=0;
	size_t len;
	FILE *f;
	
	hd1=next_param(cc,hd); hd2=next_param(cc,hd1);
	hd=getvalue(cc,hd); hd1=getvalue(cc,hd1); hd2=getvalue(cc,hd2);
	if (hd->type!=s_real && hd->type!=s_matrix &&
		hd->type!=s_complex && hd->type!=s_cmatrix && 
		hd1->type!=s_string && hd2->type!=s_real) cc_error(cc,"writematrix(mtx,\"fname\",flag)");
	flags=(unsigned int)*realof(hd2);
	getmatrix(hd,&r,&c,&m);

	if (hd->type==s_real || hd->type==s_matrix) {
		mtype=0;			/* standard real matrix */
	} else if ((hd->type==s_complex || hd->type==s_cmatrix) && (flags & 2)) {
		mtype=2;			/* complex matrix, real and imag parts split in columns */
	} else {
		mtype=1;			/* standard complex matrix */
	}

	if ((flags & 1)==0) {	/* text format */
		f=fopen(stringof(hd1),"w");
		if (f) {
			fprintf(f,"CCTX\t%d\t%d\t%u\n",c,r,mtype);
			switch (mtype) {
			case 0:
				for (int j=0; j<c; j++) {
					for (int i=0; i<r; i++) {
						fprintf(f,"\t%f",*mat(m,c,i,j));
					}
					fprintf(f,"\n");
				}
				break;
			case 1:
				for (int j=0; j<c; j++) {
					for (int i=0; i<r; i++) {
						real *p=cmat(m,c,i,j);
						fprintf(f,"\t%f%+fi",p[0],p[1]);
					}
					fprintf(f,"\n");
				}
				break;
			case 2:
				for (int j=0; j<c; j++) {
					for (int i=0; i<r; i++) {
						real *p=cmat(m,c,i,j);
						fprintf(f,"\t%f\t%f",p[0],p[1]);
					}
					fprintf(f,"\n");
				}
				break;
			default:
				break;			
			}
			fclose(f);
		} else goto err;
	} else {				/* binary format */
		f=fopen(stringof(hd1),"w");
		if (fwrite("CCBI",1,4,f)!=4) goto err_io;
		if (fwrite(&c,4,1,f)!=1) goto err_io;
		if (fwrite(&r,4,1,f)!=1) goto err_io;
		if (hd->type==s_real || hd->type==s_matrix) {
			mtype=0;			/* standard real matrix */
			len=(size_t)r*(size_t)c;
		} else {
			mtype=1;			/* standard complex matrix */
			len=(size_t)r*(size_t)c*2;
		}
		if (fwrite(&mtype,4,1,f)!=1) goto err_io;
		if (fwrite(m,sizeof(real),len,f)!=len) goto err_io;
		fclose(f);
	}
	
	result=new_matrix(cc,1,2,"");
	m=matrixof(result);
	m[0]=r;m[1]=c;

	return pushresults(cc,result);
err:
	cc_error(cc,"could not open file %s",stringof(hd1));
	return NULL;
err_io:
	fclose(f);
	cc_error(cc,"IO error while writing file %s",stringof(hd1));
	return NULL;
}

/* mreadmatrix
 *  stack:  filename -- matrix
 */
header* mreadmatrix (Calc *cc, header *hd)
{
	header *result=NULL;
	char input[LINEMAX],*oldnext;
	token_t tok;
	real *m;
	int r, c, i, j;
	unsigned int mtype=0;
	size_t len;
	FILE *f;
	char buf[4];
	
	hd=getvalue(cc,hd);
	if (hd->type!=s_string) cc_error(cc,"mread(\"filename\")");
	f=fopen(stringof(hd),"r");
	if (f) {
		if (fread(buf,1,4,f)!=4) goto err_io;
//		if(strncmp(buf,"CCTX",4)==0 && fscanf(f,"\t%d\t%d\t%u\n",&c,&r,&mtype)==3) {	/* text format */
		if(strncmp(buf,"CCTX",4)==0) {
			real crm[3];
			oldnext=cc->next;
			if (fgets(input,LINEMAX,f)!=NULL) {
				cc->next=input;
				for (i=0; i<3; i++) {
					tok=scan(cc);
					if (tok==T_REAL) {
						crm[i]=cc->val;
					} else {
						cc->next=oldnext;
						goto err_format;
					}
				}
			} else goto err_format;
			c=(int)crm[0];
			r=(int)crm[1];
			mtype=(unsigned int)crm[2];
			
			switch (mtype) {
			case 0:
				result=new_matrix(cc,r,c,"");
				m=matrixof(result);
				for (j=0; j<c; j++) {
					if (fgets(input,LINEMAX,f)!=NULL) {
						cc->next=input;
						for (i=0; i<r; i++) {
							int uminus=0, uplus=0, done=0;
							while (!done) {
								tok=scan(cc);
								switch (tok) {
								case T_REAL:
									*mat(m,c,i,j)=uminus==0 ? cc->val : -cc->val;
									uminus=0; uplus=0; done=1;
									break;
								case T_SUB:
									if (!uminus) {
										uminus=1;
									} else {
										cc->next=oldnext;
										goto err_format;
									}
									break;
								case T_ADD:
									if (!uplus) {
										uplus=1;
									} else {
										cc->next=oldnext;
										goto err_format;
									}
									break;
								default:
									cc->next=oldnext;
									goto err_format;
									break;
								}
							}
						}
					} else goto err_format;
				}
				break;
			case 1:
				result=new_cmatrix(cc,r,c,"");
				m=matrixof(result);
				for (j=0; j<c; j++) {
					if (fgets(input,LINEMAX,f)!=NULL) {
						cc->next=input;
						for (i=0; i<r; i++) {
							real *p=cmat(m,c,i,j);
							int uminus=0, uplus=0, done=0;
							while (!done) {
								tok=scan(cc);
								switch (tok) {
								case T_REAL:
									*p=uminus==0 ? cc->val : -cc->val;
									uminus=0; uplus=0;
									break;
								case T_IMAG:
									*(p+1)=uminus==0 ? cc->val : -cc->val;
									uminus=0; uplus=0; done=1;
									break;
								case T_SUB:
									if (!uminus) {
										uminus=1;
									} else {
										cc->next=oldnext;
										goto err_format;
									}
									break;
								case T_ADD:
									if (!uplus) {
										uplus=1;
									} else {
										cc->next=oldnext;
										goto err_format;
									}
									break;
								default:
									cc->next=oldnext;
									goto err_format;
									break;
								}
							}
						}
					} else goto err_format;
				}
				break;
			case 2:
				result=new_cmatrix(cc,r,c,"");
				m=matrixof(result);
				for (j=0; j<c; j++) {
					if (fgets(input,LINEMAX,f)!=NULL) {
						cc->next=input;
						for (i=0; i<r; i++) {
							real *p=cmat(m,c,i,j);
							int uminus=0, uplus=0, k=0;
							while (k!=2) {
								tok=scan(cc);
								switch (tok) {
								case T_REAL:
									*(p+k)=uminus==0 ? cc->val : -cc->val;
									uminus=0; uplus=0; k++;
									break;
								case T_SUB:
									if (!uminus) {
										uminus=1;
									} else {
										cc->next=oldnext;
										goto err_format;
									}
									break;
								case T_ADD:
									if (!uplus) {
										uplus=1;
									} else {
										cc->next=oldnext;
										goto err_format;
									}
									break;
								default:
									cc->next=oldnext;
									goto err_format;
									break;
								}
							}
						}
					} else goto err_format;
				}
				break;
			default:
				break;
			}
			cc->next=oldnext;
		} else if (strncmp(buf,"CCBI",4)==0) { /* binary format */
			if (fread(&c,4,1,f)!=1) goto err_io;
			if (fread(&r,4,1,f)!=1) goto err_io;
			if (fread(&mtype,4,1,f)!=1) goto err_io;
			if (mtype==0) {			/* standard real matrix */
				result=new_matrix(cc,r,c,"");
				m=matrixof(result);
				len=(size_t)r*(size_t)c;
			} else if (mtype==1) {	/* standard complex matrix */
				result=new_cmatrix(cc,r,c,"");
				m=matrixof(result);
				len=(size_t)r*(size_t)c*2;
			} else goto err_format;
			if (fread(m,sizeof(real),len,f)!=len) goto err_io;
		} else goto err_format;
		fclose(f);
	} else goto err;
	
	return pushresults(cc,result);
err:
	cc_error(cc,"could not open file %s",stringof(hd));
	return NULL;
err_io:
	fclose(f);
	cc_error(cc,"IO error while reading file %s",stringof(hd));
	return NULL;
err_format:
	fclose(f);
	cc_error(cc,"Bad formatting in data file %s",stringof(hd));
	return NULL;
}

header* mwritewav (Calc *cc, header *hd)
{
	return NULL;
}

header* mreadwav (Calc *cc, header *hd)
{
	return NULL;
}
