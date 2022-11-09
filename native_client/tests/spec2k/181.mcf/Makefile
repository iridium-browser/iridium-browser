SRC=\
src/implicit.c \
src/mcf.c \
src/mcfutil.c \
src/output.c \
src/pbeampp.c \
src/pbla.c \
src/pflowup.c \
src/psimplex.c \
src/pstart.c \
src/readmin.c \
src/treeup.c

IT=mcf

CFLAGS = -DWANT_STDC_PROTO
EMCCFLAGS_EXTRA_TRAIN = --preload-file data/train/input
EMCCFLAGS_EXTRA_REF = --preload-file data/ref/input
include ../Makefile.common

