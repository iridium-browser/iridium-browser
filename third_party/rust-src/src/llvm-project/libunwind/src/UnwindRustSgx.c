//===--------------------- UnwindRustSgx.c ----------------------------------===//
//
////                     The LLVM Compiler Infrastructure
////
//// This file is dual licensed under the MIT and the University of Illinois Open
//// Source Licenses. See LICENSE.TXT for details.
////
////
////===----------------------------------------------------------------------===//

#define _GNU_SOURCE
#include <link.h>

#include <elf.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include "UnwindRustSgx.h"

#define max_log 256

__attribute__((weak)) struct _IO_FILE *stderr  = (struct _IO_FILE *)-1;

static int vwrite_err(const char *format, va_list ap)
{
    int len = 0;
#ifndef NDEBUG
    char s[max_log];
    s[0]='\0';
    len = vsnprintf(s, max_log, format, ap);
    __rust_print_err((uint8_t *)s, len);
#endif
    return len;
}

static int write_err(const char *format, ...)
{
    int ret;
    va_list args;
        va_start(args, format);
    ret = vwrite_err(format, args);
    va_end(args);


    return ret;
}

__attribute__((weak)) int fprintf (FILE *__restrict __stream,
            const char *__restrict __format, ...)
{

    int ret;
    if (__stream != stderr) {
        write_err("Rust SGX Unwind supports only writing to stderr\n");
        return -1;
    } else {
        va_list args;
        ret = 0;
        va_start(args, __format);
        ret += vwrite_err(__format, args);
        va_end(args);
    }

    return ret;
}

__attribute__((weak)) int fflush (FILE *__stream)
{
    // We do not need to do anything here.
    return 0;
}

__attribute__((weak)) void __assert_fail(const char * assertion,
                                       const char * file,
                           unsigned int line,
                           const char * function)
{
    write_err("%s:%d %s %s\n", file, line, function, assertion);
    abort();
}

// We do not report stack over flow detected.
// Calling write_err uses more stack due to the way we have implemented it.
// With possible enabling of stack probes, we should not
// get into __stack_chk_fail() at all.
__attribute__((weak))  void __stack_chk_fail() {
    abort();
}

/*
 * Below are defined for all executibles compiled for
 * x86_64-fortanix-unknown-sgx rust target.
 * Ref: rust/src/libstd/sys/sgx/abi/entry.S
 */

struct libwu_rs_alloc_meta {
    size_t alloc_size;
    // Should we put a signatre guard before ptr for oob access?
    unsigned char ptr[0];
};

#define META_FROM_PTR(__PTR) (struct libwu_rs_alloc_meta *) \
    ((unsigned char *)__PTR - offsetof(struct libwu_rs_alloc_meta, ptr))

void *libuw_malloc(size_t size)
{
    struct libwu_rs_alloc_meta *meta;
    size_t alloc_size = size + sizeof(struct libwu_rs_alloc_meta);
    meta = (void *)__rust_c_alloc(alloc_size, sizeof(size_t));
    if (!meta) {
        return NULL;
    }
    meta->alloc_size = alloc_size;
    return (void *)meta->ptr;
}

void libuw_free(void *p)
{
    struct libwu_rs_alloc_meta *meta;
    if (!p) {
        return;
    }
    meta = META_FROM_PTR(p);
    __rust_c_dealloc((unsigned char *)meta, meta->alloc_size, sizeof(size_t));
}
