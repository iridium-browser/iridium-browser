export const description = `
Execution Tests for the f32 arithmetic binary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { TypeF32, TypeVec } from '../../../../util/conversion.js';
import {
  additionInterval,
  divisionInterval,
  F32Vector,
  multiplicationInterval,
  remainderInterval,
  subtractionInterval,
  toF32Vector,
} from '../../../../util/f32_interval.js';
import { fullF32Range, sparseVectorF32Range } from '../../../../util/math.js';
import { makeCaseCache } from '../case_cache.js';
import {
  allInputSources,
  generateBinaryToF32IntervalCases,
  generateF32VectorToVectorCases,
  generateVectorF32ToVectorCases,
  run,
} from '../expression.js';

import { binary, compoundBinary } from './binary.js';

// Utility wrappers around the interval generators for the scalar-vector and
// vector-scalar tests.
const additionVectorScalarInterval = (v: number[], s: number): F32Vector => {
  return toF32Vector(v.map(e => additionInterval(e, s)));
};

const additionScalarVectorInterval = (s: number, v: number[]): F32Vector => {
  return toF32Vector(v.map(e => additionInterval(s, e)));
};

const subtractionVectorScalarInterval = (v: number[], s: number): F32Vector => {
  return toF32Vector(v.map(e => subtractionInterval(e, s)));
};

const subtractionScalarVectorInterval = (s: number, v: number[]): F32Vector => {
  return toF32Vector(v.map(e => subtractionInterval(s, e)));
};

const multiplicationVectorScalarInterval = (v: number[], s: number): F32Vector => {
  return toF32Vector(v.map(e => multiplicationInterval(e, s)));
};

const multiplicationScalarVectorInterval = (s: number, v: number[]): F32Vector => {
  return toF32Vector(v.map(e => multiplicationInterval(s, e)));
};

const divisionVectorScalarInterval = (v: number[], s: number): F32Vector => {
  return toF32Vector(v.map(e => divisionInterval(e, s)));
};

const divisionScalarVectorInterval = (s: number, v: number[]): F32Vector => {
  return toF32Vector(v.map(e => divisionInterval(s, e)));
};

const remainderVectorScalarInterval = (v: number[], s: number): F32Vector => {
  return toF32Vector(v.map(e => remainderInterval(e, s)));
};

const remainderScalarVectorInterval = (s: number, v: number[]): F32Vector => {
  return toF32Vector(v.map(e => remainderInterval(s, e)));
};

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('binary/f32_arithmetic', {
  addition_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'f32-only',
      additionInterval
    );
  },
  addition_non_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'unfiltered',
      additionInterval
    );
  },
  addition_vec2_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'f32-only',
      additionVectorScalarInterval
    );
  },
  addition_vec2_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'unfiltered',
      additionVectorScalarInterval
    );
  },
  addition_vec3_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'f32-only',
      additionVectorScalarInterval
    );
  },
  addition_vec3_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'unfiltered',
      additionVectorScalarInterval
    );
  },
  addition_vec4_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'f32-only',
      additionVectorScalarInterval
    );
  },
  addition_vec4_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'unfiltered',
      additionVectorScalarInterval
    );
  },
  addition_scalar_vec2_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'f32-only',
      additionScalarVectorInterval
    );
  },
  addition_scalar_vec2_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'unfiltered',
      additionScalarVectorInterval
    );
  },
  addition_scalar_vec3_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'f32-only',
      additionScalarVectorInterval
    );
  },
  addition_scalar_vec3_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'unfiltered',
      additionScalarVectorInterval
    );
  },
  addition_scalar_vec4_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'f32-only',
      additionScalarVectorInterval
    );
  },
  addition_scalar_vec4_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'unfiltered',
      additionScalarVectorInterval
    );
  },
  subtraction_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'f32-only',
      subtractionInterval
    );
  },
  subtraction_non_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'unfiltered',
      subtractionInterval
    );
  },
  subtraction_vec2_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'f32-only',
      subtractionVectorScalarInterval
    );
  },
  subtraction_vec2_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'unfiltered',
      subtractionVectorScalarInterval
    );
  },
  subtraction_vec3_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'f32-only',
      subtractionVectorScalarInterval
    );
  },
  subtraction_vec3_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'unfiltered',
      subtractionVectorScalarInterval
    );
  },
  subtraction_vec4_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'f32-only',
      subtractionVectorScalarInterval
    );
  },
  subtraction_vec4_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'unfiltered',
      subtractionVectorScalarInterval
    );
  },
  subtraction_scalar_vec2_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'f32-only',
      subtractionScalarVectorInterval
    );
  },
  subtraction_scalar_vec2_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'unfiltered',
      subtractionScalarVectorInterval
    );
  },
  subtraction_scalar_vec3_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'f32-only',
      subtractionScalarVectorInterval
    );
  },
  subtraction_scalar_vec3_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'unfiltered',
      subtractionScalarVectorInterval
    );
  },
  subtraction_scalar_vec4_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'f32-only',
      subtractionScalarVectorInterval
    );
  },
  subtraction_scalar_vec4_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'unfiltered',
      subtractionScalarVectorInterval
    );
  },
  multiplication_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'f32-only',
      multiplicationInterval
    );
  },
  multiplication_non_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'unfiltered',
      multiplicationInterval
    );
  },
  multiplication_vec2_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'f32-only',
      multiplicationVectorScalarInterval
    );
  },
  multiplication_vec2_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'unfiltered',
      multiplicationVectorScalarInterval
    );
  },
  multiplication_vec3_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'f32-only',
      multiplicationVectorScalarInterval
    );
  },
  multiplication_vec3_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'unfiltered',
      multiplicationVectorScalarInterval
    );
  },
  multiplication_vec4_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'f32-only',
      multiplicationVectorScalarInterval
    );
  },
  multiplication_vec4_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'unfiltered',
      multiplicationVectorScalarInterval
    );
  },
  multiplication_scalar_vec2_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'f32-only',
      multiplicationScalarVectorInterval
    );
  },
  multiplication_scalar_vec2_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'unfiltered',
      multiplicationScalarVectorInterval
    );
  },
  multiplication_scalar_vec3_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'f32-only',
      multiplicationScalarVectorInterval
    );
  },
  multiplication_scalar_vec3_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'unfiltered',
      multiplicationScalarVectorInterval
    );
  },
  multiplication_scalar_vec4_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'f32-only',
      multiplicationScalarVectorInterval
    );
  },
  multiplication_scalar_vec4_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'unfiltered',
      multiplicationScalarVectorInterval
    );
  },
  division_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'f32-only',
      divisionInterval
    );
  },
  division_non_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'unfiltered',
      divisionInterval
    );
  },
  division_vec2_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'f32-only',
      divisionVectorScalarInterval
    );
  },
  division_vec2_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'unfiltered',
      divisionVectorScalarInterval
    );
  },
  division_vec3_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'f32-only',
      divisionVectorScalarInterval
    );
  },
  division_vec3_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'unfiltered',
      divisionVectorScalarInterval
    );
  },
  division_vec4_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'f32-only',
      divisionVectorScalarInterval
    );
  },
  division_vec4_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'unfiltered',
      divisionVectorScalarInterval
    );
  },
  division_scalar_vec2_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'f32-only',
      divisionScalarVectorInterval
    );
  },
  division_scalar_vec2_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'unfiltered',
      divisionScalarVectorInterval
    );
  },
  division_scalar_vec3_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'f32-only',
      divisionScalarVectorInterval
    );
  },
  division_scalar_vec3_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'unfiltered',
      divisionScalarVectorInterval
    );
  },
  division_scalar_vec4_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'f32-only',
      divisionScalarVectorInterval
    );
  },
  division_scalar_vec4_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'unfiltered',
      divisionScalarVectorInterval
    );
  },
  remainder_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'f32-only',
      remainderInterval
    );
  },
  remainder_non_const: () => {
    return generateBinaryToF32IntervalCases(
      fullF32Range(),
      fullF32Range(),
      'unfiltered',
      remainderInterval
    );
  },
  remainder_vec2_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'f32-only',
      remainderVectorScalarInterval
    );
  },
  remainder_vec2_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(2),
      fullF32Range(),
      'unfiltered',
      remainderVectorScalarInterval
    );
  },
  remainder_vec3_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'f32-only',
      remainderVectorScalarInterval
    );
  },
  remainder_vec3_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(3),
      fullF32Range(),
      'unfiltered',
      remainderVectorScalarInterval
    );
  },
  remainder_vec4_scalar_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'f32-only',
      remainderVectorScalarInterval
    );
  },
  remainder_vec4_scalar_non_const: () => {
    return generateVectorF32ToVectorCases(
      sparseVectorF32Range(4),
      fullF32Range(),
      'unfiltered',
      remainderVectorScalarInterval
    );
  },
  remainder_scalar_vec2_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'f32-only',
      remainderScalarVectorInterval
    );
  },
  remainder_scalar_vec2_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(2),
      'unfiltered',
      remainderScalarVectorInterval
    );
  },
  remainder_scalar_vec3_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'f32-only',
      remainderScalarVectorInterval
    );
  },
  remainder_scalar_vec3_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(3),
      'unfiltered',
      remainderScalarVectorInterval
    );
  },
  remainder_scalar_vec4_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'f32-only',
      remainderScalarVectorInterval
    );
  },
  remainder_scalar_vec4_non_const: () => {
    return generateF32VectorToVectorCases(
      fullF32Range(),
      sparseVectorF32Range(4),
      'unfiltered',
      remainderScalarVectorInterval
    );
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

g.test('addition_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x *= y
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
    await run(t, compoundBinary('+='), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('addition_vector_scalar')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x + y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `addition_vec${dim}_scalar_const`
        : `addition_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      binary('+'),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('addition_vector_scalar_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x += y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `addition_vec${dim}_scalar_const`
        : `addition_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      compoundBinary('+='),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('addition_scalar_vector')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x + y, where x is a scalar and y is a vector
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `addition_scalar_vec${dim}_const`
        : `addition_scalar_vec${dim}_non_const`
    );
    await run(
      t,
      binary('+'),
      [TypeF32, TypeVec(dim, TypeF32)],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
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

g.test('subtraction_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x -= y
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
    await run(t, compoundBinary('-='), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('subtraction_vector_scalar')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x - y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `subtraction_vec${dim}_scalar_const`
        : `subtraction_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      binary('-'),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('subtraction_vector_scalar_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x -= y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `subtraction_vec${dim}_scalar_const`
        : `subtraction_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      compoundBinary('-='),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('subtraction_scalar_vector')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x - y, where x is a scalar and y is a vector
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `subtraction_scalar_vec${dim}_const`
        : `subtraction_scalar_vec${dim}_non_const`
    );
    await run(
      t,
      binary('-'),
      [TypeF32, TypeVec(dim, TypeF32)],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
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

g.test('multiplication_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x *= y
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
    await run(t, compoundBinary('*='), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('multiplication_vector_scalar')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_vec${dim}_scalar_const`
        : `multiplication_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_vector_scalar_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x *= y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_vec${dim}_scalar_const`
        : `multiplication_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      compoundBinary('*='),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_scalar_vector')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a scalar and y is a vector
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_scalar_vec${dim}_const`
        : `multiplication_scalar_vec${dim}_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeF32, TypeVec(dim, TypeF32)],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
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

g.test('division_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x /= y
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
    await run(t, compoundBinary('/='), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('division_vector_scalar')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x / y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `division_vec${dim}_scalar_const`
        : `division_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      binary('/'),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('division_vector_scalar_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x /= y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `division_vec${dim}_scalar_const`
        : `division_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      compoundBinary('/='),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('division_scalar_vector')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x / y, where x is a scalar and y is a vector
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `division_scalar_vec${dim}_const`
        : `division_scalar_vec${dim}_non_const`
    );
    await run(
      t,
      binary('/'),
      [TypeF32, TypeVec(dim, TypeF32)],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
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

g.test('remainder_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x %= y
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
    await run(t, compoundBinary('%='), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('remainder_vector_scalar')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x % y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `remainder_vec${dim}_scalar_const`
        : `remainder_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      binary('%'),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('remainder_vector_scalar_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x %= y, where x is a vector and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `remainder_vec${dim}_scalar_const`
        : `remainder_vec${dim}_scalar_non_const`
    );
    await run(
      t,
      compoundBinary('%='),
      [TypeVec(dim, TypeF32), TypeF32],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });

g.test('remainder_scalar_vector')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x % y, where x is a scalar and y is a vector
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const dim = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `remainder_scalar_vec${dim}_const`
        : `remainder_scalar_vec${dim}_non_const`
    );
    await run(
      t,
      binary('%'),
      [TypeF32, TypeVec(dim, TypeF32)],
      TypeVec(dim, TypeF32),
      t.params,
      cases
    );
  });
