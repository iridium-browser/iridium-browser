import { Colors } from '../../common/util/colors.js';
import { assert, TypedArrayBufferView } from '../../common/util/util.js';

import { clamp } from './math.js';

/**
 * Encodes a JS `number` into a "normalized" (unorm/snorm) integer representation with `bits` bits.
 * Input must be between -1 and 1 if signed, or 0 and 1 if unsigned.
 */
export function floatAsNormalizedInteger(float: number, bits: number, signed: boolean): number {
  if (signed) {
    assert(float >= -1 && float <= 1);
    const max = Math.pow(2, bits - 1) - 1;
    return Math.round(float * max);
  } else {
    assert(float >= 0 && float <= 1);
    const max = Math.pow(2, bits) - 1;
    return Math.round(float * max);
  }
}

/**
 * Decodes a JS `number` from a "normalized" (unorm/snorm) integer representation with `bits` bits.
 * Input must be an integer in the range of the specified unorm/snorm type.
 */
export function normalizedIntegerAsFloat(integer: number, bits: number, signed: boolean): number {
  assert(Number.isInteger(integer));
  if (signed) {
    const max = Math.pow(2, bits - 1) - 1;
    assert(integer >= -max - 1 && integer <= max);
    if (integer === -max - 1) {
      integer = -max;
    }
    return integer / max;
  } else {
    const max = Math.pow(2, bits) - 1;
    assert(integer >= 0 && integer <= max);
    return integer / max;
  }
}

/**
 * Encodes a JS `number` into an IEEE754 floating point number with the specified number of
 * sign, exponent, mantissa bits, and exponent bias.
 * Returns the result as an integer-valued JS `number`.
 *
 * Does not handle clamping, underflow, overflow, or denormalized numbers.
 */
export function float32ToFloatBits(
  n: number,
  signBits: 0 | 1,
  exponentBits: number,
  mantissaBits: number,
  bias: number
): number {
  assert(exponentBits <= 8);
  assert(mantissaBits <= 23);
  assert(Number.isFinite(n));

  if (n === 0) {
    return 0;
  }

  if (signBits === 0) {
    assert(n >= 0);
  }

  const buf = new DataView(new ArrayBuffer(Float32Array.BYTES_PER_ELEMENT));
  buf.setFloat32(0, n, true);
  const bits = buf.getUint32(0, true);
  // bits (32): seeeeeeeefffffffffffffffffffffff

  const mantissaBitsToDiscard = 23 - mantissaBits;

  // 0 or 1
  const sign = (bits >> 31) & signBits;

  // >> to remove mantissa, & to remove sign, - 127 to remove bias.
  const exp = ((bits >> 23) & 0xff) - 127;

  // Convert to the new biased exponent.
  const newBiasedExp = bias + exp;
  assert(newBiasedExp >= 0 && newBiasedExp < 1 << exponentBits);

  // Mask only the mantissa, and discard the lower bits.
  const newMantissa = (bits & 0x7fffff) >> mantissaBitsToDiscard;
  return (sign << (exponentBits + mantissaBits)) | (newBiasedExp << mantissaBits) | newMantissa;
}

/**
 * Encodes a JS `number` into an IEEE754 16 bit floating point number.
 * Returns the result as an integer-valued JS `number`.
 *
 * Does not handle clamping, underflow, overflow, or denormalized numbers.
 */
export function float32ToFloat16Bits(n: number) {
  return float32ToFloatBits(n, 1, 5, 10, 15);
}

/**
 * Decodes an IEEE754 16 bit floating point number into a JS `number` and returns.
 */
export function float16BitsToFloat32(float16Bits: number): number {
  const buf = new DataView(new ArrayBuffer(Float32Array.BYTES_PER_ELEMENT));
  // shift exponent and mantissa bits and fill with 0 on right, shift sign bit
  buf.setUint32(0, ((float16Bits & 0x7fff) << 13) | ((float16Bits & 0x8000) << 16), true);
  // shifting for bias different: f16 uses a bias of 15, f32 uses a bias of 127
  return buf.getFloat32(0, true) * 2 ** (127 - 15);
}

/**
 * Encodes three JS `number` values into RGB9E5, returned as an integer-valued JS `number`.
 *
 * RGB9E5 represents three partial-precision floating-point numbers encoded into a single 32-bit
 * value all sharing the same 5-bit exponent.
 * There is no sign bit, and there is a shared 5-bit biased (15) exponent and a 9-bit
 * mantissa for each channel. The mantissa does NOT have an implicit leading "1.",
 * and instead has an implicit leading "0.".
 */
export function packRGB9E5UFloat(r: number, g: number, b: number): number {
  for (const v of [r, g, b]) {
    assert(v >= 0 && v < Math.pow(2, 16));
  }

  const buf = new DataView(new ArrayBuffer(Float32Array.BYTES_PER_ELEMENT));
  const extractMantissaAndExponent = (n: number) => {
    const mantissaBits = 9;
    buf.setFloat32(0, n, true);
    const bits = buf.getUint32(0, true);
    // >> to remove mantissa, & to remove sign
    let biasedExponent = (bits >> 23) & 0xff;
    const mantissaBitsToDiscard = 23 - mantissaBits;
    let mantissa = (bits & 0x7fffff) >> mantissaBitsToDiscard;

    // RGB9E5UFloat has an implicit leading 0. instead of a leading 1.,
    // so we need to move the 1. into the mantissa and bump the exponent.
    // For float32 encoding, the leading 1 is only present if the biased
    // exponent is non-zero.
    if (biasedExponent !== 0) {
      mantissa = (mantissa >> 1) | 0b100000000;
      biasedExponent += 1;
    }
    return { biasedExponent, mantissa };
  };

  const { biasedExponent: rExp, mantissa: rOrigMantissa } = extractMantissaAndExponent(r);
  const { biasedExponent: gExp, mantissa: gOrigMantissa } = extractMantissaAndExponent(g);
  const { biasedExponent: bExp, mantissa: bOrigMantissa } = extractMantissaAndExponent(b);

  // Use the largest exponent, and shift the mantissa accordingly
  const exp = Math.max(rExp, gExp, bExp);
  const rMantissa = rOrigMantissa >> (exp - rExp);
  const gMantissa = gOrigMantissa >> (exp - gExp);
  const bMantissa = bOrigMantissa >> (exp - bExp);

  const bias = 15;
  const biasedExp = exp === 0 ? 0 : exp - 127 + bias;
  assert(biasedExp >= 0 && biasedExp <= 31);
  return rMantissa | (gMantissa << 9) | (bMantissa << 18) | (biasedExp << 27);
}

/**
 * Asserts that a number is within the representable (inclusive) of the integer type with the
 * specified number of bits and signedness.
 *
 * TODO: Assert isInteger? Then this function "asserts that a number is representable" by the type
 */
export function assertInIntegerRange(n: number, bits: number, signed: boolean): void {
  if (signed) {
    const min = -Math.pow(2, bits - 1);
    const max = Math.pow(2, bits - 1) - 1;
    assert(n >= min && n <= max);
  } else {
    const max = Math.pow(2, bits) - 1;
    assert(n >= 0 && n <= max);
  }
}

/**
 * Converts a linear value into a "gamma"-encoded value using the sRGB-clamped transfer function.
 */
export function gammaCompress(n: number): number {
  n = n <= 0.0031308 ? (323 * n) / 25 : (211 * Math.pow(n, 5 / 12) - 11) / 200;
  return clamp(n, { min: 0, max: 1 });
}

/**
 * Converts a "gamma"-encoded value into a linear value using the sRGB-clamped transfer function.
 */
export function gammaDecompress(n: number): number {
  n = n <= 0.04045 ? (n * 25) / 323 : Math.pow((200 * n + 11) / 211, 12 / 5);
  return clamp(n, { min: 0, max: 1 });
}

/** Converts a 32-bit float value to a 32-bit unsigned integer value */
export function float32ToUint32(f32: number): number {
  const f32Arr = new Float32Array(1);
  f32Arr[0] = f32;
  const u32Arr = new Uint32Array(f32Arr.buffer);
  return u32Arr[0];
}

/** Converts a 32-bit unsigned integer value to a 32-bit float value */
export function uint32ToFloat32(u32: number): number {
  const u32Arr = new Uint32Array(1);
  u32Arr[0] = u32;
  const f32Arr = new Float32Array(u32Arr.buffer);
  return f32Arr[0];
}

/** Converts a 32-bit float value to a 32-bit signed integer value */
export function float32ToInt32(f32: number): number {
  const f32Arr = new Float32Array(1);
  f32Arr[0] = f32;
  const i32Arr = new Int32Array(f32Arr.buffer);
  return i32Arr[0];
}

/** Converts a 32-bit unsigned integer value to a 32-bit signed integer value */
export function uint32ToInt32(u32: number): number {
  const u32Arr = new Uint32Array(1);
  u32Arr[0] = u32;
  const i32Arr = new Int32Array(u32Arr.buffer);
  return i32Arr[0];
}

/** A type of number representable by Scalar. */
export type ScalarKind = 'f32' | 'f16' | 'u32' | 'u16' | 'u8' | 'i32' | 'i16' | 'i8' | 'bool';

/** ScalarType describes the type of WGSL Scalar. */
export class ScalarType {
  readonly kind: ScalarKind; // The named type
  readonly size: number; // In bytes
  readonly read: (buf: Uint8Array, offset: number) => Scalar; // reads a scalar from a buffer

  constructor(kind: ScalarKind, size: number, read: (buf: Uint8Array, offset: number) => Scalar) {
    this.kind = kind;
    this.size = size;
    this.read = read;
  }

  public toString(): string {
    return this.kind;
  }
}

/** ScalarType describes the type of WGSL Vector. */
export class VectorType {
  readonly width: number; // Number of elements in the vector
  readonly elementType: ScalarType; // Element type

  constructor(width: number, elementType: ScalarType) {
    this.width = width;
    this.elementType = elementType;
  }

  /**
   * @returns a vector constructed from the values read from the buffer at the
   * given byte offset
   */
  public read(buf: Uint8Array, offset: number): Vector {
    const elements: Array<Scalar> = [];
    for (let i = 0; i < this.width; i++) {
      elements[i] = this.elementType.read(buf, offset);
      offset += this.elementType.size;
    }
    return new Vector(elements);
  }

  public toString(): string {
    return `vec${this.width}<${this.elementType}>`;
  }
}

// Maps a string representation of a vector type to vector type.
const vectorTypes = new Map<string, VectorType>();

export function TypeVec(width: number, elementType: ScalarType): VectorType {
  const key = `${elementType.toString()} ${width}}`;
  let ty = vectorTypes.get(key);
  if (ty !== undefined) {
    return ty;
  }
  ty = new VectorType(width, elementType);
  vectorTypes.set(key, ty);
  return ty;
}

/** Type is a ScalarType or VectorType. */
export type Type = ScalarType | VectorType;

export const TypeI32 = new ScalarType('i32', 4, (buf: Uint8Array, offset: number) =>
  i32(new Int32Array(buf.buffer, offset)[0])
);
export const TypeU32 = new ScalarType('u32', 4, (buf: Uint8Array, offset: number) =>
  u32(new Uint32Array(buf.buffer, offset)[0])
);
export const TypeF32 = new ScalarType('f32', 4, (buf: Uint8Array, offset: number) =>
  f32(new Float32Array(buf.buffer, offset)[0])
);
export const TypeI16 = new ScalarType('i16', 2, (buf: Uint8Array, offset: number) =>
  i16(new Int16Array(buf.buffer, offset)[0])
);
export const TypeU16 = new ScalarType('u16', 2, (buf: Uint8Array, offset: number) =>
  u16(new Uint16Array(buf.buffer, offset)[0])
);
export const TypeF16 = new ScalarType('f16', 2, (buf: Uint8Array, offset: number) =>
  f16Bits(new Uint16Array(buf.buffer, offset)[0])
);
export const TypeI8 = new ScalarType('i8', 1, (buf: Uint8Array, offset: number) =>
  i8(new Int8Array(buf.buffer, offset)[0])
);
export const TypeU8 = new ScalarType('u8', 1, (buf: Uint8Array, offset: number) =>
  u8(new Uint8Array(buf.buffer, offset)[0])
);
export const TypeBool = new ScalarType('bool', 4, (buf: Uint8Array, offset: number) =>
  bool(new Uint32Array(buf.buffer, offset)[0] !== 0)
);

/** @returns the number of scalar (element) types of the given Type */
export function numElementsOf(ty: Type): number {
  if (ty instanceof ScalarType) {
    return 1;
  }
  if (ty instanceof VectorType) {
    return ty.width;
  }
  throw new Error(`unhandled type ${ty}`);
}

/** @returns the scalar (element) type of the given Type */
export function scalarTypeOf(ty: Type): ScalarType {
  if (ty instanceof ScalarType) {
    return ty;
  }
  if (ty instanceof VectorType) {
    return ty.elementType;
  }
  throw new Error(`unhandled type ${ty}`);
}

/** ScalarValue is the JS type that can be held by a Scalar */
type ScalarValue = Boolean | Number;

/** Class that encapsulates a single scalar value of various types. */
export class Scalar {
  readonly value: ScalarValue; // The scalar value
  readonly type: ScalarType; // The type of the scalar
  readonly bits: Uint8Array; // The scalar value packed in a Uint8Array

  public constructor(type: ScalarType, value: ScalarValue, bits: TypedArrayBufferView) {
    this.value = value;
    this.type = type;
    this.bits = new Uint8Array(bits.buffer);
  }

  /**
   * Copies the scalar value to the Uint8Array buffer at the provided byte offset.
   * @param buffer the destination buffer
   * @param offset the byte offset within buffer
   */
  public copyTo(buffer: Uint8Array, offset: number) {
    for (let i = 0; i < this.bits.length; i++) {
      buffer[offset + i] = this.bits[i];
    }
  }

  public toString(): string {
    if (this.type.kind === 'bool') {
      return Colors.bold(this.value.toString());
    }
    switch (this.value) {
      case 0:
      case Infinity:
      case -Infinity:
        return Colors.bold(this.value.toString());
      default:
        return Colors.bold(this.value.toString()) + ' (0x' + this.value.toString(16) + ')';
    }
  }
}

/** Create an f32 from a numeric value, a JS `number`. */
export function f32(value: number): Scalar {
  const arr = new Float32Array([value]);
  return new Scalar(TypeF32, arr[0], arr);
}
/** Create an f32 from a bit representation, a uint32 represented as a JS `number`. */
export function f32Bits(bits: number): Scalar {
  const arr = new Uint32Array([bits]);
  return new Scalar(TypeF32, new Float32Array(arr.buffer)[0], arr);
}
/** Create an f16 from a bit representation, a uint16 represented as a JS `number`. */
export function f16Bits(bits: number): Scalar {
  const arr = new Uint16Array([bits]);
  return new Scalar(TypeF16, float16BitsToFloat32(bits), arr);
}

/** Create an i32 from a numeric value, a JS `number`. */
export function i32(value: number): Scalar {
  const arr = new Int32Array([value]);
  return new Scalar(TypeI32, arr[0], arr);
}
/** Create an i16 from a numeric value, a JS `number`. */
export function i16(value: number): Scalar {
  const arr = new Int16Array([value]);
  return new Scalar(TypeI16, arr[0], arr);
}
/** Create an i8 from a numeric value, a JS `number`. */
export function i8(value: number): Scalar {
  const arr = new Int8Array([value]);
  return new Scalar(TypeI8, arr[0], arr);
}

/** Create an i32 from a bit representation, a uint32 represented as a JS `number`. */
export function i32Bits(bits: number): Scalar {
  const arr = new Uint32Array([bits]);
  return new Scalar(TypeI32, new Int32Array(arr.buffer)[0], arr);
}
/** Create an i16 from a bit representation, a uint16 represented as a JS `number`. */
export function i16Bits(bits: number): Scalar {
  const arr = new Uint16Array([bits]);
  return new Scalar(TypeI16, new Int16Array(arr.buffer)[0], arr);
}
/** Create an i8 from a bit representation, a uint8 represented as a JS `number`. */
export function i8Bits(bits: number): Scalar {
  const arr = new Uint8Array([bits]);
  return new Scalar(TypeI8, new Int8Array(arr.buffer)[0], arr);
}

/** Create a u32 from a numeric value, a JS `number`. */
export function u32(value: number): Scalar {
  const arr = new Uint32Array([value]);
  return new Scalar(TypeU32, arr[0], arr);
}
/** Create a u16 from a numeric value, a JS `number`. */
export function u16(value: number): Scalar {
  const arr = new Uint16Array([value]);
  return new Scalar(TypeU16, arr[0], arr);
}
/** Create a u8 from a numeric value, a JS `number`. */
export function u8(value: number): Scalar {
  const arr = new Uint8Array([value]);
  return new Scalar(TypeU8, arr[0], arr);
}

/** Create an u32 from a bit representation, a uint32 represented as a JS `number`. */
export function u32Bits(bits: number): Scalar {
  const arr = new Uint32Array([bits]);
  return new Scalar(TypeU32, bits, arr);
}
/** Create an u16 from a bit representation, a uint16 represented as a JS `number`. */
export function u16Bits(bits: number): Scalar {
  const arr = new Uint16Array([bits]);
  return new Scalar(TypeU16, bits, arr);
}
/** Create an u8 from a bit representation, a uint8 represented as a JS `number`. */
export function u8Bits(bits: number): Scalar {
  const arr = new Uint8Array([bits]);
  return new Scalar(TypeU8, bits, arr);
}

/** Create a boolean value. */
export function bool(value: boolean): Scalar {
  // WGSL does not support using 'bool' types directly in storage / uniform
  // buffers, so instead we pack booleans in a u32, where 'false' is zero and
  // 'true' is any non-zero value.
  const arr = new Uint32Array([value ? 1 : 0]);
  return new Scalar(TypeBool, value, arr);
}

/** A 'true' literal value */
export const True = bool(true);

/** A 'false' literal value */
export const False = bool(false);

/**
 * Class that encapsulates a vector value.
 */
export class Vector {
  readonly elements: Array<Scalar>;
  readonly type: VectorType;

  public constructor(elements: Array<Scalar>) {
    if (elements.length < 2 || elements.length > 4) {
      throw new Error(`vector element count must be between 2 and 4, got ${elements.length}`);
    }
    for (let i = 1; i < elements.length; i++) {
      const a = elements[0].type;
      const b = elements[i].type;
      if (a !== b) {
        throw new Error(
          `cannot mix vector element types. Found elements with types '${a}' and '${b}'`
        );
      }
    }
    this.elements = elements;
    this.type = TypeVec(elements.length, elements[0].type);
  }

  /**
   * Copies the vector value to the Uint8Array buffer at the provided byte offset.
   * @param buffer the destination buffer
   * @param offset the byte offset within buffer
   */
  public copyTo(buffer: Uint8Array, offset: number) {
    for (const element of this.elements) {
      element.copyTo(buffer, offset);
      offset += this.type.elementType.size;
    }
  }

  public toString(): string {
    return `${this.type}(${this.elements.map(e => e.toString()).join(', ')})`;
  }

  public get x() {
    assert(0 < this.elements.length);
    return this.elements[0];
  }

  public get y() {
    assert(1 < this.elements.length);
    return this.elements[1];
  }

  public get z() {
    assert(2 < this.elements.length);
    return this.elements[2];
  }

  public get w() {
    assert(3 < this.elements.length);
    return this.elements[3];
  }
}

/** Helper for constructing a new two-element vector with the provided values */
export function vec2(x: Scalar, y: Scalar) {
  return new Vector([x, y]);
}

/** Helper for constructing a new three-element vector with the provided values */
export function vec3(x: Scalar, y: Scalar, z: Scalar) {
  return new Vector([x, y, z]);
}

/** Helper for constructing a new four-element vector with the provided values */
export function vec4(x: Scalar, y: Scalar, z: Scalar, w: Scalar) {
  return new Vector([x, y, z, w]);
}

/** Value is a Scalar or Vector value. */
export type Value = Scalar | Vector;
