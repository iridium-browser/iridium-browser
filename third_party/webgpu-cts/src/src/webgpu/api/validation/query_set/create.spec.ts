export const description = `
Tests for validation in createQuerySet.

TODO: pipeline statistics queries are removed from core; consider moving tests to another suite.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { kQueryTypes, kMaxQueryCount } from '../../../capability_info.js';
import { ValidationTest } from '../validation_test.js';

export const g = makeTestGroup(ValidationTest);

g.test('count')
  .desc(
    `
Tests that create query set with the count for all query types:
- count {<, =, >} kMaxQueryCount
- x= {occlusion, pipeline-statistics, timestamp} query
  `
  )
  .params(u =>
    u
      .combine('type', kQueryTypes)
      .beginSubcases()
      .combine('count', [0, kMaxQueryCount, kMaxQueryCount + 1])
  )
  .fn(async t => {
    const { type, count } = t.params;

    await t.selectDeviceForQueryTypeOrSkipTestCase(type);

    const pipelineStatistics =
      type === 'pipeline-statistics' ? (['clipper-invocations'] as const) : ([] as const);

    t.expectValidationError(() => {
      t.device.createQuerySet({ type, count, pipelineStatistics });
    }, count > kMaxQueryCount);
  });

g.test('pipelineStatistics,all_types')
  .desc(
    `
Tests that create query set with the GPUPipelineStatisticName for all query types:
- pipelineStatistics is undefined or empty
- pipelineStatistics is a sequence of valid values
- x= {occlusion, pipeline-statistics, timestamp} query
  `
  )
  .params(u =>
    u
      .combine('type', kQueryTypes)
      .beginSubcases()
      .combine('pipelineStatistics', [undefined, [] as const, ['clipper-invocations'] as const])
  )
  .fn(async t => {
    const { type, pipelineStatistics } = t.params;

    await t.selectDeviceForQueryTypeOrSkipTestCase(type);

    const count = 1;
    const shouldError =
      (type !== 'pipeline-statistics' &&
        pipelineStatistics !== undefined &&
        pipelineStatistics.length > 0) ||
      (type === 'pipeline-statistics' &&
        (pipelineStatistics === undefined || pipelineStatistics.length === 0));

    t.expectValidationError(() => {
      t.device.createQuerySet({ type, count, pipelineStatistics });
    }, shouldError);
  });

g.test('pipelineStatistics,duplicates_and_all')
  .desc(
    `
Tests that create query set with the duplicate values and all values of GPUPipelineStatisticName for pipeline-statistics query.
  `
  )
  .paramsSubcasesOnly(u =>
    u //
      .combine('pipelineStatistics', [
        ['clipper-invocations', 'clipper-invocations'] as const,
        [
          'clipper-invocations',
          'clipper-primitives-out',
          'compute-shader-invocations',
          'fragment-shader-invocations',
          'vertex-shader-invocations',
        ] as const,
      ])
  )
  .fn(async t => {
    const type = 'pipeline-statistics';

    await t.selectDeviceForQueryTypeOrSkipTestCase(type);

    const count = 1;
    const pipelineStatistics = t.params.pipelineStatistics;

    t.expectValidationError(() => {
      t.device.createQuerySet({ type, count, pipelineStatistics });
    }, pipelineStatistics.length !== Array.from(new Set(pipelineStatistics)).length);
  });
