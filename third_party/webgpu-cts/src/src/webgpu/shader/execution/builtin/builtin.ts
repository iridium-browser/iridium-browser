import { Colors } from '../../../../common/util/colors.js';
import {
  TypedArrayBufferView,
  TypedArrayBufferViewConstructor,
} from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';
import { NumberType, NumberRepr } from '../../../util/conversion.js';

export type Case<T extends NumberType> = {
  input: NumberRepr<T>;
  expected: Array<NumberRepr<T>>;
};

export function runShaderTestImpl<F extends NumberType>(
  t: GPUTest,
  storageClass: string,
  storageMode: string,
  type: string,
  arrayLength: number,
  builtin: string,
  arrayType: TypedArrayBufferViewConstructor,
  cases: Array<Case<F>>,
  source: string
): void {
  const inputData: TypedArrayBufferView = new arrayType(cases.length * 4);

  // an arbitrary approach of mapping cases into inputData:
  // if the arrayLength > 1, ie. type is a vector, or matrix then
  //   fill the i-th element with the input value of cases[i+j]
  // if arrayLength = 1 or i+j > length of cases
  //   fill the i-th element with input value of  cases[i]
  // TODO(sarahM0): This is using a TypedArray to convert the value into bits,
  // but NumberRepr already has bits. There's a chance some numbers won't get the same bit pattern both ways
  // (having been converted from bits to double back to bits). Probably best to copy the bits in directly.
  for (let i = 0; i < cases.length; i++) {
    for (let j = 0; j < arrayLength; j++) {
      inputData[i * 4 + j] = (i + j < cases.length
        ? cases[i + j].input.value
        : cases[i].input.value) as number;
    }
  }

  const inputBuffer = t.makeBufferWithContents(
    inputData,
    GPUBufferUsage.COPY_SRC | GPUBufferUsage.STORAGE
  );

  const outputBuffer = t.device.createBuffer({
    // TODO(sarahM0): investigate why "size: cases.length*4*arrayLength" returns zero
    size: inputData.length * 4,
    usage: GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST | GPUBufferUsage.STORAGE,
  });

  const module = t.device.createShaderModule({ code: source });
  const pipeline = t.device.createComputePipeline({
    compute: { module, entryPoint: 'main' },
  });

  const group = t.device.createBindGroup({
    layout: pipeline.getBindGroupLayout(0),
    entries: [
      { binding: 0, resource: { buffer: inputBuffer } },
      { binding: 1, resource: { buffer: outputBuffer } },
    ],
  });

  const encoder = t.device.createCommandEncoder();
  const pass = encoder.beginComputePass();
  pass.setPipeline(pipeline);
  pass.setBindGroup(0, group);
  pass.dispatch(1);
  pass.endPass();

  t.queue.submit([encoder.finish()]);

  // Returns the string representation of num using the specified color.
  const formatNum = (num: number | bigint, color = Colors.reset) => {
    switch (num) {
      case 0:
      case Infinity:
      case -Infinity:
        return color.bold(num.toString());
      default:
        return color.bold(num.toString()) + color(' (0x' + num.toString(16) + ')');
    }
  };

  const checkExpectation = (outputData: typeof inputData) => {
    // The list of expectation failures
    const errs: string[] = [];

    // For each case...
    for (let i = 0; i < cases.length; i++) {
      // String representations of the input, output and expectation values for this case.
      const inputValue: string[] = [];
      const outputValue: string[] = [];
      const expectedValue: string[] = [];
      let caseMatched = true;

      // For each element in the case...
      for (let j = 0; j < arrayLength; j++) {
        const idx = i * 4 + j;
        const expectedIndex = i + j < cases.length ? i + j : i;
        inputValue.push(formatNum(inputData[idx]));

        // `cases[expectedIndex].expected` is an array of values that are treated as a pass.
        // Do any of these expected values match?
        const elementMatched = cases[expectedIndex].expected.some(e => e.value === outputData[idx]);
        outputValue.push(formatNum(outputData[idx], elementMatched ? Colors.green : Colors.red));

        const caseExpected: string[] = [];
        for (const e of cases[expectedIndex].expected) {
          if (outputData[idx] === e.value) {
            // This expected value matched
            caseExpected.push(formatNum(e.value, Colors.green));
          } else if (elementMatched) {
            // The element matched, but it wasn't for this value.
            caseExpected.push(formatNum(e.value, Colors.grey));
          } else {
            // The element did not match any expected value
            caseExpected.push(formatNum(e.value, Colors.red));
          }
        }
        expectedValue.push(caseExpected.join(' or '));

        // If none of the expected values matched, then the case has failed.
        caseMatched = caseMatched && elementMatched;
      }

      if (!caseMatched) {
        if (arrayLength > 1) {
          errs.push(
            `${builtin}(${type}(${inputValue.join(', ')}))\n` +
              `    returned: ${type}(${outputValue.join(', ')})\n` +
              `    expected: ${type}(${expectedValue.join(', ')})`
          );
        } else {
          errs.push(
            `${builtin}(${inputValue.join(', ')})\n` +
              `    returned: ${outputValue.join(', ')}\n` +
              `    expected: ${expectedValue.join(', ')}`
          );
        }
      }
    }

    if (errs.length > 0) {
      return new Error(errs.join('\n\n'));
    }
    return undefined;
  };

  t.expectGPUBufferValuesPassCheck(outputBuffer, checkExpectation, {
    type: arrayType,
    typedLength: inputData.length,
  });
}

// TODO(sarahM0): Perhaps instead of kBit and kValue tables we could have one table
// where every value is a NumberRepr instead of either bits or value?
// Then tests wouldn't need most of the NumberRepr.fromX calls,
// and you would probably need fewer table entries in total
// (since each NumberRepr has both bits and value).
export const kBit = {
  // Limits of int32
  i32: {
    positive: {
      min: 0x0000_0000, // 0
      max: 0x7fff_ffff, // 2147483647
    },
    negative: {
      min: 0x8000_0000, // -2147483648
      max: 0x0000_0000, // 0
    },
  },

  // Limits of uint32
  u32: {
    min: 0x0000_0000,
    max: 0xffff_ffff,
  },

  // Limits of f32
  f32: {
    positive: {
      min: 0x0080_0000,
      max: 0x7f7f_ffff,
      zero: 0x0000_0000,
    },
    negative: {
      max: 0x8080_0000,
      min: 0xff7f_ffff,
      zero: 0x8000_0000,
    },
    subnormal: {
      positive: {
        min: 0x0000_0001,
        max: 0x007f_ffff,
      },
    },
    nan: {
      negative: {
        s: 0xff80_0001,
        q: 0xffc0_0001,
      },
      positive: {
        s: 0x7f80_0001,
        q: 0x7fc0_0001,
      },
    },
    infinity: {
      positive: 0x7f80_0000,
      negative: 0xff80_0000,
    },
  },

  // 32-bit representation of power(2, n) n = {-31, ..., 31}
  // A uint32 representation as a JS `number`
  // {toMinus31, ..., to31} ie. {-31, ..., 31}
  powTwo: {
    toMinus1: 0x3f00_0000,
    toMinus2: 0x3e80_0000,
    toMinus3: 0x3e00_0000,
    toMinus4: 0x3d80_0000,
    toMinus5: 0x3d00_0000,
    toMinus6: 0x3c80_0000,
    toMinus7: 0x3c00_0000,
    toMinus8: 0x3b80_0000,
    toMinus9: 0x3b00_0000,
    toMinus10: 0x3a80_0000,
    toMinus11: 0x3a00_0000,
    toMinus12: 0x3980_0000,
    toMinus13: 0x3900_0000,
    toMinus14: 0x3880_0000,
    toMinus15: 0x3800_0000,
    toMinus16: 0x3780_0000,
    toMinus17: 0x3700_0000,
    toMinus18: 0x3680_0000,
    toMinus19: 0x3600_0000,
    toMinus20: 0x3580_0000,
    toMinus21: 0x3500_0000,
    toMinus22: 0x3480_0000,
    toMinus23: 0x3400_0000,
    toMinus24: 0x3380_0000,
    toMinus25: 0x3300_0000,
    toMinus26: 0x3280_0000,
    toMinus27: 0x3200_0000,
    toMinus28: 0x3180_0000,
    toMinus29: 0x3100_0000,
    toMinus30: 0x3080_0000,
    toMinus31: 0x3000_0000,

    to0: 0x0000_0001,
    to1: 0x0000_0002,
    to2: 0x0000_0004,
    to3: 0x0000_0008,
    to4: 0x0000_0010,
    to5: 0x0000_0020,
    to6: 0x0000_0040,
    to7: 0x0000_0080,
    to8: 0x0000_0100,
    to9: 0x0000_0200,
    to10: 0x0000_0400,
    to11: 0x0000_0800,
    to12: 0x0000_1000,
    to13: 0x0000_2000,
    to14: 0x0000_4000,
    to15: 0x0000_8000,
    to16: 0x0001_0000,
    to17: 0x0002_0000,
    to18: 0x0004_0000,
    to19: 0x0008_0000,
    to20: 0x0010_0000,
    to21: 0x0020_0000,
    to22: 0x0040_0000,
    to23: 0x0080_0000,
    to24: 0x0100_0000,
    to25: 0x0200_0000,
    to26: 0x0400_0000,
    to27: 0x0800_0000,
    to28: 0x1000_0000,
    to29: 0x2000_0000,
    to30: 0x4000_0000,
    to31: 0x8000_0000,
  },

  // 32-bit representation of  of -1 * power(2, n) n = {-31, ..., 31}
  // An int32 represented as a JS `number`
  // {toMinus31, ..., to31} ie. {-31, ..., 31}
  negPowTwo: {
    toMinus1: 0xbf00_0000,
    toMinus2: 0xbe80_0000,
    toMinus3: 0xbe00_0000,
    toMinus4: 0xbd80_0000,
    toMinus5: 0xbd00_0000,
    toMinus6: 0xbc80_0000,
    toMinus7: 0xbc00_0000,
    toMinus8: 0xbb80_0000,
    toMinus9: 0xbb00_0000,
    toMinus10: 0xba80_0000,
    toMinus11: 0xba00_0000,
    toMinus12: 0xb980_0000,
    toMinus13: 0xb900_0000,
    toMinus14: 0xb880_0000,
    toMinus15: 0xb800_0000,
    toMinus16: 0xb780_0000,
    toMinus17: 0xb700_0000,
    toMinus18: 0xb680_0000,
    toMinus19: 0xb600_0000,
    toMinus20: 0xb580_0000,
    toMinus21: 0xb500_0000,
    toMinus22: 0xb480_0000,
    toMinus23: 0xb400_0000,
    toMinus24: 0xb380_0000,
    toMinus25: 0xb300_0000,
    toMinus26: 0xb280_0000,
    toMinus27: 0xb200_0000,
    toMinus28: 0xb180_0000,
    toMinus29: 0xb100_0000,
    toMinus30: 0xb080_0000,
    toMinus31: 0xb000_0000,

    to0: 0xffff_ffff,
    to1: 0xffff_fffe,
    to2: 0xffff_fffc,
    to3: 0xffff_fff8,
    to4: 0xffff_fff0,
    to5: 0xffff_ffe0,
    to6: 0xffff_ffc0,
    to7: 0xffff_ff80,
    to8: 0xffff_ff00,
    to9: 0xffff_fe00,
    to10: 0xffff_fc00,
    to11: 0xffff_f800,
    to12: 0xffff_f000,
    to13: 0xffff_e000,
    to14: 0xffff_c000,
    to15: 0xffff_8000,
    to16: 0xffff_0000,
    to17: 0xfffe_0000,
    to18: 0xfffc_0000,
    to19: 0xfff8_0000,
    to20: 0xfff0_0000,
    to21: 0xffe0_0000,
    to22: 0xffc0_0000,
    to23: 0xff80_0000,
    to24: 0xff00_0000,
    to25: 0xfe00_0000,
    to26: 0xfc00_0000,
    to27: 0xf800_0000,
    to28: 0xf000_0000,
    to29: 0xe000_0000,
    to30: 0xc000_0000,
    to31: 0x8000_0000,
  },
} as const;

export const kValue = {
  powTwo: {
    to0: Math.pow(2, 0),
    to1: Math.pow(2, 1),
    to2: Math.pow(2, 2),
    to3: Math.pow(2, 3),
    to4: Math.pow(2, 4),
    to5: Math.pow(2, 5),
    to6: Math.pow(2, 6),
    to7: Math.pow(2, 7),
    to8: Math.pow(2, 8),
    to9: Math.pow(2, 9),
    to10: Math.pow(2, 10),
    to11: Math.pow(2, 11),
    to12: Math.pow(2, 12),
    to13: Math.pow(2, 13),
    to14: Math.pow(2, 14),
    to15: Math.pow(2, 15),
    to16: Math.pow(2, 16),
    to17: Math.pow(2, 17),
    to18: Math.pow(2, 18),
    to19: Math.pow(2, 19),
    to20: Math.pow(2, 20),
    to21: Math.pow(2, 21),
    to22: Math.pow(2, 22),
    to23: Math.pow(2, 23),
    to24: Math.pow(2, 24),
    to25: Math.pow(2, 25),
    to26: Math.pow(2, 26),
    to27: Math.pow(2, 27),
    to28: Math.pow(2, 28),
    to29: Math.pow(2, 29),
    to30: Math.pow(2, 30),
    to31: Math.pow(2, 31),
    to32: Math.pow(2, 32),

    toMinus1: Math.pow(2, -1),
    toMinus2: Math.pow(2, -2),
    toMinus3: Math.pow(2, -3),
    toMinus4: Math.pow(2, -4),
    toMinus5: Math.pow(2, -5),
    toMinus6: Math.pow(2, -6),
    toMinus7: Math.pow(2, -7),
    toMinus8: Math.pow(2, -8),
    toMinus9: Math.pow(2, -9),
    toMinus10: Math.pow(2, -10),
    toMinus11: Math.pow(2, -11),
    toMinus12: Math.pow(2, -12),
    toMinus13: Math.pow(2, -13),
    toMinus14: Math.pow(2, -14),
    toMinus15: Math.pow(2, -15),
    toMinus16: Math.pow(2, -16),
    toMinus17: Math.pow(2, -17),
    toMinus18: Math.pow(2, -18),
    toMinus19: Math.pow(2, -19),
    toMinus20: Math.pow(2, -20),
    toMinus21: Math.pow(2, -21),
    toMinus22: Math.pow(2, -22),
    toMinus23: Math.pow(2, -23),
    toMinus24: Math.pow(2, -24),
    toMinus25: Math.pow(2, -25),
    toMinus26: Math.pow(2, -26),
    toMinus27: Math.pow(2, -27),
    toMinus28: Math.pow(2, -28),
    toMinus29: Math.pow(2, -29),
    toMinus30: Math.pow(2, -30),
    toMinus31: Math.pow(2, -31),
    toMinus32: Math.pow(2, -32),
  },
  negPowTwo: {
    to0: -Math.pow(2, 0),
    to1: -Math.pow(2, 1),
    to2: -Math.pow(2, 2),
    to3: -Math.pow(2, 3),
    to4: -Math.pow(2, 4),
    to5: -Math.pow(2, 5),
    to6: -Math.pow(2, 6),
    to7: -Math.pow(2, 7),
    to8: -Math.pow(2, 8),
    to9: -Math.pow(2, 9),
    to10: -Math.pow(2, 10),
    to11: -Math.pow(2, 11),
    to12: -Math.pow(2, 12),
    to13: -Math.pow(2, 13),
    to14: -Math.pow(2, 14),
    to15: -Math.pow(2, 15),
    to16: -Math.pow(2, 16),
    to17: -Math.pow(2, 17),
    to18: -Math.pow(2, 18),
    to19: -Math.pow(2, 19),
    to20: -Math.pow(2, 20),
    to21: -Math.pow(2, 21),
    to22: -Math.pow(2, 22),
    to23: -Math.pow(2, 23),
    to24: -Math.pow(2, 24),
    to25: -Math.pow(2, 25),
    to26: -Math.pow(2, 26),
    to27: -Math.pow(2, 27),
    to28: -Math.pow(2, 28),
    to29: -Math.pow(2, 29),
    to30: -Math.pow(2, 30),
    to31: -Math.pow(2, 31),
    to32: -Math.pow(2, 32),

    toMinus1: -Math.pow(2, -1),
    toMinus2: -Math.pow(2, -2),
    toMinus3: -Math.pow(2, -3),
    toMinus4: -Math.pow(2, -4),
    toMinus5: -Math.pow(2, -5),
    toMinus6: -Math.pow(2, -6),
    toMinus7: -Math.pow(2, -7),
    toMinus8: -Math.pow(2, -8),
    toMinus9: -Math.pow(2, -9),
    toMinus10: -Math.pow(2, -10),
    toMinus11: -Math.pow(2, -11),
    toMinus12: -Math.pow(2, -12),
    toMinus13: -Math.pow(2, -13),
    toMinus14: -Math.pow(2, -14),
    toMinus15: -Math.pow(2, -15),
    toMinus16: -Math.pow(2, -16),
    toMinus17: -Math.pow(2, -17),
    toMinus18: -Math.pow(2, -18),
    toMinus19: -Math.pow(2, -19),
    toMinus20: -Math.pow(2, -20),
    toMinus21: -Math.pow(2, -21),
    toMinus22: -Math.pow(2, -22),
    toMinus23: -Math.pow(2, -23),
    toMinus24: -Math.pow(2, -24),
    toMinus25: -Math.pow(2, -25),
    toMinus26: -Math.pow(2, -26),
    toMinus27: -Math.pow(2, -27),
    toMinus28: -Math.pow(2, -28),
    toMinus29: -Math.pow(2, -29),
    toMinus30: -Math.pow(2, -30),
    toMinus31: -Math.pow(2, -31),
    toMinus32: -Math.pow(2, -32),
  },
};

export function runShaderTest<F extends NumberType>(
  t: GPUTest,
  storageClass: string,
  storageMode: string,
  type: string,
  arrayLength: number,
  builtin: string,
  arrayType: TypedArrayBufferViewConstructor,
  cases: Array<Case<F>>
): void {
  const source = `
    [[block]]
    struct Data {
      values : [[stride(16)]] array<${type}, ${cases.length}>;
    };

    [[group(0), binding(0)]] var<${storageClass}, ${storageMode}> inputs : Data;
    [[group(0), binding(1)]] var<${storageClass}, write> outputs : Data;

    [[stage(compute), workgroup_size(1)]]
    fn main() {
      for(var i = 0; i < ${cases.length}; i = i + 1) {
          let input : ${type} = inputs.values[i];
          let result : ${type} = ${builtin}(input);
          outputs.values[i] = result;
      }
    }
  `;
  runShaderTestImpl(
    t,
    storageClass,
    storageMode,
    type,
    arrayLength,
    builtin,
    arrayType,
    cases,
    source
  );
}
