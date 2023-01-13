export const description = `
Test related to stencil states, stencil op, compare func, etc.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { TypedArrayBufferView } from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';
import { TexelView } from '../../../util/texture/texel_view.js';
import { textureContentIsOKByT2B } from '../../../util/texture/texture_ok.js';

const kBaseColor = new Float32Array([1.0, 1.0, 1.0, 1.0]);
const kRedStencilColor = new Float32Array([1.0, 0.0, 0.0, 1.0]);
const kGreenStencilColor = new Float32Array([0.0, 1.0, 0.0, 1.0]);

type TestStates = {
  state: GPUDepthStencilState;
  color: Float32Array;
  stencil: number | undefined;
};

class StencilTest extends GPUTest {
  checkStencilOperation(
    stencilOperation: GPUStencilOperation,
    initialStencil: number,
    referenceStencil: number,
    expectedStencil: number
  ) {
    const depthStencilFormat: GPUTextureFormat = 'depth24plus-stencil8';

    const baseStencilState = {
      compare: 'always',
      failOp: 'keep',
      passOp: 'replace',
    } as const;

    const stencilState = {
      compare: 'always',
      failOp: 'keep',
      passOp: stencilOperation,
    } as const;

    const stencilState2 = {
      compare: 'equal',
      failOp: 'keep',
      passOp: 'keep',
    } as const;

    const baseState = {
      format: depthStencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: baseStencilState,
      stencilBack: baseStencilState,
    } as const;

    const testState = {
      format: depthStencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: stencilState,
      stencilBack: stencilState,
    } as const;

    const testState2 = {
      format: depthStencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: stencilState2,
      stencilBack: stencilState2,
    } as const;

    const testStates = [
      // Draw the base triangle with stencil reference 1. This clears the stencil buffer to 1.
      { state: baseState, color: kBaseColor, stencil: initialStencil },
      { state: testState, color: kRedStencilColor, stencil: referenceStencil },
      { state: testState2, color: kGreenStencilColor, stencil: expectedStencil },
    ];
    this.runStencilStateTest(testStates, kGreenStencilColor);
  }

  checkStencilCompareFunction(
    compareFunction: GPUCompareFunction,
    stencilRefValue: number,
    expectedColor: Float32Array
  ) {
    const depthStencilFormat: GPUTextureFormat = 'depth24plus-stencil8';

    const baseStencilState = {
      compare: 'always',
      failOp: 'keep',
      passOp: 'replace',
    } as const;

    const stencilState = {
      compare: compareFunction,
      failOp: 'keep',
      passOp: 'keep',
    } as const;

    const baseState = {
      format: depthStencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: baseStencilState,
      stencilBack: baseStencilState,
    } as const;

    const testState = {
      format: depthStencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: stencilState,
      stencilBack: stencilState,
    } as const;

    const testStates = [
      // Draw the base triangle with stencil reference 1. This clears the stencil buffer to 1.
      { state: baseState, color: kBaseColor, stencil: 1 },
      { state: testState, color: kGreenStencilColor, stencil: stencilRefValue },
    ];
    this.runStencilStateTest(testStates, expectedColor);
  }

  runStencilStateTest(
    testStates: TestStates[],
    expectedColor: Float32Array,
    isSingleEncoderMultiplePass: boolean = false
  ) {
    const renderTargetFormat = 'rgba8unorm';
    const renderTarget = this.device.createTexture({
      format: renderTargetFormat,
      size: { width: 1, height: 1, depthOrArrayLayers: 1 },
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.RENDER_ATTACHMENT,
    });

    const depthStencilFormat: GPUTextureFormat = 'depth24plus-stencil8';
    const depthTexture = this.device.createTexture({
      size: { width: 1, height: 1, depthOrArrayLayers: 1 },
      format: depthStencilFormat,
      sampleCount: 1,
      mipLevelCount: 1,
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_DST,
    });

    const depthStencilAttachment: GPURenderPassDepthStencilAttachment = {
      view: depthTexture.createView(),
      depthLoadOp: 'load',
      depthStoreOp: 'store',
      stencilLoadOp: 'load',
      stencilStoreOp: 'store',
    };

    const encoder = this.device.createCommandEncoder();
    let pass = encoder.beginRenderPass({
      colorAttachments: [
        {
          view: renderTarget.createView(),
          storeOp: 'store',
          loadOp: 'load',
        },
      ],
      depthStencilAttachment,
    });

    if (isSingleEncoderMultiplePass) {
      pass.end();
    }

    // Draw a triangle with the given stencil reference and the comparison function.
    // The color will be kGreenStencilColor if the stencil test passes, and kBaseColor if not.
    for (const test of testStates) {
      if (isSingleEncoderMultiplePass) {
        pass = encoder.beginRenderPass({
          colorAttachments: [
            {
              view: renderTarget.createView(),
              storeOp: 'store',
              loadOp: 'load',
            },
          ],
          depthStencilAttachment,
        });
      }
      const testPipeline = this.createRenderPipelineForTest(test.state);
      pass.setPipeline(testPipeline);
      if (test.stencil !== undefined) {
        pass.setStencilReference(test.stencil);
      }
      pass.setBindGroup(
        0,
        this.createBindGroupForTest(testPipeline.getBindGroupLayout(0), test.color)
      );
      pass.draw(1);

      if (isSingleEncoderMultiplePass) {
        pass.end();
      }
    }

    if (!isSingleEncoderMultiplePass) {
      pass.end();
    }
    this.device.queue.submit([encoder.finish()]);

    const expColor = {
      R: expectedColor[0],
      G: expectedColor[1],
      B: expectedColor[2],
      A: expectedColor[3],
    };
    const expTexelView = TexelView.fromTexelsAsColors(renderTargetFormat, coords => expColor);

    const result = textureContentIsOKByT2B(
      this,
      { texture: renderTarget },
      [1, 1],
      { expTexelView },
      { maxDiffULPsForNormFormat: 1 }
    );
    this.eventualExpectOK(result);
    this.trackForCleanup(renderTarget);
  }

  createRenderPipelineForTest(depthStencil: GPUDepthStencilState): GPURenderPipeline {
    return this.device.createRenderPipeline({
      layout: 'auto',
      vertex: {
        module: this.device.createShaderModule({
          code: `
            @vertex
            fn main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {
                return vec4<f32>(0.0, 0.0, 0.0, 1.0);
            }
            `,
        }),
        entryPoint: 'main',
      },
      fragment: {
        targets: [{ format: 'rgba8unorm' }],
        module: this.device.createShaderModule({
          code: `
            struct Params {
              color : vec4<f32>
            }
            @group(0) @binding(0) var<uniform> params : Params;

            @fragment fn main() -> @location(0) vec4<f32> {
                return vec4<f32>(params.color);
            }`,
        }),
        entryPoint: 'main',
      },
      primitive: { topology: 'point-list' },
      depthStencil,
    });
  }

  createBindGroupForTest(layout: GPUBindGroupLayout, data: TypedArrayBufferView): GPUBindGroup {
    return this.device.createBindGroup({
      layout,
      entries: [
        {
          binding: 0,
          resource: {
            buffer: this.makeBufferWithContents(data, GPUBufferUsage.UNIFORM),
          },
        },
      ],
    });
  }
}

export const g = makeTestGroup(StencilTest);

g.test('stencil_compare_func')
  .desc(
    `
  Tests that stencil comparison functions with the stencil reference value works as expected.
  `
  )
  .params(u =>
    u //
      .combineWithParams([
        { stencilCompare: 'always', stencilRefValue: 0, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'always', stencilRefValue: 1, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'always', stencilRefValue: 2, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'equal', stencilRefValue: 0, _expectedColor: kBaseColor },
        { stencilCompare: 'equal', stencilRefValue: 1, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'equal', stencilRefValue: 2, _expectedColor: kBaseColor },
        { stencilCompare: 'greater', stencilRefValue: 0, _expectedColor: kBaseColor },
        { stencilCompare: 'greater', stencilRefValue: 1, _expectedColor: kBaseColor },
        { stencilCompare: 'greater', stencilRefValue: 2, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'greater-equal', stencilRefValue: 0, _expectedColor: kBaseColor },
        { stencilCompare: 'greater-equal', stencilRefValue: 1, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'greater-equal', stencilRefValue: 2, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'less', stencilRefValue: 0, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'less', stencilRefValue: 1, _expectedColor: kBaseColor },
        { stencilCompare: 'less', stencilRefValue: 2, _expectedColor: kBaseColor },
        { stencilCompare: 'less-equal', stencilRefValue: 0, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'less-equal', stencilRefValue: 1, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'less-equal', stencilRefValue: 2, _expectedColor: kBaseColor },
        { stencilCompare: 'never', stencilRefValue: 0, _expectedColor: kBaseColor },
        { stencilCompare: 'never', stencilRefValue: 1, _expectedColor: kBaseColor },
        { stencilCompare: 'never', stencilRefValue: 2, _expectedColor: kBaseColor },
        { stencilCompare: 'not-equal', stencilRefValue: 0, _expectedColor: kGreenStencilColor },
        { stencilCompare: 'not-equal', stencilRefValue: 1, _expectedColor: kBaseColor },
        { stencilCompare: 'not-equal', stencilRefValue: 2, _expectedColor: kGreenStencilColor },
      ] as const)
  )
  .fn(async t => {
    const { stencilCompare, stencilRefValue, _expectedColor } = t.params;

    t.checkStencilCompareFunction(stencilCompare, stencilRefValue, _expectedColor);
  });

g.test('stencil_passOp_operation')
  .desc(
    `
    Test that each stencil operation works with the given stencil values correctly as expected.

    TODO: Need to test failOp and depthFailOp as well.
  `
  )
  .params(u =>
    u //
      .combineWithParams([
        { passOp: 'keep', initialStencil: 1, referenceStencil: 3, expectedStencil: 1 },
        { passOp: 'zero', initialStencil: 1, referenceStencil: 3, expectedStencil: 0 },
        { passOp: 'replace', initialStencil: 1, referenceStencil: 3, expectedStencil: 3 },
        {
          passOp: 'invert',
          initialStencil: 0xf0,
          referenceStencil: 3,
          expectedStencil: 0x0f,
        },
        {
          passOp: 'increment-clamp',
          initialStencil: 1,
          referenceStencil: 3,
          expectedStencil: 2,
        },
        {
          passOp: 'increment-clamp',
          initialStencil: 0xff,
          referenceStencil: 3,
          expectedStencil: 0xff,
        },
        {
          passOp: 'increment-wrap',
          initialStencil: 1,
          referenceStencil: 3,
          expectedStencil: 2,
        },
        {
          passOp: 'increment-wrap',
          initialStencil: 0xff,
          referenceStencil: 3,
          expectedStencil: 0,
        },
        {
          passOp: 'decrement-clamp',
          initialStencil: 1,
          referenceStencil: 3,
          expectedStencil: 0,
        },
        {
          passOp: 'decrement-clamp',
          initialStencil: 0,
          referenceStencil: 3,
          expectedStencil: 0,
        },
        {
          passOp: 'decrement-wrap',
          initialStencil: 1,
          referenceStencil: 3,
          expectedStencil: 0,
        },
        {
          passOp: 'decrement-wrap',
          initialStencil: 0,
          referenceStencil: 3,
          expectedStencil: 0xff,
        },
      ] as const)
  )
  .fn(async t => {
    const { passOp, initialStencil, referenceStencil, expectedStencil } = t.params;

    t.checkStencilOperation(passOp, initialStencil, referenceStencil, expectedStencil);
  });

g.test('stencil_test_fail')
  .desc(
    `
  Test that the stencil operation is executed on stencil fail. Triangle with stencil reference 2
  fails the 'less' comparison function because the base stencil reference is 1.
    - If the fail operation is 'keep', it keeps the base color.
    - If the fail operation is 'replace', it replaces the base color with the last stencil color.

  TODO: Need to test the other stencil operations?
  `
  )
  .params(u =>
    u //
      .combineWithParams([
        { operation: 'keep', _expectedColor: kBaseColor },
        { operation: 'replace', _expectedColor: kGreenStencilColor },
      ] as const)
  )
  .fn(async t => {
    const { operation, _expectedColor } = t.params;

    const depthSpencilFormat: GPUTextureFormat = 'depth24plus-stencil8';
    const stencilRefValue = 2;

    const baseStencilState = {
      compare: 'always',
      failOp: 'keep',
      passOp: 'replace',
    } as const;

    const failedStencilState = {
      compare: 'less',
      failOp: operation,
      passOp: 'keep',
    } as const;

    const stencilState = {
      compare: 'equal',
      failOp: 'keep',
      passOp: 'keep',
    } as const;

    const baseState = {
      format: depthSpencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: baseStencilState,
      stencilBack: baseStencilState,
      stencilReadMask: 0xff,
      stencilWriteMask: 0xff,
    } as const;

    const failState = {
      format: depthSpencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: failedStencilState,
      stencilBack: failedStencilState,
      stencilReadMask: 0xff,
      stencilWriteMask: 0xff,
    } as const;

    const passState = {
      format: depthSpencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: stencilState,
      stencilBack: stencilState,
      stencilReadMask: 0xff,
      stencilWriteMask: 0xff,
    } as const;

    const testStates = [
      // Draw the base triangle with stencil reference 1. This clears the stencil buffer to 1.
      { state: baseState, color: kBaseColor, stencil: 1 },
      // Always fails because the ref (2) is less than the initial stencil contents (1).
      // Therefore red is never drawn, and the stencil contents may be updated according to
      // `operation`.
      { state: failState, color: kRedStencilColor, stencil: stencilRefValue },
      // Passes iff the ref (2) equals the current stencil contents (1 or 2).
      { state: passState, color: kGreenStencilColor, stencil: stencilRefValue },
    ];
    t.runStencilStateTest(testStates, _expectedColor);
  });

g.test('stencil_read_write_mask')
  .desc(
    `
  Tests that setting a stencil read/write masks work. Basically, The base triangle sets 3 to the
  stencil, and then try to draw a triangle with different stencil values.
    - In case that 'write' mask is 1,
      * If the stencil of the triangle is 1, it draws because
        'base stencil(3) & write mask(1) == triangle stencil(1)'.
      * If the stencil of the triangle is 2, it does not draw because
        'base stencil(3) & write mask(1) != triangle stencil(2)'.

    - In case that 'read' mask is 2,
      * If the stencil of the triangle is 1, it does not draw because
        'base stencil(3) & read mask(2) != triangle stencil(1)'.
      * If the stencil of the triangle is 2, it draws because
        'base stencil(3) & read mask(2) == triangle stencil(2)'.
  `
  )
  .params(u =>
    u //
      .combineWithParams([
        { maskType: 'write', stencilRefValue: 1, _expectedColor: kRedStencilColor },
        { maskType: 'write', stencilRefValue: 2, _expectedColor: kBaseColor },
        { maskType: 'read', stencilRefValue: 1, _expectedColor: kBaseColor },
        { maskType: 'read', stencilRefValue: 2, _expectedColor: kRedStencilColor },
      ])
  )
  .fn(async t => {
    const { maskType, stencilRefValue, _expectedColor } = t.params;

    const depthSpencilFormat: GPUTextureFormat = 'depth24plus-stencil8';

    const baseStencilState = {
      compare: 'always',
      failOp: 'keep',
      passOp: 'replace',
    } as const;

    const stencilState = {
      compare: 'equal',
      failOp: 'keep',
      passOp: 'keep',
    } as const;

    const baseState = {
      format: depthSpencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: baseStencilState,
      stencilBack: baseStencilState,
      stencilReadMask: 0xff,
      stencilWriteMask: maskType === 'write' ? 0x1 : 0xff,
    } as const;

    const testState = {
      format: depthSpencilFormat,
      depthWriteEnabled: false,
      depthCompare: 'always',
      stencilFront: stencilState,
      stencilBack: stencilState,
      stencilReadMask: maskType === 'read' ? 0x2 : 0xff,
      stencilWriteMask: 0xff,
    } as const;

    const testStates = [
      // Draw the base triangle with stencil reference 3. This clears the stencil buffer to 3.
      { state: baseState, color: kBaseColor, stencil: 3 },
      { state: testState, color: kRedStencilColor, stencil: stencilRefValue },
    ];

    t.runStencilStateTest(testStates, _expectedColor);
  });

g.test('stencil_reference_initialized')
  .desc('Test that stencil reference is initialized as zero for new render pass.')
  .fn(async t => {
    const depthSpencilFormat: GPUTextureFormat = 'depth24plus-stencil8';

    const baseStencilState = {
      compare: 'always',
      passOp: 'replace',
    } as const;

    const testStencilState = {
      compare: 'equal',
      passOp: 'keep',
    } as const;

    const baseState = {
      format: depthSpencilFormat,
      stencilFront: baseStencilState,
      stencilBack: baseStencilState,
    } as const;

    const testState = {
      format: depthSpencilFormat,
      stencilFront: testStencilState,
      stencilBack: testStencilState,
    } as const;

    // First pass sets the stencil to 0x1, the second pass sets the stencil to its default
    // value, and the third pass tests if the stencil is zero.
    const testStates = [
      { state: baseState, color: kBaseColor, stencil: 0x1 },
      { state: baseState, color: kRedStencilColor, stencil: undefined },
      { state: testState, color: kGreenStencilColor, stencil: 0x0 },
    ];

    // The third draw should pass the stencil test since the second pass set it to default zero.
    t.runStencilStateTest(testStates, kGreenStencilColor, true);
  });
