export const description = `
Tests that all threads see a sequentially consistent view of the order of memory
accesses to a single memory location. Uses a parallel testing strategy along with stressing
threads to increase coverage of possible bugs.`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('corr')
  .desc(
    `Ensures two reads on one thread cannot observe an inconsistent view of a write on a second thread.
    `
  )
  .unimplemented();
