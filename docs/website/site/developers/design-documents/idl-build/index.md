---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: idl-build
title: IDL build
---

[TOC]

The IDL build is a significant source of [generated
files](/developers/generated-files), and a complex use of the build system (GYP
or GN); see their documentation for possible issues.

## Steps

...

## GYP

Components other than bindings do not need to know about the internal
organization of the bindings build. Currently the components `core` and
`modules` depend on `bindings,` specifically `bindings_core` (in
[bindings/core](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/core/))
and `bindings_modules` (in
[bindings/modules](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/modules/)),
due to cyclic dependencies `core` ⇄ `bindings_core` and `modules` ⇄
`bindings_modules`. These should just include
[bindings/core/core.gypi](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/core/core.gypi)
or
[bindings/modules/modules.gypi](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/modules/modules.gypi),
respectively, and depend on top-level targets, currently:

```none
bindings/core/v8/generated.gyp:bindings_core_v8_generated
bindings/modules/v8/generated.gyp:bindings_modules_v8_generated
```

GYP files (.gyp and .gypi) are arranged as follows:

**.gyp:** There's a generated.gyp file in each directory that generates files
(outputting to
`[gen/blink/bindings](https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/blink/bindings/)/$dir`),
namely core, core/v8, modules, and modules/v8; these correspond to global info
(core and modules) and actual V8 bindings (core/v8 and modules/v8). There's also
[scripts/scripts.gyp](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/scripts/scripts.gyp),
for precaching in scripts (lexer/parser tables, compiling templates).

**.gypi:** The .gypi files are named (as usual) as:

```none
foo/foo.gypi        # main, public: list of static files and configuration variables
foo/generated.gypi  # list of generated files
```

There is one bindings-specific complexity, namely idl.gypi files
([core/idl.gypi](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/core/idl.gypi)
and
[modules/idl.gypi](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/modules/idl.gypi)),
due to having to categorize the IDL files for the build (see [Web IDL
interfaces:
Build](http://www.chromium.org/developers/web-idl-interfaces#TOC-Build)).

There are relatively long lists of .gypi includes in the .gyp files, which are
due to factoring for componentization, and are longer than desired due to
layering violations, namely `bindings_core` → `bindings_modules` (Bug
[358074](https://code.google.com/p/chromium/issues/detail?id=358074)): these
make the (bad) dependencies explicit.

## Performance

*For build performance in compiling a single IDL file, see [IDL compiler:
Performance](http://www.chromium.org/developers/design-documents/idl-compiler#TOC-Performance)*

Build performance is one of the [IDL compiler
goals](http://www.chromium.org/developers/design-documents/idl-compiler#TOC-Goals),
though secondary to correctness and performance of the generated code.

The primary concern is minimizing *full build time* (notably full bindings
generation and compilation); the secondary concern is maximizing
*incrementality* of the build (only do full rebuilds when necessary). These
priorities are because full builds are common (e.g., on build bots and for any
compiler code changes), and changes to individual IDL files are also common,
which should only rebuild that file's bindings and any dependents, not trigger a
full rebuild.

Current performance (as of Blink
[r168630](https://src.chromium.org/viewvc/blink?revision=168630) in March 2014)
is acceptable: on a fast Linux workstation with 16 cores, full regeneration
takes ~3 seconds (~50 seconds of user time, ~80 ms/IDL file), and full rebuilds
on IDL changes only occur if a dependency IDL file (partial interface or
implemented interface) is touched, which is infrequent.

The coarsest way to profile a full build – which is sufficient for verifying
coarse improvements – is to do a build (using ninja), then `touch
idl_compiler.py` and [`time(1)`](http://en.wikipedia.org/wiki/Time_(Unix))
another build (most finely the targets should be `bindings_core_v8_generated
bindings_modules_v8_generated` but (for ninja) it's fine for the target to be
`chrome`); "user time" is most relevant. This should only rebuild the bindings,
but not recompile or do any other steps, like linking. Note that build system
(like ninja) has some overhead, so you want to compare to an empty build, and
that touching some other files may trigger other steps.

In the overall build, bindings compilation and linking are more significant
factors, but are difficult to improve: minimizing includes and minimizing and
simplifying generated code are the main approaches.

The main improvement would be precise computation of dependencies to minimize
full rebuilds, so only dependent files are rebuild if a dependency is touched
(Bug [341748](https://code.google.com/p/chromium/issues/detail?id=341748)), but
this would require GYP changes.

### Details

IDL build performance contains various components:

*   *(Preliminary):* Build file generation (GYP runtime)
*   Build system time (ninja runtime)
*   Overall IDL file recompile time (including auxiliary steps and
            parallelization)
*   Individual IDL file compile time (bindings generation time, IDL →
            C++)
*   C++ compile time (bindings compile time, C++ → object code)

Build performance criteria of the IDL build are the time for the following
tasks:

*   Generate build files (gyp time)
*   Run build system (both empty build and overhead on regeneration or
            recompile)
*   Generate all bindings
*   Compile all generated bindings
*   Incremental regeneration
*   Incremental recompile

The key techniques to optimize these are:

*   Generate build files: avoid computations in `.gyp` files,
            particularly *O*(*n*2) ones
*   Run build system: minimize size and complexity of generated build
            files (avoid excess dependencies)
*   Generate all bindings: parallelize build, and optimize individual
            IDL compilation time
*   Compile all bindings: **FIXME ???** *(minimize includes, shard
            aggregated bindings?)*
*   Incremental regeneration: compute dependencies precisely (to avoid
            full regeneration)
*   Incremental recompile: **FIXME ???**

Compilation of multiple IDL files is embarrassingly parallel, as files are
almost completely independent (code generation is independent, and reading in is
independent other than reading in implemented or referenced interfaces, which is
currently minor, and global information, which is handled in a separate step).
Thus with enough cores or a distributed build, compilation is very fast:
assuming one core per IDL file, full recompile should take 0.1 seconds, plus
distribution overhead.

### Further optimizations

The main areas at the overall build level that suggest further optimization are
as follows. These would not help significantly with full rebuilds, but would
improve incrementality. These are thus possible, but not high priorities.

*   Precise computation of dependencies, rather than conservative
            computation (Bug XXX)

This would minimize full rebuilds, and decrease the size of generated build
files: currently if a dependency IDL file (partial interface or implemented
interface) is touched, *all* bindings are regenerated (conservatively). Instead,
precise computation of dependencies would mean only the dependent files are
rebuilt, so 1 or 2 files instead of 600. Further, this would reduce the size of
the generated build files, because instead of the conservative list of about 60
dependencies x 600 main IDL files = 36,000 dependencies, this would have about
100 (some dependencies are implemented interfaces that are used multiple times);
this would speed up build time due to lower overhead of reading and checking
these dependencies.

This would require new features in GYP, to allow per-file dependency computation
in rules.

*   Split global computations as individual compute public information
            steps and a consolidation step

This would speed up incremental builds a bit, by minimizing the global
computation, but may slow down full rebuilds significantly, due to launching
*O*(*n*) processes.

Currently global computations are done in a single step, which processes all IDL
files. Thus touching any IDL file requires reading all of them to recompute the
global data. This could be replaced by a 2-step processes: compute the public
data one file at a time (*n* processes), then have a consolidation step that
reads these all in and produces the overall global data. Further, if the public
data does not change (as is usually the case), the consolidation step need not
run. This would thus speed up incremental builds. However, this would require
*n* additional processes, so it would slow down full rebuilds.

### Cautions

Certain mistakes that significantly degrade build performance are easily made;
all of these have either occurred or been proposed.

*   Touching or regenerating global dependency targets

Be careful to use "write only on diff" (correctly) for all global dependency
targets.

Touching a global dependency (a file or target that is a dependency of all IDL
files) causes a full recompile: if a global dependency changes, *all* IDL files
must be recompiled. Global dependency files are primarily the compiler itself,
but also (for now) all dependency IDL files, since we compute dependencies
conservatively in the build, not precisely. Global dependency targets (files
generated in the preliminary steps) are primarily the global context, but also
the generated global constructor IDL files, as these are partial interfaces
(hence dependencies, hence treated as global dependencies). Touching any IDL
file requires recomputing the global context and these generated global files.
However, most changes to IDL files do not change the global dependencies (global
context and global constructors), since they only change private information,
and thus should not trigger full rebuilds. The "write only on diff" option,
supported by some build systems, notably ninja, means that if these files do not
change, we do not regenerate the targets, hence avoiding a full rebuild. If this
is missed, touching any IDL file cause full regeneration.

*   Excess processes :: source of *O*(*n*) process overhead

Avoid launching processes per-IDL file: preferably only 1 process for the actual
compilation, avoiding per-file caching steps.

Launching processes is relatively expensive, particularly on Windows. For global
steps this is *O*(1) in the number of IDL files, so adding an extra global build
step is cheap, but for compiling individual IDL files this is *O*(*n*), so it's
faster to have 1 process per IDL file, i.e., compile each file in a single step.
Thus any changes that add a process per IDL file are expensive. These include:
splitting global computations into "compute per file one process at a time, then
consolidate in one step" (turns *O*(*n*) computation in 1 process into *O*(*n*)
computation in *n* + 1 processes); splitting the compile into "compute IR, then
compile IR" (turns *n* processes into 2*n* processes).

*   Doing redundant work :: source of *O*(*n*2) algorithms

Compute global data in a single preliminary step.

Computed data that is needed by more than one IDL file should generally be done
a single time, most simply in the global context computation step (or a GYP
variable); getting this wrong can turn *O*(*n*) work into O(*n*2) work. For
example, public information about an interface should be computed once, rather
than computing it every time it is used.

However, in some cases redundant work is preferable to adding extra processes.
For example, there is some redundancy in the front end, since some files
(notably implemented interfaces) are read during the compile of several IDL
files, but this is relatively rare (about 30/600=5% of files are read multiple
times) and cheap (actual parsing is cheap relative to parser initialization),
and fixing it would require a separate per-file caching step, which would
increase the number of processes.

*   *O*(*nk*) and *O*(*n*2) work or data in build files

Use precise (not conservative) computations and static lists in GYP, and be
careful of GYP rules.

GYP rules generate actions; for the IDL build this is the `binding` rule in the
`individual_generated_bindings` target, which generates one action per IDL file.
Thus any work this does or data this generates is scaled by *n*. This is an
issue for dependencies: the `inputs` are used for all generated actions, and
thus currently include all dependency files, which generates *O*(*nk*) data
(currently about 60 dependencies x 600 main IDL files = 36,000 input lines in
the actions, instead of ~100 (some dependencies are implemented interfaces that
are used multiple times)), which slows down both gyp runtime and the build
(ninja) runtime, as it needs to read and check all these dependencies. Further,
any *O*(*n*) work in the inputs, notably calling a Python script that runs a
global computation, results in *O*(*n*2) work at gyp runtime.

See also [350317](https://code.google.com/p/chromium/issues/detail?id=350317):
Reduce size of individual_generated_bindings.ninja​​.

Work can be reduces to *O*(1) if instead of using a Python script to compute
dependencies dynamically (at gyp runtime), one determines the dependencies
statically (when writing the `.gyp, .gypi` files). This is a key reason that
there are multiple variables with lists of IDL files: this allows gyp to just
expand the variables, rather than running a computation (and obviates the need
for a separate Python script).

### Rejected optimizations

Some intuitively appealing optimizations don't actually work well or at all;
these are discussed here. These generally have little impact (or negative
impact) and involve significant complexity in the build or compiler.

#### Compile multiple files in a single process (in sequence or via multiprocessing)

Python and library initialization can be further sped up by processing multiple
IDL files in a single compiler run (a single compilation process). However, this
significantly complicates the build due to needing to manually distribute the
input files to various build steps, ideally one per available core. This
interacts poorly with GYP (GYP rules are designed for a single input file and
the corresponding output), and is only useful if you cannot fully distribute the
build, and thus is not done.

Note that
[multiprocessing](http://docs.python.org/2/library/multiprocessing.html) or
multithreading Python would not allow a single Python process to compile all
files in parallel (thus minimizing initialization time), since the Python
interpreter is not concurrency safe (hence not multithreaded; concretely this is
due to the [Global Interpreter
Lock](http://docs.python.org/2/glossary.html#term-global-interpreter-lock),
GIL), and the actual parsing and templating are done in Python. A
multiprocessing Python process could launch separate (OS) processes to compile
individual files, but this offers no advantage over the build system doing it,
and is more complex.

#### Fork multiple processes

One could start a single Python process, which then forks others once it has
initialized to reduce startup time (both of Python and of the libraries): either
one per input file or distributing among them. This allows both parallelization
and minimal initialization, so it would increase speed. However, this is
platform dependent (`fork()` is not available on Windows, and would need to be
replaced by `spawn()` or
[`subprocess`](http://docs.python.org/2/library/subprocess.html)), and would
move substantial build complexity into the compiler from the build system, hence
avoided since this level of performance is not required.

#### Cache IR

The IR is computed multiple times for a few IDL files, namely implemented
interfaces that are implemented by several IDL interfaces and targets of
`[PutForwards]`. This is redundant work, so caching them would avoid this
duplication. However, this would require splitting compilation into two steps –
generate IR in one step, then compile IR in the second – and thus double the
number of processes, which would slow down the build significantly, particularly
on Windows. This can be worked around, at the expense of complicating the build,
by only doing the caching for dependencies (or indeed only for implemented
interfaces) and then checking for a cached IR file before reading the IDL file
onself, but this is significant complexity for little benefit.

#### Compute public dependencies precisely

Currently if the *global* context changes, *all* bindings are generated. This is
conservative and simple, which is why we do it this way, but it causes excess
full rebuilds: if the public data of one IDL file changes (e.g., its
`[ImplementedAs]`), only bindings that actually depend on that public
information (namely: use that interface) need to be rebuilt. Thus if IDL files
depended on the public information of individual files, rather than on the
global context, we could have more incremental rebuilds.

This is not done because it would add significant complexity for little benefit
(changes to public data are rare, and generally require a significant rebuild
anyway), and would require GYP changes (above) for file-by-file dependency
computation in rules.

### References

*   [Issue 341748: Improve bindings
            build](https://code.google.com/p/chromium/issues/detail?id=341748):
            tracking bug for build rewrite (2014)
