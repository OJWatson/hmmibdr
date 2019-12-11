
# hmmibdr

Direct wrapper for [HMMIBD](https://github.com/glipsnort/hmmIBD) in
Rcpp.

## Installation

You can install the latest version using:

``` r
#install.packages("devtools")
devtools::install_github("OJWatson/hmmibdr")
```

``` r
# Load the package
library(hmmibdr)
```

## Demonstration

Firstly, with one population:

``` r
## one population
out <- hmm_ibd(input_file = system.file("extdata/pf3k_Cambodia_13.txt", package = "hmmibdr"),
               allele_freqs =  system.file("extdata/freqs_pf3k_Cambodia_13.txt", package = "hmmibdr"),
               output_file = "output_Cambodia")
#> Maximum fit iterations allowed: 5
#> Minimum marker spacing (bp): 5
#> Minimum informative markers: 10
#> Pairs accepted with discordance in range (0.00%, 100.00%)
#> Genotyping error rate: 0.10%
#> Input file: /home/oj/R/x86_64-pc-linux-gnu-library/3.5/hmmibdr/extdata/pf3k_Cambodia_13.txt
#> Frequency files: /home/oj/R/x86_64-pc-linux-gnu-library/3.5/hmmibdr/extdata/freqs_pf3k_Cambodia_13.txt and none
#> pop1 nsample: 10 used: 10 Expected pairs: 45
#> Variants skipped for spacing: 1705
#> 18155 variants used, 52 with >2 alleles
#> sample pairs analyzed (filtered for discordance and informative markers): 45
```

And two populations:

``` r
## two pops
out <- hmm_ibd(input_file = system.file("extdata/pf3k_Cambodia_13.txt", package = "hmmibdr"),
               allele_freqs = system.file("extdata/freqs_pf3k_Cambodia_13.txt", package = "hmmibdr"),
               genotypes_sec_pop = system.file("extdata/pf3k_Ghana_13.txt", package = "hmmibdr"),
               allele_freqs_sec_pop = system.file("extdata/freqs_pf3k_Ghana_13.txt", package = "hmmibdr"),
               output_file = "output_Cambodia_Ghana")
#> Maximum fit iterations allowed: 5
#> Minimum marker spacing (bp): 5
#> Minimum informative markers: 10
#> Pairs accepted with discordance in range (0.00%, 100.00%)
#> Genotyping error rate: 0.10%
#> Input files: /home/oj/R/x86_64-pc-linux-gnu-library/3.5/hmmibdr/extdata/pf3k_Cambodia_13.txt and /home/oj/R/x86_64-pc-linux-gnu-library/3.5/hmmibdr/extdata/pf3k_Ghana_13.txt
#> Frequency files: /home/oj/R/x86_64-pc-linux-gnu-library/3.5/hmmibdr/extdata/freqs_pf3k_Cambodia_13.txt and /home/oj/R/x86_64-pc-linux-gnu-library/3.5/hmmibdr/extdata/freqs_pf3k_Ghana_13.txt
#> pop1 nsample: 10 used: 10  pop2 nsample: 10 used: 10 Expected pairs: 100
#> Variants skipped for spacing: 1705
#> 18155 variants used, 413 with >2 alleles
#> sample pairs analyzed (filtered for discordance and informative markers): 100
```
