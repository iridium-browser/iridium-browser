export const description = `
Execution Tests for the f32 arithmetic unary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { TypeF32 } from '../../../../util/conversion.js';
import { negationInterval } from '../../../../util/f32_interval.js';
import { fullF32Range } from '../../../../util/math.js';
import { allInputSources, Case, makeUnaryToF32IntervalCase, run } from '../expression.js';

import { unary } from './unary.js';

export const g = makeTestGroup(GPUTest);

g.test('negation')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: -x
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (x: number): Case => {
      return makeUnaryToF32IntervalCase(x, negationInterval);
    };

    const cases = fullF32Range({ neg_norm: 250, neg_sub: 20, pos_sub: 20, pos_norm: 250 }).map(x =>
      makeCase(x)
    );

    await run(t, unary('-'), [TypeF32], TypeF32, t.params, cases);
  });
