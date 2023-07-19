// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>

typedef int (*entry_point)(int, char**);

static char kMainRelativePath[] = "/Frameworks/HelloMain.framework/HelloMain";

static char* GetHelloMainPath(void) {
  uint32_t buffer_size = 0;
  _NSGetExecutablePath(NULL, &buffer_size);
  if (buffer_size == 0)
    return NULL;

  uint32_t suffix_len = sizeof(kMainRelativePath) + 1;
  if (buffer_size >= UINT32_MAX - suffix_len)
    return NULL;

  char* buffer = malloc(suffix_len + buffer_size);
  if (!buffer)
    return NULL;

  if (_NSGetExecutablePath(buffer, &buffer_size) != 0)
    return NULL;

  char* last_sep = strrchr(buffer, '/');
  if (!last_sep)
    return NULL;

  memcpy(last_sep, kMainRelativePath, suffix_len);
  return buffer;
}

int main(int argc, char** argv) {
  char* hello_main_path = GetHelloMainPath();
  if (!hello_main_path)
    return 1;

  void* hello_main_handle = dlopen(hello_main_path, RTLD_NOW);
  if (!hello_main_handle)
    return 1;

  free(hello_main_path);
  hello_main_path = NULL;

  entry_point hello_main_entry = dlsym(hello_main_handle, "hello_main");
  if (!hello_main_entry)
    return 1;

  int retcode = hello_main_entry(argc, argv);
  if (retcode) {
    return retcode;
  }

  if (!dlclose(hello_main_handle)) {
    return 1;
  }

  return 0;
}
