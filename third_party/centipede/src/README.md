
# Centipede - a distributed fuzzing engine. Work-in-progress.

## Why Centipede

Why not? We are currently trying to fuzz some very large and very slow targets
for which [libFuzzer](https://llvm.org/docs/LibFuzzer.html),
[AFL](https://lcamtuf.coredump.cx/afl/), and the like do not necessarily scale
well. For one of our motivating examples
see [SiliFuzz](https://arxiv.org/abs/2110.11519). While working on Centipede we
plan to experiment with new approaches to massive-scale differential fuzzing,
that the existing fuzzing engines don't try to do.

Notable features:

* Early **work-in-progress**. We test centipede within a small team on a couple
  of targets. Unless you are part of the Centipede project, or want to help us,
  **you probably don't want to read further just yet**.

* Scale. The intent is to be able to run any number of jobs concurrently, with
  very little communication overhead. We currently test with 100 local jobs and
  with 10k jobs on a cluster.

* Out of process. The target runs in a separate process. Any crashes in it will
  not affect the fuzzer. Centipede can be used in-process as well, but this mode
  is not the main goal. If your target is small and fast you probably still want
  libFuzzer.

* Integration with the sanitizers is achieved via separate builds. If during
  fuzzing you want to find bugs with
  [ASAN](https://github.com/google/sanitizers/wiki/AddressSanitizer),
  [MSAN](https://github.com/google/sanitizers/wiki/MemorySanitizer), or
  [TSAN](https://github.com/google/sanitizers/wiki/ThreadSanitizer), you will
  need to provide separate binaries for every sanitizer as well as one main
  binary for Centipede itself. The main binary should not use any of the
  sanitizers.

* No part of interface is stable. Anything may change at this stage.

## Terminology

#### Fuzzing engine a.k.a. fuzzer {#fuzzer}

A program that produces an infinite stream of inputs for a target and
orchestrates the execution.

#### Fuzz target {#target}

A binary, a library, an API, or rather anything that can consume bytes for input
and produce some sort of coverage data as an output.
A [libFuzzer](https://llvm.org/docs/LibFuzzer.html)'s
target can be a Centipede's target. Read
more [here](https://github.com/google/fuzzing/blob/master/docs/good-fuzz-target.md)
.

#### Input {#input}

A sequence of bytes that can be fed to a target. The input can be an arbitrary
bag of bytes, or some structured data, e.g. serialized proto.

#### Feature

A number that represents some unique behavior of the target. E.g. a feature
1234567 may represent the fact that a basic block number 987 in the target has
been executed 7 times. When executing an input with the target, the fuzzer
collects the features that were observed during execution.

#### Feature set

A set of features associated with one specific input.

#### Coverage

Some information about the behaviour of the target when it executes a given
input. Coverage is usually represented as feature set that the input has
triggered in the target.

#### Mutator

A function that takes bytes as input and outputs a small random mutation of the
input. See also:
[structure-aware fuzzing](https://github.com/google/fuzzing/blob/master/docs/structure-aware-fuzzing.md)
.

#### Executor {#executor}

A function that knows how to feed an input into a target and get coverage in
return (i.e. to **execute**).

#### Centipede

A customizable fuzzing engine that allows the user to substitute the Mutator and
the Executor.

#### Centipede runner {#runner}

A library that implements the executor interface expected by the Centipede
fuzzer. The runner knows how to run
a [sancov](https://clang.llvm.org/docs/SanitizerCoverage.html)-instrumented
target, collect the resulting coverage, and pass it back to Centipede.
Prospective Centipede fuzz targets can be linked with this library to make them
runnable by Centipede.

#### Corpus (_plural: corpora_)

A set of [inputs](#input).

#### Distillation (creating a _distilled corpus_)

A process of choosing a subset of a larger corpus, such that the subset has the
same coverage features as the original corpus.

#### Shard

A file representing a subset of the corpus and another file representing feature
sets for that same subset of the corpus.

#### Merging shards

To merge shard B into shard A means: for every input in shard B that has
features missing in shard A, add that input to A.

#### Job

A single fuzzer process. One job writes only to one shard, but may read multiple
shards.

#### Workdir or WD

A local or remote directory that contains data produced or consumed by a fuzzer.

## Build Centipede

```shell
git clone https://github.com/google/centipede.git
cd centipede
CENTIPEDE_SRC=`pwd`
BIN_DIR=$CENTIPEDE_SRC/bazel-bin
bazel build -c opt :all
```

What you will need for the subsequent steps:

* `$BIN_DIR/centipede` - the binary of the engine (the fuzzer).
* `$BIN_DIR/libcentipede_runner.pic.a` - the library you need to link with your fuzz target (the runner).
* `$CENTIPEDE_SRC/clang-flags.txt` - recommended clang compilation flags for the target.

You can keep these files where they are or copy them somewhere.

## Build your fuzz target

We provide two examples of building the target: one tiny single-file target and
libpng. Once you've built your target, proceed to the
[fuzz target running step](#run-step).

### The simple example

This example uses one of the simple example fuzz targets, a.k.a. _puzzles_,
included in the Centipede repo.

#### Compile

NOTE: The commands below use the flags from $CENTIPEDE_SRC/clang-flags.txt.
You may choose to use some other set of instrumentation flags:
clang-flags.txt only provides a simple default option.

```shell
FUZZ_TARGET=byte_cmp_4  # or any other source under $CENTIPEDE_SRC/puzzles
clang++ @$CENTIPEDE_SRC/clang-flags.txt -c $CENTIPEDE_SRC/puzzles/$FUZZ_TARGET.cc -o $BIN_DIR/$FUZZ_TARGET.o
```

#### Link

This step links the just-built fuzz target with libcentipede_runner.pic.a and
other required libraries.

```shell
clang++ $BIN_DIR/$FUZZ_TARGET.o $BIN_DIR/libcentipede_runner.pic.a \
    -ldl -lrt -lpthread -o $BIN_DIR/$FUZZ_TARGET
```

Skip to the [running step](#run-step).

### The libpng example

#### Download and compile libpng

```shell

LIBPNG_BRANCH=v1.6.37  # You can experiment with other branches if you'd like
git clone --branch $LIBPNG_BRANCH --single-branch https://github.com/glennrp/libpng.git
cd libpng
CC=clang CFLAGS=@$CENTIPEDE_SRC/clang-flags.txt ./configure --disable-shared
make -j
```

#### Link libpng's own fuzz target with libcentipede_runner.pic.a

```shell
FUZZ_TARGET=libpng_read_fuzzer
clang++ -include cstdlib \
    ./contrib/oss-fuzz/$FUZZ_TARGET.cc \
    ./.libs/libpng16.a \
    $BIN_DIR/libcentipede_runner.pic.a \
    -ldl -lrt -lpthread -lz \
    -o $BIN_DIR/$FUZZ_TARGET
```

## Run Centipede locally {#run-step}

Running locally will not give the full scale, but it could be useful during the
fuzzer development stage. We recommend that both the fuzzer and the target are
copied to a local directory before running in order to avoid stressing a network
file system.

### Prepare for a run

```shell
WD=$HOME/centipede_run
mkdir -p $WD
```

NOTE: You may need to add
[`llvm-symbolizer`](https://llvm.org/docs/CommandGuide/llvm-symbolizer.html)
to your `$PATH` for some of the Centipede functionality to work. The
symbolizer can be installed as part of the [LLVM](https://releases.llvm.org)
distribution:

```shell
sudo apt install llvm
which llvm-symbolizer  # normally /usr/bin/llvm-symbolizer
```

### Run one fuzzing job

```shell
rm -rf $WD/*
$BIN_DIR/centipede --binary=$BIN_DIR/$FUZZ_TARGET --workdir=$WD --num_runs=100
```

See what's in the working directory

```shell
tree $WD
```
```
...
├── <fuzz target name>-d9d90139ee2ccc687f7c9d5821bcc04b8a847df5
│   └── features.0
└── corpus.0
```

### Run 5 concurrent fuzzing jobs

WARNING: Do not exceed the number of cores on your machine for the `--j` flag.

```shell
rm -rf $WD/*
$BIN_DIR/centipede --binary=$BIN_DIR/$FUZZ_TARGET --workdir=$WD --num_runs=100 --j=5
```

See what's in the working directory:

```shell
tree $WD
```
```
...
├── <fuzz target name>-d9d90139ee2ccc687f7c9d5821bcc04b8a847df5
│   ├── features.0
│   ├── features.1
│   ├── features.2
│   ├── features.3
│   └── features.4
├── corpus.0
├── corpus.1
├── corpus.2
├── corpus.3
└── corpus.4
```

## Corpus distillation

Each Centipede shard typically does not cover all features that the entire
corpus covers. In order to distill the corpus, a Centipede process will need to
read all shards. Currently, distillation works like this:

* Run fuzzing as described above, so that all shards have their feature sets
  computed. Stop fuzzing.
* Then, run the same fuzzing jobs, but with `--distill_shards=N`. This will
  cause the first `N` jobs to produce `N` independent distilled corpus files
  (one per job). Each of the distilled corpora should have the same features as
  the full corpus, but the inputs might be very different between these
  distilled corpora.

If you need to also export the distilled corpus to a libFuzzer-style directory
(local dir with one file per input), add `--corpus_dir=DIR`.

## Coverage report

Centipede generates a simple coverage report in a form of a text file. The shard
123 generates a file `workdir/coverage-report-BINARY.000123.txt`
before the actual fuzzing begins, i.e. the report reflects the coverage as
observed by the shard 123 after loading the corpus.

The report shows functions that are fully covered (all control flow edges are
observed at least once), not covered, or partially covered. For partially
covered functions the report contains symbolic information for all covered and
uncovered edges.

The report will look something like this:

```
FULL: FUNCTION_A a.cc:1:0
NONE: FUNCTION_BB bb.cc:1:0
PARTIAL: FUNCTION_CCC ccc.cc:1:0
+ FUNCTION_CCC ccc.cc:1:0
- FUNCTION_CCC ccc.cc:2:0
- FUNCTION_CCC ccc.cc:3:0
```

## Customization

TBD

## Related Reading

* [Centipede Design](doc/DESIGN.md)
