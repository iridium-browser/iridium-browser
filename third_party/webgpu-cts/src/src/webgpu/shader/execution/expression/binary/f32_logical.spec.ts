export const description = `
Execution Tests for the f32 logical binary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { anyOf } from '../../../../util/compare.js';
import { kValue } from '../../../../util/constants.js';
import { bool, f32, Scalar, TypeBool, TypeF32 } from '../../../../util/conversion.js';
import { biasedRange, flushSubnormalScalar, linearRange } from '../../../../util/math.js';
import { Case, run } from '../expression.js';

import { binary } from './binary.js';

export const g = makeTestGroup(GPUTest);

/* Generates an array of numbers spread over the entire range of 32-bit floats */
function fullNumericRange(): Array<number> {
  return [
    ...biasedRange(kValue.f32.negative.max, kValue.f32.negative.min, 50),
    ...linearRange(kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max, 10),
    0.0,
    ...linearRange(kValue.f32.subnormal.positive.min, kValue.f32.subnormal.positive.max, 10),
    ...biasedRange(kValue.f32.positive.min, kValue.f32.positive.max, 50),
  ];
}

/**
 * @returns a test case for the provided left hand & right hand values and truth function.
 * Handles quantization and subnormals.
 */
function makeCase(
  lhs: number,
  rhs: number,
  truthFunc: (lhs: Scalar, rhs: Scalar) => boolean
): Case {
  const f32_lhs = f32(lhs);
  const f32_rhs = f32(rhs);
  const lhs_options = new Set([f32_lhs, flushSubnormalScalar(f32_lhs)]);
  const rhs_options = new Set([f32_rhs, flushSubnormalScalar(f32_rhs)]);
  const expected: Array<Scalar> = [];
  lhs_options.forEach(l => {
    rhs_options.forEach(r => {
      const result = bool(truthFunc(l, r));
      if (!expected.includes(result)) {
        expected.push(result);
      }
    });
  });

  return { input: [f32_lhs, f32_rhs], expected: anyOf(...expected) };
}

g.test('equals')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x == y
Accuracy: Correct result
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const truthFunc = (lhs: Scalar, rhs: Scalar): boolean => {
      return (lhs.value as number) === (rhs.value as number);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs, truthFunc));
      });
    });

    run(t, binary('=='), [TypeF32, TypeF32], TypeBool, t.params, cases);
  });

g.test('not_equals')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x != y
Accuracy: Correct result
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const truthFunc = (lhs: Scalar, rhs: Scalar): boolean => {
      return (lhs.value as number) !== (rhs.value as number);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs, truthFunc));
      });
    });

    run(t, binary('!='), [TypeF32, TypeF32], TypeBool, t.params, cases);
  });

g.test('less_than')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x < y
Accuracy: Correct result
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const truthFunc = (lhs: Scalar, rhs: Scalar): boolean => {
      return (lhs.value as number) < (rhs.value as number);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs, truthFunc));
      });
    });

    run(t, binary('<'), [TypeF32, TypeF32], TypeBool, t.params, cases);
  });

g.test('less_equals')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x <= y
Accuracy: Correct result
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const truthFunc = (lhs: Scalar, rhs: Scalar): boolean => {
      return (lhs.value as number) <= (rhs.value as number);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs, truthFunc));
      });
    });

    run(t, binary('<='), [TypeF32, TypeF32], TypeBool, t.params, cases);
  });

g.test('greater_than')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x > y
Accuracy: Correct result
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const truthFunc = (lhs: Scalar, rhs: Scalar): boolean => {
      return (lhs.value as number) > (rhs.value as number);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs, truthFunc));
      });
    });

    run(t, binary('>'), [TypeF32, TypeF32], TypeBool, t.params, cases);
  });

g.test('greater_equals')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x >= y
Accuracy: Correct result
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const truthFunc = (lhs: Scalar, rhs: Scalar): boolean => {
      return (lhs.value as number) >= (rhs.value as number);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs, truthFunc));
      });
    });

    run(t, binary('>='), [TypeF32, TypeF32], TypeBool, t.params, cases);
  });
