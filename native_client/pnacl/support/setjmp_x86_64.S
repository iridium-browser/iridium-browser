/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_x86_defs.h"

#if defined(__saigo__)
# define NACLSTORE(reg, add) movq reg, add
# define NACLSTORE32(reg, add) movl reg, add
#else
# define NACLSTORE(reg, add) movq reg, %nacl:add
# define NACLSTORE32(reg, add) movl reg, %nacl:add
#endif

/*
 * jump_buf is organized as follows
 * rbx, rbp, rsp, r12, r13, r14, pc
 * NOTE: r15 is neither saved nor restored
 */
    .text
    .p2align NACL_BLOCK_SHIFT
    .globl   setjmp
    .type    setjmp, @function

/* int setjmp(jmp_buf env) */
setjmp:
    /* edi has env argument*/
    pop     %rdx                         /* return address */

    /*
     * We avoid revealing the top 32 bits of rbp and rsp here.
     *
     * Also note that we don't do:
     *   pop %r11
     *   ...
     *   movq %r11, %nacl:48(%r15,%rdi)    / * save pc * /
     * because the latter instruction might be reached via a direct or
     * indirect jump when r11 contains the sandbox base address in its
     * top 32 bits, and this would write the sandbox base address into
     * memory.  We treat r11 as a write-only register to avoid
     * revealing the sandbox base address to user code.
     */
    NACLSTORE(%rbx, 0(%r15,%rdi))     /* save rbx */
    NACLSTORE32(%ebp, 8(%r15,%rdi))    /* save rbp/ebp */
    NACLSTORE32(%esp, 16(%r15,%rdi))    /* save rsp/esp */
    NACLSTORE(%r12, 24(%r15,%rdi))    /* save r12 */
    NACLSTORE(%r13, 32(%r15,%rdi))    /* save r13 */
    NACLSTORE(%r14, 40(%r15,%rdi))    /* save r14 */
    NACLSTORE(%rdx, 48(%r15,%rdi))    /* save pc */

    xorq    %rax, %rax

    movl    %edx, %r11d
#if defined(__saigo__)
    jmpq *%r11
#else
    nacljmp %r11d, %r15
#endif


    .text
    .p2align NACL_BLOCK_SHIFT
    .globl   longjmp
    .type    longjmp, @function

/* void longjmp(jmp_buf env, int val) */
longjmp:
    /* edi has env argument */
    /* esi has val argument */
    movl    %esi, %eax
    /* if val is zero, we must return 1 -- otherwise return val */
    testl   %eax, %eax
    jne     .Lskip
    movl    $1, %eax
.Lskip:
#if defined(__saigo__)
    movq 0(%r15,%rdi), %rbx          /* restore rbx */
    movq 8(%r15,%rdi), %rbp          /* restore rbp */
    movq 16(%r15,%rdi), %rsp         /* restore rsp */
    movq 24(%r15,%rdi), %r12         /* restore r12 */
    movq 32(%r15,%rdi), %r13         /* restore r13 */
    movq 40(%r15,%rdi), %r14         /* restore r14 */
    movq 48(%r15,%rdi), %r11
    jmpq *%r11
#else
    movq %nacl:0(%r15,%rdi), %rbx    /* restore rbx */
    movq %nacl:8(%r15,%rdi), %rdx    /* restore rbp */
    naclrestbp %edx, %r15
    movq %nacl:16(%r15,%rdi), %rdx   /* restore rsp */
    naclrestsp %edx, %r15
    movq %nacl:24(%r15,%rdi), %r12   /* restore r12 */
    movq %nacl:32(%r15,%rdi), %r13   /* restore r13 */
    movq %nacl:40(%r15,%rdi), %r14   /* restore r14 */
    movq %nacl:48(%r15,%rdi), %r11
    nacljmp %r11d, %r15
#endif
    .p2align NACL_BLOCK_SHIFT
