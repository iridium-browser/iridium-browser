import { Colors } from '../../common/util/colors.js';

import { f32, Scalar, Value, Vector } from './conversion.js';
import { correctlyRounded, diffULP } from './math.js';

/** Comparison describes the result of a Comparator function. */
export interface Comparison {
  matched: boolean; // True if the two values were considered a match
  got: string; // The string representation of the 'got' value (possibly with markup)
  expected: string; // The string representation of the 'expected' value (possibly with markup)
}

/** FloatMatch is a function that compares whether the two floating point numbers match. */
export interface FloatMatch {
  (got: number, expected: number): boolean;
}

/** Comparator is a function that compares whether the provided value matches an expectation. */
export interface Comparator {
  (got: Value, cmpFloats: FloatMatch): Comparison;
}

/**
 * @returns a FloatMatch that returns true iff the two numbers are equal to, or
 * less than the specified absolute error threshold.
 */
export function absThreshold(diff: number): FloatMatch {
  return (got, expected) => {
    if (got === expected) {
      return true;
    }
    if (!Number.isFinite(got) || !Number.isFinite(expected)) {
      return false;
    }
    return Math.abs(got - expected) <= diff;
  };
}

/**
 * @returns a FloatMatch that returns true iff the two numbers are within or
 * equal to the specified ULP threshold value.
 */
export function ulpThreshold(ulp: number): FloatMatch {
  return (got, expected) => {
    if (got === expected) {
      return true;
    }
    return diffULP(got, expected) <= ulp;
  };
}

/**
 * @returns a FloatMatch that returns true iff |expected| is a correctly round
 * to |got|.
 * |got| must be expressible as a float32.
 */
export function correctlyRoundedThreshold(): FloatMatch {
  return (got, expected) => {
    return correctlyRounded(f32(got), expected);
  };
}

/**
 * compare() compares 'got' to 'expected', returning the Comparison information.
 * @param got the value obtained from the test
 * @param expected the expected value
 * @param cmpFloats the FloatMatch used to compare floating point values
 * @returns the comparison results
 */
export function compare(got: Value, expected: Value, cmpFloats: FloatMatch): Comparison {
  {
    // Check types
    const gTy = got.type;
    const eTy = expected.type;
    if (gTy !== eTy) {
      return {
        matched: false,
        got: `${Colors.red(gTy.toString())}(${got})`,
        expected: `${Colors.red(eTy.toString())}(${expected})`,
      };
    }
  }

  if (got instanceof Scalar) {
    const g = got;
    const e = expected as Scalar;
    const isFloat = g.type.kind === 'f32';
    const matched =
      (isFloat && cmpFloats(g.value as number, e.value as number)) ||
      (!isFloat && g.value === e.value);
    return {
      matched,
      got: g.toString(),
      expected: matched ? Colors.green(e.toString()) : Colors.red(e.toString()),
    };
  }
  if (got instanceof Vector) {
    const gLen = got.elements.length;
    const eLen = (expected as Vector).elements.length;
    let matched = gLen === eLen;
    const gElements = new Array<string>(gLen);
    const eElements = new Array<string>(eLen);
    for (let i = 0; i < Math.max(gLen, eLen); i++) {
      if (i < gLen && i < eLen) {
        const g = got.elements[i];
        const e = (expected as Vector).elements[i];
        const cmp = compare(g, e, cmpFloats);
        matched = matched && cmp.matched;
        gElements[i] = cmp.got;
        eElements[i] = cmp.expected;
        continue;
      }
      matched = false;
      if (i < gLen) {
        gElements[i] = got.elements[i].toString();
      }
      if (i < eLen) {
        eElements[i] = (expected as Vector).elements[i].toString();
      }
    }
    return {
      matched,
      got: `${got.type}(${gElements.join(', ')})`,
      expected: `${expected.type}(${eElements.join(', ')})`,
    };
  }
  throw new Error(`unhandled type '${typeof got}`);
}

/** @returns a Comparator that checks whether a test value matches any of the provided values */
export function anyOf(...values: Value[]): Comparator {
  return (got, cmpFloats) => {
    const failed: Array<string> = [];
    for (const e of values) {
      const cmp = compare(got, e, cmpFloats);
      if (cmp.matched) {
        return cmp;
      }
      failed.push(cmp.expected);
    }
    return { matched: false, got: got.toString(), expected: failed.join(' or ') };
  };
}
