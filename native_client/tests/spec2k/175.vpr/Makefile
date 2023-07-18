SRC=\
src/check_route.c \
src/check_rr_graph.c \
src/draw.c \
src/graphics.c \
src/hash.c \
src/main.c \
src/place.c \
src/read_arch.c \
src/read_netlist.c \
src/read_place.c \
src/route.c \
src/rr_graph.c \
src/rr_graph2.c \
src/rr_graph_area.c \
src/rr_graph_sbox.c \
src/rr_graph_timing_params.c \
src/rr_graph_util.c \
src/segment_stats.c \
src/stats.c \
src/util.c



IT=vpr

CFLAGS_EXTRA = -DSPEC_CPU2000
EMCCFLAGS_EXTRA_TRAIN = --preload-file arch.in --preload-file net.in --preload-file place.in
EMCCFLAGS_EXTRA_REF = --preload-file arch.in --preload-file net.in --preload-file place.in

EXTRA_CLEAN=*.in
include ../Makefile.common
