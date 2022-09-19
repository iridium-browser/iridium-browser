import { Colors } from '../../common/util/colors.js';

import { f32, isFloatValue, Scalar, Value, Vector } from './conversion.js';
import { F32Interval } from './f32_interval.js';
import { correctlyRounded, oneULP, withinULP } from './math.js';

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
export function absMatch(diff: number): FloatMatch {
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
export function ulpMatch(ulp: number): FloatMatch {
  return (got, expected) => {
    if (got === expected) {
      return true;
    }
    return withinULP(got, expected, ulp);
  };
}

/**
 * @returns a FloatMatch that returns true iff |expected| is a correctly round
 * to |got|.
 * |got| must be expressible as a float32.
 */
export function correctlyRoundedMatch(): FloatMatch {
  return (got, expected) => {
    return correctlyRounded(f32(got), expected);
  };
}

/**
 * @returns a FloatMatch that return true iff |got| is contained in any of the intervals.
 *
 * The standard |expected| parameter is ignored.
 *
 * NB: This will be removed at the end of transition to the new FP testing framework.
 *
 * @param intervals array of intervals to test the |got| value against.
 */
export function intervalMatch(...intervals: F32Interval[]): FloatMatch {
  return (got, _) => {
    for (const i of intervals) {
      if (i.contains(got)) {
        return true;
      }
    }
    return false;
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
    const bothFloatTypes = isFloatValue(got) && isFloatValue(expected);
    if (gTy !== eTy && !bothFloatTypes) {
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
    const isFloat = g.type.kind === 'f64' || g.type.kind === 'f32' || g.type.kind === 'f16';
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

/** @returns a Comparator that checks whether a test value matches any of the provided options */
export function anyOf(...expectations: (Value | Comparator)[]): Comparator {
  return (got, cmpFloats) => {
    const failed = new Set<string>();
    for (const e of expectations) {
      let cmp: Comparison;
      if ((e as Value).type !== undefined) {
        const v = e as Value;
        cmp = compare(got, v, cmpFloats);
      } else {
        const c = e as Comparator;
        cmp = c(got, cmpFloats);
      }
      if (cmp.matched) {
        return cmp;
      }
      failed.add(cmp.expected);
    }
    return { matched: false, got: got.toString(), expected: [...failed].join(' or ') };
  };
}

/** @returns a Comparator that checks whether a result is within N * ULP of a target value, where N is defined by a function
 *
 * N is n(x), where x is the input into the function under test, not the result of the function.
 * For a function f(x) = X that is being tested, the acceptance interval is defined as within X +/- n(x) * ulp(X).
 */
export function ulpComparator(x: number, target: Scalar, n: (x: number) => number): Comparator {
  const c = n(x);
  const match = ulpMatch(c);
  return (got, _) => {
    const cmp = compare(got, target, match);
    if (cmp.matched) {
      return cmp;
    }

    const ulp = oneULP(target.value as number);
    return {
      matched: false,
      got: got.toString(),
      expected: `within ${c} * ULP (${ulp}) of ${target}`,
    };
  };
}

/** @returns a Comparator that checks whether a result is contained within the target intervals
 *
 * NB: This will be removed at the end of transition to the new FP testing framework.
 */
export function intervalComparator(...intervals: F32Interval[]): Comparator {
  const match = intervalMatch(...intervals);
  return (got, _) => {
    // The second param is ignored by match
    const cmp = compare(got, f32(0.0), match);
    if (cmp.matched) {
      return cmp;
    }
    return {
      matched: false,
      got: got.toString(),
      expected: `within ${intervals}`,
    };
  };
}
