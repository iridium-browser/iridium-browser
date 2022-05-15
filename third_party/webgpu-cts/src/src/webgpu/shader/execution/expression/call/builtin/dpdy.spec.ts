export const description = `
Execution tests for the 'dpdy' builtin function

T is f32 or vecN<f32>
fn dpdy(e:T) ->T
Partial derivative of e with respect to window y coordinates.
The result is the same as either dpdyFine(e) or dpdyCoarse(e).
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#derivative-builtin-functions')
  .desc(`f32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
