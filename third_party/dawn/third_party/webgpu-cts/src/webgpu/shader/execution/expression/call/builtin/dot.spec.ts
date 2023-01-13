export const description = `
Execution tests for the 'dot' builtin function

T is AbstractInt, AbstractFloat, i32, u32, f32, or f16
@const fn dot(e1: vecN<T>,e2: vecN<T>) -> T
Returns the dot product of e1 and e2.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32, TypeVec } from '../../../../../util/conversion.js';
import { dotInterval } from '../../../../../util/f32_interval.js';
import { vectorTestValues } from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, Case, makeVectorPairToF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('dot', {
  f32_vec2: () => {
    const makeCase = (x: number[], y: number[]): Case => {
      return makeVectorPairToF32IntervalCase(x, y, dotInterval);
    };

    return vectorTestValues(2, false).flatMap(i => {
      return vectorTestValues(2, false).map(j => {
        return makeCase(i, j);
      });
    });
  },
  f32_vec3: () => {
    const makeCase = (x: number[], y: number[]): Case => {
      return makeVectorPairToF32IntervalCase(x, y, dotInterval);
    };

    return vectorTestValues(3, false).flatMap(i => {
      return vectorTestValues(3, false).map(j => {
        return makeCase(i, j);
      });
    });
  },
  f32_vec4: () => {
    const makeCase = (x: number[], y: number[]): Case => {
      return makeVectorPairToF32IntervalCase(x, y, dotInterval);
    };

    return vectorTestValues(4, false).flatMap(i => {
      return vectorTestValues(4, false).map(j => {
        return makeCase(i, j);
      });
    });
  },
});

g.test('abstract_int')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`abstract int tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('i32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`i32 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('u32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`u32 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`abstract float test`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32_vec2');
    await run(
      t,
      builtin('dot'),
      [TypeVec(2, TypeF32), TypeVec(2, TypeF32)],
      TypeF32,
      t.params,
      cases
    );
  });

g.test('f32_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32_vec3');
    await run(
      t,
      builtin('dot'),
      [TypeVec(3, TypeF32), TypeVec(3, TypeF32)],
      TypeF32,
      t.params,
      cases
    );
  });

g.test('f32_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests using vec4s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32_vec4');
    await run(
      t,
      builtin('dot'),
      [TypeVec(4, TypeF32), TypeVec(4, TypeF32)],
      TypeF32,
      t.params,
      cases
    );
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f16 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();
