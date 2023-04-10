export const description = `
Execution Tests for the f32 matrix arithmetic binary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { TypeF32, TypeMat } from '../../../../util/conversion.js';
import {
  additionMatrixInterval,
  subtractionMatrixInterval,
  multiplicationMatrixScalarInterval,
  multiplicationScalarMatrixInterval,
} from '../../../../util/f32_interval.js';
import { sparseF32Range, sparseMatrixF32Range } from '../../../../util/math.js';
import { makeCaseCache } from '../case_cache.js';
import {
  allInputSources,
  generateMatrixPairToMatrixCases,
  generateMatrixScalarToMatrixCases,
  generateScalarMatrixToMatrixCases,
  run,
} from '../expression.js';

import { binary } from './binary.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('binary/f32_matrix_arithmetic', {
  addition_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_2x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_2x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(2, 3),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_2x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_2x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(2, 4),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_2x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_3x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(3, 2),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_3x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_3x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(3, 3),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_3x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_3x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(3, 4),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_3x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_4x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(4, 2),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_4x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_4x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(4, 3),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_4x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      additionMatrixInterval
    );
  },
  addition_4x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(4, 4),
      'f32-only',
      additionMatrixInterval
    );
  },
  addition_4x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      additionMatrixInterval
    );
  },
  multiplication_2x2_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_2x2_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_2x3_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_2x3_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_2x4_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_2x4_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_3x2_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_3x2_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_3x3_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_3x3_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_3x4_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_3x4_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_4x2_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_4x2_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_4x3_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_4x3_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_4x4_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseF32Range(),
      'f32-only',
      multiplicationMatrixScalarInterval
    );
  },
  multiplication_4x4_scalar_non_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseF32Range(),
      'unfiltered',
      multiplicationMatrixScalarInterval
    );
  },

  multiplication_scalar_2x2_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(2, 2),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_2x2_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_2x3_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(2, 3),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_2x3_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_2x4_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(2, 4),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_2x4_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_3x2_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(3, 2),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_3x2_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_3x3_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(3, 3),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_3x3_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_3x4_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(3, 4),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_3x4_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_4x2_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(4, 2),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_4x2_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_4x3_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(4, 3),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_4x3_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_4x4_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(4, 4),
      'f32-only',
      multiplicationScalarMatrixInterval
    );
  },
  multiplication_scalar_4x4_non_const: () => {
    return generateScalarMatrixToMatrixCases(
      sparseF32Range(),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      multiplicationScalarMatrixInterval
    );
  },
  subtraction_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_2x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_2x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(2, 3),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_2x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_2x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(2, 4),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_2x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_3x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(3, 2),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_3x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_3x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(3, 3),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_3x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_3x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(3, 4),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_3x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_4x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(4, 2),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_4x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_4x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(4, 3),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_4x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
  subtraction_4x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(4, 4),
      'f32-only',
      subtractionMatrixInterval
    );
  },
  subtraction_4x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      subtractionMatrixInterval
    );
  },
});

g.test('addition')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x + y, where x and y are matrices
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('cols', [2, 3, 4] as const)
      .combine('rows', [2, 3, 4] as const)
  )
  .fn(async t => {
    const cols = t.params.cols;
    const rows = t.params.rows;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `addition_${cols}x${rows}_const`
        : `addition_${cols}x${rows}_non_const`
    );
    await run(
      t,
      binary('+'),
      [TypeMat(cols, rows, TypeF32), TypeMat(cols, rows, TypeF32)],
      TypeMat(cols, rows, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_matrix_scalar')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a matrix and y is a scalar
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('cols', [2, 3, 4] as const)
      .combine('rows', [2, 3, 4] as const)
  )
  .fn(async t => {
    const cols = t.params.cols;
    const rows = t.params.rows;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_${cols}x${rows}_scalar_const`
        : `multiplication_${cols}x${rows}_scalar_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeMat(cols, rows, TypeF32), TypeF32],
      TypeMat(cols, rows, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_scalar_matrix')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a scalar and y is a matrix
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('cols', [2, 3, 4] as const)
      .combine('rows', [2, 3, 4] as const)
  )
  .fn(async t => {
    const cols = t.params.cols;
    const rows = t.params.rows;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_scalar_${cols}x${rows}_const`
        : `multiplication_scalar_${cols}x${rows}_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeF32, TypeMat(cols, rows, TypeF32)],
      TypeMat(cols, rows, TypeF32),
      t.params,
      cases
    );
  });

g.test('subtraction')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x - y, where x and y are matrices
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('cols', [2, 3, 4] as const)
      .combine('rows', [2, 3, 4] as const)
  )
  .fn(async t => {
    const cols = t.params.cols;
    const rows = t.params.rows;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `subtraction_${cols}x${rows}_const`
        : `subtraction_${cols}x${rows}_non_const`
    );
    await run(
      t,
      binary('-'),
      [TypeMat(cols, rows, TypeF32), TypeMat(cols, rows, TypeF32)],
      TypeMat(cols, rows, TypeF32),
      t.params,
      cases
    );
  });
