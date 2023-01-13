---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: how-to-write-assembler-for-x86-nacl-platform
title: How to write assembler for x86 NaCl platform.
---

Before we'll go any further let me remid you that NaCl is OS-independent but
CPU-dependent technology while pNaCl is both OS-independent and CPU-independent
technology. Assembler is, of course, CPU-dependent and as such is not suitable
for pNaCl.

Ok. So we are dealing with NaCl and want to write some kind of assembler
program.

First of all, I don't plan to explain here how to write your program entirley in
assembler languge.

It's doable, but quite hard and rarely needed. I'll concentrate on a case of
C/C++ program “with some assebler on the side”.

Ok, so we want to call some assembler instruction not available via intrinsic.
Let's use cpuid for our simple example.

$ cat cpuid.c

#include &lt;stdint.h&gt;

#include &lt;stdio.h&gt;

int main() {

volatile uint32_t reg\[4\];

__asm__ __volatile__(

"cpuid"

: "=a"(reg\[3\]), "=b"(reg\[0\]), "=c"(reg\[2\]), "=d"(reg\[1\])

: "a"(0)

: "cc");

printf("%s\\n", (char \*)reg);

}

$ pepper_33/toolchain/linux_x86_newlib/bin/i686-nacl-gcc cpuid.c -o
cpuid-32.nexe

$ pepper_33/toolchain/linux_x86_newlib/bin/x86_64-nacl-gcc cpuid.c -o
cpuid-64.nexe

$ pepper_33/tools/sel_ldr.py cpuid-32.nexe

DEBUG MODE ENABLED (bypass acl)

GenuineIntel

$ pepper_33/tools/sel_ldr.py cpuid-64.nexe

DEBUG MODE ENABLED (bypass acl)

GenuineIntel

Looks like we can easily call cpuid from our NaCl program. How cool is that?
Well, cool enough to write some simple program. In 32 bit mode. If you are
lucky. In 64 bit mode. If you **very** lucky.

Why? It's easy: only provably safe code can be executed on NaCl platform. And
sooner or later you'll write code which will be declared UNSAFE by NaCl and
it'll refuse to run it. In particular NaCl does not allow you to use arbitrary
instructions in your code. Only “safe” instructions are allowed.

Where is the list of safe instructions? [Here it
is.](https://codereview.chromium.org/49183002/patch/2530001/2540004) This file
is created automatically and lists all single instructions allowed in x86 32 bit
NaCl mode. If you use instructions from it - you are golden, if you use
instructions not mentioned there then validator will reject your code.

There are a lot of instructions in this list and you can do a lot of things
using them, but couple of instructions are suspiciously missing: call and ret.
Well, call is there but only as call %eip (which basically means “call with any
32bit offset” since script used to generate said list always uses offset equal
to zero for the simplification), but ret is not present at all—and this **not**
an error in script!

Why would you need a call without ret? How could you return from function? And
can you ever call function indirecly? Contemporary technologies use function
pointers (in different forms) quite extensively. The answer to this question
brings us to a *superinstruction* notion.

In 32 bit case there are exactly two *superinstructions*: naclcall and nacljmp.
They can be used with any 32 bit general purpose register (and only register,
never memory!) to do an indirect jump. And i686-nacl-as also gives you the
naclret macro which simply calls pop %ecx and then nacljmp %ecx (%ecx is picked
because it's neither caller-saved register not callee-saved register in x86 ELF
ABI).

There are nothing magical in naclret, but naclcall and nacljmp **are** magical.
How come? Let's see:

$ cat nacljmp.s

nacljmp %eax

$ pepper_33/toolchain/linux_x86_newlib/bin/i686-nacl-as nacljmp.s -o nacljmp.o

$ pepper_33/toolchain/linux_x86_newlib/bin/i686-nacl-objdump -d nacljmp.o

nacljmp.o: file format elf32-i386-nacl

Disassembly of section .text:

00000000 &lt;.text&gt;:

0:      83 e0 e0        and $0xffffffe0,%eax

3:      ff e0   jmp \*%eax

As you can see this *superinstruction* actually combines two different
instructions: and and jmp. This combination guarantees that target address for
nacljmp is always aligned: you can not use nacljmp (or naclcall) to jump in the
middle of 32-byte *bundle*. And i686-nacl-as guarantees that instructions in
your code will never straggle boundary of such *bundle*. These two facts
combined mean that code can be **statically** disassebled and verified. Which in
turn means that NaCl validator does not effect performance of your code at all:
it does it's work once, and then your code is executed by CPU directly without
additional overhead (bundles and lack of ret will create some small overhead, of
course, but it's very small). That's really, really cool. There is one tiny
problem though: what happens if address which you are using as a target is not
actually aligned? IOW: how can call work in this scheme. The answer is simple:
call is magical in i686-nacl-as, too (and naclcall is doubly magical):
i686-nacl-as always moves it to the end of bundle which means that address in
stack is properly aligned.

And now our description of 32 bit nacl assembler is essentially complete. You
have the list of safe instructions, you use naclcall, nacljmp and naclret where
needed and that's it.

Now we are switching to 64 bit case. Before we'll discuss it we need to think a
bit. All our manipulations work if **all** the code in your process adheres to
the NaCl rules… but not all code in NaCl process image are like that! NaCl
loaders include bunch of code compiled with a normal (non-NaCl) compiler and,
more importantly, it's linked with system libraries which most definitely can
not be recompiled with NaCl compiler (well… under Linux they could, but this
approach will not work with MacOS or Windows). One jump to stray aligned ret—and
the whole system is compromised. How **that** problem is solved? In 32 bit case
the answer is simple: ***segment registers***! *Segment registers* are used to
limit the reach of your code and that means that *untrusted code*, indeed, can
not call *trusted code* (at least directly). There are some tiny pieces of
*trusted code* injected in *untrusted zone* (*trampolines* and *springboards*)
which are used to manage communication between trusted code and untrusted code
but you never deal with these directly.

But in 64 bit case this plan will not work! There are no segments in 64 bit
mode!

Right - and this is why 64 bit mode is significantly more involved and
complicated. [Here is the list of safe
instructions](https://codereview.chromium.org/49183002/patch/2530001/2540005).
Let's take a look on a simple mov instruction. In 32 bit mode it was described
as

Instruction: 89 XX mov \[%eax..%edi\],\[%eax..%edi or memory\]

Very simple instruction straight from Intel's (or AMD) manual. Any 32 bit
general-purpose register can be moved to general-purpose register or memory. In
64 bit case the same instruction is described quite differently:

Instruction: \[REX:40..47\]? 89 XX mov \[%eax..%r15d\],\[%eax..%ebx|%esi..%r14d
or memory\]

That is: any 32 bit register can be moved to any 32bit register… except for
%rbp, %rsp and %r15. Uhm... Why? What makes these registers special? Answer:
they point to the *untrusted zone*. %r15 always points to the beginning of the
untrusted zone (and can not be changed) while %rbp and %rsp can point to
arbitrary point in the *untrusted zone*—but can only be changed by a limited set
of instructions.

Why do we do that? How can it limit memory accesses and make sure they never
reach out of the *unstrusted zone*? Does at mean that you can only access memory
placed on a fixed distance from %r15, %rbp, or %rsp? It'll be quite inefficient!

Yes, it'll be inefficiant and that's why we introduce yet another concept:
concept of restricted register. Conceptually restricted register is just a
register which is guaranteed to be in the interval from 0 to 4'294'967'295. But,
again, we don't try to create complex data models at the validation time.
Instead we have a very simple rule: one instruction can produce restricted
register while another one can consume restricted register—and these two
instructions must be *immediately adjacent in the same bundle*. Here we use the
interesting property of x86-64 ISA: any instruction which stores to 32bit
register automatically cleans up top half of the corresponding 64bit register.
That's what notes input_rr=… and output_rr=… mean in the safe list. They show
which instruction produce restricted register and which instructions consume
restricted register. Note that while many instructions can produce %rbp and/or
%rsp as restricted register only few instruction accept %rbp and/or %rsp as
restricted register. Which means that most %rbp and/or %rsp modifications are
done as two-step operation: first you are doing something with %**e**bp or
%**e**sp, then you immediatery add %**r**15 to %**r**bp or %**r**sp using lea or
add.

This is all well and good, but here we have a complication: validator knows that
such tightly tied instructions must reside in a single bundle, but
x86_64-nacl-as does not know that! It will happily place these two instructions
into a different bundles and then code will be rejected by validator.

How can we solve that problem? This is done with human's assistance. If you want
to “glue two instruction together” you need to use the following construct:

.bundle_lock

lea (%rax,%rbx),%ecx

mov (%rbp,%rcx),%edx

mov (%r15,%rdx),%rax

.bundle_unlock

There are couple of interesting things to note here:

*   lea with 32 bit address registers is forbidden, use 64 bit version
            with 32 bit register as destination—it's shorter and is just as
            effective.
*   you can chain more than two instructions together—but you can not
            reuse restricted register again. This sequence will be rejected:

    .bundle_lock

    lea (%rax,%rbx),%ecx

    mov (%rbp,%rcx),%edx

    mov (%r15,%rcx),%rax

    .bundle_unlock

This is all well and good but how can you do that in the inline assembler when
you refer the arguments? When you use %0 will give you either 32 bit register or
64 bit register (depending on what size argument was supplied to asm directive)
and here we often need to deal with 32 bit registers and their 64 bit siblings!

Actually GCC always had the appropriate mechanism, it's not NaCl-specific at
all, it's just such mix of sizes is not common in “normal” programs. Consider:

$ cat sizes_test.c

int foo() {

int i;

long long ll;

asm("mov %q0,%0"::"r"(i));

asm("mov %q0,%0"::"r"(ll));

asm("mov %k0,%0"::"r"(i));

asm("mov %k0,%0"::"r"(ll));

}

$ gcc -S -O2 sizes_test.c -o-

…

foo:

.LFB0:

.cfi_startproc

movl    $1, %eax

#APP

# 4 "sizes_test.c" 1

mov %rax,%eax

# 0 "" 2

#NO_APP

movl    $1, %eax

#APP

# 5 "sizes_test.c" 1

mov %rax,%rax

# 0 "" 2

# 6 "sizes_test.c" 1

mov %eax,%eax

# 0 "" 2

#NO_APP

movl    $1, %eax

#APP

# 7 "sizes_test.c" 1

mov %eax,%rax

# 0 "" 2

#NO_APP

ret

.cfi_endproc

…

As you can see it's not hard to ask assembler to produce 32 bit or 64 bit
registers on demand. Just keep in mind that NaCl used ILP32 model which means
that by default it'll produce 32 bit registers there! Which probably means that
you'll use q modifier significanly more often then k modifier.

Note that .bundle_lock/.bundle_unlock machinery is only available in NaCl SDK
starting from PPAPI 33. Before that you were forced to use %nacl pseudo-prefix
and and bunch of special instructions to produce validateable code [as explaines
in the SFI
document](http://www.chromium.org/nativeclient/design-documents/nacl-sfi-model-on-x86-64-systems).
This approach was slower (because it was impossible to combine address
calculation with register restriction) and more cryptic, but if you need to deal
with PPAPI 32 or below then it's your only choice.
