#include "util.h"

void rcpp_out(bool quiet, std::string message){

  if(!quiet){
    Rcpp::Rcout << message;
  }

}


void rcpp_err(bool quiet, std::string message){

  if(!quiet){
    Rcpp::Rcerr << message;
  }

}

char * recover_filename(FILE * f) {
  int fd;
  char fd_path[255];
  char * filename = (char *)malloc(255);
  ssize_t n;

  fd = fileno(f);
  sprintf(fd_path, "/proc/self/fd/%d", fd);
  n = readlink(fd_path, filename, 255);
  if (n < 0)
    return NULL;
  filename[n] = '\0';
  return filename;
}
