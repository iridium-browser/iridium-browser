SRC=\
src/agcollec.c \
src/aggroup.c \
src/blister.c \
src/coding.c \
src/costab.c \
src/cyclotom.c \
src/eval.c \
src/finfield.c \
src/function.c \
src/gap.c \
src/gasman.c \
src/idents.c \
src/integer.c \
src/list.c \
src/pcpresen.c \
src/permutat.c \
src/plist.c \
src/polynom.c \
src/range.c \
src/rational.c \
src/read.c \
src/record.c \
src/scanner.c \
src/set.c \
src/statemen.c \
src/string.c \
src/system.c \
src/tietze.c \
src/unknown.c \
src/vecffe.c \
src/vector.c \
src/word.c

IT=gap

CFLAGS_EXTRA = -DSYS_HAS_CALLOC_PROTO  -DSYS_IS_USG -DSYS_HAS_IOCTL_PROTO -DSYS_HAS_TIME_PROTO
# TODO(stichnot): package up files once stdin problem is solved.
EMCCFLAGS_EXTRA_TRAIN =
EMCCFLAGS_EXTRA_REF =

# 254.gap relies on signed arithmetic overflow being well-defined.
CFLAGS_EXTRA += -fwrapv

include ../Makefile.common

