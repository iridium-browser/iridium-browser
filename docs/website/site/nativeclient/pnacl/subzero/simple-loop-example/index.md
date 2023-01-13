---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/pnacl
  - PNaCl
- - /nativeclient/pnacl/subzero
  - Subzero
page_name: simple-loop-example
title: Simple loop example
---

Here we show the steps that Subzero takes in translating a simple loop example.

For translating this example to native code, we assume a fairly constrained
platform: x86-32 with only 3 registers (eax, ecx, edx) available for local
register allocation, and no global register allocation.

Start with the following C example, which computes the reduction of an integer
array.

```none
int simple_loop(int *a, int n) {
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    sum += a[i];
  }
  return sum;
}
```

The corresponding PNaCl bitcode after compiling with `pnacl-clang -O2` is the
following:

```none
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

Generate high-level Vanilla ICE, including lowering phi instructions. Where
possible, v_\*_phi assignments are pushed up to just past their last use in the
basic block, and past a compare/branch that ends the basic block.

```none
simple_loop(v_a, v_n):
l_entry:
  v_i_phi = 0
  v_sum_phi = 0
  v_sum2_phi = 0
  v_c = icmp sgt v_n, 0
  br v_c, l_body, l_end
l_body:
  //v_i = phi [v_inc, l_body], [0, l_entry]
  v_i = v_i_phi
  //v_sum = phi [v_add, l_body], [0, l_entry]
  v_sum = v_sum_phi
  v_t1 = mul v_i, 4
  v_t2 = add v_a, v_t1
  v_elt = load v_t2
  v_add = add v_elt, v_sum
  v_sum_phi = v_add
  v_sum2_phi = v_add
  v_inc = add v_i, 1
  v_i_phi = v_inc
  v_c2 = icmp slt v_inc, v_n
  br v_c2, l_body, l_end
l_end:
  //v_sum2 = phi [0, l_entry], [v_add, l_body]
  v_sum2 = v_sum2_phi
  ret v_sum2
```

Liveness analysis determines which source operands in each instruction are the
end of the operand's live range. As a bonus, it also determines the set of live
operands at the beginning of each basic block. If liveness analysis is omitted,
we assume all operands are live at the beginning of every basic block, and no
operands' live ranges end in any instruction. The results are the `LIVE`
annotations at the start of each basic block, and the `LIVEEND` annotations on
each instruction.

```none
simple_loop(v_a, v_n):
l_entry: // LIVE={v_a,v_n}
  v_i_phi = 0
  v_sum_phi = 0
  v_sum2_phi = 0;
  v_c = icmp sgt v_n, 0
  br v_c, l_body, l_end // LIVEEND={v_c}
l_body: // LIVE={v_i_phi,v_sum_phi,v_a,v_n}
  v_i = v_i_phi              // LIVEEND={v_i_phi}
  v_sum = v_sum_phi          // LIVEEND={v_sum_phi}
  v_t1 = mul v_i, 4
  v_t2 = add v_a, v_t1       // LIVEEND={v_t1}
  v_elt = load v_t2          // LIVEEND={v_t2}
  v_add = add v_elt, v_sum   // LIVEEND={v_elt,v_sum}
  v_sum_phi = v_add
  v_sum2_phi = v_add         // LIVEEND={v_add}
  v_inc = add v_i, 1         // LIVEEND={v_i}
  v_i_phi = v_inc
  v_c2 = icmp slt v_inc, v_n // LIVEEND={v_inc}
  br v_c2, l_body, l_end     // LIVEEND={v_c2}
l_end: // LIVE={v_sum2_phi}
  v_sum2 = v_sum2_phi // LIVEEND={v_sum2_phi}
  ret v_sum2          // LIVEEND={v_sum2}
```

Addressing mode inference (x86 only) identifies operands that pattern-match to
an expression suitable to the special x86 addressing modes. The results are an
annotation on the use of the calculated address, as well as annotations on the
uses of the core operands as the subexpressions are built up into the calculated
address. These annotations are labeled `ADDROPT` and `ADDR`, respectively. The
code selector makes use of the `ADDROPT` annotation, and the local register
manager takes the `ADDR` annotations into account when assigning registers.

```none
simple_loop(v_a, v_n):
l_entry: // LIVE={v_a,v_n}
  v_i_phi = 0
  v_sum_phi = 0
  v_sum2_phi = 0
  v_c = icmp sgt v_n, 0
  br v_c, l_body, l_end // LIVEEND={v_c}
l_body: // LIVE={v_i_phi, v_sum_phi, v_a, v_n}
  v_i = v_i_phi              // LIVEEND={v_i_phi}
  v_sum = v_sum_phi          // LIVEEND={v_sum_phi}
  v_t1 = mul v_i, 4          // ADDR={v_i}
  v_t2 = add v_a, v_t1       // LIVEEND={v_t1}, ADDR={v_a}
  v_elt = load v_t2          // LIVEEND={v_t2}, ADDROPT=v_a+4*v_i+0
  v_add = add v_elt, v_sum   // LIVEEND={v_elt, v_sum}
  v_sum_phi = v_add
  v_sum2_phi = v_add         // LIVEEND={v_add}
  v_inc = add v_i, 1         // LIVEEND={v_i}
  v_i_phi = v_inc
  v_c2 = icmp slt v_inc, v_n // LIVEEND={v_inc}
  br v_c2, l_body, l_end     // LIVEEND={v_c2}
l_end: // LIVE={v_sum2_phi}
  v_sum2 = v_sum2_phi // LIVEEND={v_sum2_phi}
  ret v_sum2          // LIVEEND={v_sum2}
```

Instruction selection includes local register allocation, using 3 registers but
calling them `r1`, `r2`, and `r3` so that the specific mappings to `eax`, `ecx`,
and `edx` can be deferred. In the code below, for each instruction, we retain
the originating Vanilla ICE instruction in a comment for clarity, and each
low-level ICE instruction is annotated with the local register manager state.
This state includes the LRU order (the first register listed is the next
register to be given out), and the set of operands and constants available in
each register. When the instruction selection code works with the local register
manager, the selector may take advantage of operator commutativity to minimize
the instruction expansion (see the highlighted instruction).

```none
simple_loop(v_a, v_n):
l_entry: // LIVE={v_a,v_n}, r1={}, r2={}, r3={}
  // v_i_phi = 0
  mov r1, 0
  mov v_i_phi, r1 // r2={}, r3={}, r1={0,v_i_phi}
  // v_sum_phi = 0
  mov r2, r1
  mov v_sum_phi, r2 // r3={}, r1={0,v_i_phi}, r2={0,v_sum_phi}
  // v_sum2_phi = 0
  mov r3, r1
  mov v_sum2_phi, r3 // r1={0,v_i_phi}, r2={0,v_sum_phi}, r3={0,v_sum2_phi}
  cmp v_n, 0
  ble l_end
  br l_body
l_body: // LIVE={v_i_phi, v_sum_phi, v_a, v_n}, r1={}, r2={}, r3={}
  //v_i = v_i_phi // LIVEEND={v_i_phi}
  mov r1, v_i_phi // r2={}, r3={}, r1={v_i_phi}
  mov v_i, r1 // r2={}, r3={}, r1={v_i}
  //v_sum = v_sum_phi // LIVEEND={v_sum_phi}
  mov r2, v_sum_phi // r3={}, r1={v_i}, r2={v_sum_phi}
  mov v_sum, r2 // r3={}, r1={v_i}, r2={v_sum}
  //v_t1 = mul v_i, 4 // ADDR={v_i}
  mov r3, r1
  shl r3, 2
  mov v_t1, r3 // r2={v_sum}, r3={v_t1}, r1={v_i}
  //v_t2 = add v_a, v_t1 // LIVEEND={v_t1}, ADDR={v_a}
  mov r2, v_a // r3={v_t1}, r1={v_i}, r2={v_a}
  add r3, r2
  mov v_t2, r3 // r3={v_t2}, r1={v_i}, r2={v_a}
  //v_elt = load v_t2 // LIVEEND={v_t2}, ADDROPT=v_a+4*v_i+0
  mov r3, [r2+4*r1]
  mov v_elt, r3 // r1={v_i}, r2={v_a}, r3={v_elt}
  //v_add = add v_elt, v_sum // LIVEEND={v_elt, v_sum}
  add r3, v_sum
  mov v_add, r3 // r1={v_i}, r2={v_a}, r3={v_add}
  //v_sum_phi = v_add // PHI
  mov v_sum_phi, r3 // r1={v_i}, r2={v_a}, r3={v_add,v_sum_phi}
  //v_sum2_phi = v_add // PHI, LIVEEND={v_add}
  mov v_sum2_phi, r3 // r1={v_i}, r2={v_a}, r3={v_add,v_sum_phi,v_sum2_phi}
  //v_inc = add v_i, 1 // LIVEEND={v_i}
  add r1, 1
  mov v_inc, r1 // r2={v_a}, r3={v_add,v_sum_phi,v_sum2_phi}, r1={v_inc}
  //v_i_phi = v_inc // PHI
  mv v_i_phi, r1 // r2={v_a}, r3={v_add,v_sum_phi,v_sum2_phi}, r1={v_inc,v_i_phi}
  //v_c2 = icmp slt v_inc, v_n // LIVEEND={v_inc}
  cmp r1, v_n
  //br v_c2, l_body, l_end // LIVEEND={v_c2}
  blt l_body
  br l_end
l_end: // LIVE={v_sum2_phi}, r1={}, r2={}, r3={}
  //v_sum2 = v_sum2_phi // LIVEEND={v_sum2_phi}
  mov r1, v_sum2_phi
  mov v_sum2, r1 // r1={v_sum2_phi,v_sum2}
  //ret v_sum2 // LIVEEND={v_sum2}
  mov eax, r1
  ret
```

Multiblock register allocation starts by identifying register/operand candidates
(`CAND`) for each basic block in which the operand is live on entry and is the
first value assigned to the register in the basic block. These candidates are
used in conjunction with physical register preferences to decide on mappings
between `r1/r2/r3` and `eax/ecx/edx` in each basic block. For example, `mov eax,
r1` creates a preference for `r1` to be `eax` in the last basic block, and that
preference propagates when possible to the predecessor basic blocks. Edges may
need to be split to add compensation code when the preferences are
overconstrained.

```none
simple_loop(v_a, v_n):
l_entry: // LIVE={v_a,v_n}, r1={}, r2={}, r3={}
// CAND={}
// REG={r3:eax, r1:ecx,r2:edx}
  mov r1, 0
  mov v_i_phi, r1 // r2={}, r3={}, r1={0,v_i_phi}
  mov r2, r1
  mov v_sum_phi, r2 // r3={}, r1={0,v_i_phi}, r2={0,v_sum_phi}
  mov r3, r1
  mov v_sum2_phi, r3 // r1={0,v_i_phi}, r2={0,v_sum_phi}, r3={0,v_sum2_phi}
  cmp v_n, 0
  ble l_end
  br l_body
l_body_split1:
  mov edx, eax // mov r2:v_sum_phi, r3:v_sum_phi
l_body: // LIVE={v_i_phi, v_sum_phi, v_a, v_n}, r1={}, r2={}, r3={}
// CAND={v_i_phi:r1,v_sum_phi:r2}
// REG={r3:eax, r1:ecx,r2:edx}
  ////mov r1, v_i_phi // r2={}, r3={}, r1={v_i_phi}
  mov v_i, r1 // r2={}, r3={}, r1={v_i}
  ////mov r2, v_sum_phi // r3={}, r1={v_i}, r2={v_sum_phi}
  mov v_sum, r2 // r3={}, r1={v_i}, r2={v_sum}
  mov r3, r1
  shl r3, 2
  mov v_t1, r3 // r2={v_sum}, r3={v_t1}, r1={v_i}
  mov r2, v_a // r3={v_t1}, r1={v_i}, r2={v_a}
  add r3, r2
  mov v_t2, r3 // r3={v_t2}, r1={v_i}, r2={v_a}
  mov r3, [r2+4*r1]
  mov v_elt, r3 // r1={v_i}, r2={v_a}, r3={v_elt}
  add r3, v_sum
  mov v_add, r3 // r1={v_i}, r2={v_a}, r3={v_add}
  mov v_sum_phi, r3 // r1={v_i}, r2={v_a}, r3={v_add,v_sum_phi}
  mov v_sum2_phi, r3 // r1={v_i}, r2={v_a}, r3={v_add,v_sum_phi,v_sum2_phi}
  add r1, 1
  mov v_inc, r1 // r2={v_a}, r3={v_add,v_sum_phi,v_sum2_phi}, r1={v_inc}
  mov v_i_phi, r1 // r2={v_a}, r3={v_add,v_sum_phi,v_sum2_phi}, r1={v_inc,v_i_phi}
  cmp r1, v_n
  blt l_body_split1
  br l_end
l_end: // LIVE={v_sum2_phi}, r1={}, r2={}, r3={}
// CAND={v_sum2_phi:r1}
// REG={r1:eax, r2:ecx,r3:edx}
  ////mov r1, v_sum2_phi
  mov v_sum2, r1 // r1={v_sum2_phi,v_sum2}
  mov eax, r1
  ret
```

After physical register substitution:

```none
simple_loop(v_a, v_n):
l_entry: // LIVE={v_a,v_n}, r1={}, r2={}, r3={}
// CAND={}
// REG={r3:eax, r1:ecx,r2:edx}
  mov ecx, 0
  mov v_i_phi, ecx // edx={}, eax={}, ecx={0,v_i_phi}
  mov edx, ecx
  mov v_sum_phi, edx // eax={}, ecx={0,v_i_phi}, edx={0,v_sum_phi}
  mov eax, ecx
  mov v_sum2_phi, eax // ecx={0,v_i_phi}, edx={0,v_sum_phi}, eax={0,v_sum2_phi}
  cmp v_n, 0
  ble l_end
  br l_body
l_body_split1:
  mov edx, eax
l_body: // LIVE={v_i_phi, v_sum_phi, v_a, v_n}, r1={}, r2={}, r3={}
// CAND={v_i_phi:r1,v_sum_phi:r2}
// REG={r3:eax, r1:ecx,r2:edx}
  mov v_i, ecx // edx={}, eax={}, ecx={v_i}
  mov v_sum, edx // eax={}, ecx={v_i}, edx={v_sum}
  mov eax, ecx
  shl eax, 2
  mov v_t1, eax // edx={v_sum}, eax={v_t1}, ecx={v_i}
  mov edx, v_a // eax={v_t1}, ecx={v_i}, edx={v_a}
  add eax, edx
  mov v_t2, eax // eax={v_t2}, ecx={v_i}, edx={v_a}
  mov eax, [edx+4*ecx]
  mov v_elt, eax // ecx={v_i}, edx={v_a}, eax={v_elt}
  add eax, v_sum
  mov v_add, eax // ecx={v_i}, edx={v_a}, eax={v_add}
  mov v_sum_phi, eax // ecx={v_i}, edx={v_a}, eax={v_add,v_sum_phi}
  mov v_sum2_phi, eax // ecx={v_i}, edx={v_a}, eax={v_add,v_sum_phi,v_sum2_phi}
  add ecx, 1
  mov v_inc, ecx // edx={v_a}, eax={v_add,v_sum_phi,v_sum2_phi}, ecx={v_inc}
  mov v_i_phi, ecx // edx={v_a}, eax={v_add,v_sum_phi,v_sum2_phi}, ecx={v_inc,v_i_phi}
  cmp ecx, v_n
  blt l_body_split1
l_end: // LIVE={v_sum2_phi}, r1={}, r2={}, r3={}
// CAND={v_sum2_phi:r1}
// REG={r1:eax, r2:ecx,r3:edx}
  mov v_sum2, eax // eax={v_sum2_phi,v_sum2}
  mov eax, eax
  ret
```

Dead instruction elimination cleans up dead stores and no-longer-necessary
calculations of intermediate components of addressing modes.

```none
simple_loop(v_a, v_n):
l_entry:
  mov ecx, 0
  mov edx, ecx
  mov eax, ecx
  cmp v_n, 0
  ble l_end
  br l_body
l_body_split1:
  mov edx, eax
l_body:
  mov v_sum, edx
  mov edx, v_a
  mov eax, [edx+4*ecx]
  add eax, v_sum
  add ecx, 1
  cmp ecx, v_n
  blt l_body_split1
l_end:
  ret
```

In this example, there are no clear peephole optimizations available, and since
only one stack operand is left, there is no need for local variable coalescing.
Thus the difference between the code above and the actual code emitted would
involve:

*   Frame setup in the prolog
*   Frame destruction in the epilog
*   Substituting stack frame addressing modes where generic stack
            operands appear
*   Loop alignment as desired
*   SFI sequences
*   Bundling
