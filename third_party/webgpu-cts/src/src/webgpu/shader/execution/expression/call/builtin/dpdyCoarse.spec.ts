export const description = `
Execution tests for the 'dpdyCoarse' builtin function

T is f32 or vecN<f32>
fn dpdyCoarse(e:T) ->T
Returns the partial derivative of e with respect to window y coordinates using local differences.
This may result in fewer unique positions that dpdyFine(e).
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#derivative-builtin-functions')
  .desc(`f32 test`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
