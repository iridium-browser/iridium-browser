export const description = `
Tests for properties of the WebGPU memory model involving two memory locations.
Specifically, the acquire/release ordering provided by WebGPU's barriers can be used to disallow
weak behaviors in several classic memory model litmus tests.`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('message_passing_workgroup_memory')
  .desc(
    `Checks whether two reads on one thread can observe two writes in another thread in a way
    that is inconsistent with sequential consistency.
    `
  )
  .unimplemented();
