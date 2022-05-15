SRC=\
src/analyze-linkage.c \
src/and.c \
src/build-disjuncts.c \
src/extract-links.c \
src/fast-match.c \
src/idiom.c \
src/main.c \
src/massage.c \
src/parse.c \
src/post-process.c \
src/print.c \
src/prune.c \
src/read-dict.c \
src/strncasecmp.c \
src/utilities.c \
src/word-file.c \
src/xalloc.c

IT=parser

CFLAGS_EXTRA = -DSPEC_CPU2000 -D_POSIX_SOURCE
# TODO(stichnot): package up files once stdin problem is solved.
EMCCFLAGS_EXTRA_TRAIN =
EMCCFLAGS_EXTRA_REF =


EXTRA_CLEAN=2.1.dict words

include ../Makefile.common

