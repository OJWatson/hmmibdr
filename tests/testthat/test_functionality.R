context("Functionality")

test_that("hmmibdr runs", {

  # grab files and test they are correct
  input_file = system.file("extdata/pf3k_Cambodia_13.txt", package = "hmmibdr")
  expect_equal(basename(input_file), "pf3k_Cambodia_13.txt")

  allele_freqs = system.file("extdata/freqs_pf3k_Cambodia_13.txt", package = "hmmibdr")
  expect_equal(basename(allele_freqs), "freqs_pf3k_Cambodia_13.txt")

  # run hmmibd and check outputs
  tf <- tempfile()
  out <- hmm_ibd(input_file = input_file,
               allele_freqs =  allele_freqs,
               output_file = tf)

  expect_equal(names(out), c("fract", "segments"))
  expect_equal(ncol(out$fract), 11)
  expect_equal(ncol(out$segments), 7)

  # check that if reran it will grab the save
  expect_silent(out2 <- hmm_ibd(input_file = input_file,
                 allele_freqs =  allele_freqs,
                 output_file = tf))

  # and that this is the same
  expect_identical(out, out2)

  # is this also true when grabbing only the fract
  # check that if reran it will grab the save
  expect_silent(out3 <- hmm_ibd(input_file = input_file,
                                allele_freqs =  allele_freqs,
                                output_file = tf,
                                fract_only = TRUE))

  # and is it the same output
  expect_identical(names(out3), "fract")
  expect_identical(out3$fract, out2$fract)

  # clean up the output files from the test
  files <- grep(tf,
                list.files(dirname(tf), full.names = TRUE),
                value=TRUE)
  file.remove(files)
})
