
// Calculate segments of haploid genome identity between pairs of samples.
// Can filter out sample pairs with high discordance (based on sites where at least
//  1 of the genomes has the minor allele) to skip pairs with little or no sharing
// Handles both within and across population comparisons. There are always two populations in
//  the code; in case of single pop, the 2nd pop just points to the first. (iflag2 == 1)
//  means two pops.
// States: 0=IBD (identical by descent), 1=non-identical
//  HMM notation follows Rabiner (Proc of the IEEE, V77, p257 (1989)).
//  Note: alpha/beta underflow can be handled either by working in log space + logsumexp trick, or
//   by scaling (as in Rabiner). I use scaling.


#include <stdio.h>
#include <stdlib.h>
#include <Rcpp.h>
#include <string>
#include <unistd.h>
#include <cstring>
#include "altstrsep.h"

int hmmibd_c(Rcpp::List param_list);
