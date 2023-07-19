#!/bin/bash -eu
# Build Liblouis
./autogen.sh
./configure
make -j$(nproc) V=1

cd tests/fuzzing
cp ../tables/empty.ctb $OUT/
find ../.. -name "*.o" -exec ar rcs fuzz_lib.a {} \;
$CXX $CXXFLAGS -c table_fuzzer.cc -I/src/liblouis -o table_fuzzer.o
$CXX $CXXFLAGS $LIB_FUZZING_ENGINE table_fuzzer.o -o $OUT/table_fuzzer fuzz_lib.a

# Build corpus
zip $OUT/table_fuzzer_seed_corpus.zip $SRC/liblouis/tables/latinLetterDef6Dots.uti
