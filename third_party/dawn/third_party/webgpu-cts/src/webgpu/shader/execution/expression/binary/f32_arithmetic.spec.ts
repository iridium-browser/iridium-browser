export const description = `
Execution Tests for the f32 arithmetic binary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { TypeF32 } from '../../../../util/conversion.js';
import {
  additionInterval,
  divisionInterval,
  multiplicationInterval,
  remainderInterval,
  subtractionInterval,
} from '../../../../util/f32_interval.js';
import { cartesianProduct, fullF32Range } from '../../../../util/math.js';
import { allInputSources, Case, makeBinaryToF32IntervalCase, run } from '../expression.js';

import { binary } from './binary.js';

export const g = makeTestGroup(GPUTest);

const kTestValues: number[][] = cartesianProduct(fullF32Range(), fullF32Range());

g.test('addition')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x + y
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, additionInterval);
    };

    const cases = kTestValues.map((v: number[]) => {
      return makeCase(v[0], v[1]);
    });

    await run(t, binary('+'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('subtraction')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x - y
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, subtractionInterval);
    };

    const cases = kTestValues.map((v: number[]) => {
      return makeCase(v[0], v[1]);
    });

    await run(t, binary('-'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('multiplication')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, multiplicationInterval);
    };

    const cases = kTestValues.map((v: number[]) => {
      return makeCase(v[0], v[1]);
    });

    await run(t, binary('*'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('division')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x / y
Accuracy: 2.5 ULP for |y| in the range [2^-126, 2^126]
`
  )
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, divisionInterval);
    };

    const cases = kTestValues.map((v: number[]) => {
      return makeCase(v[0], v[1]);
    });

    await run(t, binary('/'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('remainder')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x % y
Accuracy: Derived from x - y * trunc(x/y)
`
  )
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, remainderInterval);
    };

    const cases = kTestValues.map((v: number[]) => {
      return makeCase(v[0], v[1]);
    });

    await run(t, binary('%'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });
