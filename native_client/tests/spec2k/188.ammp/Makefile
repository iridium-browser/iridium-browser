SRC= \
src/ammp.c \
src/analyze.c \
src/angle.c \
src/animate.c \
src/anonbon.c \
src/atoms.c \
src/bonds.c \
src/box.c \
src/eval.c \
src/gsdg.c \
src/hybrid.c \
src/math.c \
src/mom.c \
src/monitor.c \
src/noel.c \
src/optimist.c \
src/random.c \
src/rectmm.c \
src/restrain.c \
src/significance.c \
src/tailor.c \
src/tether.c \
src/tgroup.c \
src/torsion.c \
src/tset.c \
src/unonbon.c \
src/variable.c \
src/vnonbon.c


IT=ammp
CFLAGS_EXTRA = -DSPEC_CPU2000
# TODO(stichnot): package up files once stdin problem is solved.
EMCCFLAGS_EXTRA_TRAIN =
EMCCFLAGS_EXTRA_REF =

EXTRA_CLEAN= ref.* train.*

include ../Makefile.common
