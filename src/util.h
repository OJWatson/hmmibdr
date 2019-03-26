#ifndef UTIL_H
#define UTIL_H

#include <Rcpp.h>
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Print.h>
#include <unistd.h>
#include <stdio.h>

void rcpp_out(bool quiet, std::string messsage);
void rcpp_err(bool quiet, std::string messsage);
char * recover_filename(FILE * f);

#endif
