
#include "hmmIBD.h"


//' @title
//' hmmIBD
//' @description
//' hmmIBD implementation from https://github.com/glipsnort/hmmIBD
//'
//' @param param_list A list of parameters created with \code{hmm_ibd}
//'
//' @details
//' \code{hmmibd_c} implements hidden Markov model for detecting segments
//'   of shared ancestry (identity by descent) in genetic sequence data.
//'
//' @export
// [[Rcpp::export]]
int hmmibd_c(Rcpp::List param_list) {

  /* User-settable parameters */
  double eps = Rcpp::as<double>(param_list["eps"]);         // error rate in genotype calls
  int min_inform = Rcpp::as<int>(param_list["min_inform"]); // minimum number of informative sites in a pairwise
  //  comparison (those w/ minor allele)
  double min_discord = Rcpp::as<double>(param_list["min_discord"]); // minimum discordance in comparison; set > 0 to skip identical pairs
  double max_discord = Rcpp::as<double>(param_list["max_discord"]); // set < 1 to skip unrelated pairs
  int nchrom = Rcpp::as<int>(param_list["nchrom"]); // 14 for falciparum
  int min_snp_sep = Rcpp::as<int>(param_list["min_snp_sep"]); // skip next snp(s) if too close to last one; in bp
  double rec_rate = Rcpp::as<double>(param_list["rec_rate"]); // 7.4e-5 cM/bp or 13.5 kb/cM Miles et al, Genome Res 26:1288-1299 (2016)
  //  const double rec_rate = 5.8e-7;   // 5.8e-5 cM/bp, or 17kb/cM

  /* end user-settable parameters */
  const double fit_thresh_dpi = .001;
  const double fit_thresh_dk = .01;
  const double fit_thresh_drelk = .001;

  double k_rec_init = 1.0;          // starting value for N generation parameter
  double k_rec, k_rec_max = 0.;  // working and max value for same
  const int max_all = 8;
  int niter = 5;    // maximum number of iterations of fit; can be overriden by -m
  int max_snp = 30000;
  char data_file1[128], data_file2[128], *erp;
  char out_filebase[128], freq_file1[128], freq_file2[128], good_file[128], bad_file[128];
  int max_bad = 100;
  int max_good = 200;
  int linesize = 4000;
  char *newLine1, *newLine2, *token, *running, **sample1, **sample2, *head;
  char *file, **bad_samp=NULL, **good_pair[2]={NULL};
  int itoken, nsample1=0, nsample2=0, isamp, chr, sum, iall, all, js, snp_ind;
  int **geno1, **geno2, chr2, pos2, majall, npair_report;
  double **discord, pright, seq_ibd_fb=0, seq_dbd_fb=0, p_ibd, fmean, fmeani, fmeanj;
  double **freq1=NULL, **freq2=NULL, *ffreq1=NULL, *ffreq2=NULL, xisum, xi[2][2], trans_pred, trans_obs;
  double *phi[2], pinit[2], pi[2], *b[2], a[2][2], ptrans, *alpha[2], *beta[2], *scale;
  double maxval, max_phi=0, max_phiL, seq_ibd, seq_dbd, count_ibd_fb, count_dbd_fb;
  double gamma[2], last_pi=0, last_prob=0, last_krec=0, delpi, delprob, delk, maxfreq;
  FILE *inf1=NULL, *inf2=NULL, *outf=NULL, *pf=NULL, *ff1=NULL, *ff2=NULL;
  int *diff=NULL, *same_min=NULL, jsamp, *allcount1, *allcount2, *use_sample1, *use_sample2;
  int *traj, add_seq, nsample_use2;
  int nsample_use1, nsnp, ipair, npair, isnp, chrlen, *pos, *psi[2], max, iline;
  int *nmiss_bypair=NULL, totall1, totall2, *start_chr=NULL, *end_chr=NULL, is, maxlen;
  int **use_pair=NULL, *nall=NULL, killit, nuse_pair=0, gi, gj, delpos;
  int ntri=0, ibad, nbad, start_snp, ex_all=0, last_snp, iflag1, iflag2, oflag;
  int freq_flag1, freq_flag2, fpos=0, fchr=0, iter, ntrans, mflag, finish_fit, bflag, gflag;
  int prev_chrom, ngood, nflag, nskipped=0, nsite, jstart;
  int count_ibd_vit, count_dbd_vit;

  pinit[0] = 0.5;  // flat prior
  pinit[1] = 0.5;

  char usage_string[512];
  strcpy(usage_string, "Usage: hmm -i <input file, pop1> -o <output filename> [-I <input file, pop2>] \n");
  strcat(usage_string, "[-m <max fit iter>] [-f <allele freq file, pop1>] [-F <allele freq file, pop2>]\n");
  strcat(usage_string,  "[-b <file with samples to skip>] [-n <max N generation>]");
  strcat(usage_string, "  [-g <file with sample pairs to use>]\n");

  opterr = 0;
  mflag = iflag1 = iflag2 = oflag = freq_flag1 = freq_flag2 = bflag = gflag = nflag = 0;

  // parse parameters
  if (param_list["f"] != R_NilValue){
    freq_flag1 = 1;
    strcpy(freq_file1, param_list["f"]);
  }
  if (param_list["F"] != R_NilValue){
    freq_flag2 = 1;
    strcpy(freq_file2, param_list["F"]);
  }
  if (param_list["b"] != R_NilValue){
    bflag = 1;
    strcpy(bad_file, param_list["b"]);
  }
  if (param_list["g"] != R_NilValue){
    gflag = 1;
    strcpy(good_file, param_list["g"]);
  }
  if (param_list["m"] != R_NilValue){
    mflag = 1;
    niter = Rcpp::as<int>(param_list["m"]);
  }
  if (param_list["n"] != R_NilValue){
    nflag = 1;
    k_rec_max = Rcpp::as<double>(param_list["n"]);
  }
  if (param_list["i"] != R_NilValue){
    iflag1 = 1;
    strcpy(data_file1, param_list["i"]);
  }
  if (param_list["I"] != R_NilValue){
    iflag2 = 1;
    strcpy(data_file2, param_list["I"]);
  }
  if (param_list["o"] != R_NilValue){
    oflag = 1;
    strcpy(out_filebase, param_list["o"]);
  }

  if (freq_flag2 == 1 && iflag2 == 0) {
    REprintf("Inconsistent options: frequency file for 2nd population specified");
    REprintf(" with no data file for that population\n");
    Rcpp::stop("");
  }

  allcount1 = (int *)malloc((max_all+1) * sizeof(int));
  if (freq_flag1 == 1) {
    ff1 = fopen(freq_file1, "r");
    if (ff1 == NULL) {REprintf("Could not open frequency file %s\n", freq_file1); Rcpp::stop("");}
  }
  if (freq_flag2 == 1) {
    ff2 = fopen(freq_file2, "r");
    if (ff2 == NULL) {REprintf("Could not open frequency file %s\n", freq_file2); Rcpp::stop("");}
  }
  bad_samp = (char **)malloc(max_bad * sizeof(char*));
  good_pair[0] = (char **)malloc(max_good * sizeof(char*));
  good_pair[1] = (char **)malloc(max_good * sizeof(char*));
  newLine1 =(char *) malloc((linesize+1) * sizeof(char));
  assert(newLine1 != NULL);
  nall = (int *)malloc(max_snp * sizeof(int));
  freq1 = (double **)malloc(max_snp * sizeof(double*));
  pos = (int *)malloc(max_snp * sizeof(int));
  start_chr = (int *)malloc((nchrom+1) * sizeof(int));
  end_chr = (int *)calloc(nchrom+1, sizeof(int));
  for (isnp = 0; isnp < max_snp; isnp++) {
    freq1[isnp] = (double *)malloc((max_all+1) * sizeof(double));
  }
  ffreq1 = (double *)malloc((max_all+1) * sizeof(double));
  for (chr = 1; chr <= nchrom; chr++) {
    start_chr[chr] = 10000000;
    end_chr[chr] = -1;
  }
  for (isamp = 0; isamp < max_bad; isamp++) {
    bad_samp[isamp] = (char *)malloc(64 * sizeof(char));
  }
  for (isamp = 0; isamp < max_good; isamp++) {
    good_pair[0][isamp] = (char *)calloc(64, sizeof(char));
    good_pair[1][isamp] = (char *)calloc(64, sizeof(char));
  }
  if (iflag2 == 1) {
    allcount2 = (int *)malloc((max_all+1) * sizeof(int));
    freq2 = (double **)malloc(max_snp * sizeof(double*));
    for (isnp = 0; isnp < max_snp; isnp++) {
      freq2[isnp] = (double *)malloc((max_all+1) * sizeof(double));
    }
    ffreq2 = (double *)malloc((max_all+1) * sizeof(double));
  }
  else {
    allcount2 = allcount1;
    freq2 = freq1;
    ffreq2 = NULL;
  }

  ngood = nbad = 0;
  if (bflag == 1) {
    inf1 = fopen(bad_file, "r");
    if (inf1 == NULL) {
      REprintf("Could not open file of bad samples: %s\n", bad_file);
      bflag = 0;
    }
    else {
      while (fgets(newLine1, linesize, inf1) != NULL) {
        newLine1[strcspn(newLine1, "\r\n")] = 0;
        strncpy(bad_samp[nbad], newLine1, 63);
        nbad++;
        if (nbad == max_bad) {
          bad_samp = (char **)realloc(bad_samp, 2*max_bad*sizeof(char*));
          for (isamp = max_bad; isamp < 2*max_bad; isamp++) {
            bad_samp[isamp] = (char *)malloc(64 * sizeof(char));
          }
          max_bad *= 2;
        }
      }
      fclose(inf1);
    }
  }

  if (gflag == 1) {
    inf1 = fopen(good_file, "r");
    if (inf1 == NULL) {
      REprintf("Could not open file of good sample pairs: %s\n", good_file);
      gflag = 0;
    }
    else {
      while (fgets(newLine1, linesize, inf1) != NULL) {
        newLine1[strcspn(newLine1, "\r\n")] = 0;
        for (running = newLine1, itoken = 0; (token = strsep(&running, "\t")) != NULL && itoken < 2;
        itoken++) {
          strncpy(good_pair[itoken][ngood], token, 63);
        }
        if (good_pair[1][ngood][0] == 0) {
          REprintf("Skipped line with only one record in file of good sample pairs: %s.\n",
                   newLine1);
          continue;
        }
        ngood++;
        if (ngood == max_good) {
          good_pair[0] = (char **)realloc(good_pair[0], 2*max_good*sizeof(char*));
          good_pair[1] = (char **)realloc(good_pair[1], 2*max_good*sizeof(char*));
          for (isamp = max_good; isamp < 2*max_good; isamp++) {
            good_pair[0][isamp] = (char *)calloc(64, sizeof(char));
            good_pair[1][isamp] = (char *)calloc(64, sizeof(char));
          }
          max_good *= 2;
        }
      }
      fclose(inf1);
    }
  }

  inf1 = fopen(data_file1, "r");
  if (inf1 == NULL) {REprintf("Could not open input file %s\n", data_file1); Rcpp::stop("");}
  if (iflag2 == 1) {
    inf2 = fopen(data_file2, "r");
    if (inf2 == NULL) {REprintf("Could not open input file %s\n", data_file2); Rcpp::stop("");}
  }
  file = (char *)calloc(size_t(strlen(out_filebase) + strlen(".hmm.txt") + 1), sizeof(char));
  if (file == NULL) { REprintf("Could not allocate memory for filename(s).\n"); Rcpp::stop(""); }
  sprintf(file, "%s.hmm.txt", out_filebase);
  outf = fopen(file, "w");
  if (outf == NULL) {REprintf("Could not open output file %s\n", file); Rcpp::stop("");}
  fprintf(outf, "sample1\tsample2\tchr\tstart\tend\tdifferent\tNsnp\n");
  sprintf(file, "%s.hmm_fract.txt", out_filebase);
  pf = fopen(file, "w");
  if (pf == NULL) {REprintf("Could not open output file %s\n", file); Rcpp::stop("");}
  fprintf(pf, "sample1\tsample2\tN_informative_sites\tdiscordance\tlog_p\tN_fit_iteration\tN_generation");
  fprintf(pf, "\tN_state_transition\tseq_shared_best_traj\tfract_sites_IBD\tfract_vit_sites_IBD\n");

  // Check line size
  if(fgets(newLine1, linesize, inf1) != NULL) { // header1
    while (strlen(newLine1) > (unsigned long) linesize-2) {
      fseek(inf1, 0, 0);
      linesize *= 2;
      free(newLine1);
      newLine1 = (char *)malloc((linesize+1) * sizeof(char));
      if(fgets(newLine1, linesize, inf1) == NULL) { // header1
        REprintf("Could not read string from stream %s\n", inf1);
        Rcpp::stop("");
      }
    }
  } else {
    REprintf("Could not read string from stream %s\n", inf1);
    Rcpp::stop("");
  }

  newLine1[strcspn(newLine1, "\r\n")] = 0;
  head = (char *)malloc((linesize+1) * sizeof(char));
  assert(head != NULL);
  strcpy(head, newLine1);
  for (running = newLine1, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
    if (itoken > 1) {nsample1++;}
  }

  sample1 = (char **)malloc(nsample1 * sizeof(char*));
  assert(sample1 != NULL);
  geno1 = (int **)malloc(nsample1 * sizeof(int*));
  assert(geno1 != NULL);
  use_sample1 = (int *)malloc(nsample1 * sizeof(int));
  assert(use_sample1 != NULL);
  discord = (double **)malloc(nsample1 * sizeof(double*));
  assert(discord != NULL);
  use_pair = (int **)malloc(nsample1 * sizeof(int*));   // nsamp1 x nsamp2
  assert(use_pair != NULL);
  for (isamp = 0; isamp < nsample1; isamp++) {
    geno1[isamp] = (int *)malloc(max_snp * sizeof(int));
    assert(geno1[isamp] != NULL);
    sample1[isamp] = (char *)malloc(64 * sizeof(char));
    assert(sample1[isamp] != NULL);
  }
  prev_chrom = -1;

  // Parse header1, store sample names after screening for excluded sample ids
  isamp = nsample_use1 = nsample_use2 = 0;
  for (running = head, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
    if (itoken > 1) {
      strncpy(sample1[isamp], token, 63);
      use_sample1[isamp] = 1;
      for (ibad = 0; ibad < nbad; ibad++) {
        if (strcmp(token, bad_samp[ibad]) == 0) {
          use_sample1[isamp] = 0;
          Rprintf("killing sample %s\n", token);
          break;
        }
      }
      if (use_sample1[isamp] == 1) {
        nsample_use1++;
      }
      isamp++;
    }
  }

  // Note: using newLine1 for both files until we start reading genotypes. This way newLine2 only
  //  has to be allocated once, after both headers have been read and the line length possibly increased
  if (iflag2 == 1) {
    if(fgets(newLine1, linesize, inf2) != NULL){ // header2
    while (strlen(newLine1) > (unsigned long) linesize-2) {
      fseek(inf2, 0, 0);
      linesize *= 2;
      free(newLine1);
      newLine1 = (char *)malloc((linesize+1) * sizeof(char));
      if(fgets(newLine1, linesize, inf2) == NULL) { // header2
        REprintf("Could not read string from stream %s\n", inf2);
        Rcpp::stop("");
      }
    }
    } else {
      REprintf("Could not read string from stream %s\n", inf2);
      Rcpp::stop("");
    }

    newLine2 = (char *)malloc((linesize+1) * sizeof(char));
    newLine1[strcspn(newLine1, "\r\n")] = 0;
    free(head);
    head = (char *)malloc((linesize+1) * sizeof(char));
    assert(head != NULL);
    strcpy(head, newLine1);
    for (running = newLine1, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
      if (itoken > 1) {nsample2++;}
    }

    sample2 = (char **)malloc(nsample2 * sizeof(char*));
    assert(sample2 != NULL);
    geno2 = (int **)malloc(nsample2 * sizeof(int*));
    assert(geno2 != NULL);
    use_sample2 = (int *)malloc(nsample2 * sizeof(int));
    assert(use_sample2 != NULL);
    for (isamp = 0; isamp < nsample2; isamp++) {
      geno2[isamp] = (int *)malloc(max_snp * sizeof(int));
      assert(geno2[isamp] != NULL);
      sample2[isamp] = (char *)malloc(64 * sizeof(char));
      assert(sample2[isamp] != NULL);
    }
  }
  else {
    sample2 = sample1;
    nsample2 = nsample1;
    geno2 = geno1;
    use_sample2 = use_sample1;
  }

  for (isamp = 0; isamp < nsample1; isamp++) {
    use_pair[isamp] = (int *)calloc(nsample2, sizeof(int));
    assert(use_pair[isamp] != NULL);
    discord[isamp] = (double *)malloc(nsample2 * sizeof(double));
    assert(discord[isamp] != NULL);
  }

  // Parse header2, store sample names after screening for excluded sample ids
  if (iflag2 == 1) {
    isamp = 0;
    for (running = head, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
      if (itoken > 1) {
        strncpy(sample2[isamp], token, 63);
        use_sample2[isamp] = 1;
        for (ibad = 0; ibad < nbad; ibad++) {
          if (strcmp(token, bad_samp[ibad]) == 0) {
            use_sample2[isamp] = 0;
            Rprintf("killing sample %s\n", token);
            break;
          }
        }
        if (use_sample2[isamp] == 1) {
          nsample_use2++;
        }
        isamp++;
      }
    }
  }
  else {
    // Single pop
    nsample_use2 = nsample_use1;
  }

  Rprintf("Maximum fit iterations allowed: %d\n", niter);
  Rprintf("Minimum marker spacing (bp): %d\n", min_snp_sep);
  Rprintf("Minimum informative markers: %d\n", min_inform);
  Rprintf("Pairs accepted with discordance in range (%.2f%%, %.2f%%)\n",
          min_discord*100, max_discord*100);
  Rprintf("Genotyping error rate: %.2f%%\n", eps*100);
  if (iflag2 == 1) {
    Rprintf("Input files: %s and %s\n", data_file1, data_file2);
  }
  else {
    Rprintf("Input file: %s\n", data_file1);
  }
  Rprintf("Frequency files: ");
  if (freq_flag1 == 1) {Rprintf("%s and ", freq_file1);}
  else {Rprintf("none and ");}
  if (freq_flag2 == 1) {Rprintf("%s\n", freq_file2);}
  else {Rprintf("none\n");}
  if (nflag == 1) {Rprintf("Number of generations capped at %.2f\n", k_rec_max);}

  npair = nsample_use1 * nsample_use2;
  npair_report = nsample_use1 * (nsample_use1-1) / 2;
  Rprintf("pop1 nsample: %d used: %d", nsample1, nsample_use1);
  if (iflag2 == 1) {
    npair_report = npair;
    Rprintf("  pop2 nsample: %d used: %d", nsample2, nsample_use2);}
  Rprintf(" Expected pairs: %d\n", npair_report);

  nmiss_bypair = (int *)calloc(npair, sizeof(int));
  same_min = (int *)calloc(npair, sizeof(int));
  diff = (int *)calloc(npair, sizeof(int));

  // All pairs are to be used by default, unless we've read a file of good pairs
  //   Except single-pop samples should not be compared with themselves
  for (isamp = 0; isamp < nsample1; isamp++) {
    jstart = (iflag2 == 1) ? 0 : isamp+1;
    for (jsamp = jstart; jsamp < nsample2; jsamp++) {
      if (gflag == 1) {
        use_pair[isamp][jsamp] = 0;
        for (ipair = 0; ipair < ngood; ipair++) {
          if ( (strcmp(good_pair[0][ipair], sample1[isamp]) == 0 &&
               strcmp(good_pair[1][ipair], sample2[jsamp]) == 0) ||
               (strcmp(good_pair[0][ipair], sample2[jsamp]) == 0 &&
               strcmp(good_pair[1][ipair], sample1[isamp]) == 0) ) {
            use_pair[isamp][jsamp] = 1;
          }
        }
      }
      else {
        if (isamp != jsamp || iflag2 == 1) {
          use_pair[isamp][jsamp] = 1;
        }
      }
    }
  }

  nsnp = 0;
  iline = -1;
  while (fgets(newLine1, linesize, inf1) != NULL) {
    newLine1[strcspn(newLine1, "\r\n")] = 0;
    if (iflag2 == 1) {
      if(fgets(newLine2, linesize, inf2) != NULL) {;
      newLine2[strcspn(newLine2, "\r\n")] = 0;
      } else {
        REprintf("Could not read string from stream %s\n", inf2);
        Rcpp::stop("");
      }
    }
    if (nsnp == max_snp) {
      nall = (int *)realloc(nall, 2*max_snp*sizeof(int));
      assert(nall != NULL);
      pos = (int *)realloc(pos, 2*max_snp*sizeof(int));
      assert(pos != NULL);
      for (isamp = 0; isamp < nsample1; isamp++) {
        geno1[isamp] = (int *)realloc(geno1[isamp], 2*max_snp*sizeof(int));
        assert(geno1[isamp] != NULL);
      }
      freq1 = (double **)realloc(freq1, 2*max_snp*sizeof(double*));
      assert(freq1 != NULL);
      for (isnp = max_snp; isnp < 2*max_snp; isnp++) {
        freq1[isnp] = (double *)malloc((max_all+1) * sizeof(double));
        assert(freq1[isnp] != NULL);
      }
      if (iflag2 == 1) {
        for (isamp = 0; isamp < nsample2; isamp++) {
          geno2[isamp] = (int *)realloc(geno2[isamp], 2*max_snp*sizeof(int));
          assert(geno2[isamp] != NULL);
        }
        freq2 = (double **)realloc(freq2, 2*max_snp*sizeof(double*));
        assert(freq2 != NULL);
        for (isnp = max_snp; isnp < 2*max_snp; isnp++) {
          freq2[isnp] = (double *)malloc((max_all+1) * sizeof(double));
          assert(freq2[isnp] != NULL);
        }
      }   // end if iflag2 == 1
      else {
        geno2 = geno1;
        freq2 = freq1;
      }
      max_snp *= 2;
    }  // end reallocating space
    iline++;
    totall1 = totall2 = killit = 0;
    for (iall = 0; iall <= max_all; iall++) {allcount1[iall] = allcount2[iall] = 0;}

    // Parse line, pop1
    for (running = newLine1, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
      if (itoken == 0) {
        chr = strtol(token, &erp, 10);
        if (token == erp) {
          REprintf("Invalid chromosome %s (must be integer)\n", token);
          Rcpp::stop("");
        }
      }
      else if (itoken == 1) {
        pos[nsnp] = strtol(token, &erp, 10);
        if (token == erp) {
          REprintf("Invalid position %s (must be integer)\n", token);
          Rcpp::stop("");
        }
        if ( (chr == prev_chrom && pos[nsnp] < pos[nsnp-1]) || chr < prev_chrom) {
          REprintf("Variants are out of order\n");
          Rcpp::stop("");
        }
        if (nsnp > 0 && chr == prev_chrom && pos[nsnp] - pos[nsnp-1] < min_snp_sep) {
          nskipped++;
          killit = 1;
          break;
        }
      }
      else {
        all = strtol(token, &erp, 10);
        if (token == erp) {
          REprintf("Invalid allele %s (must be integer)\n", token);
          Rcpp::stop("");
        }
        if (all > max_all) {
          killit = 1;
          ex_all++;
          break;
        }
        geno1[itoken-2][nsnp] = all;
        if (use_sample1[itoken-2] == 1) {
          if (all >= 0) {
            allcount1[all]++;
            totall1++;
          }
        }
      }
    } // end parsing input line
    if (freq_flag1 != 1 && totall1 == 0) {killit = 1;}   // no valid calls to calculate frequency

    // Parse line, pop2
    if (iflag2 == 1) {
      for (running = newLine2, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
        if (itoken == 0) {
          chr2 = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid chromosome %s (must be integer)\n", token);
            Rcpp::stop("");
          }
        }
        else if (itoken == 1) {
          pos2 = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid position %s (must be integer)\n", token);
            Rcpp::stop("");
          }
          if (pos2 != pos[nsnp] || chr2 != chr) {
            REprintf("Data files do not agree on SNPs\n");
            Rcpp::stop("");
          }
        }
        else {
          all = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid allele %s (must be integer)\n", token);
            Rcpp::stop("");
          }
          if (all > max_all) {
            killit = 1;
            ex_all++;
            break;
          }
          geno2[itoken-2][nsnp] = all;
          if (use_sample2[itoken-2] == 1) {
            if (all >= 0) {
              allcount2[all]++;
              totall2++;
            }
          }
        }
      } // end parsing 2nd input line
      if (freq_flag2 != 1 && totall2 == 0) {killit = 1;}   // no valid calls to calculate frequency
    }
    if (chr > nchrom) {killit = 1;}

    // if reading freqs from file, read one (pop1)
    if (freq_flag1 == 1) {
      // Clear previous frequencies (since might have skipped previous snp via 'continue')
      for (iall = 0; iall <= max_all; iall++) {
        ffreq1[iall] = 0;
      }

      if (fgets(newLine1, linesize, ff1) == NULL) {
        REprintf("Could not read string from stream %s\n", ff1);
        Rcpp::stop("");
      };
      fpos = fchr = 0;
      for (running = newLine1, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
        if (itoken == 0) {
          fchr = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid chromosome %s (must be integer)\n", token);
            Rcpp::stop("");
          }
        }
        else if (itoken == 1) {
          fpos = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid position %s (must be integer)\n", token);
            Rcpp::stop("");
          }
        }
        else if (itoken > 1) {ffreq1[itoken-2] = strtod(token, NULL);}
      }
      if (fchr != chr || fpos != pos[nsnp]) {
        REprintf("Mismatch between data file and frequency file. Data file (chr/pos): %d/%d vs freq file: %d/%d\n",
                chr, pos[nsnp], fchr, fpos);
        Rcpp::stop("");
      }
    }

    // if reading freqs from file, read one (pop2)
    if (freq_flag2 == 1) {
      // Clear previous frequencies (since might have skipped previous snp via 'continue')
      for (iall = 0; iall <= max_all; iall++) {
        ffreq2[iall] = 0;
      }
      if (fgets(newLine2, linesize, ff2) == NULL) {
        REprintf("Could not read string from stream %s\n", ff2);
        Rcpp::stop("");
      };
      fpos = fchr = 0;
      for (running = newLine2, itoken = 0; (token = strsep(&running, "\t")) != NULL; itoken++) {
        if (itoken == 0) {
          fchr = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid chromosome %s (must be integer)\n", token);
            Rcpp::stop("");
          }
        }
        else if (itoken == 1) {
          fpos = strtol(token, &erp, 10);
          if (token == erp) {
            REprintf("Invalid position %s (must be integer)\n", token);
            Rcpp::stop("");
          }
        }
        else if (itoken > 1) {ffreq2[itoken-2] = strtod(token, NULL);}
      }
      if (fchr != chr || fpos != pos[nsnp]) {
        REprintf("Mismatch between data file and frequency file. Data file (chr/pos): %d/%d vs freq file: %d/%d\n",
                chr, pos[nsnp], fchr, fpos);
        Rcpp::stop("");
      }
    }

    nall[nsnp] = 0;
    majall = -1;
    maxfreq = 0;
    // process this variant -- calculate allele frequencies for each pop
    for (iall = 0; iall <= max_all; iall++) {
      if (freq_flag1 == 1) {
        freq1[nsnp][iall] = ffreq1[iall];
      }
      else {
        freq1[nsnp][iall] = (double) allcount1[iall] / totall1;
      }
      if (iflag2 == 1) {
        if (freq_flag2 == 1) {
          freq2[nsnp][iall] = ffreq2[iall];
        }
        else {
          freq2[nsnp][iall] = (double) allcount2[iall] / totall2;
        }
      }
      fmean = (freq1[nsnp][iall] + freq2[nsnp][iall]) / 2;
      if (fmean > maxfreq) {
        maxfreq = fmean;
        majall = iall;
      }
      if (fmean > 0) {
        nall[nsnp]++;
      }
    }
    if (killit == 1) {continue;}
    prev_chrom = chr;

    if (nall[nsnp] > 2) {
      ntri++;
    }

    // Tabulate differences by pair for calculating discordance
    ipair = 0;
    for (isamp = 0; isamp < nsample1; isamp++) {
      if (use_sample1[isamp] == 0) {continue;}
      // If 2 pops, need to loop over all combinations, but not if one pop
      jstart = (iflag2 == 1) ? 0 : isamp+1;
      for (jsamp = jstart; jsamp < nsample2; jsamp++) {
        if (use_sample2[jsamp] == 0) {continue;}
        if (geno1[isamp][nsnp] != -1 && geno2[jsamp][nsnp] != -1) {
          if (geno1[isamp][nsnp] == geno2[jsamp][nsnp]) {
            if (geno1[isamp][nsnp] != majall) {
              same_min[ipair]++;
            }
          }
          else {diff[ipair]++;}
        }
        else {
          nmiss_bypair[ipair]++;
        }
        ipair++;
      }
    }
    if (nsnp < start_chr[chr]) {
      start_chr[chr] = nsnp;
    }
    if (nsnp > end_chr[chr]) {
      end_chr[chr] = nsnp;
    }
    nsnp++;
  }

  Rprintf("Variants skipped for spacing: %d\n", nskipped);
  if (ex_all > 0) {
    Rprintf("Variants with too many alleles: %d\n", ex_all);
  }
  Rprintf("%d variants used,\t%d with >2 alleles\n", nsnp, ntri);
  ipair = 0;
  for (isamp = 0; isamp < nsample1; isamp++) {
    if (use_sample1[isamp] == 0) {continue;}
    jstart = (iflag2 == 1) ? 0 : isamp+1;
    for (jsamp = jstart; jsamp < nsample2; jsamp++) {
      if (use_sample2[jsamp] == 0) {continue;}
      sum = diff[ipair] + same_min[ipair];
      if (sum == 0) {
        discord[isamp][jsamp]  = 0;
      }
      else {
        discord[isamp][jsamp] = (double) diff[ipair] / sum;
      }
      if (use_pair[isamp][jsamp] == 1) {
        if (discord[isamp][jsamp] >  max_discord || sum < min_inform ||
            discord[isamp][jsamp] < min_discord) {
          use_pair[isamp][jsamp] = 0;
        }
        else {
          nuse_pair++;
        }
      }
      ipair++;
    }
  }
  Rprintf("sample pairs analyzed (filtered for discordance and informative markers): %d\n",
          nuse_pair);

  maxlen = 0;
  for (chr = 1; chr <= nchrom; chr++) {
    if (end_chr[chr] - start_chr[chr] + 1 > maxlen) {maxlen = end_chr[chr] - start_chr[chr] + 1;}
  }
  for (is = 0; is < 2; is++) {
    psi[is] = (int *)malloc(maxlen * sizeof(int));
    phi[is] = (double *)malloc(maxlen * sizeof(double));
    b[is] = (double *)malloc(maxlen * sizeof(double));
    alpha[is] = (double *)malloc(maxlen * sizeof(double));
    beta[is] = (double *)malloc(maxlen * sizeof(double));
  }
  scale = (double *)malloc(maxlen * sizeof(double));
  traj = (int *)malloc(maxlen * sizeof(int));

  ipair = 0;
  for (isamp = 0; isamp < nsample1; isamp++) {
    if (use_sample1[isamp] == 0) {continue;}
    jstart = (iflag2 == 1) ? 0 : isamp+1;
    for (jsamp = jstart; jsamp < nsample2; jsamp++) {
      if (use_sample2[jsamp] == 0) {continue;}
      sum = diff[ipair] + same_min[ipair];
      if (use_pair[isamp][jsamp] == 1) {
        last_prob = 0;
        last_pi = pi[0] = pinit[0];  // initialize with prior
        pi[1] = pinit[1];
        last_krec = k_rec = k_rec_init;
        finish_fit = 0;
        for (iter = 0; iter < niter; iter++) {
          trans_obs = trans_pred = ntrans = 0;
          seq_ibd = seq_dbd = count_ibd_fb = count_dbd_fb = seq_ibd_fb = seq_dbd_fb = 0;
          count_ibd_vit = count_dbd_vit = 0;
          max_phi = 0;
          for (chr = 1; chr <= nchrom; chr++) {
            nsite = 0;
            if (end_chr[chr] < 0) {continue;}
            chrlen = pos[end_chr[chr]] - pos[start_chr[chr]];
            for (isnp = start_chr[chr]; isnp <= end_chr[chr]; isnp++) {
              snp_ind = isnp - start_chr[chr];
              gi = geno1[isamp][isnp];
              gj = geno2[jsamp][isnp];
              pright = 1 - eps * (nall[isnp] - 1);
              if (gi == -1 || gj == -1) {
                // O = n (missing data)
                b[0][snp_ind] = b[1][snp_ind] = 1.0;
              }
              else if (gi == gj) {
                // homozygote
                nsite++;
                fmean = (freq1[isnp][gi] + freq2[isnp][gi]) / 2;
                b[0][snp_ind] = pright * pright * fmean + eps * eps * (1 - fmean); //IBD
                b[1][snp_ind] = pright * pright * freq1[isnp][gi]*freq2[isnp][gj] +
                  pright * eps * freq1[isnp][gi] * (1-freq2[isnp][gj]) +
                  pright * eps * (1 - freq1[isnp][gi]) * freq2[isnp][gj] +
                  eps * eps * (1-freq1[isnp][gi]) * (1-freq2[isnp][gj]);
              }
              else {
                // O = aA
                nsite++;
                fmeani = (freq1[isnp][gi] + freq2[isnp][gi]) / 2;
                fmeanj = (freq1[isnp][gj] + freq2[isnp][gj]) / 2;
                b[0][snp_ind] = pright*eps*(fmeani+fmeanj) +
                  eps * eps * (1 - fmeani - fmeanj);
                b[1][snp_ind] = pright * pright * freq1[isnp][gi] * freq2[isnp][gj] +
                  pright*eps*( freq1[isnp][gi]*(1-freq2[isnp][gj]) + freq2[isnp][gj]*(1-freq1[isnp][gi]) ) +
                  eps * eps * (1 - freq1[isnp][gi]) * (1 - freq2[isnp][gj]);
              }
              if (isnp == start_chr[chr]) {
                psi[0][snp_ind] = psi[1][snp_ind] = 0;
                for (is = 0; is < 2; is++) {
                  phi[is][snp_ind] = log(pi[is]) + log(b[is][snp_ind]);
                  alpha[is][snp_ind] = pi[is] * b[is][snp_ind];
                  scale[is] = 1.;
                }
              }
              else {
                ptrans = k_rec * rec_rate * (pos[isnp] - pos[isnp-1]);
                a[0][1] = 1 - pi[0] - (1 - pi[0]) * exp(-ptrans);
                a[1][0] = 1 - pi[1] - (1 - pi[1]) * exp(-ptrans);
                a[0][0] = 1 - a[0][1];
                a[1][1] = 1 - a[1][0];
                for (js = 0; js < 2; js++) {    // index over state of current snp
                  maxval = -10000000;
                  alpha[js][snp_ind] = scale[snp_ind] = 0;
                  for (is = 0; is < 2; is++) {    // index over state of previous snp
                    if (phi[is][snp_ind-1] + log(a[is][js]) > maxval ) {
                      maxval = phi[is][snp_ind-1] + log(a[is][js]);
                      psi[js][snp_ind] = is;
                    }
                    phi[js][snp_ind] = maxval + log(b[js][snp_ind]);
                    alpha[js][snp_ind] += alpha[is][snp_ind-1] * a[is][js] * b[js][snp_ind];
                  }
                  scale[snp_ind] += alpha[js][snp_ind];
                }
                for (js = 0; js < 2; js++) {    // scale alpha to prevent underflow (Rabiner eqns 92)
                  alpha[js][snp_ind] /= scale[snp_ind];
                }
              }   // end if initializing/continuing
            }  // end snp loop
            last_snp = snp_ind;
            max_phiL = phi[1][snp_ind];
            if (phi[0][snp_ind] > phi[1][snp_ind]) {max_phiL = phi[0][snp_ind];}
            max = (phi[1][snp_ind] > phi[0][snp_ind]) ? 1 : 0;
            traj[snp_ind] = max;
            max_phi += max_phiL;

            // Loop backward to calculate betas (backward part of forward-backward)
            isnp = end_chr[chr];
            snp_ind = isnp - start_chr[chr];
            beta[0][snp_ind] = beta[1][snp_ind] = 1;
            for (isnp = end_chr[chr]-1; isnp >= start_chr[chr]; isnp--) {
              snp_ind = isnp - start_chr[chr];
              ptrans = k_rec * rec_rate * (pos[isnp+1] - pos[isnp]);
              a[0][1] = 1 - pi[0] - (1 - pi[0]) * exp(-ptrans);
              a[1][0] = 1 - pi[1] - (1 - pi[1]) * exp(-ptrans);
              a[0][0] = 1 - a[0][1];
              a[1][1] = 1 - a[1][0];
              for (is = 0; is < 2; is++) {    // index over state of current snp
                beta[is][snp_ind] = 0;
                for (js = 0; js < 2; js++) {
                  // index over state of previous snp (= snp+1, since looping backward)
                  beta[is][snp_ind] += beta[js][snp_ind+1] * a[is][js] * b[js][snp_ind+1] / scale[snp_ind];
                }
              }
            }
            // inverse loop for Viterbi
            for (snp_ind = last_snp-1; snp_ind >= 0; snp_ind--) {
              traj[snp_ind] = psi[max][snp_ind+1];
              max = traj[snp_ind];
            }
            // tabulate FB results
            for (snp_ind = 0; snp_ind <= last_snp; snp_ind++) {
              p_ibd = alpha[0][snp_ind] * beta[0][snp_ind] /
                (alpha[0][snp_ind] * beta[0][snp_ind] + alpha[1][snp_ind] * beta[1][snp_ind]);
              count_ibd_fb += p_ibd;
              count_dbd_fb += (1-p_ibd);
              if (snp_ind < last_snp) {
                isnp = snp_ind + start_chr[chr];
                delpos = pos[isnp+1] - pos[isnp];
                ptrans = k_rec * rec_rate * delpos;
                a[0][1] = 1 - pi[0] - (1 - pi[0]) * exp(-ptrans);
                a[1][0] = 1 - pi[1] - (1 - pi[1]) * exp(-ptrans);
                a[0][0] = 1 - a[0][1];
                a[1][1] = 1 - a[1][0];
                xisum = 0;
                for (is = 0; is < 2; is++) {
                  for (js = 0; js < 2; js++) {
                    xi[is][js] = alpha[is][snp_ind] * a[is][js] * b[js][snp_ind+1] * beta[js][snp_ind+1];
                    xisum += xi[is][js];
                  }
                }
                for (is = 0; is < 2; is++) {
                  gamma[is] = 0;
                  for (js = 0; js < 2; js++) {
                    xi[is][js] /= xisum;
                    gamma[is] += xi[is][js];
                  }
                }
                // xi(0,0) in Rabiner notation = prob of being in state 0 at t and 0 at t+1
                seq_ibd_fb += delpos * xi[0][0];
                seq_dbd_fb += delpos * xi[1][1];
                trans_obs += (xi[0][1] + xi[1][0]);
                trans_pred += gamma[0] * a[0][1] + gamma[1] * a[1][0];
              }
            }

            // print final Viterbi trajectory
            // start
            if (iter == niter - 1 || finish_fit != 0) {
              if (nsite > 0) {
                fprintf(outf, "%s\t%s\t%d\t%d", sample1[isamp], sample2[jsamp], chr, pos[0+start_chr[chr]]);
                start_snp = 0;
                for (isnp = 1; isnp < end_chr[chr] - start_chr[chr] + 1; isnp++) {
                  if (traj[isnp] != traj[isnp-1]) {
                    ntrans++;
                    add_seq = pos[isnp - 1 + start_chr[chr]] - pos[start_snp + start_chr[chr]] + 1;
                    if (traj[isnp-1] == 0) {seq_ibd += add_seq;}
                    else {seq_dbd += add_seq;}
                    // end one and start another
                    fprintf(outf, "\t%d\t%d\t%d\n",
                            pos[isnp - 1 + start_chr[chr]], traj[isnp-1], isnp - start_snp);
                    fprintf(outf, "%s\t%s\t%d\t%d", sample1[isamp],
                            sample2[jsamp], chr, pos[isnp + start_chr[chr]]);
                    start_snp = isnp;
                  }
                }
                isnp = end_chr[chr] - start_chr[chr];
                add_seq = pos[isnp + start_chr[chr]] - pos[start_snp + start_chr[chr]] + 1;
                if (traj[isnp] == 0) {seq_ibd += add_seq;}
                else {seq_dbd += add_seq;}
                fprintf(outf, "\t%d\t%d\t%d\n", pos[end_chr[chr]], traj[isnp], isnp - start_snp + 1);
                // Tabulate sites by state for Viterbi trajectory
                for (isnp = 0; isnp < end_chr[chr] - start_chr[chr] + 1; isnp++) {
                  if (traj[isnp] == 0) {count_ibd_vit++;}
                  else {count_dbd_vit++;}
                }
              }
            }
          }  // end chrom loop

          // quit if fit converged on previous iteration; otherwise, update parameters
          if (finish_fit != 0) {break;}
          if (count_ibd_fb + count_dbd_fb == 0) {
            REprintf("Insufficient information to estimate parameters.\n");
            REprintf("Do you have only one variant per chromosome?\n");
            Rcpp::stop("");
          }
          pi[0] = count_ibd_fb / (count_ibd_fb + count_dbd_fb);
          k_rec *= trans_obs / trans_pred;
          if (nflag == 1 && k_rec > k_rec_max) {k_rec = k_rec_max;}
          // ad hoc attempt to avoid being trapped in extremum
          if (iter < niter-1 && finish_fit == 0) {
            if (pi[0] < 1e-5) {pi[0] = 1e-5;}
            else if (pi[0] > 1- 1e-5) {pi[0] = 1 - 1e-5;}
            if (k_rec < 1e-5) {k_rec = 1e-5;}
          }
          pi[1] = 1 - pi[0];
          delpi = pi[0] - last_pi;
          delk = k_rec - last_krec;
          if (nflag == 1 && k_rec > k_rec_max) {delk = k_rec_max - last_krec;}
          delprob = max_phi - last_prob;
          last_pi = pi[0];
          last_krec = k_rec;
          last_prob = max_phi;

          // Evaluate fit
          if (fabs(delpi) < fit_thresh_dpi &&
              (fabs(delk) < fit_thresh_dk || fabs(delk/k_rec) < fit_thresh_drelk) ) {
            finish_fit = 1;
          }
        }  // end parameter fitting loop

        fprintf(
          pf,
          "%s\t%s\t%d\t%.4f\t%.5e\t%d\t%.3f\t%d\t%.5f\t%.5f\t%.5f\n",
          (sample1[isamp]),
          (sample2[jsamp]),
          sum,
          (discord[isamp][jsamp]),
          max_phi,
          iter,
          k_rec,
          ntrans,
          (double) seq_ibd / (seq_ibd + seq_dbd),
          count_ibd_fb / (count_ibd_fb + count_dbd_fb),
          (double) count_ibd_vit / (count_ibd_vit + count_dbd_vit)
          );

      }   // end if use pair

      ipair++;
      if (ipair%10000 == 0) {Rprintf("Starting pair %d\n", ipair);}
    }
  }

  fclose(pf);
  fclose(outf);

  return(0);
}
