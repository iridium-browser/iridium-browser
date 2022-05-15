# Wuffs Mirror (release/c)

This repository mirrors a subset of the
[Wuffs](https://github.com/google/wuffs) (Wrangling Untrusted File Formats
Safely) repository.

Wuffs is both a programming language (Wuffs-the-language) and a standard
library (Wuffs-the-library) written in that language. The Wuffs toolchain
converts from Wuffs-the-language to C and Wuffs-the-library is available as a
single C file - a [header file
library](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt).

People who just want to *use* Wuffs-the-library in its C form (instead of
*modifying* its Wuffs form or otherwise needing Wuffs-the-language tools) only
need that one C file.

This repository's [`release/c`](./release/c) directory contains that C file (at
various versions). It mirrors the `release/c` directory of the [original Wuffs
repository](https://github.com/google/wuffs) (except for the "unsupported
snapshot" version). By excluding everything else, this repository is much
smaller and changes much less frequently.


## Updates

This repository's `release/c` directory is manually updated by the
`script/sync.sh` shell script, which also logs to [`sync.txt`](./sync.txt).


## Disclaimer

This is not an official Google product, it is just code that happens to be
owned by Google.


---

Updated on March 2021.
