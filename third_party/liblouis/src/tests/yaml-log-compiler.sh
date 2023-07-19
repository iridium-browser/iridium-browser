#!/usr/bin/env bash

## This script sets the LOUIS_TABLEPATH variable based on the location
## of the test file. Autotools can not handle this. For tests in the
## braille-specs subdirectory, LOUIS_TABLEPATH is set to
## ${abs_top_srcdir}/tables. For other tests it is set to
## ${abs_top_srcdir} (this happens in Makefile.am). The braille-specs
## tests require that LOUIS_TABLEPATH contains the tables under test
## otherwise the tables can not be found via queries.

## An alternative approach would be to invent a new environment
## variable specially for lou_checkyaml to call lou_indexTables
## explicitly instead of relying on liblouis to automatically index
## LOUIS_TABLEPATH.

yaml_test=$1

case $yaml_test in
    */braille-specs/*)
        export LOUIS_TABLEPATH=${LOUIS_TABLEPATH}/tables
        ;;
    *)
        ;;
esac
${WINE} lou_checkyaml${EXEEXT} $yaml_test
