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
  subtractionInterval,
} from '../../../../util/f32_interval.js';
import { biasedRange, fullF32Range } from '../../../../util/math.js';
import { allInputSources, Case, makeBinaryF32IntervalCase, run } from '../expression.js';

import { binary } from './binary.js';

export const g = makeTestGroup(GPUTest);

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
      return makeBinaryF32IntervalCase(lhs, rhs, additionInterval);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullF32Range();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('+'), [TypeF32, TypeF32], TypeF32, t.params, cases);
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
      return makeBinaryF32IntervalCase(lhs, rhs, subtractionInterval);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullF32Range();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('-'), [TypeF32, TypeF32], TypeF32, t.params, cases);
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
      return makeBinaryF32IntervalCase(lhs, rhs, multiplicationInterval);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullF32Range();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });
    run(t, binary('*'), [TypeF32, TypeF32], TypeF32, t.params, cases);
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
      return makeBinaryF32IntervalCase(lhs, rhs, divisionInterval);
    };

    const cases: Array<Case> = [];
    const lhs_range = fullF32Range();
    const rhs_range = biasedRange(2 ** -126, 2 ** 126, 200);

    lhs_range.forEach(lhs => {
      rhs_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('/'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

// Will be implemented as part larger derived accuracy task
g.test('modulus')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x % y
Accuracy: Derived from x - y * trunc(x/y)
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();
