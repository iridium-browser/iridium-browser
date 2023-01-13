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
import { vectorTestValues } from '../../../../util/math.js';
import { makeCaseCache } from '../case_cache.js';
import { allInputSources, Case, makeBinaryToF32IntervalCase, run } from '../expression.js';

import { binary } from './binary.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('binary/f32_arithmetic', {
  addition_non_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, additionInterval);
    };

    return vectorTestValues(2, false).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  addition_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, additionInterval);
    };

    return vectorTestValues(2, true).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  subtraction_non_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, subtractionInterval);
    };

    return vectorTestValues(2, false).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  subtraction_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, subtractionInterval);
    };

    return vectorTestValues(2, true).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  multiplication_non_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, multiplicationInterval);
    };

    return vectorTestValues(2, false).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  multiplication_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, multiplicationInterval);
    };

    return vectorTestValues(2, true).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  division_non_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, divisionInterval);
    };

    return vectorTestValues(2, false).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  division_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, divisionInterval);
    };

    return vectorTestValues(2, true).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  remainder_non_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, remainderInterval);
    };

    return vectorTestValues(2, false).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
  remainder_const: () => {
    const makeCase = (lhs: number, rhs: number): Case => {
      return makeBinaryToF32IntervalCase(lhs, rhs, remainderInterval);
    };

    return vectorTestValues(2, true).map(v => {
      return makeCase(v[0], v[1]);
    });
  },
});

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
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'addition_const' : 'addition_non_const'
    );
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
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'subtraction_const' : 'subtraction_non_const'
    );
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
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'multiplication_const' : 'multiplication_non_const'
    );
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
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'division_const' : 'division_non_const'
    );
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
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'remainder_const' : 'remainder_non_const'
    );
    await run(t, binary('%'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });
