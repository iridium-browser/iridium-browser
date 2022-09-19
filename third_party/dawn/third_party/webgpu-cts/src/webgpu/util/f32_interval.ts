import { assert } from '../../common/util/util.js';

import { kValue } from './constants.js';
import {
  correctlyRoundedF32,
  flushSubnormalNumber,
  isF32Finite,
  isSubnormalNumber,
  oneULP,
} from './math.js';

/** Represents a closed interval in the f32 range */
export class F32Interval {
  public readonly begin: number;
  public readonly end: number;
  private static _infinite: F32Interval;

  /** Constructor
   *
   * Bounds that are out of range for F32 are converted to appropriate edge or
   * infinity values, so that all values above/below the f32 range are lumped
   * together.
   *
   * @param begin number indicating the lower bound of the interval
   * @param end number indicating the upper bound of the interval
   */
  public constructor(begin: number, end: number) {
    assert(!Number.isNaN(begin) && !Number.isNaN(end), `bounds need to be non-NaN`);
    assert(begin <= end, `begin (${begin}) must be equal or before end (${end})`);

    if (begin === Number.NEGATIVE_INFINITY || begin < kValue.f32.negative.min) {
      this.begin = Number.NEGATIVE_INFINITY;
    } else if (begin === Number.POSITIVE_INFINITY || begin > kValue.f32.positive.max) {
      this.begin = kValue.f32.positive.max;
    } else {
      this.begin = begin;
    }

    if (end === Number.POSITIVE_INFINITY || end > kValue.f32.positive.max) {
      this.end = Number.POSITIVE_INFINITY;
    } else if (end === Number.NEGATIVE_INFINITY || end < kValue.f32.negative.min) {
      this.end = kValue.f32.negative.min;
    } else {
      this.end = end;
    }
  }

  /** @returns if a point or interval is completely contained by this interval
   *
   * Due to values that are above/below the f32 range being indistinguishable
   * from other values out of range in the same way, there some unintuitive
   * behaviours here, for example:
   *   [0, greater than max f32].contains(+∞) will return true.
   */
  public contains(n: number | F32Interval): boolean {
    if (Number.isNaN(n)) {
      // Being the infinite interval indicates that the accuracy is not defined
      // for this test, so the test is just checking that this input doesn't
      // cause the implementation to misbehave, so NaN is acceptable.
      return this.begin === Number.NEGATIVE_INFINITY && this.end === Number.POSITIVE_INFINITY;
    }
    const i = toInterval(n);
    return this.begin <= i.begin && this.end >= i.end;
  }

  /** @returns if this interval contains a single point */
  public isPoint(): boolean {
    return this.begin === this.end;
  }

  /** @returns an interval with the tightest bounds that includes all provided intervals */
  static span(...intervals: F32Interval[]): F32Interval {
    assert(intervals.length > 0, `span of an empty list of F32Intervals is not allowed`);
    let begin = Number.POSITIVE_INFINITY;
    let end = Number.NEGATIVE_INFINITY;
    intervals.forEach(i => {
      begin = Math.min(i.begin, begin);
      end = Math.max(i.end, end);
    });
    return new F32Interval(begin, end);
  }

  /** @returns a string representation for logging purposes */
  public toString(): string {
    return `[${this.begin}, ${this.end}]`;
  }

  /** @returns a singleton for the infinite interval
   * This interval is used in situations where accuracy is not defined, so any
   * result is valid.
   */
  public static infinite(): F32Interval {
    if (this._infinite === undefined) {
      this._infinite = new F32Interval(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY);
    }
    return this._infinite;
  }
}

/** @returns an interval containing the point or the original interval */
function toInterval(n: number | F32Interval): F32Interval {
  if (n instanceof F32Interval) {
    return n;
  }
  return new F32Interval(n, n);
}

/**
 * A function that converts a point to an acceptance interval.
 * This is the public facing API for builtin implementations that is called
 * from tests.
 */
export interface PointToInterval {
  (x: number): F32Interval;
}

/** Operation used to implement a PointToInterval */
interface PointToIntervalOp {
  /** @returns acceptance interval for a function at point x */
  impl: (x: number) => F32Interval;

  /**
   * Calculates where in the domain defined by x the min/max extrema of impl
   * occur and returns a span of those points to be used as the domain instead.
   *
   * If not defined the ends of the existing domain are assumed to be the
   * extrema.
   *
   * This is only implemented for functions that meet all of the following
   * criteria:
   *   a) non-monotonic
   *   b) used in inherited accuracy calculations
   *   c) need to take in an interval for b)
   *      i.e. fooInterval takes in x: number | F32Interval, not x: number
   */
  extrema?: (x: F32Interval) => F32Interval;
}

/**
 * A function that converts a pair of points to an acceptance interval.
 * This is the public facing API for builtin implementations that is called
 * from tests.
 */
export interface BinaryToInterval {
  (x: number, y: number): F32Interval;
}

/** Operation used to implement a BinaryToInterval */
interface BinaryToIntervalOp {
  /** @returns acceptance interval for a function at point (x, y) */
  impl: (x: number, y: number) => F32Interval;
  /**
   * Calculates where in domain defined by x & y the min/max extrema of impl
   * occur and returns spans of those points to be used as the domain instead.
   *
   * If not defined the ends of the existing domain are assumed to be the
   * extrema.
   *
   * This is only implemented for functions that meet all of the following
   * criteria:
   *   a) non-monotonic
   *   b) used in inherited accuracy calculations
   *   c) need to take in an interval for b)
   */
  extrema?: (x: F32Interval, y: F32Interval) => [F32Interval, F32Interval];
}

/**
 * A function that converts a triplet of points to an acceptance interval.
 * This is the public facing API for builtin implementations that is called
 * from tests.
 */
export interface TernaryToInterval {
  (x: number, y: number, z: number): F32Interval;
}

/** Operation used to implement a TernaryToInterval */
interface TernaryToIntervalOp {
  /** @returns acceptance interval for a function at point (x, y, z) */
  impl: (x: number, y: number, z: number) => F32Interval;
  // All current ternary operations that are used in inheritance (clamp*) are
  // monotonic, so extrema calculation isn't needed. Re-using the Op interface
  // pattern for symmetry with the other operations
}

/** Converts a point to an acceptance interval, using a specific function
 *
 * This handles correctly rounding and flushing inputs as needed.
 * Duplicate inputs are pruned before invoking op.impl.
 * op.extrema is invoked before this point in the call stack.
 *
 * @param n value to flush & round then invoke op.impl on
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
function roundAndFlushPointToInterval(n: number, op: PointToIntervalOp) {
  assert(!Number.isNaN(n), `flush not defined for NaN`);
  const values = correctlyRoundedF32(n);
  const inputs = new Set<number>([...values, ...values.map(flushSubnormalNumber)]);
  const results = new Set<F32Interval>([...inputs].map(op.impl));
  return F32Interval.span(...results);
}

/** Converts a pair to an acceptance interval, using a specific function
 *
 * This handles correctly rounding and flushing inputs as needed.
 * Duplicate inputs are pruned before invoking op.impl.
 * All unique combinations of x & y are run.
 * op.extrema is invoked before this point in the call stack.
 *
 * @param x first param to flush & round then invoke op.impl on
 * @param y second param to flush & round then invoke op.impl on
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
function roundAndFlushBinaryToInterval(x: number, y: number, op: BinaryToIntervalOp): F32Interval {
  assert(!Number.isNaN(x), `flush not defined for NaN`);
  assert(!Number.isNaN(y), `flush not defined for NaN`);
  const x_values = correctlyRoundedF32(x);
  const y_values = correctlyRoundedF32(y);
  const x_inputs = new Set<number>([...x_values, ...x_values.map(flushSubnormalNumber)]);
  const y_inputs = new Set<number>([...y_values, ...y_values.map(flushSubnormalNumber)]);
  const intervals = new Set<F32Interval>();
  x_inputs.forEach(inner_x => {
    y_inputs.forEach(inner_y => {
      intervals.add(op.impl(inner_x, inner_y));
    });
  });
  return F32Interval.span(...intervals);
}

/** Converts a triplet to an acceptance interval, using a specific function
 *
 * This handles correctly rounding and flushing inputs as needed.
 * Duplicate inputs are pruned before invoking op.impl.
 * All unique combinations of x, y & z are run.
 * op.extrema is invoked before this point in the call stack.
 *
 * @param x first param to flush & round then invoke op.impl on
 * @param y second param to flush & round then invoke op.impl on
 * @param z third param to flush & round then invoke op.impl on
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
function roundAndFlushTernaryToInterval(
  x: number,
  y: number,
  z: number,
  op: TernaryToIntervalOp
): F32Interval {
  assert(!Number.isNaN(x), `flush not defined for NaN`);
  assert(!Number.isNaN(y), `flush not defined for NaN`);
  assert(!Number.isNaN(z), `flush not defined for NaN`);
  const x_values = correctlyRoundedF32(x);
  const y_values = correctlyRoundedF32(y);
  const z_values = correctlyRoundedF32(z);
  const x_inputs = new Set<number>([...x_values, ...x_values.map(flushSubnormalNumber)]);
  const y_inputs = new Set<number>([...y_values, ...y_values.map(flushSubnormalNumber)]);
  const z_inputs = new Set<number>([...z_values, ...z_values.map(flushSubnormalNumber)]);
  const intervals = new Set<F32Interval>();
  // prettier-ignore
  x_inputs.forEach(inner_x => {
    y_inputs.forEach(inner_y => {
      z_inputs.forEach(inner_z => {
        intervals.add(op.impl(inner_x, inner_y, inner_z));
      });
    });
  });

  return F32Interval.span(...intervals);
}

/** Calculate the acceptance interval for a unary function over an interval
 *
 * If the interval is actually a point, this just decays to
 * roundAndFlushPointToInterval.
 *
 * The provided domain interval may be adjusted if the operation defines an
 * extrema function.
 *
 * @param x input domain interval
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
function runPointOp(x: F32Interval, op: PointToIntervalOp): F32Interval {
  if (x.isPoint()) {
    return roundAndFlushPointToInterval(x.begin, op);
  }

  if (op.extrema !== undefined) {
    x = op.extrema(x);
  }
  return F32Interval.span(
    roundAndFlushPointToInterval(x.begin, op),
    roundAndFlushPointToInterval(x.end, op)
  );
}

/** Calculate the acceptance interval for a binary function over an interval
 *
 * The provided domain intervals may be adjusted if the operation defines an
 * extrema function.
 *
 * @param x first input domain interval
 * @param y second input domain interval
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
// Will be used in test implementations
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function runBinaryOp(x: F32Interval, y: F32Interval, op: BinaryToIntervalOp): F32Interval {
  if (op.extrema !== undefined) {
    [x, y] = op.extrema(x, y);
  }
  const x_values = new Set<number>([x.begin, x.end]);
  const y_values = new Set<number>([y.begin, y.end]);

  const results = new Set<F32Interval>();
  x_values.forEach(inner_x => {
    y_values.forEach(inner_y => {
      results.add(roundAndFlushBinaryToInterval(inner_x, inner_y, op));
    });
  });

  return F32Interval.span(...results);
}

/** Calculate the acceptance interval for a ternary function over an interval
 *
 * The provided domain intervals may be adjusted if the operation defines an
 * extrema function.
 *
 * @param x first input domain interval
 * @param y second input domain interval
 * @param z third input domain interval
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
// Will be used in test implementations
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function runTernaryOp(
  x: F32Interval,
  y: F32Interval,
  z: F32Interval,
  op: TernaryToIntervalOp
): F32Interval {
  const x_values = new Set<number>([x.begin, x.end]);
  const y_values = new Set<number>([y.begin, y.end]);
  const z_values = new Set<number>([z.begin, z.end]);
  const results = new Set<F32Interval>();
  x_values.forEach(inner_x => {
    y_values.forEach(inner_y => {
      z_values.forEach(inner_z => {
        results.add(roundAndFlushTernaryToInterval(inner_x, inner_y, inner_z, op));
      });
    });
  });

  return F32Interval.span(...results);
}

const CorrectlyRoundedIntervalOp: PointToIntervalOp = {
  impl: (n: number) => {
    assert(!Number.isNaN(n), `absolute not defined for NaN`);
    return toInterval(n);
  },
};

/** @returns an interval of the correctly rounded values around the point */
export function correctlyRoundedInterval(n: number): F32Interval {
  return roundAndFlushPointToInterval(n, CorrectlyRoundedIntervalOp);
}

/** @returns a PointToIntervalOp for [n - error_range, n + error_range] */
function AbsoluteErrorIntervalOp(error_range: number): PointToIntervalOp {
  return {
    impl: (n: number) => {
      if (!isF32Finite(n)) {
        return toInterval(n);
      }

      assert(!Number.isNaN(n), `absolute not defined for NaN`);
      return new F32Interval(n - error_range, n + error_range);
    },
  };
}

/** @returns an interval of the absolute error around the point */
export function absoluteErrorInterval(n: number, error_range: number): F32Interval {
  error_range = Math.abs(error_range);
  return roundAndFlushPointToInterval(n, AbsoluteErrorIntervalOp(error_range));
}

/** @returns a PointToIntervalOp for [n - numULP * ULP(n), n + numULP * ULP(n)] */
function ULPIntervalOp(numULP: number): PointToIntervalOp {
  return {
    impl: (n: number) => {
      if (!isF32Finite(n)) {
        assert(!Number.isNaN(n), `ULP not defined for NaN`);
        return toInterval(n);
      }

      const ulp = oneULP(n);
      const begin = n - numULP * ulp;
      const end = n + numULP * ulp;

      return new F32Interval(
        Math.min(begin, flushSubnormalNumber(begin)),
        Math.max(end, flushSubnormalNumber(end))
      );
    },
  };
}

/** @returns an interval of N * ULP around the point */
export function ulpInterval(n: number, numULP: number): F32Interval {
  numULP = Math.abs(numULP);
  return roundAndFlushPointToInterval(n, ULPIntervalOp(numULP));
}

const AbsIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return correctlyRoundedInterval(Math.abs(n));
  },
};

/** Calculate an acceptance interval for abs(n) */
export function absInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), AbsIntervalOp);
}

const AdditionInnerOp = {
  impl: (x: number, y: number): F32Interval => {
    if (!isF32Finite(x) && isF32Finite(y)) {
      return correctlyRoundedInterval(x);
    }

    if (isF32Finite(x) && !isF32Finite(y)) {
      return correctlyRoundedInterval(y);
    }

    if (!isF32Finite(x) && !isF32Finite(y)) {
      if (Math.sign(x) === Math.sign(y)) {
        return correctlyRoundedInterval(x);
      } else {
        return F32Interval.infinite();
      }
    }
    return correctlyRoundedInterval(x + y);
  },
};

const AdditionIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    return roundAndFlushBinaryToInterval(x, y, AdditionInnerOp);
  },
};

/** Calculate an acceptance interval of x + y */
export function additionInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), AdditionIntervalOp);
}

const AtanIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return ulpInterval(Math.atan(n), 4096);
  },
};

/** Calculate an acceptance interval of atan(x) */
export function atanInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), AtanIntervalOp);
}

const Atan2IntervalOp: BinaryToIntervalOp = {
  impl: (y: number, x: number): F32Interval => {
    const numULP = 4096;
    if (y === 0) {
      if (x === 0) {
        return F32Interval.infinite();
      } else {
        return F32Interval.span(
          ulpInterval(kValue.f32.negative.pi.whole, numULP),
          ulpInterval(kValue.f32.positive.pi.whole, numULP)
        );
      }
    }
    return ulpInterval(Math.atan2(y, x), numULP);
  },
  extrema: (y: F32Interval, x: F32Interval): [F32Interval, F32Interval] => {
    if (y.contains(0)) {
      if (x.contains(0)) {
        return [toInterval(0), toInterval(0)];
      }
      return [toInterval(0), x];
    }
    return [y, x];
  },
};

/** Calculate an acceptance interval of atan2(y, x) */
export function atan2Interval(y: number | F32Interval, x: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(y), toInterval(x), Atan2IntervalOp);
}

const CeilIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return correctlyRoundedInterval(Math.ceil(n));
  },
};

/** Calculate an acceptance interval of ceil(x) */
export function ceilInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), CeilIntervalOp);
}

const ClampMedianIntervalOp: TernaryToIntervalOp = {
  impl: (x: number, y: number, z: number): F32Interval => {
    return correctlyRoundedInterval(
      // Default sort is string sort, so have to implement numeric comparison.
      // Cannot use the b-a one liner, because that assumes no infinities.
      [x, y, z].sort((a, b) => {
        if (a < b) {
          return -1;
        }
        if (a > b) {
          return 1;
        }
        return 0;
      })[1]
    );
  },
};

/** Calculate an acceptance interval of clamp(x, y, z) via median(x, y, z) */
export function clampMedianInterval(
  x: number | F32Interval,
  y: number | F32Interval,
  z: number | F32Interval
): F32Interval {
  return runTernaryOp(toInterval(x), toInterval(y), toInterval(z), ClampMedianIntervalOp);
}

const ClampMinMaxIntervalOp: TernaryToIntervalOp = {
  impl: (x: number, low: number, high: number): F32Interval => {
    return correctlyRoundedInterval(Math.min(Math.max(x, low), high));
  },
};

/** Calculate an acceptance interval of clamp(x, high, low) via min(max(x, low), high) */
export function clampMinMaxInterval(
  x: number | F32Interval,
  low: number | F32Interval,
  high: number | F32Interval
): F32Interval {
  return runTernaryOp(toInterval(x), toInterval(low), toInterval(high), ClampMinMaxIntervalOp);
}

const CosIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return kValue.f32.negative.pi.whole <= n && n <= kValue.f32.positive.pi.whole
      ? absoluteErrorInterval(Math.cos(n), 2 ** -11)
      : F32Interval.infinite();
  },
};

/** Calculate an acceptance interval of cos(x) */
export function cosInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), CosIntervalOp);
}

const DivisionIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    assert(
      !isSubnormalNumber(y),
      `divisionInterval impl should never receive y === 0 or flush(y) === 0`
    );
    return ulpInterval(x / y, 2.5);
  },
};

/** Calculate an acceptance interval of x / y */
export function divisionInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  {
    const Y = toInterval(y);
    const lower_bound = 2 ** -126;
    const upper_bound = 2 ** 126;
    // division accuracy is not defined outside of |denominator| on [2 ** -126, 2 ** 126]
    if (
      !new F32Interval(-upper_bound, -lower_bound).contains(Y) &&
      !new F32Interval(lower_bound, upper_bound).contains(Y)
    ) {
      return F32Interval.infinite();
    }
  }

  return runBinaryOp(toInterval(x), toInterval(y), DivisionIntervalOp);
}

const ExpIntervalOp: PointToIntervalOp = {
  impl: (x: number): F32Interval => {
    return ulpInterval(Math.exp(x), 3 + 2 * Math.abs(x));
  },
};

/** Calculate an acceptance interval for exp(x) */
export function expInterval(x: number | F32Interval): F32Interval {
  return runPointOp(toInterval(x), ExpIntervalOp);
}

const Exp2IntervalOp: PointToIntervalOp = {
  impl: (x: number): F32Interval => {
    return ulpInterval(Math.pow(2, x), 3 + 2 * Math.abs(x));
  },
};

/** Calculate an acceptance interval for exp2(x) */
export function exp2Interval(x: number | F32Interval): F32Interval {
  return runPointOp(toInterval(x), Exp2IntervalOp);
}

const FloorIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return correctlyRoundedInterval(Math.floor(n));
  },
};

/** Calculate an acceptance interval of floor(x) */
export function floorInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), FloorIntervalOp);
}

const FractIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    // fract(x) = x - floor(x) is defined in the spec.
    // For people coming from a non-graphics background this will cause some unintuitive results. For example,
    // fract(-1.1) is not 0.1 or -0.1, but instead 0.9.
    // This is how other shading languages operate and allows for a desirable wrap around in graphics programming.
    const result = subtractionInterval(n, floorInterval(n));
    if (result.contains(1)) {
      // Very small negative numbers can lead to catastrophic cancellation, thus calculating a fract of 1.0, which is
      // technically not a fractional part, so some implementations clamp the result to next nearest number.
      return F32Interval.span(result, toInterval(kValue.f32.positive.less_than_one));
    }
    return result;
  },
};

/** Calculate an acceptance interval of fract(x) */
export function fractInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), FractIntervalOp);
}

const InverseSqrtIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    if (n <= 0) {
      // 1 / sqrt(n) for n <= 0 is not meaningfully defined for real f32
      return F32Interval.infinite();
    }
    return ulpInterval(1 / Math.sqrt(n), 2);
  },
};

/** Calculate an acceptance interval of inverseSqrt(x) */
export function inverseSqrtInterval(n: number | F32Interval): F32Interval {
  return runPointOp(toInterval(n), InverseSqrtIntervalOp);
}

const LogIntervalOp: PointToIntervalOp = {
  impl: (x: number): F32Interval => {
    // log is not defined in the real plane for x <= 0
    if (x <= 0.0) {
      return F32Interval.infinite();
    }

    if (x >= 0.5 && x <= 2.0) {
      return absoluteErrorInterval(Math.log(x), 2 ** -21);
    }
    return ulpInterval(Math.log(x), 3);
  },
};

/** Calculate an acceptance interval of log(x) */
export function logInterval(x: number | F32Interval): F32Interval {
  return runPointOp(toInterval(x), LogIntervalOp);
}

const Log2IntervalOp: PointToIntervalOp = {
  impl: (x: number): F32Interval => {
    // log2 is not defined in the real plane for x <= 0
    if (x <= 0.0) {
      return F32Interval.infinite();
    }

    if (x >= 0.5 && x <= 2.0) {
      return absoluteErrorInterval(Math.log2(x), 2 ** -21);
    }
    return ulpInterval(Math.log2(x), 3);
  },
};

/** Calculate an acceptance interval of log2(x) */
export function log2Interval(x: number | F32Interval): F32Interval {
  return runPointOp(toInterval(x), Log2IntervalOp);
}

const MaxIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    return correctlyRoundedInterval(Math.max(x, y));
  },
};

/** Calculate an acceptance interval of max(x, y) */
export function maxInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), MaxIntervalOp);
}

const MinIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    return correctlyRoundedInterval(Math.min(x, y));
  },
};

/** Calculate an acceptance interval of min(x, y) */
export function minInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), MinIntervalOp);
}

const MultiplicationInnerOp = {
  impl: (x: number, y: number): F32Interval => {
    if (x === 0 || y === 0) {
      return correctlyRoundedInterval(0);
    }

    const appropriate_infinity =
      Math.sign(x) === Math.sign(y) ? Number.POSITIVE_INFINITY : Number.NEGATIVE_INFINITY;

    if (!isF32Finite(x) || !isF32Finite(y)) {
      return correctlyRoundedInterval(appropriate_infinity);
    }

    return correctlyRoundedInterval(x * y);
  },
};

const MultiplicationIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    return roundAndFlushBinaryToInterval(x, y, MultiplicationInnerOp);
  },
};

/** Calculate an acceptance interval of x * y */
export function multiplicationInterval(
  x: number | F32Interval,
  y: number | F32Interval
): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), MultiplicationIntervalOp);
}

const NegationIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return correctlyRoundedInterval(-n);
  },
};

/** Calculate an acceptance interval of -x */
export function negationInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), NegationIntervalOp);
}

const SinIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return kValue.f32.negative.pi.whole <= n && n <= kValue.f32.positive.pi.whole
      ? absoluteErrorInterval(Math.sin(n), 2 ** -11)
      : F32Interval.infinite();
  },
};

/** Calculate an acceptance interval of sin(x) */
export function sinInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), SinIntervalOp);
}

const SubtractionInnerOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    if (!isF32Finite(x) && isF32Finite(y)) {
      return correctlyRoundedInterval(x);
    }

    if (isF32Finite(x) && !isF32Finite(y)) {
      const result = Math.sign(y) > 0 ? Number.NEGATIVE_INFINITY : Number.POSITIVE_INFINITY;
      return correctlyRoundedInterval(result);
    }

    if (!isF32Finite(x) && !isF32Finite(y)) {
      if (Math.sign(x) === -Math.sign(y)) {
        return correctlyRoundedInterval(x);
      } else {
        return F32Interval.infinite();
      }
    }
    return correctlyRoundedInterval(x - y);
  },
};

const SubtractionIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    return roundAndFlushBinaryToInterval(x, y, SubtractionInnerOp);
  },
};

/** Calculate an acceptance interval of x - y */
export function subtractionInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), SubtractionIntervalOp);
}

const TanIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return divisionInterval(sinInterval(n), cosInterval(n));
  },
};

/** Calculate an acceptance interval of tan(x) */
export function tanInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), TanIntervalOp);
}
