// This is a shim file with all the old/deprecated f32 API calls.
// They currently just pass-through to the refactored FPContext implementation.
// As CTS migrates over to directly calling the new API, these will be removed.

import { FPInterval, FPMatrix, FPVector, IntervalBounds, FP } from './floating_point.js';

// Interfaces

export interface MatrixToScalar {
  (m: number[][]): FPInterval;
}

export interface MatrixToMatrix {
  (m: number[][]): FPMatrix;
}

export interface MatrixPairToMatrix {
  (x: number[][], y: number[][]): FPMatrix;
}

export interface MatrixScalarToMatrix {
  (x: number[][], y: number): FPMatrix;
}

export interface ScalarMatrixToMatrix {
  (x: number, y: number[][]): FPMatrix;
}

export interface MatrixVectorToVector {
  (x: number[][], y: number[]): FPVector;
}

export interface VectorMatrixToVector {
  (x: number[], y: number[][]): FPVector;
}

// Containers

export type F32Vector = FPVector;

// Utilities

export function toF32Interval(n: number | IntervalBounds | FPInterval): FPInterval {
  return FP.f32.toInterval(n);
}

export function isF32Vector(v: (number | IntervalBounds | FPInterval)[]): v is FPVector {
  return FP.f32.isVector(v);
}

export function toF32Vector(v: (number | IntervalBounds | FPInterval)[]): FPVector {
  return FP.f32.toVector(v);
}

export function spanF32Intervals(...intervals: FPInterval[]): FPInterval {
  return FP.f32.spanIntervals(...intervals);
}

export function isF32Matrix(
  m: (number | IntervalBounds | FPInterval)[][] | FPVector[]
): m is FPMatrix {
  return FP.f32.isMatrix(m);
}

export function toF32Matrix(m: (number | IntervalBounds | FPInterval)[][] | FPVector[]): FPMatrix {
  return FP.f32.toMatrix(m);
}

// Accuracy Interval

export function correctlyRoundedInterval(n: number | FPInterval): FPInterval {
  return FP.f32.correctlyRoundedInterval(n);
}

export function correctlyRoundedMatrix(m: number[][]): FPMatrix {
  return FP.f32.correctlyRoundedMatrix(m);
}

export function absoluteErrorInterval(n: number, error_range: number): FPInterval {
  return FP.f32.absoluteErrorInterval(n, error_range);
}

export function ulpInterval(n: number, numULP: number): FPInterval {
  return FP.f32.ulpInterval(n, numULP);
}

export function additionMatrixInterval(x: number[][], y: number[][]): FPMatrix {
  return FP.f32.additionMatrixInterval(x, y);
}

export function determinantInterval(m: number[][]): FPInterval {
  return FP.f32.determinantInterval(m);
}

export function faceForwardIntervals(
  x: number[],
  y: number[],
  z: number[]
): (FPVector | undefined)[] {
  return FP.f32.faceForwardIntervals(x, y, z);
}

export function modfInterval(n: number): { fract: FPInterval; whole: FPInterval } {
  return FP.f32.modfInterval(n);
}

export function multiplicationMatrixScalarInterval(mat: number[][], scalar: number): FPMatrix {
  return FP.f32.multiplicationMatrixScalarInterval(mat, scalar);
}

export function multiplicationScalarMatrixInterval(scalar: number, mat: number[][]): FPMatrix {
  return FP.f32.multiplicationScalarMatrixInterval(scalar, mat);
}

export function multiplicationMatrixMatrixInterval(mat_x: number[][], mat_y: number[][]): FPMatrix {
  return FP.f32.multiplicationMatrixMatrixInterval(mat_x, mat_y);
}

export function multiplicationMatrixVectorInterval(x: number[][], y: number[]): FPVector {
  return FP.f32.multiplicationMatrixVectorInterval(x, y);
}

export function multiplicationVectorMatrixInterval(x: number[], y: number[][]): FPVector {
  return FP.f32.multiplicationVectorMatrixInterval(x, y);
}

export function refractInterval(i: number[], s: number[], r: number): FPVector {
  return FP.f32.refractInterval(i, s, r);
}

export function subtractionMatrixInterval(x: number[][], y: number[][]): FPMatrix {
  return FP.f32.subtractionMatrixInterval(x, y);
}

export function transposeInterval(m: number[][]): FPMatrix {
  return FP.f32.transposeInterval(m);
}
