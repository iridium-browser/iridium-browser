export const description = `
createRenderPipeline and createRenderPipelineAsync validation tests.

TODO: review existing tests, write descriptions, and make sure tests are complete.
      Make sure the following is covered. Consider splitting the file if too large/disjointed.
> - various attachment problems
>
> - interface matching between vertex and fragment shader
>     - superset, subset, etc.
>
> - vertex stage {valid, invalid}
> - fragment stage {valid, invalid}
> - primitive topology all possible values
> - rasterizationState various values
> - multisample count {0, 1, 3, 4, 8, 16, 1024}
> - multisample mask {0, 0xFFFFFFFF}
> - alphaToCoverage:
>     - alphaToCoverageEnabled is { true, false } and sampleCount { = 1, = 4 }.
>       The only failing case is (true, 1).
>     - output SV_Coverage semantics is statically used by fragmentStage and
>       alphaToCoverageEnabled is { true (fails), false (passes) }.
>     - sampleMask is being used and alphaToCoverageEnabled is { true (fails), false (passes) }.

`;

import { makeTestGroup } from '../../../common/framework/test_group.js';
import { unreachable } from '../../../common/util/util.js';
import {
  kTextureFormats,
  kRenderableColorTextureFormats,
  kTextureFormatInfo,
} from '../../capability_info.js';
import { kTexelRepresentationInfo } from '../../util/texture/texel_data.js';

import { ValidationTest } from './validation_test.js';

class F extends ValidationTest {
  getFragmentShaderCode(sampleType: GPUTextureSampleType, componentCount: number): string {
    const v = ['0', '1', '0', '1'];

    let fragColorType;
    let suffix;
    switch (sampleType) {
      case 'sint':
        fragColorType = 'i32';
        suffix = '';
        break;
      case 'uint':
        fragColorType = 'u32';
        suffix = 'u';
        break;
      default:
        fragColorType = 'f32';
        suffix = '.0';
        break;
    }

    let outputType;
    let result;
    switch (componentCount) {
      case 1:
        outputType = fragColorType;
        result = `${v[0]}${suffix}`;
        break;
      case 2:
        outputType = `vec2<${fragColorType}>`;
        result = `${outputType}(${v[0]}${suffix}, ${v[1]}${suffix})`;
        break;
      case 3:
        outputType = `vec3<${fragColorType}>`;
        result = `${outputType}(${v[0]}${suffix}, ${v[1]}${suffix}, ${v[2]}${suffix})`;
        break;
      case 4:
        outputType = `vec4<${fragColorType}>`;
        result = `${outputType}(${v[0]}${suffix}, ${v[1]}${suffix}, ${v[2]}${suffix}, ${v[3]}${suffix})`;
        break;
      default:
        unreachable();
    }

    return `
    [[stage(fragment)]] fn main() -> [[location(0)]] ${outputType} {
      return ${result};
    }`;
  }

  getDescriptor(
    options: {
      topology?: GPUPrimitiveTopology;
      targets?: GPUColorTargetState[];
      sampleCount?: number;
      depthStencil?: GPUDepthStencilState;
      fragmentShaderCode?: string;
      noFragment?: boolean;
    } = {}
  ): GPURenderPipelineDescriptor {
    const defaultTargets: GPUColorTargetState[] = [{ format: 'rgba8unorm' }];
    const {
      topology = 'triangle-list',
      targets = defaultTargets,
      sampleCount = 1,
      depthStencil,
      fragmentShaderCode = this.getFragmentShaderCode(
        kTextureFormatInfo[targets[0] ? targets[0].format : 'rgba8unorm'].sampleType,
        4
      ),
      noFragment = false,
    } = options;

    return {
      vertex: {
        module: this.device.createShaderModule({
          code: `
            [[stage(vertex)]] fn main() -> [[builtin(position)]] vec4<f32> {
              return vec4<f32>(0.0, 0.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
      },
      fragment: noFragment
        ? undefined
        : {
            module: this.device.createShaderModule({
              code: fragmentShaderCode,
            }),
            entryPoint: 'main',
            targets,
          },
      layout: this.getPipelineLayout(),
      primitive: { topology },
      multisample: { count: sampleCount },
      depthStencil,
    };
  }

  getPipelineLayout(): GPUPipelineLayout {
    return this.device.createPipelineLayout({ bindGroupLayouts: [] });
  }

  createTexture(params: { format: GPUTextureFormat; sampleCount: number }): GPUTexture {
    const { format, sampleCount } = params;

    return this.device.createTexture({
      size: { width: 4, height: 4, depthOrArrayLayers: 1 },
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
      format,
      sampleCount,
    });
  }

  doCreateRenderPipelineTest(
    isAsync: boolean,
    _success: boolean,
    descriptor: GPURenderPipelineDescriptor
  ) {
    if (isAsync) {
      if (_success) {
        this.shouldResolve(this.device.createRenderPipelineAsync(descriptor));
      } else {
        this.shouldReject('OperationError', this.device.createRenderPipelineAsync(descriptor));
      }
    } else {
      this.expectValidationError(() => {
        this.device.createRenderPipeline(descriptor);
      }, !_success);
    }
  }
}

export const g = makeTestGroup(F);

g.test('basic_use_of_createRenderPipeline')
  .params(u => u.combine('isAsync', [false, true]))
  .fn(async t => {
    const { isAsync } = t.params;
    const descriptor = t.getDescriptor();

    t.doCreateRenderPipelineTest(isAsync, true, descriptor);
  });

g.test('create_vertex_only_pipeline_with_without_depth_stencil_state')
  .desc(
    `Test creating vertex-only render pipeline. A vertex-only render pipeline have no fragment
state (and thus have no color state), and can be create with or without depth stencil state.`
  )
  .params(u =>
    u
      .combine('isAsync', [false, true])
      .beginSubcases()
      .combine('depthStencilFormat', [
        'depth24plus',
        'depth24plus-stencil8',
        'depth32float',
        '',
      ] as const)
      .combine('haveColor', [false, true])
  )
  .fn(async t => {
    const { isAsync, depthStencilFormat, haveColor } = t.params;

    let depthStencilState: GPUDepthStencilState | undefined;
    if (depthStencilFormat === '') {
      depthStencilState = undefined;
    } else {
      depthStencilState = { format: depthStencilFormat };
    }

    // Having targets or not should have no effect in result, since it will not appear in the
    // descriptor in vertex-only render pipeline
    const descriptor = t.getDescriptor({
      noFragment: true,
      depthStencil: depthStencilState,
      targets: haveColor ? [{ format: 'rgba8unorm' }] : [],
    });

    t.doCreateRenderPipelineTest(isAsync, true, descriptor);
  });

g.test('at_least_one_color_state_is_required_for_complete_pipeline')
  .params(u => u.combine('isAsync', [false, true]))
  .fn(async t => {
    const { isAsync } = t.params;

    const goodDescriptor = t.getDescriptor({
      targets: [{ format: 'rgba8unorm' }],
    });

    // Control case
    t.doCreateRenderPipelineTest(isAsync, true, goodDescriptor);

    // Fail because lack of color states
    const badDescriptor = t.getDescriptor({
      targets: [],
    });

    t.doCreateRenderPipelineTest(isAsync, false, badDescriptor);
  });

g.test('color_formats_must_be_renderable')
  .params(u => u.combine('isAsync', [false, true]).combine('format', kTextureFormats))
  .fn(async t => {
    const { isAsync, format } = t.params;
    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const descriptor = t.getDescriptor({ targets: [{ format }] });

    t.doCreateRenderPipelineTest(isAsync, info.renderable && info.color, descriptor);
  });

g.test('sample_count_must_be_valid')
  .params(u =>
    u.combine('isAsync', [false, true]).combineWithParams([
      { sampleCount: 0, _success: false },
      { sampleCount: 1, _success: true },
      { sampleCount: 2, _success: false },
      { sampleCount: 3, _success: false },
      { sampleCount: 4, _success: true },
      { sampleCount: 8, _success: false },
      { sampleCount: 16, _success: false },
    ])
  )
  .fn(async t => {
    const { isAsync, sampleCount, _success } = t.params;

    const descriptor = t.getDescriptor({ sampleCount });

    t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
  });

g.test('pipeline_output_targets')
  .desc(
    `Pipeline fragment output types must be compatible with target color state format
  - The scalar type (f32, i32, or u32) must match the sample type of the format.
  - The componentCount of the fragment output (e.g. f32, vec2, vec3, vec4) must not have fewer
    channels than that of the color attachment texture formats. Extra components are allowed and are discarded.
  `
  )
  .params(u =>
    u
      .combine('isAsync', [false, true])
      .combine('format', kRenderableColorTextureFormats)
      .beginSubcases()
      .combine('sampleType', ['float', 'uint', 'sint'] as const)
      .combine('componentCount', [1, 2, 3, 4])
  )
  .fn(async t => {
    const { isAsync, format, sampleType, componentCount } = t.params;
    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const descriptor = t.getDescriptor({
      targets: [{ format }],
      fragmentShaderCode: t.getFragmentShaderCode(sampleType, componentCount),
    });

    const _success =
      info.sampleType === sampleType &&
      componentCount >= kTexelRepresentationInfo[format].componentOrder.length;
    t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
  });

g.test('pipeline_output_targets,blend')
  .desc(
    `On top of requirements from pipeline_output_targets, when blending is enabled and alpha channel is read indicated by any blend factor, an extra requirement is added:
  - fragment output must be vec4.
  `
  )
  .params(u =>
    u
      .combine('isAsync', [false, true])
      .combine('format', ['r8unorm', 'rg8unorm', 'rgba8unorm', 'bgra8unorm'] as const)
      .beginSubcases()
      .combine('componentCount', [1, 2, 3, 4])
      .combineWithParams([
        // extra requirement does not apply
        {
          colorSrcFactor: 'one',
          colorDstFactor: 'zero',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'zero',
        },
        {
          colorSrcFactor: 'dst-alpha',
          colorDstFactor: 'zero',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'zero',
        },
        // extra requirement applies, fragment output must be vec4 (contain alpha channel)
        {
          colorSrcFactor: 'src-alpha',
          colorDstFactor: 'one',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'zero',
        },
        {
          colorSrcFactor: 'one',
          colorDstFactor: 'one-minus-src-alpha',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'zero',
        },
        {
          colorSrcFactor: 'src-alpha-saturated',
          colorDstFactor: 'one',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'zero',
        },
        {
          colorSrcFactor: 'one',
          colorDstFactor: 'zero',
          alphaSrcFactor: 'one',
          alphaDstFactor: 'zero',
        },
        {
          colorSrcFactor: 'one',
          colorDstFactor: 'zero',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'src',
        },
        {
          colorSrcFactor: 'one',
          colorDstFactor: 'zero',
          alphaSrcFactor: 'zero',
          alphaDstFactor: 'src-alpha',
        },
      ] as const)
  )
  .fn(async t => {
    const sampleType = 'float';
    const {
      isAsync,
      format,
      componentCount,
      colorSrcFactor,
      colorDstFactor,
      alphaSrcFactor,
      alphaDstFactor,
    } = t.params;
    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const descriptor = t.getDescriptor({
      targets: [
        {
          format,
          blend: {
            color: {
              srcFactor: colorSrcFactor,
              dstFactor: colorDstFactor,
              operation: 'add',
            },
            alpha: {
              srcFactor: alphaSrcFactor,
              dstFactor: alphaDstFactor,
              operation: 'add',
            },
          },
        },
      ],
      fragmentShaderCode: t.getFragmentShaderCode(sampleType, componentCount),
    });

    const colorBlendReadsSrcAlpha =
      colorSrcFactor.includes('src-alpha') || colorDstFactor.includes('src-alpha');
    const meetsExtraBlendingRequirement = !colorBlendReadsSrcAlpha || componentCount === 4;
    const _success =
      info.sampleType === sampleType &&
      componentCount >= kTexelRepresentationInfo[format].componentOrder.length &&
      meetsExtraBlendingRequirement;
    t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
  });

g.test('pipeline_layout,device_mismatch')
  .desc(
    'Tests createRenderPipeline(Async) cannot be called with a pipeline layout created from another device'
  )
  .paramsSubcasesOnly(u => u.combine('isAsync', [true, false]).combine('mismatched', [true, false]))
  .fn(async t => {
    const { isAsync, mismatched } = t.params;

    if (mismatched) {
      await t.selectMismatchedDeviceOrSkipTestCase(undefined);
    }

    const layoutDescriptor = { bindGroupLayouts: [] };
    const layout = mismatched
      ? t.mismatchedDevice.createPipelineLayout(layoutDescriptor)
      : t.device.createPipelineLayout(layoutDescriptor);

    const descriptor = {
      layout,
      vertex: {
        module: t.device.createShaderModule({
          code: `
        [[stage(vertex)]] fn main() -> [[builtin(position)]] vec4<f32> {
          return vec4<f32>(0.0, 0.0, 0.0, 1.0);
        }
      `,
        }),
        entryPoint: 'main',
      },
      fragment: {
        module: t.device.createShaderModule({ code: t.getFragmentShaderCode('float', 4) }),
        entryPoint: 'main',
        targets: [{ format: 'rgba8unorm' }] as const,
      },
    };

    t.doCreateRenderPipelineTest(isAsync, !mismatched, descriptor);
  });

g.test('shader_module,device_mismatch')
  .desc(
    'Tests createRenderPipeline(Async) cannot be called with a shader module created from another device'
  )
  .paramsSubcasesOnly(u =>
    u.combine('isAsync', [true, false]).combineWithParams([
      { vertex_mismatched: false, fragment_mismatched: false, _success: true },
      { vertex_mismatched: true, fragment_mismatched: false, _success: false },
      { vertex_mismatched: false, fragment_mismatched: true, _success: false },
    ])
  )
  .fn(async t => {
    const { isAsync, vertex_mismatched, fragment_mismatched, _success } = t.params;

    if (vertex_mismatched || fragment_mismatched) {
      await t.selectMismatchedDeviceOrSkipTestCase(undefined);
    }

    const code = `
      [[stage(vertex)]] fn main() -> [[builtin(position)]] vec4<f32> {
        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
      }
    `;

    const descriptor = {
      vertex: {
        module: vertex_mismatched
          ? t.mismatchedDevice.createShaderModule({ code })
          : t.device.createShaderModule({ code }),
        entryPoint: 'main',
      },
      fragment: {
        module: fragment_mismatched
          ? t.mismatchedDevice.createShaderModule({ code: t.getFragmentShaderCode('float', 4) })
          : t.device.createShaderModule({ code: t.getFragmentShaderCode('float', 4) }),
        entryPoint: 'main',
        targets: [{ format: 'rgba8unorm' }] as const,
      },
      layout: t.getPipelineLayout(),
    };

    t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
  });
