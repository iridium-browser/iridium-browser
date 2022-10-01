# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class SyscallThreadTest(gdb_test.GdbTest):

  def CheckBacktrace(self, backtrace, functions):
    all_functions = [frame[b'frame'][b'func'] for frame in backtrace]
    # Check that 'functions' is a subsequence of 'all_functions'
    s1 = b'|' + b'|'.join(all_functions) + b'|'
    s2 = b'|' + b'|'.join(functions) + b'|'
    self.assertIn(s2, s1, '%s not in %s' % (functions, all_functions))

  def test_syscall_thread(self):
    self.gdb.Command('break inside_f3')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    # Check we stopped in inside_f3
    backtrace = self.gdb.Command('-stack-list-frames')
    self.CheckBacktrace(backtrace[b'stack'], [b'inside_f3', b'f3'])
    # Check we have one more thread
    thread_info = self.gdb.Command('-thread-info')
    self.assertEquals(len(thread_info[b'threads']), 2)
    # Select another thread
    syscall_thread_id = thread_info[b'threads'][0][b'id']
    if syscall_thread_id == thread_info[b'current-thread-id']:
      syscall_thread_id = thread_info[b'threads'][1][b'id']
    self.gdb.Command(b'-thread-select %s' % syscall_thread_id)
    # Check that thread waits in usleep
    backtrace = self.gdb.Command('-stack-list-frames')
    self.CheckBacktrace(
        backtrace[b'stack'], [b'pthread_join', b'test_syscall_thread'])


if __name__ == '__main__':
  gdb_test.Main()
