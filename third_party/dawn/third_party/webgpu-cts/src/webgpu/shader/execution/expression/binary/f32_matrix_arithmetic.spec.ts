export const description = `
Execution Tests for the f32 matrix arithmetic binary expression operations
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';
import { TypeF32, TypeMat, TypeVec } from '../../../../util/conversion.js';
import {
  additionMatrixInterval,
  multiplicationMatrixMatrixInterval,
  multiplicationMatrixScalarInterval,
  multiplicationMatrixVectorInterval,
  multiplicationScalarMatrixInterval,
  multiplicationVectorMatrixInterval,
  subtractionMatrixInterval,
} from '../../../../util/f32_interval.js';
import {
  sparseF32Range,
  sparseMatrixF32Range,
  sparseVectorF32Range,
} from '../../../../util/math.js';
import { makeCaseCache } from '../case_cache.js';
import {
  allInputSources,
  generateMatrixPairToMatrixCases,
  generateMatrixScalarToMatrixCases,
  generateMatrixVectorToVectorCases,
  generateScalarMatrixToMatrixCases,
  generateVectorMatrixToVectorCases,
  run,
} from '../expression.js';

import { binary, compoundBinary } from './binary.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('binary/f32_matrix_arithmetic', {
  addition_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
  multiplication_2x2_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x2_2x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x3_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(2, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x3_2x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x2_3x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(3, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x2_3x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x3_3x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(3, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x3_3x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x4_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(2, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x4_2x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x2_4x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(4, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x2_4x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x4_4x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(4, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x4_4x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x3_4x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(4, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x3_4x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 3),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x4_3x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(3, 2),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x4_3x2_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 4),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x3_3x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(3, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x3_3x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x2_3x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(3, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x2_3x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x3_2x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(2, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x3_2x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x2_2x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(2, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x2_2x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x4_3x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(3, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x4_3x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x3_4x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(4, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x3_4x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 3),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x4_4x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(4, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x4_4x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x2_4x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(4, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x2_4x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 2),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x4_2x3_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(2, 3),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_3x4_2x3_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(3, 4),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x4_4x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(4, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x4_4x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x2_4x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(4, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x2_4x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x4_2x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(2, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x4_2x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x2_2x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(2, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x2_2x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x3_4x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(4, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x3_4x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x4_3x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(3, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x4_3x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 4),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x3_3x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(3, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x3_3x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x2_3x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(3, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x2_3x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 2),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x3_2x4_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(2, 4),
      'finite',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_4x3_2x4_non_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(4, 3),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      multiplicationMatrixMatrixInterval
    );
  },
  multiplication_2x2_scalar_const: () => {
    return generateMatrixScalarToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseF32Range(),
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
  multiplication_2x2_vec2_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(2, 2),
      sparseVectorF32Range(2),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_2x2_vec2_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(2, 2),
      sparseVectorF32Range(2),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_2x3_vec2_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(2, 3),
      sparseVectorF32Range(2),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_2x3_vec2_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(2, 3),
      sparseVectorF32Range(2),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_2x4_vec2_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(2, 4),
      sparseVectorF32Range(2),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_2x4_vec2_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(2, 4),
      sparseVectorF32Range(2),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_3x2_vec3_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(3, 2),
      sparseVectorF32Range(3),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_3x2_vec3_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(3, 2),
      sparseVectorF32Range(3),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_3x3_vec3_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(3, 3),
      sparseVectorF32Range(3),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_3x3_vec3_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(3, 3),
      sparseVectorF32Range(3),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_3x4_vec3_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(3, 4),
      sparseVectorF32Range(3),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_3x4_vec3_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(3, 4),
      sparseVectorF32Range(3),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_4x2_vec4_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(4, 2),
      sparseVectorF32Range(4),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_4x2_vec4_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(4, 2),
      sparseVectorF32Range(4),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_4x3_vec4_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(4, 3),
      sparseVectorF32Range(4),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_4x3_vec4_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(4, 3),
      sparseVectorF32Range(4),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_4x4_vec4_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(4, 4),
      sparseVectorF32Range(4),
      'finite',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_4x4_vec4_non_const: () => {
    return generateMatrixVectorToVectorCases(
      sparseMatrixF32Range(4, 4),
      sparseVectorF32Range(4),
      'unfiltered',
      multiplicationMatrixVectorInterval
    );
  },
  multiplication_vec2_2x2_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(2),
      sparseMatrixF32Range(2, 2),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec2_2x2_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(2),
      sparseMatrixF32Range(2, 2),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec2_3x2_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(2),
      sparseMatrixF32Range(3, 2),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec2_3x2_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(2),
      sparseMatrixF32Range(3, 2),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec2_4x2_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(2),
      sparseMatrixF32Range(4, 2),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec2_4x2_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(2),
      sparseMatrixF32Range(4, 2),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec3_2x3_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(3),
      sparseMatrixF32Range(2, 3),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec3_2x3_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(3),
      sparseMatrixF32Range(2, 3),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec3_3x3_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(3),
      sparseMatrixF32Range(3, 3),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec3_3x3_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(3),
      sparseMatrixF32Range(3, 3),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec3_4x3_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(3),
      sparseMatrixF32Range(4, 3),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec3_4x3_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(3),
      sparseMatrixF32Range(4, 3),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec4_2x4_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(4),
      sparseMatrixF32Range(2, 4),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec4_2x4_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(4),
      sparseMatrixF32Range(2, 4),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec4_3x4_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(4),
      sparseMatrixF32Range(3, 4),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec4_3x4_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(4),
      sparseMatrixF32Range(3, 4),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec4_4x4_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(4),
      sparseMatrixF32Range(4, 4),
      'finite',
      multiplicationVectorMatrixInterval
    );
  },
  multiplication_vec4_4x4_non_const: () => {
    return generateVectorMatrixToVectorCases(
      sparseVectorF32Range(4),
      sparseMatrixF32Range(4, 4),
      'unfiltered',
      multiplicationVectorMatrixInterval
    );
  },
  subtraction_2x2_const: () => {
    return generateMatrixPairToMatrixCases(
      sparseMatrixF32Range(2, 2),
      sparseMatrixF32Range(2, 2),
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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
      'finite',
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

g.test('addition_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x =+ y, where x and y are matrices
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
      compoundBinary('+='),
      [TypeMat(cols, rows, TypeF32), TypeMat(cols, rows, TypeF32)],
      TypeMat(cols, rows, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_matrix_matrix')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a matrix and y is a matrix
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('common_dim', [2, 3, 4] as const)
      .combine('x_rows', [2, 3, 4] as const)
      .combine('y_cols', [2, 3, 4] as const)
  )
  .fn(async t => {
    const x_cols = t.params.common_dim;
    const x_rows = t.params.x_rows;
    const y_cols = t.params.y_cols;
    const y_rows = t.params.common_dim;

    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_${x_cols}x${x_rows}_${y_cols}x${y_rows}_const`
        : `multiplication_${x_cols}x${x_rows}_${y_cols}x${y_rows}_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeMat(x_cols, x_rows, TypeF32), TypeMat(y_cols, y_rows, TypeF32)],
      TypeMat(y_cols, x_rows, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_matrix_matrix_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x *= y, where x is a matrix and y is a matrix
Accuracy: Correctly rounded
`
  )
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('common_dim', [2, 3, 4] as const)
      .combine('x_rows', [2, 3, 4] as const)
  )
  .fn(async t => {
    const x_cols = t.params.common_dim;
    const x_rows = t.params.x_rows;
    const y_cols = x_cols;
    const y_rows = t.params.common_dim;

    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_${x_cols}x${x_rows}_${y_cols}x${y_rows}_const`
        : `multiplication_${x_cols}x${x_rows}_${y_cols}x${y_rows}_non_const`
    );
    await run(
      t,
      compoundBinary('*='),
      [TypeMat(x_cols, x_rows, TypeF32), TypeMat(y_cols, y_rows, TypeF32)],
      TypeMat(y_cols, x_rows, TypeF32),
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

g.test('multiplication_matrix_scalar_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x *= y, where x is a matrix and y is a scalar
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
      compoundBinary('*='),
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

g.test('multiplication_matrix_vector')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a matrix and y is a vector
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
        ? `multiplication_${cols}x${rows}_vec${cols}_const`
        : `multiplication_${cols}x${rows}_vec${cols}_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeMat(cols, rows, TypeF32), TypeVec(cols, TypeF32)],
      TypeVec(rows, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_vector_matrix')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x * y, where x is a vector and y is is a matrix
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
        ? `multiplication_vec${rows}_${cols}x${rows}_const`
        : `multiplication_vec${rows}_${cols}x${rows}_non_const`
    );
    await run(
      t,
      binary('*'),
      [TypeVec(rows, TypeF32), TypeMat(cols, rows, TypeF32)],
      TypeVec(cols, TypeF32),
      t.params,
      cases
    );
  });

g.test('multiplication_vector_matrix_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x *= y, where x is a vector and y is is a matrix
Accuracy: Correctly rounded
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('dim', [2, 3, 4] as const))
  .fn(async t => {
    const cols = t.params.dim;
    const rows = t.params.dim;
    const cases = await d.get(
      t.params.inputSource === 'const'
        ? `multiplication_vec${rows}_${cols}x${rows}_const`
        : `multiplication_vec${rows}_${cols}x${rows}_non_const`
    );
    await run(
      t,
      compoundBinary('*='),
      [TypeVec(rows, TypeF32), TypeMat(cols, rows, TypeF32)],
      TypeVec(cols, TypeF32),
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

g.test('subtraction_compound')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
Expression: x -= y, where x and y are matrices
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
      compoundBinary('-='),
      [TypeMat(cols, rows, TypeF32), TypeMat(cols, rows, TypeF32)],
      TypeMat(cols, rows, TypeF32),
      t.params,
      cases
    );
  });
