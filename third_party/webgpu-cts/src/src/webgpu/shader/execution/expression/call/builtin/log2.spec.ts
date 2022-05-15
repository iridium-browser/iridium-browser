export const description = `
Execution Tests for the 'log2' builtin function
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { absThreshold, FloatMatch, ulpThreshold } from '../../../../../util/compare.js';
import { kValue } from '../../../../../util/constants.js';
import { f32, TypeF32 } from '../../../../../util/conversion.js';
import { biasedRange, linearRange, quantizeToF32 } from '../../../../../util/math.js';
import { Case, CaseList, Config, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('f32')
  .uniqueId('9ed120de1990296a')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#float-builtin-functions')
  .desc(
    `
log2:
T is f32 or vecN<f32> log2(e: T ) -> T Returns the base-2 logarithm of e. Component-wise when T is a vector. (GLSLstd450Log2)

TODO(#792): Decide what the ground-truth is for these tests. [1]
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('range', ['low', 'mid', 'high'] as const)
  )
  .fn(async t => {
    // [1]: Need to decide what the ground-truth is.
    const makeCase = (x: number): Case => {
      const f32_x = quantizeToF32(x);
      return { input: f32(x), expected: f32(Math.log2(f32_x)) };
    };

    const runRange = (match: FloatMatch, cases: CaseList) => {
      const cfg: Config = t.params;
      cfg.cmpFloats = match;
      run(t, builtin('log2'), [TypeF32], TypeF32, cfg, cases);
    };

    // log2's accuracy is defined in three regions { [0, 0.5), [0.5, 2.0], (2.0, +∞] }
    switch (t.params.range) {
      case 'low': // [0, 0.5)
        runRange(
          ulpThreshold(3),
          linearRange(kValue.f32.positive.min, 0.5, 20).map(x => makeCase(x))
        );
        break;
      case 'mid': // [0.5, 2.0]
        runRange(
          absThreshold(2 ** -21),
          linearRange(0.5, 2.0, 20).map(x => makeCase(x))
        );
        break;
      case 'high': // (2.0, +∞]
        runRange(
          ulpThreshold(3),
          biasedRange(2.0, 2 ** 32, 1000).map(x => makeCase(x))
        );
        break;
    }
  });
