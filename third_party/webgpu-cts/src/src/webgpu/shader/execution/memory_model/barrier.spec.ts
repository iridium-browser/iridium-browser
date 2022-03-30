export const description = `
Tests for non-atomic memory synchronization within a workgroup in the presence of a WebGPU barrier`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('workgroup_barrier_store_load')
  .desc(
    `Checks whether the workgroup barrier properly synchronizes a non-atomic write and read on
    separate threads in the same workgroup.
    `
  )
  .unimplemented();
