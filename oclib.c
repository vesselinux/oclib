/* Copyright 2007 Vesselin Velichkov, Miroslav Knezevic
 *
 *  This file is part of OC.
 *
 *  OC is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OC.  If not, see <http://www.gnu.org/licenses/>.
 */
/* {implementation of online codes, vpv 20070808, ref "Online Codes" Petar Maymounkov--- */
#ifndef OCLIB_H
#include "oclib.h"
#endif 

/* 
 * signatures:
 *		 OC - online codes
 *		 CB - check block
 *		 CMB - composite message block; composed of original message blocks and auxiliary blocks;
 */

/* AVRT -> (long buffer) -> OC-ECD -> (buffer to CMB-s) -> generate a CB from CMB-s -> AVRT -> ...CB series... -> peer AVRT */


i32_t nrec = 0;
i32_t ntotal=0;
i32_t g_nomb = 0;					  /* number of OMBs - global */

/* qsort (cb->adj, OC_MAX_DEG, sizeof (cb->adj[0]), cmp_index);*/	  /* sort the adjacent-blocks arrey in ascending order - all zeros first */
/* compare two indexes - used in qsort() for inverse sorting ie in descending order - all zeros to the right */
i32_t cmp_index (const void *pi, const void *pj)
{
  i32_t i,j;

  i = *(i32_t *)pi;
  j = *(i32_t *)pj;

  if (i > j)
	 return -1;
  if (i < j)
	 return 1;
  return 0;
}

float prng (float min, float max) 
{
  float r;

  struct timeval *tv;

  if (NULL == (tv = calloc ( 1, sizeof (struct timeval)))) {
	 perror("calloc()");
	 exit (EXIT_FAILURE);
  }

  if ((-1 == gettimeofday(tv, NULL))) {
	 perror("gettimeofday()");
	 exit (EXIT_FAILURE);
  }
  /* printf ("%ld sec %ld usec\n", tv->tv_sec, tv->tv_usec); */
  srand((unsigned)tv->tv_usec); /* seed prng with microseconds */
  r = min + (float) ((float)(max) * (rand() / (RAND_MAX + 1.0)));
  assert ( (r >= min) );
  assert ( (r < max) );
  free (tv);
  return r;
}

/* random integer between min and max */
i32_t nprng (i32_t min, i32_t max) 
{
  i32_t r;
  struct timeval *tv;

  if (NULL == (tv = calloc ( 1, sizeof (struct timeval)))) {
	 perror("calloc()");
	 exit (EXIT_FAILURE);
  }

  if ((-1 == gettimeofday(tv, NULL))) {
	 perror("gettimeofday()");
	 exit (EXIT_FAILURE);
  }
  /* printf ("%ld sec %ld usec\n", tv->tv_sec, tv->tv_usec); */
  srand((unsigned)tv->tv_usec); /* seed prng with microseconds */
  r = min + (rand() % (max - min));
  assert ( (r >= min) );
  assert ( (r < max) );
  free (tv);
  return r;
}

/* syn: xor two buffers byte by byte; store the result in the first buffer */
void xor (u8_t *buf1, u8_t *buf2, u32_t len)
{
  u32_t ibyte;

  assert ((buf1 != NULL));
  assert ((buf2 != NULL));

  ibyte = len;
  while (ibyte--)
	 buf1[ibyte]^=buf2[ibyte];
}


/* add a new CB to the list; return the address to the head of the
 * list; we pass the list as double pointer, so that we are able to
 * modify it
 */
cb_list_t *cbl_add (cb_list_t **cbl, cb_t *cb, i32_t ncmb) 
{
  i32_t i;
  char *rp;						  /* return pointer */
  cb_list_t *new_cb;
  i32_t ideg;

  assert ((cb != NULL));

#ifdef DCD_ADJ_TEST
  for (i=0; i < OC_MAX_DEG; i++) {
	 if (cb->adj[i] >= ncmb) {
		printf ("line #%d adj[%d]=%d ", __LINE__, i, cb->adj[i]);
		exit (EXIT_FAILURE);
	 }
  }
#endif /* #ifdef DCD_ADJ_TEST */

  new_cb = malloc (sizeof (cb_list_t));
  assert (new_cb);
  rp = memcpy (new_cb->cb.block, cb->block, OC_BLOCK_LEN); /* copy CB data */
  assert (rp);
  memset (new_cb->cb.adj, -1, OC_MAX_DEG * sizeof (new_cb->cb.adj[0]));
  ideg=0;
  i =0;
  while (ideg < cb->d) {
	 /* for (i = 0; i < OC_MAX_DEG; i++) { */
	 if (cb->adj[i] != -1) {
		new_cb->cb.adj[i] = cb->adj[i]; /* copy indices of adjacent CMBs */
		assert ( (new_cb->cb.adj[i] < ncmb) );
		assert ( (cb->adj[i] < ncmb) );
		ideg++;
	 }
	 i++;
  }
  new_cb->cb.d = cb->d;	  /* copy the degree */
  assert ( (new_cb->cb.adj[0] < ncmb) );
  new_cb->next = *cbl;
  *cbl = new_cb;			  /* make the new entry the
									* head of the list */
  assert ( ((*cbl)->cb.adj[0] < ncmb) );
  /* printf ("total number of CBs in list [%d]\n", ncb++); */

#ifdef DCD_ADJ_TEST
  for (i=0; i < OC_MAX_DEG; i++) {
	 if ((*cbl)->cb.adj[i] >= ncmb) {
		printf ("line #%d adj[%d]=%d ", __LINE__, i, (*cbl)->cb.adj[i]);
		exit (EXIT_FAILURE);
	 }
  }
#endif /* #ifdef DCD_ADJ_TEST */

  return new_cb;
}

void cbl_free (cb_list_t **cbl)
{
  while (*cbl != NULL) {
	 cb_list_t *head = *cbl;
	 *cbl = (*cbl)->next;
	 free (head);
  }
}

/* print list */
void cbl_print (cb_list_t **cbl)
{
#ifdef LIST_TEST
  cb_list_t *icbl = *cbl;
  printf ("head: ");
  while (icbl != NULL) {
	 printf ("%d->", icbl->cb.d);
	 icbl = icbl->next;
  }
  printf ("tail\n");
#endif /* #ifdef LIST_TEST */
}

/* remove the edge from CMB 'icmb' to one adjacent CB */
bool_t cb_remove_edge (cb_t *cb, i32_t iadj, i32_t icmb, cmb_t *cm, i32_t ncmb)
{
  assert ( cb );
  assert ( (icmb < ncmb) );
  assert ( (cm[icmb].isrec == YES) );

  xor (cb->block, cm[icmb].block, OC_BLOCK_LEN);
  /* remove a recovered CMB from the CB list of adjacent CMBs */
  cb->adj[iadj] = -1; /*0; */
  cb->d--; 
  if (cb->d < 0)
	 printf ("error: cb->d = %d\n", cb->d);
  if (cb->d == 0)
	 printf ("error: cb->d = %d\n", cb->d);
  assert ( (cb->d > 0) );
  return YES;
}

/* check if CMB with index iadj is recovered */
bool_t cbl_isrecovered (u32_t iadj, cmb_t *cm, i32_t ncmb) 
{
  assert ( (iadj < ncmb) );
  return cm[iadj].isrec;
}

/* for a given CB, for each of its adjacent CMBs, check which of them are already recovered (known) and xor the CB with them */
i32_t cb_dcd_with_known (cb_t *cb, cmb_t *cm, i32_t ncmb) 
{
  i32_t icmb=0;					  /* index of a recovered CMB */
  i32_t i;
  i32_t nr=0;					  /* number of removed adjacent nodes */
  i32_t d;						  /* degree */
  i32_t ideg=0;

  d = cb->d;					  /* store the original degree */
  /* for (i = 0; i < d; i++) { */


  /* vpv-20070831 */
  ideg=0;
  i=0;
  while (ideg < d) { 
	 icmb = cb->adj[i];
	 /* printf ("line #%d: i=%d, ideg=%d, d=%d, icmb=%d, cb->adj[i]=%d, ncmb=%d\n", __LINE__,i, ideg,  d, icmb, cb->adj[i], ncmb); */
	 assert ( (icmb >= -1) );
	 assert ( (icmb < ncmb) );
	 if (icmb != -1) {
		if (cm[icmb].isrec == YES) {
		  if (cb->d > 1) {
			 cb_remove_edge ( cb, i, icmb, cm, ncmb);	/* xor and remove from the adjacent list */
			 nr++;
		  }
		}
		ideg++;
	 }
	 assert((i < OC_MAX_DEG));
	 i++;
  }
  assert ( (d == (cb->d + nr)) );
  /* sort the adjacent-blocks arrey in descending order - all zeros last (right)	 */
#if 0
  /* vpv-20070831 */
  if (nr)
	 /* iqsort (cb->adj, 0, d-1); */
	 qsort (cb->adj, d, sizeof (i32_t), cmp_index);
#endif /* #cbl_addif 0 */

  return nr;					  /* return number of removed adjacent nodes */
}

/* go through the whole list, and for each entry - decode with known; if degree is lowered to 1, leave the entry; next call recover_deg1() to remove all nodes of degree 1 */
i32_t cbl_dcd_with_known (cb_list_t **cbl, cmb_t *cm, i32_t ncmb) 
{
  cb_list_t *icbl = *cbl;	/* CB list index */
  i32_t nr=0;
  i32_t ndeg1=0;					/* number of lowered d */

  while (icbl != NULL) {

	 if (icbl->cb.d > 1) {
		nr = cb_dcd_with_known (&(icbl->cb), cm, ncmb);
		if (nr) {				  
		  /* nr = 0; */
#ifdef CBL_TEST
		  INFO ("");
		  printf ("degree lowered to %d\n", icbl->cb.d); 
#endif /* #ifdef CBL_TEST */
		  assert ( (icbl->cb.d >=1) );
		  if (icbl->cb.d == 1)
			 ndeg1++;	/* count the number of degree 1 CBs */
		}
	 }
	 icbl = icbl->next;	  /* move on */
  }
  return ndeg1;
}

/* add a recovered CMB to the CM arrey */
void cm_add_recovered (cb_t *cb, i32_t icmb, cmb_t *cm, i32_t ncmb) 
{
  void *rp;

  assert ( (cb != NULL) );
  assert ( (icmb < ncmb) );
  assert ( (cm[icmb].isrec == NO) );

  /* copy to CM arrey  */
  rp = memcpy (cm[icmb].block, cb->block, OC_BLOCK_LEN);
  assert (rp);
  cm[icmb].isrec = YES; /* set flag to 'recovered' */
  if (icmb < g_nomb) {
	 nrec++;	  /* increase global counter, only if we have recovered an OMB (and not an AB) */
#ifdef NORMAL
	 printf ("received [%d] recovered [%d] out of [%d] omb\r", ntotal, nrec, g_nomb); /* mire-20070917 */
	 /*			  printf ("%s\n", cb.block); */
#else
	 printf ("received [%d] recovered [%d] out of [%d] omb\n", ntotal, nrec, g_nomb);
#endif /* #ifdef NORMAL */
#ifdef PRINT_RECOVERED
	 printf ("OM block #%d recovered\n", icmb);
#endif /* #ifdef PRINT_RECOVERED */
  }
#ifdef PRINT_RECOVERED
  else 
	 printf ("AB block #%d recovered\n", icmb);
#endif /* #ifdef PRINT_RECOVERED */
  /* printf ("block #%d recovered; nrec = %d\n", icmb, nrec); */
}

/* recover and/or remove all CBs of degree 1 */
i32_t cbl_recover_deg1 (cb_list_t **cbl, cmb_t *cm, i32_t ncmb)
{
  i32_t icmb;					  /* index CMB */
  cb_list_t *del;	  /* index CBL */
  bool_t bremoved = NO;
  bool_t brecovered = NO;
  i32_t nr=0;				  /* number of recovered */
  i32_t nret=0;				  /* return value */

  cb_list_t *head = *cbl;

  if ( (head == NULL) )
	 return nr;

  /* {vpv-20070829 */

#ifdef CBL_TEST
  INFO ("");
  printf ("before---> ");
  cbl_print (cbl);
#endif /* #ifdef CBL_TEST */

  /* traverse the list until there are no more newly recovered OMBs  */
  do {
	 /* check first for the head */
	 do
		{
		  brecovered = NO;
		  if ((*cbl)->cb.d != 1)
			 nret = cb_dcd_with_known (&((*cbl)->cb), cm, ncmb);
		  if ((*cbl)->cb.d == 1) {
			 /* vpv-20070831 */
			 i32_t i=0;
			 while ((*cbl)->cb.adj[i] == -1) {
				i++;
			 }
			 icmb = (*cbl)->cb.adj[i];
			 assert ( (icmb < ncmb) );
			 if (cm[icmb].isrec == NO) {/* make sure it is not already recovered */
				cm_add_recovered (&((*cbl)->cb), icmb, cm, ncmb);
				brecovered = YES;
				nr++;		  /* count recovered */
				del = (*cbl);
				(*cbl) = (*cbl)->next; /* short-cut the CB */
				free (del);/* delete the obsolete CB */
				/* INFO ("head recovered"); */
			 }
		  }
		}
	 while (brecovered && (*cbl != NULL));

	 /* now process rest of the list - all after the head */
	 if (*cbl != NULL) {

		head = *cbl;
		brecovered = NO;
	
		/* go through the list and remove/recover CBs of degree 1 */
		while (((head)->next != NULL)) {

		  bremoved = NO;

		  assert ( ((head) != NULL) );
		  assert ( ((head)->next != NULL) );

		  if ((head)->next->cb.d != 1) {
#ifdef DCD_DEG_TEST
			 i32_t i=0;
			 for (i=0; i < OC_MAX_DEG; i++) {
				if ((head)->next->cb.adj[i] >= ncmb) {
				  printf ("adj[%d]=%d ", i, (head)->next->cb.adj[i]);
				  exit (EXIT_FAILURE);
				}
			 }
#endif /* #ifdef DCD_DEG_TEST */
			 nret = cb_dcd_with_known (&((head)->next->cb), cm, ncmb);
			 if (nret) {				  
				assert ( ((head)->next->cb.d >=1) );
			 }
		  }

		  if ((head)->next->cb.d == 1) {
			 /* vpv-20070831 */
			 i32_t i=0;
			 assert (((head)->next->cb.d == 1));
			 while ((head)->next->cb.adj[i] == -1) {
				i++;
			 }
			 icmb = (head)->next->cb.adj[i];
			 assert ( (icmb < ncmb) );

			 if (cm[icmb].isrec == NO) {/* make sure it is not already recovered */
				cm_add_recovered (&((head)->next->cb), icmb, cm, ncmb);
				brecovered = YES;
				nr++;		  /* count recovered */
			 }
			 del = (head)->next;
			 (head)->next = (head)->next->next; /* short-cut the CB */
			 free (del);/* delete the obsolete CB */
			 bremoved = YES;/* set flag */
		  }

		  if (!bremoved)
			 (head) = (head)->next;
		}
	 }
  }
  while (brecovered);
#ifdef CBL_TEST
  INFO ("");
  printf ("after---> ");
  if (*cbl != NULL)
	 cbl_print (cbl);
#endif /* #ifdef CBL_TEST */
  /* CHECK ("nr",nr); */
  return nr;
}
/*while (bremoved || brecovered || ((head)->next->next != NULL));*/

/* if 'bencoding' is FALSE then 'om' can be NULL  */
void calc_am (i32_t abid, cb_t **am, i32_t nab, bool_t bencoding, oc_block_t *om, i32_t nomb)
{
  i32_t iab;					  /* index AB */
  i32_t riab;					  /* random index AB */
  i32_t iomb;					  /* index of OMB */
  i32_t iq;						  /* index of ABs per OMB - from 0 to Q */

  assert (nomb);				  /* number of OMBs cannot be zero */
  assert (nab);					  /* if here, number of ABs cannot be zero */
  if (bencoding)				  /* if encoding om MUST NOT be NULL */
	 assert ((*om));

  /* seed the prng with the AB id */
  srand (abid);

  /* arrey of lists of adjacent OMBs for each AB: abadj[iab][iomb] */
  /* *am = calloc (nab, sizeof (ab_t)); */
  *am = calloc (nab, sizeof (cb_t));
  assert (*am);

  /* init all ABs with zero */
  for (iab = 0; iab < nab; iab++) { /* for each AB */
	 (*am)[iab].d = 0;		  /* inititalize degrees */
#ifdef EQUAL_DEG
	 (*am)[iab].d = (i32_t)((nomb) * Q/(nab));
#endif /* #ifdef EQUAL_DEG */
  }

#ifndef EQUAL_DEG
  /* printf ("calculating the degrees of %d auxiliary blocks...\n", nab); */ /* mire-20070917 */
  /* calculate AB degrees */
  for (iomb = 0; iomb < nomb; iomb++)/* for each OMB */
	 for (iq = 0; iq < Q; iq++) { /* each CMB chooses up to Q ABs */

		/* generate a Q random AB indexes - number between nomb and ncmb */
		/* generate pseudo random AB index between 0 and (nab) */
		riab = 0 + rand () % nab;
		assert ((riab >= 0));
		assert ((riab < nab));

		/* printf ("%d ", riab); */

		/* this shouldn't happen, dammit!  */
		if (((*am)[riab].d) >= OC_MAX_DEG) {
		  printf ("%d : d=%d, maxd=%d\n", __LINE__, (*am)[riab].d, OC_MAX_DEG);
		  exit (EXIT_FAILURE);
		}
		/* silent check - same as above */
		assert ((((*am)[riab].d) < OC_MAX_DEG));


		if (bencoding) { /* if encoding, we know the OM, so just do the xor */
		  i32_t ndeg = (*am)[riab].d;
		  (*am)[riab].adj[ndeg] = iomb; /* store the id of the adjacent OMB */
		  /* exor the AB with the OMB which chose it */
		  xor ((*am)[riab].block, om[iomb], OC_BLOCK_LEN);
		}
		else {	 /* if decoding, OM is unknown, so just save the neighbours */
		  i32_t ndeg = (*am)[riab].d;
		  (*am)[riab].adj[ndeg] = iomb; /* store the id of the adjacent OMB */
		}
		/* increase the degree of the AB */
		(*am)[riab].d++;
	 }
#endif /* #ifndef EQUAL_DEG */
}

/* go through the arrey of ABs; for each AB check which of the adjacent OMBs are already recovered; if an adjacent OMB is recovered, xor the AB with it, and remove it from the list of adjacent */
i32_t recover_ab (cb_t *am, i32_t nab, cmb_t *cm, i32_t ncmb)
{
  i32_t icmb=0;
  i32_t iomb=0;
  i32_t iab=0;
  i32_t iadj=0;
  i32_t nomb=0;
  i32_t ndeg=0;
  i32_t nret=0;
  bool_t bneedsort=NO;
  bool_t brecovered=NO;

  nomb = ncmb - nab;
  /* do  */
  {
	 brecovered=NO;
	 for (iab = 0; iab < nab; iab++) {

		icmb = nomb + iab;/* ABs in the arrey go from 0 to nab; in the CMB they are from nomb to nomb+nab */
		/* sanity checks */
		assert ((icmb < ncmb));
		assert ((icmb >= nomb));
		assert ((am[iab].d >= 0)); /* ! can be zero, when the OMB is recovered ! */
		assert ((am[iab].d <= OC_MAX_DEG));

		if (am[iab].d > 1) {
		  i32_t ideg=0; /* vpv-20070831 */
		  iadj = 0;
		  /*else {*/			  /* CMB not recovered and CM degreenot 1, then go through the adjacent list and see which are recovered */
		  ndeg = am[iab].d;	/* save the original degree */
		  /* for (iadj = 0; iadj < ndeg; iadj++) { */
		  while (ideg < ndeg) {
			 iomb = am[iab].adj[iadj];	
			 assert ((iomb < nomb));
			 if (iomb != -1) {
				assert ((iomb >= 0));
				if (cm[iomb].isrec == YES) {
				  if (am[iab].d > 1) {	/* if degree is 1 - leave it for later */
					 /* if some of the adjacent OMBs for the AB is recovered, xor the AB with it, remove it from the arrey (zero the entry), resort the arrey, lower the degree of the AB */
					 /* INFO ("xor AB with known OMB"); */
					 xor (am[iab].block, cm[iomb].block, OC_BLOCK_LEN);
					 am[iab].adj[iadj] = -1; /*0;*/
					 am[iab].d--;
					 bneedsort=YES;
#ifdef RECOVER_AB
					 printf ("line#[%d] deree of AB#[%d] lowered to [%d]\n", __LINE__, icmb, am[iab].d);
#endif /* #ifdef RECOVER_AB */
				  }
				}
				ideg++;
			 }
			 iadj++;
		  }
#if 0
		  if(bneedsort) {
			 qsort (am[iab].adj, ndeg, sizeof (i32_t), cmp_index);
			 bneedsort=NO;
		  }
#endif /* #if 0 */
		}
#if 0
	 }
	 for (iab = 0; iab < nab; iab++) {
		icmb = nomb + iab;/* ABs in the arrey go from 0 to nab; in the CMB they are from nomb to nomb+nab */
		/* sanity checks */
		assert ((icmb < ncmb));
		assert ((icmb >= nomb));
		assert ((am[iab].d >= 0)); /* ! can be zero, when the OMB is recovered ! */
		assert ((am[iab].d <= OC_MAX_DEG));
		/* if the AB is recovered, copy it to the AB list */

#endif /* #if 0 */

		/*if (cm[icmb].isrec == NO)*/ 
		/* if degree is 1, and not yet recovered - recover it */
		if (am[iab].d == 1) {
		  i32_t i=0;
		  while (am[iab].adj[i] == -1)
			 i++;
		  iomb = am[iab].adj[i];
		  assert ((iomb >= 0));
		  assert ((iomb < nomb));
		  if (cm[icmb].isrec == YES) {	/* if the AB is recovered - here */

			 /* recover the OMB */
			 xor (am[iab].block, cm[icmb].block, OC_BLOCK_LEN);
			 am[iab].d--;	/* lower the degree - now should be zero */
			 assert ((am[iab].d == 0));

			 if (cm[iomb].isrec == NO) { /* if the OMB is not recovered */

				/* copy recovered OMB to CM */
				memcpy (cm[iomb].block, am[iab].block, OC_BLOCK_LEN);
				cm[iomb].isrec = YES; /* set flag to 'recovered' */
				nrec++;/* increase the global counter */
				brecovered=YES;

				nret++; /* increase the return value */
#ifdef RECOVER_AB
				printf ("line#[%d] will recover OMB#[%d] from AB#[%d]; nret=%d\n", __LINE__, iomb, icmb, nret);
#endif /* #ifdef RECOVER_AB */
				/*exit (EXIT_FAILURE);*/ /* just to test */
				/* printf ("line#[%d] : exits...\n", __LINE__); */
				/* return nret; */
			 }
		  }
		}
	 }
  }
  /* while (brecovered); */

  return nret;
}

//#define RCV_FRAC 10				  /* receive fraction - the dcd recv buffer to be of length this fraction of OMBs  */
i32_t g_rcvcnt=0; 					  /* length of receiving queue at the dcd side */
i32_t g_nrcv=0; 				  /* counter of received CBs */

bool_t isamcalculated=NO;
/* 
 * syn : decode one CB 
 * params:
 *		  in:			 cb - check block to decode
 *		  in/out:	 cbl - check block list in which to store received CBs
 *						 cm - composite message
 *						 ncmb - number of CMBs in CM
 *                 abid - seed for prng for 
 *		  return:	 number of recovered CMB-s
 */
i32_t dcd (cb_t *cb, cb_list_t **cbl, cmb_t *cm, i32_t ncmb, cb_t **am, i32_t nab, i32_t abid) 
{
  i32_t nr=0;					  /* number CBs for which the degree was lowerd*/
  i32_t nrab=0;					  /* number ABs for which the degree was lowerd*/
  /*i32_t ndeg1=0;*/				  /* number of CBs of degree 1 */
  i32_t ideg;					  /* degree index */
  i32_t icmb=0;
  i32_t nomb=0;

  nomb = ncmb - nab;

  /* reconstruct ABs based on the abid - done only one time - at the beginning of encoding */
  if (isamcalculated == NO) { /* if AM not calculated already */
	 if (nab) {					  /* if there are any ABs */
		/* printf ("\ndcd(%d) : riab = ", __LINE__); */
		calc_am (abid, am, nab, NO/*bencoding*/, NULL/*om*/, nomb);
		isamcalculated = YES;
	 }
  }

  /* check input parameters */
  assert ( (cb != NULL) );

  if (cb->d > OC_MAX_DEG) {
	 /* printf ("(!) received big degree skipped: cb->d=[%d]\n", cb->d); */
	 /* return nrec; */
	 printf ("(!) dcd() : CB degree [%d] bigger than max [%d]\n", cb->d, OC_MAX_DEG);
	 exit (EXIT_FAILURE);
  }
  /* if CB degree is 0 or bigger than max - exit */
  if (cb->d <= 0)	{
	 ERROR ("invalid CB degree");
	 exit (EXIT_FAILURE);
  }
#ifdef DCD_TEST
  printf ("received CB with id [%d]; degree [%d]\n", cb->id, cb->d);
#endif /* #ifdef DCD_TEST */
  assert ((cb->id >=0 ));

  /* seed prng with the id - in this way rabd() will generate the same sequence of OMB with which the CB was encoded */
  srand (cb->id);

  /* init the adjacent list */
  for (ideg = 0; ideg < OC_MAX_DEG; ideg++) {
	 cb->adj[ideg] = -1;	  /* vpv-20070831 */
  }

#ifdef DCD_TEST
  INFO ("CB adjacent list:");
#endif /* #ifdef DCD_TEST */

  /* for (ideg = 0; ideg < cb->d; ideg++) { */
  ideg = 0;
  while (ideg < cb->d) {
	 icmb = 0 + rand () % ncmb;
	 assert ((icmb < ncmb));
	 if (icmb < nomb) {/* if an OMB */
		cb->adj[ideg] = icmb;
#ifdef DCD_TEST
		/* CHECK("",(cb->adj)[ideg]); */
		printf ("%d ", (cb->adj)[ideg]);
#endif /* #ifdef DCD_TEST */
		ideg++;
	 } 
	 else {
		if ( (icmb >= nomb) && (icmb < ncmb) ) {/* if an AB */
		  cb->adj[ideg] = icmb;
#ifdef DCD_TEST
		  printf ("{%d} ", cb->adj[ideg]);
#endif /* #ifdef DCD_TEST */
		  ideg++;
		}
		else {					  /* normally we shouldn't be here */
		  ERROR ("Invalid random CMB index");
		  exit (EXIT_FAILURE);
		}
	 }
	 assert ((ideg <= OC_MAX_DEG));
  }
  assert ((ideg == cb->d));


#if 1 								  /* ! */
  /* if degree is bigger than 1 try to lower it by decoding with recovered CMBs */
  if (cb->d > 1) {			  
	 nr = cb_dcd_with_known (cb, cm, ncmb);
	 if (nr) {
#ifdef DCD_TEST
		/* INFO (""); */
		printf ("degree lowered to %d\n", cb->d);
#endif /* #ifdef DCD_TEST */
	 }
  }

  /* make sure the degree is still in the valid range */
  assert ( (cb->d >= 1) );

  /* if degree is 1, but it is already recovered, then nothing to do - return */
  if (cb->d == 1) {			 
	 i32_t i=0;
	 while (cb->adj[i] == -1) /* vpv-20070831 */
		i++;
	 if (cm[cb->adj[i]].isrec == YES) {
#ifdef DCD_TEST
		INFO ("degree 1 but already recovered - skipped.");
#endif /* #ifdef DCD_TEST */
		return nrec;	  /* nrec global counter */
	 }
  }

#ifdef DCD_ADJ_TEST
  i32_t i=0;
  for (i=0; i < OC_MAX_DEG; i++) {
	 if (cb->adj[i] >= ncmb) {
		printf ("line #%d adj[%d]=%d ", __LINE__, i, cb->adj[i]);
		exit (EXIT_FAILURE);
	 }
  }
#endif /* #ifdef DCD_ADJ_TEST */
#endif /* #if 0 */

  /* finally, store the CB in the list */
  cbl_add (cbl, cb, ncmb);	
  g_rcvcnt++;					  /* increase rcv counter */

  /* ---!--- */

#ifdef DCD_ADJ_TEST
  for (i=0; i < OC_MAX_DEG; i++) {
	 if (cb->adj[i] >= ncmb) {
		printf ("line #%d adj[%d]=%d ", __LINE__, i, cb->adj[i]);
		exit (EXIT_FAILURE);
	 }
  }
#endif /* #ifdef DCD_ADJ_TEST */

  if (g_rcvcnt >= g_nrcv) {
#ifdef DCD_TEST
	 printf ("line #[%d] : received %d CBs; decoding ...\n", __LINE__, g_rcvcnt);
	 cbl_print (cbl);
#endif /* #ifdef DCD_TEST */

	 /* while new CBs of degree 1 come up (ndeg1!=0) AND new CMBs are recovered (nr!=0), keep decoding */
	 do {
		nrab = recover_ab (*am, nab, cm, ncmb);
		/* ndeg1 = cbl_dcd_with_known (cbl, cm, ncmb); */
#ifdef DCD_ADJ_TEST
		for (i=0; i < OC_MAX_DEG; i++) {
		  if (cb->adj[i] >= ncmb) {
			 printf ("line #%d adj[%d]=%d ", __LINE__, i, cb->adj[i]);
			 exit (EXIT_FAILURE);
		  }
		}
#endif /* #ifdef DCD_ADJ_TEST */
		nr = cbl_recover_deg1 (cbl, cm, ncmb);
		if (nrec >= nomb)
		  return nrec;
#ifdef DCD_TEST
		if (nrab)
		  CHECK ("nrab", nrab);
		CHECK ("nr", nr);
		CHECK ("nrab", nrab);
		/* CHECK ("ndeg1", ndeg1); */
#endif /* #ifdef DCD_TEST */
	 } while (nr || /*ndeg1 ||*/ nrab);
	 /* } while (nr && ndeg1); */
	 g_rcvcnt = 0;				  /* zero the counter */
	 /* if we're above the evaluated limit, make the step by 10 */
	 if (ntotal > nomb/**(1+3*EPS)*/) 
		/* if (abs(ntotal - nomb*(1+3*EPS)) < (i32_t)(0.03*nomb)) */
		g_nrcv = 100;
  }

  return nrec;	 /* global counter of recovered CMBs - TODO: make it local */
}

/* at the encoding side calculate OM from original file; return it in om; om is allocated in this
 * function, but must be freed by the caller; in nomb return the
 * number of allocated OC blocks in om */

/* om - original message; nomb - number of auxiliary message blocks; am - auxiliary message; nab - number of auxiliary blocks; abid - seed for prng before it starts to generate ABs */
void calc_cm(i8_t *buf, i32_t len, oc_block_t **om, i32_t *nomb, /*ab_t*/cb_t **am, i32_t *nab, i32_t *abid)
{
  i32_t cnt=0;			/* counter */
  u8_t *rp;			/* ret pointer */

  assert (buf);

  /* calculate number of OMBs */
  *nomb = len / OC_BLOCK_LEN;
  if (len % OC_BLOCK_LEN) /* if there is trailing data */
	 (*nomb)++;		/* add one more block for trailing data */

  g_nomb = *nomb;					  /* store num OMBs in global variable */

  (*nab) = (float)ABCONST * (*nomb);	/* calclulate number of ABs */

  /* printf ("number ABs = %d, number OMBs = %d, ABCONST=%g ; EPS=%g; Q=%d; max degree = %d\n", *nab, *nomb, ABCONST, EPS, Q, OC_MAX_DEG); */ /* mire-20070917 */

  (*om) = calloc (*nomb, sizeof (oc_block_t));
  assert (*om);

  cnt = 0;
  /* copy all OMBs, but the last one */
  while (cnt < (*nomb-1)) {
	 rp = memcpy ( (*om)[cnt], buf + cnt*OC_BLOCK_LEN, OC_BLOCK_LEN); /* ! */
	 assert ( rp );
	 cnt++;
  }

  /* copy the last OMB, padded with zeros */
  rp = memcpy ((*om)[cnt], buf + cnt * OC_BLOCK_LEN, len % OC_BLOCK_LEN);
  assert (rp);
  /* the rest of the blocks - from cnt+1 to *ncmb are for ABs */

  /* generate ABs */
  if (*nab) {					  /* if number of necessary ABs is not 0 */
	 /* printf ("calc_cm(%d) : riab = ", __LINE__); */
	 /* generate a uniqe ab seed - the decoder needs this to recalculate the ABs at the decoding side */
	 *abid = nprng (0, RAND_MAX);
	 assert ((*abid >= 0));
	 calc_am (*abid, am, *nab, YES/*bencoding*/, *om, *nomb);
  }
}

/* calculate the OC probability distribution; store it in pd; pd
 * is allocated by the caller; nprob is the number of elements in the
 * pdist arrey; probability distribution function - accumulation of 
 * probabilities; probabilities are ordered like this: 
 *
 * pd[0]=0, pd[1]=p1, ..., pd[i]=pi, ..., pd[F]=pF 
 *
 */
void calc_pd (float *pd, i32_t nprob)
{
  float eps;  
  float f=0;		/* calculate F; pd should contain F+1 el. */
  float x=0;					  /* statistical expectation */
  float s=0;					  /* sum of all probabilities */
  float p1;						  /* first probability value */
  float pi;						  /* probability index */
  int i;							  /* index */

  /* sanity check */
  eps = (float)EPS;
  f = ((log10f(eps*eps/4))/log10f(1-eps/2));

#ifdef PDF_TEST
  printf ("f = %3.10f test : %s\n", f, ((i32_t)f == OC_MAX_DEG) ? "ok" : "fail");
#endif /* #ifdef PDF_TEST */
  /* assert ( ((i32_t)f == F) ); */
  assert ( (nprob == (i32_t)f+1) );

  /* printf ("Calculating OC probability distribution with parameters: F=%3.10g; eps=%g; OC block size=%dB\n", f, eps, OC_BLOCK_LEN); */ /* mire-20070917 */

  /* calculate the first probability value */
  p1 = 1-(1+1/f)/(1+eps);

#ifdef PDF_TEST
  printf ("p1 = %3.10f\n", p1);
#endif /* #ifdef PDF_TEST */

  /* init sum, expectation and PD */
  x=1*p1;			/* statistical expectation is calculated as: 1*p1 + 2*p2 + ... + F*pF */
  pd[0]=0;			/* calc the zero-th probability */
  pd[1]=pd[0]+p1;		/* calc the 1-st the probability */
  s=p1;			/* init the sum */

#ifdef PDF_TEST
  printf ("pd[%d]=%3.10f\n", 0, pd[0]);
  printf ("pd[%d]=%3.10f\n", 1, pd[1]);
#endif /* #ifdef PDF_TEST */

  /* calculate the rest of the prob distribution */
  for (i = 2; i <= (i32_t)f; ++i) {

	 pi = ((1-p1)*f) / ((f-1)*i*(i-1)); /* i-th probability */
	 s+=pi;		/* total sum  */
	 x+=i*pi;		/* expectation */
	 pd[i]=pd[i-1]+pi;	/* PDF */

	 /* printf ("p%d=%3.10f\n", (int)i, pi); */
	 /* printf ("pd[%d]=%3.10f\n", i, pd[i]); */
  }
#ifdef PDF_TEST
  /* printf ("\n"); */
  printf ("pd[%d]=%3.10f\n", (i32_t)f, pd[(i32_t)f]);
  printf ("sum=%f\n", s);
  printf ("mean=%f\n", s/(i32_t)f);
#endif /* #ifdef PDF_TEST */
  /* printf ("expectation=%3.10f; sum=%f; mean=%f\n", x, s, s/(i32_t)f); */ /* mire-20070917 */
#ifdef PDF_TEST
  for (i = 0; i <= F; ++i) {
	 printf ("pd[%d]=%3.10f\n", i, pd[i]);
  }
#endif /* #ifdef PDF_TEST */

}

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
i32_t calc_deg (float *pd, i32_t nprob)
{
  i32_t d=0;			/* degree */
  float pr;			/* probability */
  float f, eps;

  eps = (float)EPS;
  f = ((log10f(eps*eps/4))/log10f(1-eps/2));

  assert ( (nprob == (i32_t)f+1) );
 
  pr = prng (0.0, 1.0)/(float)(1.0);/* generate number between 0.0 and 1.0 */
  assert ( (pr >= 0.0) );
  assert ( (pr <= 1.0) );

  /* generated a random degree according to the OC special probability distribution */
  d=0; /* start from index 0 - zero probability */
  while ((pr > pd[d]) && (d <= (i32_t)f))
	 d++;

#ifdef CALC_DEG_TEST
  /* if (d == 1) */
  printf ("degree %d : pr(%f) -> pd[%d]=%f\n", d, pr, d, pd[d]);
#endif /* #ifdef CALC_DEG_TEST */

  return d;
} 

bool_t isrecovered (cmb_t *rcm, i32_t nomb)
{

  i32_t iomb = 0;
  bool_t result = YES;

  /* stop at the first unrecovered block */
  while ( (result == YES) || (iomb < nomb) ) {
	 result = rcm[iomb++].isrec;
  }
  return result;
}
/* OC encoder - generate a CB from CM=OM+AM; store the CB in cb; return the
 * degree of the CB; cb is allocated by the caller */

i32_t g_count=0;					  /* global counter */

/* om - original message; nomb - number of auxiliary message blocks; am - auxiliary message; nab - number of auxiliary blocks; d - degree of the CB; store the generated CB in *cb */
void ecd (oc_block_t *om, i32_t nomb, /*ab_t*/cb_t *am, i32_t nab, i32_t d, cb_t *cb) 
{
  i32_t icmb;		/* index of an CM block */
  i32_t iab;					  /* index of AM block */
  i32_t ncmb;					  /* number of CMBs */
  void *rp;						  /* return pointer for error checking */

  assert (om);
  assert (nomb);
  if (nab)
	 assert (am);
  assert (cb);

  ncmb = nomb + nab;			  /* get length of CM */

  rp = memset (cb->block, 0, OC_BLOCK_LEN);
  assert (rp);

  /* generate unique truly random CB id : 1 out of RAND_MAX=2147483647=0x7FFFFFFF */
  cb->id = nprng (0, RAND_MAX);
  assert ((cb->id >= 0));
#ifdef ECD_TEST
  CHECK ("Starts to encode CB with id [%d]", cb->id);
#endif /* #ifdef ECD_TEST */

  /* seed the prng with the id */
  srand (cb->id);

  /* init the CB degree */
  cb->d = 0;
  /* for (ideg = 0; ideg < d; ideg++) { */
  /* keep choosing CMBs until we reach the desired degree 'd' or until we reach the max allowed (whichever comes first); note: the max degree for a CB is approx. [F + Q*(nomb*(Q/nab))]	*/
  while ((cb->d < d) && (cb->d < OC_MAX_DEG)) {

	 /* generate pseudo random CMB index between 0 and (ncmb) */
	 icmb = 0 + rand () % ncmb;
	 assert ((icmb >= 0));
	 assert ((icmb < ncmb));

	 if (icmb < nomb)	{	  /* if an OMB */
		xor (cb->block, om[icmb], OC_BLOCK_LEN);
		cb->d++;
#ifdef ECD_TEST
		printf ("%d ", icmb);
		/* printf ("xored CB with random OMB index: %d\n", icmb); */
#endif /* #ifdef ECD_TEST */
	 }
	 else {
		if ( (icmb >= nomb) && (icmb < ncmb) ) {/* if an AB */
#ifdef ECD_TEST
		  printf ("{%d} ", icmb);
#endif /* #ifdef ECD_TEST */

		  iab = icmb - nomb; /* get the AB index */
		  assert ((iab >=0));
		  assert ((iab < nab));

		  xor (cb->block, am[iab].block, OC_BLOCK_LEN);	/* xor the CB with the AB */
		  cb->d++;
		  /*d+=am[iab].d;*/ /* (!) just increase the total CB degree 'd' */
		}
		else {					  /* normally we shouldn't be here */
		  ERROR ("Invalid random CMB index");
		  exit (EXIT_FAILURE);
		}
	 }
  }
  assert ((cb->d > 0));
  assert ((cb->d <= OC_MAX_DEG));
#ifdef ECD_TEST
  printf ("cb->d=%d \n", cb->d);
  if (cb->d == OC_MAX_DEG)
	 printf ("(!) calculated CB degree [%d] is too big; degree set to max allowable [%d]\n", d, cb->d);
#endif /* #ifdef ECD_TEST */

#ifdef ECD_TEST_PRNG
  /* seed again the prng with the CB id */
  {
	 i32_t ideg;			/* both are degree index iterators */
	 CHECK("nomb", nomb);
	 CHECK("ncmb", ncmb);
	 INFO ("CB adjacent list is (values in {} are ABs): ");
	 srand (cb->id);
	 for (ideg = 0; ideg < cb->d; ideg++) {
		icmb = 0 + rand () % ncmb;
		assert ( (icmb >= 0) );
		assert ( (icmb < ncmb) );
		if ((icmb >= 0) && (icmb < nomb)) {
		  printf ("%d ", icmb);
		}
		else {
		  if ((icmb >= nomb) && (icmb < ncmb)) {
			 printf ("{%d} ", icmb);
		  }
		  else {
			 ERROR ("rand() generates invalid CMB");
			 printf("rand() generates invalid CMB [%d]; nomb=[%d]; ncmb=[%d]; exiting...\n", icmb, nomb, ncmb);
			 exit (EXIT_FAILURE);
		  }
		}
	 }
	 INFO ("end of adjacent list\n");
  }
#endif /* #ifdef ECD_TEST_PRNG */
}

/* thanks to http://www.steve.org.uk/Reference/Unix/faq_3.html */
int get_file_size(char *path, off_t *size)
{
  struct stat file_stats;

  if(stat(path,&file_stats))
	 return -1;

  *size = file_stats.st_size;
  return 0;
}
