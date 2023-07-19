# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class StackTraceTest(gdb_test.GdbTest):

  def test_stack_trace(self):
    self.gdb.Command('break leaf_call')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    result = self.gdb.Command('-stack-list-frames 0 2')
    self.assertEquals(result[b'stack'][0][b'frame'][b'func'], b'leaf_call')
    self.assertEquals(result[b'stack'][1][b'frame'][b'func'], b'nested_calls')
    self.assertEquals(result[b'stack'][2][b'frame'][b'func'], b'main')

    result = self.gdb.Command('-stack-list-arguments 1 0 1')
    self.assertEquals(result[b'stack-args'][0][b'frame'][b'args'][0][b'value'],
                      b'2')
    self.assertEquals(result[b'stack-args'][1][b'frame'][b'args'][0][b'value'],
                      b'1')
    self.gdb.Command('return')
    self.gdb.ResumeAndExpectStop('finish', 'function-finished')
    self.assertEquals(self.gdb.Eval('global_var'), b'1')


if __name__ == '__main__':
  gdb_test.Main()
