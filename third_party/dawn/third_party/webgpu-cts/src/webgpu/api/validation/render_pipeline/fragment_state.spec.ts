export const description = `
This test dedicatedly tests validation of GPUFragmentState of createRenderPipeline.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert, range } from '../../../../common/util/util.js';
import {
  kTextureFormats,
  kRenderableColorTextureFormats,
  kTextureFormatInfo,
  kBlendFactors,
  kBlendOperations,
} from '../../../capability_info.js';
import {
  getFragmentShaderCodeWithOutput,
  getPlainTypeInfo,
  kDefaultFragmentShaderCode,
} from '../../../util/shader.js';
import { kTexelRepresentationInfo } from '../../../util/texture/texel_data.js';

import { CreateRenderPipelineValidationTest } from './common.js';

export const g = makeTestGroup(CreateRenderPipelineValidationTest);

const values = [0, 1, 0, 1];

g.test('color_target_exists')
  .desc(`Tests creating a complete render pipeline requires at least one color target state.`)
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

g.test('max_color_attachments_limit')
  .desc(
    `Tests that color state targets length must not be larger than device.limits.maxColorAttachments.`
  )
  .params(u => u.combine('isAsync', [false, true]).combine('targetsLength', [8, 9]))
  .fn(async t => {
    const { isAsync, targetsLength } = t.params;

    const descriptor = t.getDescriptor({
      targets: range(targetsLength, i => {
        // Set writeMask to 0 for attachments without fragment output
        return { format: 'rgba8unorm', writeMask: i === 0 ? 0xf : 0 };
      }),
      fragmentShaderCode: kDefaultFragmentShaderCode,
    });

    t.doCreateRenderPipelineTest(
      isAsync,
      targetsLength <= t.device.limits.maxColorAttachments,
      descriptor
    );
  });

g.test('targets_format_renderable')
  .desc(`Tests that color target state format must have RENDER_ATTACHMENT capability.`)
  .params(u => u.combine('isAsync', [false, true]).combine('format', kTextureFormats))
  .beforeAllSubcases(t => {
    const { format } = t.params;
    const info = kTextureFormatInfo[format];
    t.selectDeviceOrSkipTestCase(info.feature);
  })
  .fn(async t => {
    const { isAsync, format } = t.params;
    const info = kTextureFormatInfo[format];

    const descriptor = t.getDescriptor({ targets: [{ format }] });

    t.doCreateRenderPipelineTest(isAsync, info.renderable && info.color, descriptor);
  });

g.test('targets_format_filterable')
  .desc(`Tests that color target state format must be filterable if blend is not undefined.`)
  .params(u =>
    u
      .combine('isAsync', [false, true])
      .combine('format', kRenderableColorTextureFormats)
      .beginSubcases()
      .combine('hasBlend', [false, true])
  )
  .beforeAllSubcases(t => {
    const { format } = t.params;
    const info = kTextureFormatInfo[format];
    t.selectDeviceOrSkipTestCase(info.feature);
  })
  .fn(async t => {
    const { isAsync, format, hasBlend } = t.params;
    const info = kTextureFormatInfo[format];

    const descriptor = t.getDescriptor({
      targets: [
        {
          format,
          blend: hasBlend ? { color: {}, alpha: {} } : undefined,
        },
      ],
    });

    t.doCreateRenderPipelineTest(isAsync, !hasBlend || info.sampleType === 'float', descriptor);
  });

g.test('targets_blend')
  .desc(
    `
  For the blend components on either GPUBlendState.color or GPUBlendState.alpha:
  - Tests if the combination of 'srcFactor', 'dstFactor' and 'operation' is valid (if the blend
    operation is "min" or "max", srcFactor and dstFactor must be "one").
  `
  )
  .params(u =>
    u
      .combine('isAsync', [false, true])
      .combine('component', ['color', 'alpha'] as const)
      .beginSubcases()
      .combine('srcFactor', kBlendFactors)
      .combine('dstFactor', kBlendFactors)
      .combine('operation', kBlendOperations)
  )
  .fn(async t => {
    const { isAsync, component, srcFactor, dstFactor, operation } = t.params;

    const defaultBlendComponent: GPUBlendComponent = {
      srcFactor: 'src-alpha',
      dstFactor: 'dst-alpha',
      operation: 'add',
    };
    const blendComponentToTest: GPUBlendComponent = {
      srcFactor,
      dstFactor,
      operation,
    };
    const format = 'rgba8unorm';

    const descriptor = t.getDescriptor({
      targets: [
        {
          format,
          blend: {
            color: component === 'color' ? blendComponentToTest : defaultBlendComponent,
            alpha: component === 'alpha' ? blendComponentToTest : defaultBlendComponent,
          },
        },
      ],
    });

    if (operation === 'min' || operation === 'max') {
      const _success = srcFactor === 'one' && dstFactor === 'one';
      t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
    } else {
      t.doCreateRenderPipelineTest(isAsync, true, descriptor);
    }
  });

g.test('targets_write_mask')
  .desc(`Tests that color target state write mask must be < 16.`)
  .params(u => u.combine('isAsync', [false, true]).combine('writeMask', [0, 0xf, 0x10, 0x80000001]))
  .fn(async t => {
    const { isAsync, writeMask } = t.params;

    const descriptor = t.getDescriptor({
      targets: [
        {
          format: 'rgba8unorm',
          writeMask,
        },
      ],
    });

    t.doCreateRenderPipelineTest(isAsync, writeMask < 16, descriptor);
  });

g.test('pipeline_output_targets')
  .desc(
    `Pipeline fragment output types must be compatible with target color state format
  - The scalar type (f32, i32, or u32) must match the sample type of the format.
  - The componentCount of the fragment output (e.g. f32, vec2, vec3, vec4) must not have fewer
    channels than that of the color attachment texture formats. Extra components are allowed and are discarded.

  Otherwise, color state write mask must be 0.

  MAINTENANCE_TODO: update this test after the WebGPU SPEC ISSUE 50 "define what 'compatible' means
  for render target formats" is resolved.`
  )
  .params(u =>
    u
      .combine('isAsync', [false, true])
      .combine('writeMask', [0, 0x1, 0x2, 0x4, 0x8, 0xf])
      .combine('format', [undefined, ...kRenderableColorTextureFormats] as const)
      .beginSubcases()
      .combine('hasShaderOutput', [false, true])
      .filter(p => p.format === undefined || p.hasShaderOutput === true)
      .combine('sampleType', ['float', 'uint', 'sint'] as const)
      .combine('componentCount', [1, 2, 3, 4])
  )
  .beforeAllSubcases(t => {
    const { format } = t.params;
    if (format) {
      const info = kTextureFormatInfo[format];
      t.selectDeviceOrSkipTestCase(info.feature);
    }
  })
  .fn(async t => {
    const { isAsync, writeMask, format, hasShaderOutput, sampleType, componentCount } = t.params;
    const info = format ? kTextureFormatInfo[format] : null;

    const descriptor = t.getDescriptor({
      targets: format ? [{ format, writeMask }] : [],
      // To have a dummy depthStencil attachment to avoid having no attachment at all which is invalid
      depthStencil: { format: 'depth24plus' },
      fragmentShaderCode: getFragmentShaderCodeWithOutput(
        hasShaderOutput ? [{ values, plainType: getPlainTypeInfo(sampleType), componentCount }] : []
      ),
    });

    let _success = true;
    if (hasShaderOutput && info) {
      // there is a target correspond to the pipeline output
      assert(format !== undefined);
      const sampleTypeSuccess =
        info.sampleType === 'float' || info.sampleType === 'unfilterable-float'
          ? sampleType === 'float'
          : info.sampleType === sampleType;
      _success =
        sampleTypeSuccess &&
        componentCount >= kTexelRepresentationInfo[format].componentOrder.length;
    }

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
  .beforeAllSubcases(t => {
    const { format } = t.params;
    const info = kTextureFormatInfo[format];
    t.selectDeviceOrSkipTestCase(info.feature);
  })
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
      fragmentShaderCode: getFragmentShaderCodeWithOutput([
        { values, plainType: getPlainTypeInfo(sampleType), componentCount },
      ]),
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
