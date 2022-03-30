export const description = `Tests for the atomicity of atomic read-modify-write instructions.`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('atomicity')
  .desc(
    `Checks whether a store on one thread can interrupt an atomic RMW on a second thread.
    `
  )
  .unimplemented();
