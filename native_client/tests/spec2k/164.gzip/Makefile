# Note: src/getopt.c has been removed from the list because it doesn't
# link properly with emcc, and doesn't actually seem to be necessary
# for any other compilers.
SRC=\
src/bits.c \
src/deflate.c \
src/gzip.c \
src/inflate.c \
src/lzw.c \
src/spec.c \
src/trees.c \
src/unlzh.c \
src/unlzw.c \
src/unpack.c \
src/unzip.c \
src/util.c \
src/zip.c

IT=gzip

CFLAGS_EXTRA = -DSPEC_CPU2000
EMCCFLAGS_EXTRA_TRAIN = --preload-file data/train/input
EMCCFLAGS_EXTRA_REF = --preload-file data/ref/input
include ../Makefile.common

