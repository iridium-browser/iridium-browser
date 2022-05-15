export const description = `
Execution Tests for the f32 arithmetic unary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { anyOf, correctlyRoundedThreshold } from '../../../../util/compare.js';
import { kValue } from '../../../../util/constants.js';
import { f32, TypeF32 } from '../../../../util/conversion.js';
import {
  biasedRange,
  isSubnormalNumber,
  linearRange,
  quantizeToF32,
} from '../../../../util/math.js';
import { Case, Config, run } from '../expression.js';

import { unary } from './unary.js';

export const g = makeTestGroup(GPUTest);

g.test('negation')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: -x
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const cfg: Config = t.params;
    cfg.cmpFloats = correctlyRoundedThreshold();

    const makeCase = (x: number): Case => {
      const f32_x = quantizeToF32(x);
      if (isSubnormalNumber(f32_x)) {
        return { input: [f32(x)], expected: anyOf(f32(-f32_x), f32(0.0)) };
      } else {
        return { input: [f32(x)], expected: f32(-f32_x) };
      }
    };

    const numeric_range = [
      ...biasedRange(kValue.f32.negative.max, kValue.f32.negative.min, 50),
      ...linearRange(kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max, 10),
      0.0,
      ...linearRange(kValue.f32.subnormal.positive.min, kValue.f32.subnormal.positive.max, 10),
      ...biasedRange(kValue.f32.positive.min, kValue.f32.positive.max, 10),
    ];
    const cases = numeric_range.map(x => makeCase(x));

    run(t, unary('-'), [TypeF32], TypeF32, cfg, cases);
  });
