/*
 * Copyright (c) 2021 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_COMMON_SUPERINSTRUCTIONS_H_
#define NATIVE_CLIENT_TESTS_COMMON_SUPERINSTRUCTIONS_H_

/*
 * This file contains definitions for superinstruction macros used in various
 * tests. They replace the naclret, naclcall and nacljmp instructions used
 * before.
 */

#if defined(__i386__) && defined(__saigo__)
# define NACLCALL(ARG) "calll *" ARG "\n"
# define NACLCALL_REG(REG) "calll *" REG "\n"
#elif defined(__i386__)
# define NACLCALL(ARG) "naclcall " ARG "\n"
# define NACLCALL_REG(REG) "naclcall " REG "\n"
#elif defined(__saigo__) && defined(__x86_64__)
# define NACLCALL(ARG) "callq *" ARG "\n"
# define NACLCALL_REG(REG32, REG64, R15) "callq *" REG64 "\n"
#elif defined(__x86_64__)
# define NACLCALL(ARG) "naclcall " ARG "\n"
# define NACLCALL_REG(REG32, REG64, R15) "naclcall " REG32 ", " R15 "\n"
#endif

#if defined(__saigo__) && defined(__i386__)
#define NACLJMP(REG) "jmpl *" REG "\n"
#elif defined(__i386__)
#define NACLJMP(REG) "nacljmp " REG "\n"
#elif defined(__saigo__) && defined(__x86_64__)
#define NACLJMP(REG32, REG64, R15) "jmpq *" REG64 "\n"
#elif defined(__x86_64__)
#define NACLJMP(REG32, REG64, R15) "nacljmp " REG32 ", " R15 "\n"
#endif

#if defined(__saigo__)
#define NACLRET "ret\n"
#define naclret ret
#else
#define NACLRET "naclret\n"
#endif

#if defined(__saigo__)
#define NACLSTORE32(reg, add) "movl "reg", "add"\n"
#else
#define NACLSTORE32(reg, add) "movl "reg", %%nacl:"add"\n"
#endif

#if defined(__saigo__)
#define NACLASP(VAL) addq VAL, %rsp
#define NACLSSP(VAL) subq VAL, %rsp
#else
#define NACLASP(VAL) naclaspq VAL, %r15
#define NACLSSP(VAL) naclsspq VAL, %r15
#endif

#if defined(__saigo__)
#define NACLRESTBP(MEM, R15) "mov "MEM", %%ebp\n"
#define NACLRESTSP(MEM, R15) "mov "MEM", %%esp\n"
#else
#define NACLRESTBP(MEM, R15) "naclrestbp "MEM", "R15"\n"
#define NACLRESTSP(MEM, R15) "naclrestsp "MEM", "R15"\n"
#endif

#endif  // NATIVE_CLIENT_TESTS_COMMON_SUPERINSTRUCTIONS_H_
