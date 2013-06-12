#ifndef OCLIB_H
#define OCLIB_H

#include <stdlib.h>
#include <stdio.h>				  /* printf() */
#include <math.h>					  /* log10() */
#include <string.h>				  /* strncat(), strncpy() */
#include <sys/types.h>			  /* stat() */
#include <sys/stat.h>			  /* stat() */
#include <sys/time.h>			  /* setitimer */
#include <assert.h>

#define STDIO
#define NORMAL
#define WIRED
/* #define PDF_TEST */
/* #define CALC_DEG_TEST */
/* #define DEGREE_TEST */
/* #define PRINT_RECOVERED */
/* #define DCD_TEST  */
/* #define ECD_TEST  */
/* #define RECOVER_AB */
/* #define CBL_TEST */
/* #define LIST_TEST */

#define FILE_MAX_SIZE 0xFFFFFFFF	 /* 2^32/1024 == 4194304 KB == 4194 MB == 4.2 GB */
/* OC algorithm recommended parameters */
/*#define F 2114*/			/* algorithm parameter */
#define EPS 0.01	/* epsilon - algorithm parameter */
#define Q 3			/* q */
#define ABCONST 0.55*Q*EPS		/* used to calculate the number of ABs */

#define F ((log10f(EPS*EPS/4))/log10f(1-EPS/2))

/* the average degree of an AB is calculated as nomb*(Q/nab) => thus the max degree for a CB is [F + Q*(nomb*(Q/nab))] */
#define OC_MAX_DEG 2114/*should be 2*F*//*30394*/ /*4787 -> 0.05*/ /*2396 -> 0.09*//*2114 -> 0.01*//*30394 -> 0.001*/
#define OC_BLOCK_LEN 100/*30*//*30*//*512*//*512*//*30*/

#define ERROR(s) fprintf (stderr, "\nError[%s,%d]:%s() %s\n",__FILE__,__LINE__,__FUNCTION__,s);
#define INFO(s) fprintf (stderr, "\nInfo[%s,%d]:%s() %s\n",__FILE__,__LINE__,__FUNCTION__,s);
#define WARN(s) fprintf (stderr, "\nWarning[%s,%d]:%s() %s\n",__FILE__,__LINE__,__FUNCTION__,s);
#define CHECK(s,n) fprintf (stderr, "\nCheck[%s,%d]:%s() %s=%d\n",__FILE__,__LINE__,__FUNCTION__,s,n);
/* next stolen from Tor, util.h, vpv-05feb */
#define ASSERT(expr)																		\
  do																							\
    {																							\
      if (!expr)  /* if 'expr' is 0 */												\
        {																					\
          fprintf (stderr,																\
                   "Error[%s:%d]%s() - Assertion (%s) failed; aborting. \n", \
                   __FILE__, __LINE__, __FUNCTION__, #expr);				\
          abort ();																		\
        }																					\
    }																							\
  while (0)

/* 
 * types
 */
typedef unsigned int u32_t;
typedef int i32_t;
typedef unsigned short u16_t;
typedef short i16_t;
typedef unsigned char u8_t;
typedef char i8_t;

/* 
 * OC message block 
 *	  "A block is a fixed-length bit-string, which represents the
 *		smallest logical unit of information" 
 *			 --"Online Codes" paper, Petar Maymounkov
 */
typedef u8_t oc_block_t[OC_BLOCK_LEN];

/* AB */
typedef struct {
  i32_t d;							 /* degree */
  i32_t id;						  /* seed for the prng from which 'd' random adjacent CBs are generated */
} ab_t;

/* CB */
typedef struct {
  i32_t d;							 /* degree */
  i32_t id;						  /* seed for the prng from which 'd' random adjacent CBs are generated */
  i32_t adj[OC_MAX_DEG];		/* index list of adjacent CMB-s - filled in the decoder */
  oc_block_t block;		/* check block data	*/
} cb_t;

/* list to store incoming CBs */
typedef struct cb_list {
  cb_t cb;
  struct cb_list *next;
} cb_list_t;

typedef enum { NO, YES } bool_t;

/* composite message block OMB */
typedef struct {
  bool_t isrec;				  /* is recovered */
  oc_block_t block;		/* check block data	*/
} cmb_t;

/* composite message */
struct cmb_t *cm;

//i32_t ncb=0;							  /* total number of CBs */

#define RCV_FRAC 10				  /* receive fraction - the dcd recv buffer to be of length this fraction of OMBs  */
//i32_t g_nrcv=0; 				  /* counter of received CBs */
//i32_t nrec = 0;
//i32_t ntotal=0;
extern i32_t g_nrcv;
extern i32_t nrec;
extern i32_t ntotal;




/* om - original message; nomb - number of auxiliary message blocks; am - auxiliary message; nab - number of auxiliary blocks; abid - seed for prng before it starts to generate ABs */
void calc_cm(i8_t *buf, i32_t len, oc_block_t **om, i32_t *nomb, /*ab_t*/cb_t **am, i32_t *nab, i32_t *abid);

/* calculate the OC probability distribution; store it in pd; pd
 * is allocated by the caller; nprob is the number of elements in the
 * pdist arrey; probability distribution function - accumulation of 
 * probabilities; probabilities are ordered like this: 
 *
 * pd[0]=0, pd[1]=p1, ..., pd[i]=pi, ..., pd[F]=pF 
 *
 */
void calc_pd (float *pd, i32_t nprob);

/* calculate a CB degree according to probability distribution 'pd' */
/* 
 * find to which PD value the generated random probability 
 * belongs - the index i of the found value is our 
 * probabilistically generated random number - we do it like
 * this: while the generated random probability pr is above
 * the value pd[i] of the PD function, keep crawling upwards
 * on the PD curve (ie. increase index i) ; once pr falls bellow
 * pd[i], we associate pr to the discrete value pd[i] and select
 * i as the probabilistically generated random number;
 * condition (i <= F) is a sanity check - normally it should 
 * always be TRUE, vpv
 */
i32_t calc_deg (float *pd, i32_t nprob);

/* om - original message; nomb - number of auxiliary message blocks; am - auxiliary message; nab - number of auxiliary blocks; d - degree of the CB; store the generated CB in *cb */
void ecd (oc_block_t *om, i32_t nomb, /*ab_t*/cb_t *am, i32_t nab, i32_t d, cb_t *cb);

i32_t dcd (cb_t *cb, cb_list_t **cbl, cmb_t *cm, i32_t ncmb, cb_t **am, i32_t nab, i32_t abid);

int get_file_size(char *path, off_t *size);

void cbl_free (cb_list_t **cbl);

#endif  /* #define OCLIB_H */
