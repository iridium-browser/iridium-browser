export const description = `
Execution Tests for the f32 arithmetic binary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { anyOf, correctlyRoundedThreshold, ulpThreshold } from '../../../../util/compare.js';
import { kValue } from '../../../../util/constants.js';
import { f32, Scalar, TypeF32 } from '../../../../util/conversion.js';
import {
  biasedRange,
  isSubnormalNumber,
  linearRange,
  quantizeToF32,
} from '../../../../util/math.js';
import { Case, Config, run } from '../expression.js';

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
 * Produces all of the results for a binary op and a specific pair of params, accounting for if subnormal results can be
 * flushed to zero.
 * Does not account for the inputs being flushed.
 * @param lhs the left hand side to pass into the binary operation
 * @param rhs the rhs hand side to pass into the binary operation
 * @param op callback that implements the truth function for the binary operation
 */
function calculateResults(
  lhs: number,
  rhs: number,
  op: (l: number, r: number) => number
): Array<Scalar> {
  const results: Array<Scalar> = [];
  const value = op(lhs, rhs);
  results.push(f32(value));
  if (isSubnormalNumber(value)) {
    results.push(f32(0.0));
  }
  return results;
}

/**
 * Generates a Case for the params and binary op provide.
 * @param lhs the left hand side to pass into the binary operation
 * @param rhs the rhs hand side to pass into the binary operation
 * @param op callback that implements the truth function for the binary operation
 * @param skip_rhs_zero_flush should the builder skip cases where the rhs would be flushed to 0, this is primarily for
 *                            avoid doing division by 0. The caller is responsible for making sure the initial rhs isn't
 *                            0.
 */
function makeCaseImpl(
  lhs: number,
  rhs: number,
  op: (l: number, r: number) => number,
  skip_rhs_zero_flush: boolean = false
): Case {
  const f32_lhs = quantizeToF32(lhs);
  const f32_rhs = quantizeToF32(rhs);
  const is_lhs_subnormal = isSubnormalNumber(f32_lhs);
  const is_rhs_subnormal = isSubnormalNumber(f32_rhs);
  const expected = calculateResults(f32_lhs, f32_rhs, op);
  if (is_lhs_subnormal) {
    expected.push(...calculateResults(0.0, f32_rhs, op));
  }
  if (!skip_rhs_zero_flush && is_rhs_subnormal) {
    expected.push(...calculateResults(f32_lhs, 0.0, op));
  }
  if (!skip_rhs_zero_flush && is_lhs_subnormal && is_rhs_subnormal) {
    expected.push(...calculateResults(0.0, 0.0, op));
  }

  return { input: [f32(lhs), f32(rhs)], expected: anyOf(...expected) };
}

g.test('addition')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x + y
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

    const makeCase = (lhs: number, rhs: number): Case => {
      return makeCaseImpl(lhs, rhs, (l: number, r: number): number => {
        return l + r;
      });
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('+'), [TypeF32, TypeF32], TypeF32, cfg, cases);
  });

g.test('subtraction')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x - y
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

    const makeCase = (lhs: number, rhs: number): Case => {
      return makeCaseImpl(lhs, rhs, (l: number, r: number): number => {
        return l - r;
      });
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('-'), [TypeF32, TypeF32], TypeF32, cfg, cases);
  });

g.test('multiplication')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y
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

    const makeCase = (lhs: number, rhs: number): Case => {
      return makeCaseImpl(lhs, rhs, (l: number, r: number): number => {
        return l * r;
      });
    };

    const cases: Array<Case> = [];
    const numeric_range = fullNumericRange();
    numeric_range.forEach(lhs => {
      numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('*'), [TypeF32, TypeF32], TypeF32, cfg, cases);
  });

g.test('division')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x / y
Accuracy: 2.5 ULP for |y| in the range [2^-126, 2^126]
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const cfg: Config = t.params;
    cfg.cmpFloats = ulpThreshold(2.5);

    const makeCase = (lhs: number, rhs: number): Case => {
      return makeCaseImpl(
        lhs,
        rhs,
        (l: number, r: number): number => {
          return l / r;
        },
        true
      );
    };

    const cases: Array<Case> = [];
    const lhs_numeric_range = fullNumericRange();
    const rhs_numeric_range = biasedRange(2 ** -126, 2 ** 126, 200).filter(value => {
      return value !== 0.0;
    });
    lhs_numeric_range.forEach(lhs => {
      rhs_numeric_range.forEach(rhs => {
        cases.push(makeCase(lhs, rhs));
      });
    });

    run(t, binary('/'), [TypeF32, TypeF32], TypeF32, cfg, cases);
  });

// Will be implemented as part larger derived accuracy task
g.test('modulus')
  .uniqueId('xxxxxxxxx')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x % y
Accuracy: Derived from x - y * trunc(x/y)
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();
