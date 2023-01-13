/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2014 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "internal.h"

static char log_buffer[1024];
static int log_buffer_pos = 0;

static void
log_to_buffer(logLevels level, const char *message)
{
  switch(level) {
    case LOU_LOG_DEBUG:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[DEBUG] %s\n", message);
      break;
    case LOU_LOG_INFO:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[INFO] %s\n", message);
      break;
    case LOU_LOG_WARN:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[WARN] %s\n", message);
      break;
    case LOU_LOG_ERROR:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[ERROR] %s\n", message);
      break;
    case LOU_LOG_FATAL:
      log_buffer_pos += sprintf(&log_buffer[log_buffer_pos], "[FATAL] %s\n", message);
      break;  
  }
}

static int
assert_string_equals(char * expected, char * actual)
{
  if (strcmp(expected, actual))
    {
      printf("Expected \"%s\" but received \"%s\"\n", expected, actual);
      return 1;
    }
  return 0;
}
  
static int
assert_log_buffer_equals(char * expected)
{
  return assert_string_equals(expected, log_buffer);
}

int
main(int argc, char **argv)
{
  lou_registerLogCallback(log_to_buffer);
  log_buffer_pos = 0;
  lou_setLogLevel(LOU_LOG_WARN);
  _lou_logMessage(LOU_LOG_ERROR, "foo");
  _lou_logMessage(LOU_LOG_INFO, "bar");
  lou_setLogLevel(LOU_LOG_INFO);
  _lou_logMessage(LOU_LOG_INFO, "baz");
  return assert_log_buffer_equals("[ERROR] foo\n[INFO] baz\n");
}
