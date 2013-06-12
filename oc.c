#ifndef OCLIB_H
#include "oclib.h"
#endif

/*	  char buf [] = "There was nothing so VERY remarkable in that; nor did Alice think it so VERY much out of the way to hear the Rabbit say to itself, `Oh dear!	Oh dear!	 I shall be late!'  (when she thought it over afterwards, it occurred to her that she ought to have wondered at this, but at the time it all seemed quite natural); but when the Rabbit actually TOOK A WATCH OUT OF ITS WAISTCOAT- POCKET, and looked at it, and then hurried on, Alice started to her feet, for it flashed across her mind that she had never before seen a rabbit with either a waistcoat-pocket, or a watch to take out of it, and burning with curiosity, she ran across the field after it, and fortunately was just in time to see it pop down a large rabbit-hole under the hedge.\0";*/

/* for oc, the main() is equivalent to avrt	*/
#if 1
int main (int argc, char **argv)
{
  cb_t cb;
  i32_t ncmb = 0;
  oc_block_t *om;		 /* original message */
  i32_t nomb = 0;
  cb_t /*ab_t **/*am;		 /* auxiliary message */
  i32_t nab=0;					  /* number of ABs */
  i32_t abid;					  /* seed for prng for constructing ABs - calculated by ecd(); sent to dcd() */
  cmb_t *rcm;				  /* recovered CM */
  cb_t *rab;					  /* recovered AM */
  char *rbuf;					  /* read buffer - store data to encode */
  char *wbuf;					  /* write buffer - store decoded data */
  off_t *fsize;				  /* size of file/size of buffers */
  i32_t buflen = 0;			  /* set to buflen */
  cb_list_t *cbl = NULL;
  /* arrey containg the values of the probability distribution function */
  float *pd;		
  i32_t i=0;
  i32_t d;						  /* degree */
  i32_t maxd=0;				  /* max degree */
  float f, eps;
  FILE *fdr;					  /* file read */
  FILE *fdw;
  char *fnr;					  /* file name read */
  char *fnw;					  /* file name write */
  i32_t fnlen;					  /* file name len */
  void *rp;						  /* return pointer */
  char fnappend [] = ".dcd\0";		  /* append this string to the name of the file containg the decoded data */

  float p;

  /* get file name from command-line */
  if (argc != 2) {
	 ERROR ("Bad number of arguments");
	 exit (EXIT_FAILURE);
  }
  assert (argv[argc-1]);

  fnlen = strlen (argv[argc-1]);
  assert (fnlen);
#ifdef FILE_TEST
  CHECK("fnlen",fnlen);
#endif /* #ifdef FILE_TEST */

  /* open file to encode */
  fnr = calloc (fnlen, sizeof (char));
  assert (fnr);

  rp = memcpy (fnr, argv[argc-1], fnlen);
  assert (rp);
  /* printf ("infile='%s'\n", fnr); */  /* mire-20070917 */


  /* get file size */
  fsize = malloc (sizeof (off_t *));
  assert (fsize);

  if (0 < get_file_size(fnr, fsize)) {
	 perror ("get_file_size");
	 exit(EXIT_FAILURE);
  }

  buflen = (int)*fsize;
  /* printf ("file '%s', size %d\n", fnr, buflen); */ /* mire-20070917 */

  /* read file data into buffer */
  rbuf = calloc (buflen, sizeof (char)); 
  assert (rbuf);
  if ( ( buflen > FILE_MAX_SIZE) ) {
	 ERROR ("calloc()");
	 printf ("file is bigger %d Bytes than maximum allowable %d Bytes\n", buflen, FILE_MAX_SIZE);
	 exit(EXIT_FAILURE);
  }

  /* file write */
  fdr = fopen (fnr, "r");	  /* xvpv */
  if (!fdr) {
	 perror ("fopen()");
	 exit(EXIT_FAILURE);
  }
  if (buflen != fread (rbuf, sizeof (char), buflen, fdr)) {
	 perror ("fread()");
	 exit(EXIT_FAILURE);
  }

  /* sanity check */
  eps = (float)EPS;
  f = ((log10f(eps*eps/4))/log10f(1-eps/2));
  pd = malloc ((f+1) * sizeof (float));

  calc_pd (pd, (i32_t)f+1);		/* calculate OC pdf */

  calc_cm(rbuf, buflen, &om, &nomb, &am, &nab, &abid);/* calculate original message + auxiliary message */
  /* calc_cm (rbuf, buflen, &cm, &ncmb, &abadj, &nab, &nabadj);  */
  assert ((om != NULL));
  assert ( (nomb > 0) );
  if (nab) {
	 assert ((am != NULL));
	 assert ( (nab > 0) );
	 assert (abid);
  }
  ncmb = nomb + nab;

  /* set the length of the recv buffer for the decoder to 10% of the length of the OM */
  g_nrcv = 1 + (i32_t) nomb / RCV_FRAC;
     

  /* set the length of the recv buffer for the decoder to the evaluated overhead! */
  /* g_nrcv = (i32_t)(nomb*(1+3*EPS) - nomb); */

  /* printf ("line#[%d] : dcd recv buffer is [%d] blocks\n", __LINE__, g_nrcv);  */ /* mire-20070917 */

  /* allocate memory for the decoded CM */
  rcm = malloc (ncmb * sizeof (cmb_t));
  assert (rcm);
  /* set all flags to "not recovered" */
  for (i = 0; i < ncmb; i++) 
	 rcm[i].isrec = NO;

  /*		 assert ( (ncmb <= OC_MAX_DEG) ); */
  /* for max degree choose either the total number of blocks in the
	* CM message 'ncmb' or theabsolute	max degree OC_MAX_DEG */

  maxd = ((ncmb < OC_MAX_DEG) ? ncmb : OC_MAX_DEG);
  /* maxd = OC_MAX_DEG; */

  /* printf ("original block:\n"); */
  /* printf ("%s\n", buf); */

  /* while (nrec < ncmb) { */
  /*while ((NO == isrecovered (rcm, ncmb - nab))) {*/ /* ! */

  p = rand()/((double)RAND_MAX);
  /* printf("file size =  %dB\n", (int)*fsize); */ /* mire-20070917 */
  /* mire-20070905; initialization - END */

  while (nrec < nomb) {
	 i=2;
#if 0
	 d = 1 + i % maxd;
	 i=0;
	 {			/* calculate degree */
		d = maxd + 1;
		/* printf("calculating CB degree <= %d ...\n", maxd); */
		if (maxd > 1)
		  while (d > maxd) {
			 d = calc_deg (pd, (i32_t)f+1);
			 if (d!=1)
				/*d = d-1*/;			/* !!! */
		  }
		else
		  d = 1;
	 }
#endif /* #if 0 */
	 d = calc_deg (pd, (i32_t)f+1);

#ifdef DEGREE_TEST
	 if (d!=1)
		d = 1;			/* !!! */
#endif /* #ifdef SEQUENTIAL_TEST */

#ifdef PD_TEST
	 printf ("CB degree = %d\n", d);
#endif /* #ifdef PD_TEST */

	 /* ecd (cm, ncmb, &cb, d, abadj, &nab, nabadj); */

	 ecd (om, nomb, am, nab, d, &cb); /* encode a CB of degree d */
	 dcd (&cb, &cbl, rcm, ncmb, &rab, nab, abid); /* decode CB */ 

	 ++ntotal;
	 printf ("sent %d recovered %d out of %d omb\r", ntotal, nrec, nomb);
	 /* printf ("%s\n", cb.block); */
  }

  printf ("%dB  %dB  %d  %d  %g  %d  %d  %f\n", (int)*fsize, OC_BLOCK_LEN, ntotal, ntotal, nomb*(1+3*EPS)/*ncmb*(1-EPS/2)*//*ABCONST*nomb + nomb*/, nrec, nomb, 100*((double)(ntotal-nrec)/(double)nrec)); /* mire-20070905 */

  wbuf = malloc (ncmb * OC_BLOCK_LEN);
  /* assert (wbuf); */

  /* copy the recovered blocks */
  for (i = 0; i < ncmb; i++) {
	 rp = memcpy (wbuf + i*OC_BLOCK_LEN, rcm[i].block, OC_BLOCK_LEN);
	 assert (rp);
  }
  assert ( (buflen < ncmb * OC_BLOCK_LEN) );
  wbuf[buflen] = '\0';

  /* printf ("%s\n", wbuf); */

  /* copy buffer to outfile */
  fnw = calloc (fnlen + strlen(fnappend), sizeof (char));
  assert (fnw);

  rp = strncpy (fnw, fnr, fnlen);
  assert (rp);
  rp = strncat (fnw, fnappend, fnlen);
  assert (rp);
  /* printf ("outfile='%s'\n", fnw); */ /* mire-20070917 */

  fdw = fopen (fnw, "wb");
  if (!fdw) {
	 perror ("fopen()");
	 exit(EXIT_FAILURE);
  }
  if (buflen != fwrite (wbuf, sizeof (char), buflen, fdw)) {
	 perror ("fwrite()");
	 exit(EXIT_FAILURE);
  }
  fclose (fdw);

  INFO("OK");

  if (am)
	 free (am);
  if (rab)
	 free (rab);
  free (fsize);
  free (fnw);
  free (fnr);
  free (rbuf);
  free (wbuf);
  cbl_free (&cbl);
  free (om);
  free (rcm);
  free (pd);

  return 1;
}
#endif  /* #if 0 */
