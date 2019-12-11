
# hmmibdr

[![Project Status: Active – The project has reached a stable, usable
state and is being actively
developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)
[![Travis-CI Build
Status](https://travis-ci.org/ojwatson/hmmibdr.png?branch=master)](https://travis-ci.org/ojwatson/hmmibdr)
[![codecov.io](https://codecov.io/github/ojwatson/hmmibdr/coverage.svg?branch=master)](https://codecov.io/github/ojwatson/hmmibdr?branch=master)

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
tf <- tempfile(pattern = "output_Cambodia")
out <- hmm_ibd(input_file = system.file("extdata/pf3k_Cambodia_13.txt", package = "hmmibdr"),
               allele_freqs =  system.file("extdata/freqs_pf3k_Cambodia_13.txt", package = "hmmibdr"),
               output_file = tf)
#> Maximum fit iterations allowed: 5
#> Minimum marker spacing (bp): 683262674
#> Minimum informative markers: 10
#> Pairs accepted with discordance in range (0.00%, 100.00%)
#> Genotyping error rate: 0.10%
#> Input file: /home/oj/R/x86_64-pc-linux-gnu-library/3.6/hmmibdr/extdata/pf3k_Cambodia_13.txt
#> Frequency files: /home/oj/R/x86_64-pc-linux-gnu-library/3.6/hmmibdr/extdata/freqs_pf3k_Cambodia_13.txt and none
#> pop1 nsample: 10 used: 10 Expected pairs: 45
#> Variants skipped for spacing: 1705
#> 18155 variants used, 52 with >2 alleles
#> sample pairs analyzed (filtered for discordance and informative markers): 45
```

`hmmibdr` now returns the relevant fractions and segments as a list.
Firstly the fraction of sites IBD:

``` r
head(out$fract)
#>     sample1   sample2 N_informative_sites discordance    log_p N_fit_iteration
#> 1 PH0047-Cx PH0051-Cx                 860      0.8465 -2543.59               5
#> 2 PH0047-Cx  PH0055-C                 851      0.9542 -2302.10               5
#> 3 PH0047-Cx  PH0057-C                 885      0.8655 -2619.67               5
#> 4 PH0047-Cx  PH0059-C                 904      0.8507 -2736.71               5
#> 5 PH0047-Cx  PH0060-C                 840      0.8548 -2677.95               5
#> 6 PH0047-Cx  PH0064-C                 832      0.8438 -2681.54               5
#>   N_generation N_state_transition seq_shared_best_traj fract_sites_IBD
#> 1        1.342                 12              0.04230         0.07733
#> 2        0.011                  0              0.00000         0.00000
#> 3        0.950                  4              0.02333         0.05227
#> 4        0.099                  2              0.08338         0.06970
#> 5        2.249                  2              0.00090         0.04890
#> 6        2.332                  4              0.01187         0.05730
#>   fract_vit_sites_IBD
#> 1             0.04440
#> 2             0.00000
#> 3             0.02357
#> 4             0.07089
#> 5             0.00270
#> 6             0.01162
```

And the segments compared:

``` r
head(out$segments)
#>     sample1   sample2 chr  start    end different Nsnp
#> 1 PH0047-Cx PH0051-Cx   1  92914 575873         1 4273
#> 2 PH0047-Cx PH0051-Cx   2 106099 373325         1 2222
#> 3 PH0047-Cx PH0051-Cx   2 373459 382818         0   84
#> 4 PH0047-Cx PH0051-Cx   2 383280 455608         1  466
#> 5 PH0047-Cx PH0051-Cx   2 456647 482294         0  155
#> 6 PH0047-Cx PH0051-Cx   2 483362 839251         1 2855
```

`hmmibdr` also can be run for two populations:

``` r
## two pops
tf2 <- tempfile("output_Cambodia_Ghana")
out <- hmm_ibd(input_file = system.file("extdata/pf3k_Cambodia_13.txt", package = "hmmibdr"),
               allele_freqs = system.file("extdata/freqs_pf3k_Cambodia_13.txt", package = "hmmibdr"),
               genotypes_sec_pop = system.file("extdata/pf3k_Ghana_13.txt", package = "hmmibdr"),
               allele_freqs_sec_pop = system.file("extdata/freqs_pf3k_Ghana_13.txt", package = "hmmibdr"),
               output_file = tf2)
#> Maximum fit iterations allowed: 5
#> Minimum marker spacing (bp): 683262674
#> Minimum informative markers: 10
#> Pairs accepted with discordance in range (0.00%, 100.00%)
#> Genotyping error rate: 0.10%
#> Input files: /home/oj/R/x86_64-pc-linux-gnu-library/3.6/hmmibdr/extdata/pf3k_Cambodia_13.txt and /home/oj/R/x86_64-pc-linux-gnu-library/3.6/hmmibdr/extdata/pf3k_Ghana_13.txt
#> Frequency files: /home/oj/R/x86_64-pc-linux-gnu-library/3.6/hmmibdr/extdata/freqs_pf3k_Cambodia_13.txt and /home/oj/R/x86_64-pc-linux-gnu-library/3.6/hmmibdr/extdata/freqs_pf3k_Ghana_13.txt
#> pop1 nsample: 10 used: 10  pop2 nsample: 10 used: 10 Expected pairs: 100
#> Variants skipped for spacing: 1705
#> 18155 variants used, 413 with >2 alleles
#> sample pairs analyzed (filtered for discordance and informative markers): 100
```
