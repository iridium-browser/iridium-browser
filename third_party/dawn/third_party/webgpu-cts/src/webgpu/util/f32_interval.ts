import { assert } from '../../common/util/util.js';

import { kValue } from './constants.js';
import { correctlyRoundedF32, flushSubnormalNumber, isF32Finite, oneULP } from './math.js';

/** Represents a closed interval in the f32 range */
export class F32Interval {
  public readonly begin: number;
  public readonly end: number;
  private static _any: F32Interval;

  /** Constructor
   *
   * @param bounds a pair of numbers indicating the beginning then the end of the interval
   */
  public constructor(...bounds: [number, number]) {
    const [begin, end] = bounds;
    assert(!Number.isNaN(begin) && !Number.isNaN(end), `bounds need to be non-NaN`);
    assert(begin <= end, `bounds[0] (${begin}) must be less than or equal to bounds[1]  (${end})`);

    this.begin = begin;
    this.end = end;
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
      // Being the undefined interval indicates that the accuracy is not defined
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

  /** @returns if this interval only contains f32 finite values */
  public isFinite(): boolean {
    return isF32Finite(this.begin) && isF32Finite(this.end);
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

  /** @returns a singleton for interval of all possible values
   * This interval is used in situations where accuracy is not defined, so any
   * result is valid.
   */
  public static any(): F32Interval {
    if (this._any === undefined) {
      this._any = new F32Interval(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY);
    }
    return this._any;
  }
}

/** @returns an interval containing the point or the original interval */
function toInterval(n: number | F32Interval): F32Interval {
  if (n instanceof F32Interval) {
    return n;
  }
  return new F32Interval(n, n);
}

/** F32Interval of [-π, π] */
const kNegPiToPiInterval = new F32Interval(
  kValue.f32.negative.pi.whole,
  kValue.f32.positive.pi.whole
);

/** F32Interval of values greater than 0 and less than or equal to f32 max */
const kGreaterThanZeroInterval = new F32Interval(
  kValue.f32.subnormal.positive.min,
  kValue.f32.positive.max
);

/**
 * A function that converts a point to an acceptance interval.
 * This is the public facing API for builtin implementations that is called
 * from tests.
 */
export interface PointToInterval {
  (x: number): F32Interval;
}

/** Operation used to implement a PointToInterval */
export interface PointToIntervalOp {
  /** @returns acceptance interval for a function at point x */
  impl: PointToInterval;

  /**
   * Calculates where in the domain defined by x the min/max extrema of impl
   * occur and returns a span of those points to be used as the domain instead.
   *
   * Used by runPointOp before invoking impl.
   * If not defined, the bounds of the existing domain are assumed to be the
   * extrema.
   *
   * This is only implemented for operations that meet all of the following
   * criteria:
   *   a) non-monotonic
   *   b) used in inherited accuracy calculations
   *   c) need to take in an interval for b)
   *      i.e. fooInterval takes in x: number | F32Interval, not x: number
   */
  extrema?: (x: F32Interval) => F32Interval;
}

/**
 * Restrict the inputs to an PointToInterval operation
 *
 * Only used for operations that have tighter domain requirements than 'must be
 * f32 finite'.
 *
 * @param domain interval to restrict inputs to
 * @param impl operation implementation to run if input is within the required domain
 * @returns a PointToInterval that calls impl if domain contains the input,
 *          otherwise it returns the any() interval */
function limitPointToIntervalDomain(domain: F32Interval, impl: PointToInterval): PointToInterval {
  return (n: number): F32Interval => {
    return domain.contains(n) ? impl(n) : F32Interval.any();
  };
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
  impl: BinaryToInterval;
  /**
   * Calculates where in domain defined by x & y the min/max extrema of impl
   * occur and returns spans of those points to be used as the domain instead.
   *
   * Used by runBinaryOp before invoking impl.
   * If not defined, the bounds of the existing domain are assumed to be the
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

/** Domain for a BinaryToInterval implementation */
interface BinaryToIntervalDomain {
  x: F32Interval;
  // y is an array to support handling domains composed of discrete intervals
  y: F32Interval[];
}

/**
 * Restrict the inputs to an BinaryToInterval
 *
 * Only used for operations that have tighter domain requirements than 'must be
 * f32 finite'.
 *
 * @param domain set of intervals to restrict inputs to
 * @param impl operation implementation to run if input is within the required domain
 * @returns a BinaryToInterval that calls impl if domain contains the input,
 *          otherwise it returns the any() interval */
function limitBinaryToIntervalDomain(
  domain: BinaryToIntervalDomain,
  impl: BinaryToInterval
): BinaryToInterval {
  return (x: number, y: number): F32Interval => {
    if (!domain.x.contains(x)) {
      return F32Interval.any();
    }

    if (!domain.y.some(d => d.contains(y))) {
      return F32Interval.any();
    }

    return impl(x, y);
  };
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
  // Re-using the *Op interface pattern for symmetry with the other operations.
  /** @returns acceptance interval for a function at point (x, y, z) */
  impl: TernaryToInterval;
}

/** Converts a point to an acceptance interval, using a specific function
 *
 * This handles correctly rounding and flushing inputs as needed.
 * Duplicate inputs are pruned before invoking op.impl.
 * op.extrema is invoked before this point in the call stack.
 * op.domain is tested before this point in the call stack.
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
 * op.domain is tested before this point in the call stack.
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
  if (!x.isFinite()) {
    return F32Interval.any();
  }

  if (op.extrema !== undefined) {
    x = op.extrema(x);
  }

  const result = F32Interval.span(
    roundAndFlushPointToInterval(x.begin, op),
    roundAndFlushPointToInterval(x.end, op)
  );
  return result.isFinite() ? result : F32Interval.any();
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
function runBinaryOp(x: F32Interval, y: F32Interval, op: BinaryToIntervalOp): F32Interval {
  if (!x.isFinite() || !y.isFinite()) {
    return F32Interval.any();
  }

  if (op.extrema !== undefined) {
    [x, y] = op.extrema(x, y);
  }

  const x_values = new Set<number>([x.begin, x.end]);
  const y_values = new Set<number>([y.begin, y.end]);

  const outputs = new Set<F32Interval>();
  x_values.forEach(inner_x => {
    y_values.forEach(inner_y => {
      outputs.add(roundAndFlushBinaryToInterval(inner_x, inner_y, op));
    });
  });

  const result = F32Interval.span(...outputs);
  return result.isFinite() ? result : F32Interval.any();
}

/** Calculate the acceptance interval for a ternary function over an interval
 *
 * @param x first input domain interval
 * @param y second input domain interval
 * @param z third input domain interval
 * @param op operation defining the function being run
 * @returns a span over all of the outputs of op.impl
 */
function runTernaryOp(
  x: F32Interval,
  y: F32Interval,
  z: F32Interval,
  op: TernaryToIntervalOp
): F32Interval {
  if (!x.isFinite() || !y.isFinite() || !z.isFinite()) {
    return F32Interval.any();
  }

  const x_values = new Set<number>([x.begin, x.end]);
  const y_values = new Set<number>([y.begin, y.end]);
  const z_values = new Set<number>([z.begin, z.end]);
  const outputs = new Set<F32Interval>();
  x_values.forEach(inner_x => {
    y_values.forEach(inner_y => {
      z_values.forEach(inner_z => {
        outputs.add(roundAndFlushTernaryToInterval(inner_x, inner_y, inner_z, op));
      });
    });
  });

  const result = F32Interval.span(...outputs);
  return result.isFinite() ? result : F32Interval.any();
}

/** Defines a PointToIntervalOp for an interval of the correctly rounded values around the point */
const CorrectlyRoundedIntervalOp: PointToIntervalOp = {
  impl: (n: number) => {
    assert(!Number.isNaN(n), `absolute not defined for NaN`);
    return toInterval(n);
  },
};

/** @returns an interval of the correctly rounded values around the point */
export function correctlyRoundedInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), CorrectlyRoundedIntervalOp);
}

/** @returns a PointToIntervalOp for [n - error_range, n + error_range] */
function AbsoluteErrorIntervalOp(error_range: number): PointToIntervalOp {
  const op: PointToIntervalOp = {
    impl: (_: number) => {
      return F32Interval.any();
    },
  };

  if (isF32Finite(error_range)) {
    op.impl = (n: number) => {
      assert(!Number.isNaN(n), `absolute error not defined for NaN`);
      return new F32Interval(n - error_range, n + error_range);
    };
  }

  return op;
}

/** @returns an interval of the absolute error around the point */
export function absoluteErrorInterval(n: number, error_range: number): F32Interval {
  error_range = Math.abs(error_range);
  return runPointOp(toInterval(n), AbsoluteErrorIntervalOp(error_range));
}

/** @returns a PointToIntervalOp for [n - numULP * ULP(n), n + numULP * ULP(n)] */
function ULPIntervalOp(numULP: number): PointToIntervalOp {
  const op: PointToIntervalOp = {
    impl: (_: number) => {
      return F32Interval.any();
    },
  };

  if (isF32Finite(numULP)) {
    op.impl = (n: number) => {
      assert(!Number.isNaN(n), `ULP error not defined for NaN`);

      const ulp = oneULP(n);
      const begin = n - numULP * ulp;
      const end = n + numULP * ulp;

      return new F32Interval(
        Math.min(begin, flushSubnormalNumber(begin)),
        Math.max(end, flushSubnormalNumber(end))
      );
    };
  }

  return op;
}

/** @returns an interval of N * ULP around the point */
export function ulpInterval(n: number, numULP: number): F32Interval {
  numULP = Math.abs(numULP);
  return runPointOp(toInterval(n), ULPIntervalOp(numULP));
}

const AbsIntervalOp: PointToIntervalOp = {
  impl: (n: number) => {
    return correctlyRoundedInterval(Math.abs(n));
  },
};

/** Calculate an acceptance interval for abs(n) */
export function absInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), AbsIntervalOp);
}

const AdditionIntervalOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
    return correctlyRoundedInterval(x + y);
  },
};

/** Calculate an acceptance interval of x + y */
export function additionInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), AdditionIntervalOp);
}

const AtanIntervalOp: PointToIntervalOp = {
  impl: (n: number) => {
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
        return F32Interval.any();
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
  impl: limitPointToIntervalDomain(
    kNegPiToPiInterval,
    (n: number): F32Interval => {
      return absoluteErrorInterval(Math.cos(n), 2 ** -11);
    }
  ),
};

/** Calculate an acceptance interval of cos(x) */
export function cosInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), CosIntervalOp);
}

const CoshIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    // cosh(x) = (exp(x) + exp(-x)) * 0.5
    const minus_n = negationInterval(n);
    return multiplicationInterval(additionInterval(expInterval(n), expInterval(minus_n)), 0.5);
  },
};

/** Calculate an acceptance interval of cosh(x) */
export function coshInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), CoshIntervalOp);
}

const DegreesIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return multiplicationInterval(n, 57.295779513082322865);
  },
};

/** Calculate an acceptance interval of degrees(x) */
export function degreesInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), DegreesIntervalOp);
}

const DivisionIntervalOp: BinaryToIntervalOp = {
  impl: limitBinaryToIntervalDomain(
    {
      x: new F32Interval(kValue.f32.negative.min, kValue.f32.positive.max),
      y: [new F32Interval(-(2 ** 126), -(2 ** -126)), new F32Interval(2 ** -126, 2 ** 126)],
    },
    (x: number, y: number): F32Interval => {
      if (y === 0) {
        return F32Interval.any();
      }
      return ulpInterval(x / y, 2.5);
    }
  ),
  extrema: (x: F32Interval, y: F32Interval): [F32Interval, F32Interval] => {
    // division has a discontinuity at y = 0.
    if (y.contains(0)) {
      y = toInterval(0);
    }
    return [x, y];
  },
};

/** Calculate an acceptance interval of x / y */
export function divisionInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), DivisionIntervalOp);
}

const ExpIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return ulpInterval(Math.exp(n), 3 + 2 * Math.abs(n));
  },
};

/** Calculate an acceptance interval for exp(x) */
export function expInterval(x: number | F32Interval): F32Interval {
  return runPointOp(toInterval(x), ExpIntervalOp);
}

const Exp2IntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return ulpInterval(Math.pow(2, n), 3 + 2 * Math.abs(n));
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
  impl: limitPointToIntervalDomain(
    kGreaterThanZeroInterval,
    (n: number): F32Interval => {
      return ulpInterval(1 / Math.sqrt(n), 2);
    }
  ),
};

/** Calculate an acceptance interval of inverseSqrt(x) */
export function inverseSqrtInterval(n: number | F32Interval): F32Interval {
  return runPointOp(toInterval(n), InverseSqrtIntervalOp);
}

const LdexpIntervalOp: BinaryToIntervalOp = {
  impl: limitBinaryToIntervalDomain(
    // Implementing SPIR-V's more restrictive domain until
    // https://github.com/gpuweb/gpuweb/issues/3134 is resolved
    {
      x: new F32Interval(kValue.f32.negative.min, kValue.f32.positive.max),
      y: [new F32Interval(-126, 128)],
    },
    (e1: number, e2: number): F32Interval => {
      // Though the spec says the result of ldexp(e1, e2) = e1 * 2 ^ e2, the
      // accuracy is listed as correctly rounded to the true value, so the
      // inheritance framework does not need to be invoked to determine bounds.
      // Instead the value at a higher precision is calculated and passed to
      // correctlyRoundedInterval.
      const result = e1 * 2 ** e2;
      if (Number.isNaN(result)) {
        // Overflowed TS's number type, so definitely out of bounds for f32
        return F32Interval.any();
      }
      return correctlyRoundedInterval(result);
    }
  ),
};

/** Calculate an acceptance interval of ldexp(e1, e2) */
export function ldexpInterval(e1: number, e2: number): F32Interval {
  return roundAndFlushBinaryToInterval(e1, e2, LdexpIntervalOp);
}

const LogIntervalOp: PointToIntervalOp = {
  impl: limitPointToIntervalDomain(
    kGreaterThanZeroInterval,
    (n: number): F32Interval => {
      if (n >= 0.5 && n <= 2.0) {
        return absoluteErrorInterval(Math.log(n), 2 ** -21);
      }
      return ulpInterval(Math.log(n), 3);
    }
  ),
};

/** Calculate an acceptance interval of log(x) */
export function logInterval(x: number | F32Interval): F32Interval {
  return runPointOp(toInterval(x), LogIntervalOp);
}

const Log2IntervalOp: PointToIntervalOp = {
  impl: limitPointToIntervalDomain(
    kGreaterThanZeroInterval,
    (n: number): F32Interval => {
      if (n >= 0.5 && n <= 2.0) {
        return absoluteErrorInterval(Math.log2(n), 2 ** -21);
      }
      return ulpInterval(Math.log2(n), 3);
    }
  ),
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

const MixImpreciseIntervalOp: TernaryToIntervalOp = {
  impl: (x: number, y: number, z: number): F32Interval => {
    // x + (y - x) * z =
    //  x + t, where t = (y - x) * z
    const t = multiplicationInterval(subtractionInterval(y, x), z);
    return additionInterval(x, t);
  },
};

/** Calculate an acceptance interval of mix(x, y, z) using x + (y - x) * z */
export function mixImpreciseInterval(x: number, y: number, z: number): F32Interval {
  return runTernaryOp(toInterval(x), toInterval(y), toInterval(z), MixImpreciseIntervalOp);
}

const MixPreciseIntervalOp: TernaryToIntervalOp = {
  impl: (x: number, y: number, z: number): F32Interval => {
    // x * (1.0 - z) + y * z =
    //   t + s, where t = x * (1.0 - z), s = y * z
    const t = multiplicationInterval(x, subtractionInterval(1.0, z));
    const s = multiplicationInterval(y, z);
    return additionInterval(t, s);
  },
};

/** Calculate an acceptance interval of mix(x, y, z) using x * (1.0 - z) + y * z */
export function mixPreciseInterval(x: number, y: number, z: number): F32Interval {
  return runTernaryOp(toInterval(x), toInterval(y), toInterval(z), MixPreciseIntervalOp);
}

const MultiplicationInnerOp = {
  impl: (x: number, y: number): F32Interval => {
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

const PowIntervalOp: BinaryToIntervalOp = {
  // pow(x, y) has no explicit domain restrictions, but inherits the x <= 0
  // domain restriction from log2(x). Invoking log2Interval(x) in impl will
  // enforce this, so there is no need to wrap the impl call here.
  impl: (x: number, y: number): F32Interval => {
    return exp2Interval(multiplicationInterval(y, log2Interval(x)));
  },
};

/** Calculate an acceptance interval of pow(x, y) */
export function powInterval(x: number | F32Interval, y: number | F32Interval): F32Interval {
  return runBinaryOp(toInterval(x), toInterval(y), PowIntervalOp);
}

const RadiansIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return multiplicationInterval(n, 0.017453292519943295474);
  },
};

/** Calculate an acceptance interval of radians(x) */
export function radiansInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), RadiansIntervalOp);
}

const RoundIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    const k = Math.floor(n);
    const diff_before = n - k;
    const diff_after = k + 1 - n;
    if (diff_before < diff_after) {
      return correctlyRoundedInterval(k);
    } else if (diff_before > diff_after) {
      return correctlyRoundedInterval(k + 1);
    }

    // n is in the middle of two integers.
    // The tie breaking rule is 'k if k is even, k + 1 if k is odd'
    if (k % 2 === 0) {
      return correctlyRoundedInterval(k);
    }
    return correctlyRoundedInterval(k + 1);
  },
};

/** Calculate an acceptance interval of round(x) */
export function roundInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), RoundIntervalOp);
}

/**
 * Calculate an acceptance interval of saturate(n) as clamp(n, 0.0, 1.0)
 *
 * The definition of saturate is such that both possible implementations of
 * clamp will return the same value, so arbitrarily picking the minmax version
 * to use.
 */
export function saturateInterval(n: number): F32Interval {
  return runTernaryOp(toInterval(n), toInterval(0.0), toInterval(1.0), ClampMinMaxIntervalOp);
}

const SignIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    if (n > 0.0) {
      return correctlyRoundedInterval(1.0);
    }
    if (n < 0.0) {
      return correctlyRoundedInterval(-1.0);
    }

    return correctlyRoundedInterval(0.0);
  },
};

/** Calculate an acceptance interval of sin(x) */
export function signInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), SignIntervalOp);
}

const SinIntervalOp: PointToIntervalOp = {
  impl: limitPointToIntervalDomain(
    kNegPiToPiInterval,
    (n: number): F32Interval => {
      return absoluteErrorInterval(Math.sin(n), 2 ** -11);
    }
  ),
};

/** Calculate an acceptance interval of sin(x) */
export function sinInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), SinIntervalOp);
}

const SinhIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    // sinh(x) = (exp(x) - exp(-x)) * 0.5
    const minus_n = negationInterval(n);
    return multiplicationInterval(subtractionInterval(expInterval(n), expInterval(minus_n)), 0.5);
  },
};

/** Calculate an acceptance interval of sinh(x) */
export function sinhInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), SinhIntervalOp);
}

const StepIntervalOp: BinaryToIntervalOp = {
  impl: (edge: number, x: number): F32Interval => {
    if (edge <= x) {
      return correctlyRoundedInterval(1.0);
    }
    return correctlyRoundedInterval(0.0);
  },
};

const SqrtIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return divisionInterval(1.0, inverseSqrtInterval(n));
  },
};

/** Calculate an acceptance interval of sqrt(x) */
export function sqrtInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), SqrtIntervalOp);
}

/** Calculate an acceptance 'interval' for step(edge, x)
 *
 * step only returns two possible values, so its interval requires special
 * interpretation in CTS tests.
 * This interval will be one of four values: [0, 0], [0, 1], [1, 1] & [-∞, +∞].
 * [0, 0] and [1, 1] indicate that the correct answer in point they encapsulate.
 * [0, 1] should not be treated as a span, i.e. 0.1 is acceptable, but instead
 * indicate either 0.0 or 1.0 are acceptable answers.
 * [-∞, +∞] is treated as the any interval, since an undefined or infinite value was passed in.
 */
export function stepInterval(edge: number, x: number): F32Interval {
  return runBinaryOp(toInterval(edge), toInterval(x), StepIntervalOp);
}

const SubtractionInnerOp: BinaryToIntervalOp = {
  impl: (x: number, y: number): F32Interval => {
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

const TruncIntervalOp: PointToIntervalOp = {
  impl: (n: number): F32Interval => {
    return correctlyRoundedInterval(Math.trunc(n));
  },
};

/** Calculate an acceptance interval of trunc(x) */
export function truncInterval(n: number): F32Interval {
  return runPointOp(toInterval(n), TruncIntervalOp);
}
