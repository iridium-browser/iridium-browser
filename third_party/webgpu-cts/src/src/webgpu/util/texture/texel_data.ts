import { assert, unreachable } from '../../../common/util/util.js';
import { UncompressedTextureFormat } from '../../capability_info.js';
import {
  assertInIntegerRange,
  float32ToFloatBits,
  float32ToFloat16Bits,
  floatAsNormalizedInteger,
  gammaCompress,
  gammaDecompress,
  normalizedIntegerAsFloat,
  packRGB9E5UFloat,
} from '../conversion.js';

/** A component of a texture format: R, G, B, A, Depth, or Stencil. */
export const enum TexelComponent {
  R = 'R',
  G = 'G',
  B = 'B',
  A = 'A',
  Depth = 'Depth',
  Stencil = 'Stencil',
}

/** Arbitrary data, per component of a texel format. */
export type PerTexelComponent<T> = { [c in TexelComponent]?: T };

/** How a component is encoded in its bit range of a texel format. */
export type ComponentDataType = 'uint' | 'sint' | 'unorm' | 'snorm' | 'float' | 'ufloat' | null;

/**
 * Maps component values to component values
 * @param {PerTexelComponent<number>} components - The input components.
 * @returns {PerTexelComponent<number>} The new output components.
 */
type ComponentMapFn = (components: PerTexelComponent<number>) => PerTexelComponent<number>;

/**
 * Packs component values as an ArrayBuffer
 * @param {PerTexelComponent<number>} components - The input components.
 * @returns {ArrayBuffer} The packed data.
 */
type ComponentPackFn = (components: PerTexelComponent<number>) => ArrayBuffer;

/**
 * Create a PerTexelComponent object filled with the same value for all components.
 * @param {TexelComponent[]} components - The component names.
 * @param {T} value - The value to assign to each component.
 * @returns {PerTexelComponent<T>}
 */
function makePerTexelComponent<T>(components: TexelComponent[], value: T): PerTexelComponent<T> {
  const values: PerTexelComponent<T> = {};
  for (const c of components) {
    values[c] = value;
  }
  return values;
}

/**
 * Create a function which applies clones a `PerTexelComponent<number>` and then applies the
 * function `fn` to each component of `components`.
 * @param {(value: number) => number} fn - The mapping function to apply to component values.
 * @param {TexelComponent[]} components - The component names.
 * @returns {ComponentMapFn} The map function which clones the input component values, and applies
 *                           `fn` to each of component of `components`.
 */
function applyEach(fn: (value: number) => number, components: TexelComponent[]): ComponentMapFn {
  return (values: PerTexelComponent<number>) => {
    values = Object.assign({}, values);
    for (const c of components) {
      assert(values[c] !== undefined);
      values[c] = fn(values[c]!);
    }
    return values;
  };
}

/**
 * A `ComponentMapFn` for encoding sRGB.
 * @param {PerTexelComponent<number>} components - The input component values.
 * @returns {TexelComponent<number>} Gamma-compressed copy of `components`.
 */
const encodeSRGB: ComponentMapFn = components => {
  assert(
    components.R !== undefined && components.G !== undefined && components.B !== undefined,
    'sRGB requires all of R, G, and B components'
  );
  return applyEach(gammaCompress, kRGB)(components);
};

/**
 * A `ComponentMapFn` for decoding sRGB.
 * @param {PerTexelComponent<number>} components - The input component values.
 * @returns {TexelComponent<number>} Gamma-decompressed copy of `components`.
 */
const decodeSRGB: ComponentMapFn = components => {
  components = Object.assign({}, components);
  assert(
    components.R !== undefined && components.G !== undefined && components.B !== undefined,
    'sRGB requires all of R, G, and B components'
  );
  return applyEach(gammaDecompress, kRGB)(components);
};

/**
 * Helper function to pack components as an ArrayBuffer.
 * @param {TexelComponent[]} componentOrder - The order of the component data.
 * @param {PerTexelComponent<number>} components - The input component values.
 * @param {number | PerTexelComponent<number>} bitLengths - The length in bits of each component.
 *   If a single number, all components are the same length, otherwise this is a dictionary of
 *   per-component bit lengths.
 * @param {ComponentDataType | PerTexelComponent<ComponentDataType>} componentDataTypes -
 *   The type of the data in `components`. If a single value, all components have the same value.
 *   Otherwise, this is a dictionary of per-component data types.
 * @returns {ArrayBuffer} The packed component data.
 */
function packComponents(
  componentOrder: TexelComponent[],
  components: PerTexelComponent<number>,
  bitLengths: number | PerTexelComponent<number>,
  componentDataTypes: ComponentDataType | PerTexelComponent<ComponentDataType>
): ArrayBuffer {
  const bitLengthMap =
    typeof bitLengths === 'number' ? makePerTexelComponent(componentOrder, bitLengths) : bitLengths;

  const componentDataTypeMap =
    typeof componentDataTypes === 'string' || componentDataTypes === null
      ? makePerTexelComponent(componentOrder, componentDataTypes)
      : componentDataTypes;

  const totalBitLength = Object.entries(bitLengthMap).reduce((acc, [, value]) => {
    assert(value !== undefined);
    return acc + value;
  }, 0);
  assert(totalBitLength % 8 === 0);

  const data = new ArrayBuffer(totalBitLength / 8);
  let bitOffset = 0;
  for (const c of componentOrder) {
    const value = components[c];
    const type = componentDataTypeMap[c];
    const bitLength = bitLengthMap[c];
    assert(value !== undefined);
    assert(type !== undefined);
    assert(bitLength !== undefined);

    const byteOffset = Math.floor(bitOffset / 8);
    const byteLength = Math.ceil(bitLength / 8);
    switch (type) {
      case 'uint':
      case 'unorm':
        if (byteOffset === bitOffset / 8 && byteLength === bitLength / 8) {
          switch (byteLength) {
            case 1:
              new DataView(data, byteOffset, byteLength).setUint8(0, value);
              break;
            case 2:
              new DataView(data, byteOffset, byteLength).setUint16(0, value, true);
              break;
            case 4:
              new DataView(data, byteOffset, byteLength).setUint32(0, value, true);
              break;
            default:
              unreachable();
          }
        } else {
          // Packed representations are all 32-bit and use Uint as the data type.
          // ex.) rg10b11float, rgb10a2unorm
          const view = new DataView(data);
          switch (view.byteLength) {
            case 4: {
              const currentValue = view.getUint32(0, true);

              let mask = 0xffffffff;
              const bitsToClearRight = bitOffset;
              const bitsToClearLeft = 32 - (bitLength + bitOffset);

              mask = (mask >>> bitsToClearRight) << bitsToClearRight;
              mask = (mask << bitsToClearLeft) >>> bitsToClearLeft;

              const newValue = (currentValue & ~mask) | (value << bitOffset);

              view.setUint32(0, newValue, true);
              break;
            }
            default:
              unreachable();
          }
        }
        break;
      case 'sint':
      case 'snorm':
        assert(byteOffset === bitOffset / 8 && byteLength === bitLength / 8);
        switch (byteLength) {
          case 1:
            new DataView(data, byteOffset, byteLength).setInt8(0, value);
            break;
          case 2:
            new DataView(data, byteOffset, byteLength).setInt16(0, value, true);
            break;
          case 4:
            new DataView(data, byteOffset, byteLength).setInt32(0, value, true);
            break;
          default:
            unreachable();
        }
        break;
      case 'float':
        assert(byteOffset === bitOffset / 8 && byteLength === bitLength / 8);
        switch (byteLength) {
          case 4:
            new DataView(data, byteOffset, byteLength).setFloat32(0, value, true);
            break;
          default:
            unreachable();
        }
        break;
      case 'ufloat':
      case null:
        unreachable();
    }

    bitOffset += bitLength;
  }

  return data;
}

/**
 * Create an entry in `kTexelRepresentationInfo` for normalized integer texel data with constant
 * bitlength.
 * @param {TexelComponent[]} componentOrder - The order of the component data.
 * @param {number} bitLength - The number of bits in each component.
 * @param {{signed: boolean; sRGB: boolean}} opt - Boolean flags for `signed` and `sRGB`.
 */
function makeNormalizedInfo(
  componentOrder: TexelComponent[],
  bitLength: number,
  opt: { signed: boolean; sRGB: boolean }
) {
  const encodeNonSRGB = applyEach(
    (n: number) => floatAsNormalizedInteger(n, bitLength, opt.signed),
    componentOrder
  );
  const decodeNonSRGB = applyEach(
    (n: number) => normalizedIntegerAsFloat(n, bitLength, opt.signed),
    componentOrder
  );

  let encode: ComponentMapFn;
  let decode: ComponentMapFn;
  if (opt.sRGB) {
    encode = components => encodeNonSRGB(encodeSRGB(components));
    decode = components => decodeSRGB(decodeNonSRGB(components));
  } else {
    encode = encodeNonSRGB;
    decode = decodeNonSRGB;
  }

  const dataType = opt.signed ? 'snorm' : ('unorm' as ComponentDataType);
  return {
    componentOrder,
    componentInfo: makePerTexelComponent(componentOrder, {
      dataType,
      bitLength,
    }),
    encode,
    decode,
    pack: (components: PerTexelComponent<number>) =>
      packComponents(componentOrder, components, bitLength, dataType),
  };
}

/**
 * Create an entry in `kTexelRepresentationInfo` for integer texel data with constant bitlength.
 * @param {TexelComponent[]} componentOrder - The order of the component data.
 * @param {number} bitLength - The number of bits in each component.
 * @param {{signed: boolean}} opt - Boolean flag for `signed`.
 */
function makeIntegerInfo(
  componentOrder: TexelComponent[],
  bitLength: number,
  opt: { signed: boolean }
) {
  const encode = applyEach(
    (n: number) => (assertInIntegerRange(n, bitLength, opt.signed), n),
    componentOrder
  );
  const decode = applyEach(
    (n: number) => (assertInIntegerRange(n, bitLength, opt.signed), n),
    componentOrder
  );

  const dataType = opt.signed ? 'sint' : ('uint' as ComponentDataType);
  return {
    componentOrder,
    componentInfo: makePerTexelComponent(componentOrder, {
      dataType,
      bitLength,
    }),
    encode,
    decode,
    pack: (components: PerTexelComponent<number>) =>
      packComponents(componentOrder, components, bitLength, dataType),
  };
}

/**
 * Create an entry in `kTexelRepresentationInfo` for floating point texel data with constant
 * bitlength.
 * @param {TexelComponent[]} componentOrder - The order of the component data.
 * @param {number} bitLength - The number of bits in each component.
 */
function makeFloatInfo(componentOrder: TexelComponent[], bitLength: number) {
  // MAINTENANCE_TODO: Use |bitLength| to round float values based on precision.
  const encode = applyEach(identity, componentOrder);
  const decode = applyEach(identity, componentOrder);

  return {
    componentOrder,
    componentInfo: makePerTexelComponent(componentOrder, {
      dataType: 'float' as ComponentDataType,
      bitLength,
    }),
    encode,
    decode,
    pack: (components: PerTexelComponent<number>) => {
      switch (bitLength) {
        case 16:
          components = applyEach(
            (n: number) => float32ToFloat16Bits(n),
            componentOrder
          )(components);
          return packComponents(componentOrder, components, 16, 'uint');
        case 32:
          return packComponents(componentOrder, components, bitLength, 'float');
        default:
          unreachable();
      }
    },
  };
}

const kR = [TexelComponent.R];
const kRG = [TexelComponent.R, TexelComponent.G];
const kRGB = [TexelComponent.R, TexelComponent.G, TexelComponent.B];
const kRGBA = [TexelComponent.R, TexelComponent.G, TexelComponent.B, TexelComponent.A];
const kBGRA = [TexelComponent.B, TexelComponent.G, TexelComponent.R, TexelComponent.A];

const identity = (n: number) => n;

export type TexelRepresentationInfo = {
  /** Order of components in the packed representation. */
  readonly componentOrder: TexelComponent[];
  /** Data type and bit length of each component in the format. */
  readonly componentInfo: PerTexelComponent<{
    dataType: ComponentDataType;
    bitLength: number;
  }>;
  /** Encode shader values into their data representation. ex.) float 1.0 -> unorm8 255 */
  readonly encode: ComponentMapFn;
  /** Decode the data representation into the shader values. ex.) unorm8 255 -> float 1.0 */
  readonly decode: ComponentMapFn;
  /** Pack texel component values into an ArrayBuffer. ex.) rg8unorm {r: 0, g:255} -> 0xFF00 */
  readonly pack: ComponentPackFn;
  // Add fields as needed
};
export const kTexelRepresentationInfo: {
  readonly [k in UncompressedTextureFormat]: TexelRepresentationInfo;
} = {
  .../* prettier-ignore */ {
    'r8unorm':               makeNormalizedInfo(   kR,  8, { signed: false, sRGB: false }),
    'r8snorm':               makeNormalizedInfo(   kR,  8, { signed:  true, sRGB: false }),
    'r8uint':                makeIntegerInfo(      kR,  8, { signed: false }),
    'r8sint':                makeIntegerInfo(      kR,  8, { signed:  true }),
    'r16uint':               makeIntegerInfo(      kR, 16, { signed: false }),
    'r16sint':               makeIntegerInfo(      kR, 16, { signed:  true }),
    'r16float':              makeFloatInfo(        kR, 16),
    'rg8unorm':              makeNormalizedInfo(  kRG,  8, { signed: false, sRGB: false }),
    'rg8snorm':              makeNormalizedInfo(  kRG,  8, { signed:  true, sRGB: false }),
    'rg8uint':               makeIntegerInfo(     kRG,  8, { signed: false }),
    'rg8sint':               makeIntegerInfo(     kRG,  8, { signed:  true }),
    'r32uint':               makeIntegerInfo(      kR, 32, { signed: false }),
    'r32sint':               makeIntegerInfo(      kR, 32, { signed:  true }),
    'r32float':              makeFloatInfo(        kR, 32),
    'rg16uint':              makeIntegerInfo(     kRG, 16, { signed: false }),
    'rg16sint':              makeIntegerInfo(     kRG, 16, { signed:  true }),
    'rg16float':             makeFloatInfo(       kRG, 16),
    'rgba8unorm':            makeNormalizedInfo(kRGBA,  8, { signed: false, sRGB: false }),
    'rgba8unorm-srgb':       makeNormalizedInfo(kRGBA,  8, { signed: false, sRGB:  true }),
    'rgba8snorm':            makeNormalizedInfo(kRGBA,  8, { signed:  true, sRGB: false }),
    'rgba8uint':             makeIntegerInfo(   kRGBA,  8, { signed: false }),
    'rgba8sint':             makeIntegerInfo(   kRGBA,  8, { signed:  true }),
    'bgra8unorm':            makeNormalizedInfo(kBGRA,  8, { signed: false, sRGB: false }),
    'bgra8unorm-srgb':       makeNormalizedInfo(kBGRA,  8, { signed: false, sRGB:  true }),
    'rg32uint':              makeIntegerInfo(     kRG, 32, { signed: false }),
    'rg32sint':              makeIntegerInfo(     kRG, 32, { signed:  true }),
    'rg32float':             makeFloatInfo(       kRG, 32),
    'rgba16uint':            makeIntegerInfo(   kRGBA, 16, { signed: false }),
    'rgba16sint':            makeIntegerInfo(   kRGBA, 16, { signed:  true }),
    'rgba16float':           makeFloatInfo(     kRGBA, 16),
    'rgba32uint':            makeIntegerInfo(   kRGBA, 32, { signed: false }),
    'rgba32sint':            makeIntegerInfo(   kRGBA, 32, { signed:  true }),
    'rgba32float':           makeFloatInfo(     kRGBA, 32),
  },
  ...{
    rgb10a2unorm: {
      componentOrder: kRGBA,
      componentInfo: {
        R: { dataType: 'unorm', bitLength: 10 },
        G: { dataType: 'unorm', bitLength: 10 },
        B: { dataType: 'unorm', bitLength: 10 },
        A: { dataType: 'unorm', bitLength: 2 },
      },
      encode: components => {
        return {
          R: floatAsNormalizedInteger(components.R ?? unreachable(), 10, false),
          G: floatAsNormalizedInteger(components.G ?? unreachable(), 10, false),
          B: floatAsNormalizedInteger(components.B ?? unreachable(), 10, false),
          A: floatAsNormalizedInteger(components.A ?? unreachable(), 2, false),
        };
      },
      decode: components => {
        return {
          R: normalizedIntegerAsFloat(components.R ?? unreachable(), 10, false),
          G: normalizedIntegerAsFloat(components.G ?? unreachable(), 10, false),
          B: normalizedIntegerAsFloat(components.B ?? unreachable(), 10, false),
          A: normalizedIntegerAsFloat(components.A ?? unreachable(), 2, false),
        };
      },
      pack: components =>
        packComponents(
          kRGBA,
          components,
          {
            R: 10,
            G: 10,
            B: 10,
            A: 2,
          },
          'uint'
        ),
    },
    rg11b10ufloat: {
      componentOrder: kRGB,
      encode: applyEach(identity, kRGB),
      decode: applyEach(identity, kRGB),
      componentInfo: {
        R: { dataType: 'ufloat', bitLength: 11 },
        G: { dataType: 'ufloat', bitLength: 11 },
        B: { dataType: 'ufloat', bitLength: 10 },
      },
      pack: components => {
        components = {
          R: float32ToFloatBits(components.R ?? unreachable(), 0, 5, 6, 15),
          G: float32ToFloatBits(components.G ?? unreachable(), 0, 5, 6, 15),
          B: float32ToFloatBits(components.B ?? unreachable(), 0, 5, 5, 15),
        };
        return packComponents(
          kRGB,
          components,
          {
            R: 11,
            G: 11,
            B: 10,
          },
          'uint'
        );
      },
    },
    rgb9e5ufloat: {
      componentOrder: kRGB,
      componentInfo: makePerTexelComponent(kRGB, {
        dataType: 'ufloat',
        bitLength: -1, // Components don't really have a bitLength since the format is packed.
      }),
      encode: applyEach(identity, kRGB),
      decode: applyEach(identity, kRGB),
      pack: components =>
        new Uint32Array([
          packRGB9E5UFloat(
            components.R ?? unreachable(),
            components.G ?? unreachable(),
            components.B ?? unreachable()
          ),
        ]).buffer,
    },
    depth32float: {
      componentOrder: [TexelComponent.Depth],
      encode: applyEach((n: number) => (assert(n >= 0 && n <= 1.0), n), [TexelComponent.Depth]),
      decode: applyEach((n: number) => (assert(n >= 0 && n <= 1.0), n), [TexelComponent.Depth]),
      componentInfo: { Depth: { dataType: 'float', bitLength: 32 } },
      pack: components => packComponents([TexelComponent.Depth], components, 32, 'float'),
    },
    depth16unorm: makeNormalizedInfo([TexelComponent.Depth], 16, { signed: false, sRGB: false }),
    depth24plus: {
      componentOrder: [TexelComponent.Depth],
      componentInfo: { Depth: { dataType: null, bitLength: 24 } },
      encode: applyEach(() => unreachable('depth24plus cannot be encoded'), [TexelComponent.Depth]),
      decode: applyEach(() => unreachable('depth24plus cannot be decoded'), [TexelComponent.Depth]),
      pack: () => unreachable('depth24plus data cannot be packed'),
    },
    stencil8: {
      componentOrder: [TexelComponent.Stencil],
      componentInfo: { Stencil: { dataType: 'uint', bitLength: 8 } },
      encode: components => {
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return components;
      },
      decode: components => {
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return components;
      },
      pack: components => packComponents([TexelComponent.Stencil], components, 8, 'uint'),
    },
    'depth24unorm-stencil8': {
      componentOrder: [TexelComponent.Depth, TexelComponent.Stencil],
      componentInfo: {
        Depth: {
          dataType: 'unorm',
          bitLength: 24,
        },
        Stencil: {
          dataType: 'uint',
          bitLength: 8,
        },
      },
      encode: components => {
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return {
          Depth: floatAsNormalizedInteger(components.Depth ?? unreachable(), 24, false),
          Stencil: components.Stencil,
        };
      },
      decode: components => {
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return {
          Depth: normalizedIntegerAsFloat(components.Depth ?? unreachable(), 24, false),
          Stencil: components.Stencil,
        };
      },
      pack: () => unreachable('depth24unorm-stencil8 data cannot be packed'),
    },
    'depth32float-stencil8': {
      componentOrder: [TexelComponent.Depth, TexelComponent.Stencil],
      componentInfo: {
        Depth: {
          dataType: 'float',
          bitLength: 32,
        },
        Stencil: {
          dataType: 'uint',
          bitLength: 8,
        },
      },
      encode: components => {
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return components;
      },
      decode: components => {
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return components;
      },
      pack: () => unreachable('depth32float-stencil8 data cannot be packed'),
    },
    'depth24plus-stencil8': {
      componentOrder: [TexelComponent.Depth, TexelComponent.Stencil],
      componentInfo: {
        Depth: {
          dataType: null,
          bitLength: 24,
        },
        Stencil: {
          dataType: 'uint',
          bitLength: 8,
        },
      },
      encode: components => {
        assert(components.Depth === undefined, 'depth24plus cannot be encoded');
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return components;
      },
      decode: components => {
        assert(components.Depth === undefined, 'depth24plus cannot be decoded');
        assert(components.Stencil !== undefined);
        assertInIntegerRange(components.Stencil, 8, false);
        return components;
      },
      pack: () => unreachable('depth24plus-stencil8 data cannot be packed'),
    },
  },
};

/**
 * Get the `ComponentDataType` for a format. All components must have the same type.
 * @param {UncompressedTextureFormat} format - The input format.
 * @returns {ComponentDataType} The data of the components.
 */
export function getSingleDataType(format: UncompressedTextureFormat): ComponentDataType {
  const infos = Object.values(kTexelRepresentationInfo[format].componentInfo);
  assert(infos.length > 0);
  return infos.reduce((acc, cur) => {
    assert(cur !== undefined);
    assert(acc === undefined || acc === cur.dataType);
    return cur.dataType;
  }, infos[0]!.dataType);
}

/**
 *  Get traits for generating code to readback data from a component.
 * @param {ComponentDataType} dataType - The input component data type.
 * @returns A dictionary containing the respective `ReadbackTypedArray` and `shaderType`.
 */
export function getComponentReadbackTraits(dataType: ComponentDataType) {
  switch (dataType) {
    case 'ufloat':
    case 'float':
    case 'unorm':
    case 'snorm':
      return {
        ReadbackTypedArray: Float32Array,
        shaderType: 'f32' as const,
      };
    case 'uint':
      return {
        ReadbackTypedArray: Uint32Array,
        shaderType: 'u32' as const,
      };
    case 'sint':
      return {
        ReadbackTypedArray: Int32Array,
        shaderType: 'i32' as const,
      };
    default:
      unreachable();
  }
}
