---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/pnacl
  - PNaCl
page_name: experimenting-with-generated-bitcode
title: Experimenting with generated bitcode
---

Sample code (note the `noinline` attribute - makes it simpler to locate the
code, otherwise it will get inlined all the way into `_start`):

```none
int __attribute__((noinline)) simple_loop(int *a, int n) {
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    sum += a[i];
  }
  return sum;
}
int main(int argc, char** argv) {
  return simple_loop((int*)argv[0], argc);
}
```

Will be using the canary SDK here, so set `NACL_SDK_ROOT` to point to an install
of the NaCl SDK` /pepper_canary` directory.

Run the compiler to produce optimized bitcode:

```none
$NACL_SDK_ROOT/toolchain/linux_pnacl/bin/pnacl-clang ~/Downloads/loop.c -O2 -o loop.o2.bc
```

Note: this is LLVM bitcode:

```none
$ $NACL_SDK_ROOT/toolchain/linux_pnacl/bin/pnacl-dis --file-type loop.o2.bc 
loop.o2.bc: LLVM bitcode
```

We'll keep it in LLVM bitcode format, because the NaCl bitcode writer/reader
can't handle some of the types removed by stripping. So fully finalizing (into a
stable pexe) a non-stripped file doesn't currently work.

```none
$ $NACL_SDK_ROOT/toolchain/linux_pnacl/bin/pnacl-dis loop.o2.bc |grep -A20 "define internal i32 @simple_loop"
define internal i32 @simple_loop(i32 %a, i32 %n) {
entry:
  %cmp4 = icmp sgt i32 %n, 0
  br i1 %cmp4, label %for.body, label %for.end
for.body:                                         ; preds = %for.body, %entry
  %i.06 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %sum.05 = phi i32 [ %add, %for.body ], [ 0, %entry ]
  %gep_array = mul i32 %i.06, 4
  %gep = add i32 %a, %gep_array
  %gep.asptr = inttoptr i32 %gep to i32*
  %0 = load i32* %gep.asptr, align 1
  %add = add i32 %0, %sum.05
  %inc = add i32 %i.06, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.end
for.end:                                          ; preds = %for.body, %entry
  %sum.0.lcssa = phi i32 [ 0, %entry ], [ %add, %for.body ]
  ret i32 %sum.0.lcssa
}
```

Finally, translate that into a .nexe in -O0 mode, asking `pnacl-translate` to
accept LLVM bitcode explicitly (will be refused otherwise):

```none
$ $NACL_SDK_ROOT/toolchain/linux_pnacl/bin/pnacl-translate loop.o2.bc -o loop.x64.nexe -arch x86-64 -O0 --allow-llvm-bitcode-input
```

Use `objdump` to examine the produced nexe.
