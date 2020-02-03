#------------------------------------------------
#' The Real McCOIL categorical method function
#'
#' This function triggers the c code for the categorical method
#'
#' @param input_file File of genotype data. See below for format.
#' @param output_file Output file name. Two output files will be produced, with
#'   ".hmm.txt" and ".hmm_fract.txt" appended to the supplied name.
#' @param allele_freqs File of allele frequencies for the sample population.
#'   Format: tab-delimited, no header, one variant per row.
#'   Line format: <chromosome (int)> <position (bp, int)> <allele 1 freq> <all 2 freq> [<all 3 freq>] ...
#'   The genotype and frequency files must contain exactly the same variants,
#'   in the same order. If no file is supplied, allele frequencies are
#'   calculated from the input data file.
#' @param genotypes_sec_pop File of genotype data from a second population;
#'   same format as for -i. (added in 2.0.0)
#' @param allele_freqs_sec_pop File of allele frequencies for the second
#'   population; same format as for -f. (added in 2.0.0)
#' @param max_fit_iterations Maximum number of fit iterations (defaults to 5).
#' @param exclude_ids File of sample ids to exclude from all analysis.
#'   Format: no header, one id (string) per row.
#'   (Note: b stands for "bad samples".)
#' @param analysis_ids File of sample pairs to analyze; all others are not
#'   processed by the HMM (but are still used to calculate allele frequencies).
#'   Format: no header,	tab-delimited, two sample ids (strings) per row.
#'   (Note: "g" stands for "good pairs".)
#' @param num_gens Cap on the number of generations (floating point).
#'   Sets the maximum value for that parameter in the fit. This is useful
#'   if you are interested in recent IBD and are working with a population
#'   with substantial linkage disequilbrium. Specifying a small value will
#'   force the program to assume little recombination and thus a low
#'   transition rate; otherwise it will identify the small blocks of LD as
#'   ancient IBD, and will force the number of generations to be large.
#' @param overwrite Boolean detailing if output files already exist, should
#'   they be overwritten. Deafult = FALSE
#' @param fract_only Boolean detailing whether to rturn just the fract.
#'   Default = FALSE
#' @param eps Numeric for error rate in genotype calls. Default = .001
#' @param min_inform Minimum number of informative sites in a pairwise. Default = 10
#' @param min_discord Minimum discordance in comparison. Default = 0. Set > 0 to skip identical pairs
#' @param max_discord Maximum discordance in comparison. Default = 1. Set < 1 to skip unrelated pairs
#' @param nchrom Number of chromosomes. Default = 14 for falciparum
#' @param min_snp_sep Minimum snp distance, i.e. skip next snp(s) if too close to last one. Default = 5 (bp)
#' @param rec_rate Recombination rate. Default = 7.4e-7. (7.4e-5 cM/bp or 13.5 kb/cM Miles et al, Genome Res 26:1288-1299 (2016))
#'
#' @return return list of summary data frames of hmmIBD output
#'
#' @export
#'



hmm_ibd <- function(input_file,
                    output_file,
                    allele_freqs = NULL,
                    genotypes_sec_pop = NULL,
                    allele_freqs_sec_pop = NULL,
                    max_fit_iterations = NULL,
                    exclude_ids = NULL,
                    analysis_ids = NULL,
                    num_gens = NULL,
                    overwrite = FALSE,
                    fract_only = FALSE,
                    eps = 0.001,
                    min_inform = 10,
                    min_discord = 0,
                    max_discord = 1,
                    nchrom = 14,
                    min_snp_sep = 5,
                    rec_rate = 7.4-7) {


  files <- grep(basename(output_file),
                list.files(dirname(output_file), full.names = TRUE),
                value=TRUE)

  if (length(grep("fract", files, value=TRUE)) == 0 || overwrite) {

    # create param list
    param_list <- list(
      f = allele_freqs,
      F = allele_freqs_sec_pop,
      i = input_file,
      I = genotypes_sec_pop,
      o = output_file,
      m = max_fit_iterations,
      b = exclude_ids,
      g = analysis_ids,
      n = num_gens,
      eps = eps,
      min_inform = min_inform,
      min_discord = min_discord,
      max_discord = max_discord,
      nchrom = nchrom,
      min_snp_sep = min_snp_sep,
      rec_rate = rec_rate
    )

    # run cpp code
    ret <- hmmibd_c(param_list)

    # check result
    if (ret) {
      stop("c++ somehow returned non-integer")
    }

  }
  # get results
  files <- grep(output_file,
                list.files(dirname(output_file), full.names = TRUE),
                value=TRUE)

  # read files in and create results list
  fract <- read.csv(grep("fract", files, value=TRUE), sep = "\t")
  if (!fract_only) {
    segments <- read.csv(files[-grep("fract", files)], sep = "\t")
    res <- list("fract" = fract, "segments" = segments)
  } else {
    res <- list("fract" = fract)
  }

  return(res)


}
