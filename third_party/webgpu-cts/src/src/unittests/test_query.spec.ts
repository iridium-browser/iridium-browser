export const description = `
Tests for TestQuery
`;
import { makeTestGroup } from '../common/framework/test_group.js';
import {
  TestQueryMultiFile,
  TestQueryMultiTest,
  TestQueryMultiCase,
  TestQuerySingleCase,
  TestQuery,
} from '../common/internal/query/query.js';

import { UnitTest } from './unit_test.js';

class F extends UnitTest {
  expectToString(q: TestQuery, exp: string) {
    this.expect(q.toString() === exp);
  }
}

export const g = makeTestGroup(F);

g.test('constructor').fn(t => {
  t.shouldThrow('Error', () => new TestQueryMultiTest('suite', [], []));

  t.shouldThrow('Error', () => new TestQueryMultiCase('suite', ['a'], [], {}));
  t.shouldThrow('Error', () => new TestQueryMultiCase('suite', [], ['c'], {}));
  t.shouldThrow('Error', () => new TestQueryMultiCase('suite', [], [], {}));

  t.shouldThrow('Error', () => new TestQuerySingleCase('suite', ['a'], [], {}));
  t.shouldThrow('Error', () => new TestQuerySingleCase('suite', [], ['c'], {}));
  t.shouldThrow('Error', () => new TestQuerySingleCase('suite', [], [], {}));
});

g.test('toString').fn(t => {
  t.expectToString(new TestQueryMultiFile('s', []), 's:*');
  t.expectToString(new TestQueryMultiFile('s', ['a']), 's:a,*');
  t.expectToString(new TestQueryMultiFile('s', ['a', 'b']), 's:a,b,*');
  t.expectToString(new TestQueryMultiTest('s', ['a', 'b'], []), 's:a,b:*');
  t.expectToString(new TestQueryMultiTest('s', ['a', 'b'], ['c']), 's:a,b:c,*');
  t.expectToString(new TestQueryMultiTest('s', ['a', 'b'], ['c', 'd']), 's:a,b:c,d,*');
  t.expectToString(new TestQueryMultiCase('s', ['a', 'b'], ['c', 'd'], {}), 's:a,b:c,d:*');
  t.expectToString(
    new TestQueryMultiCase('s', ['a', 'b'], ['c', 'd'], { x: 1 }),
    's:a,b:c,d:x=1;*'
  );
  t.expectToString(
    new TestQueryMultiCase('s', ['a', 'b'], ['c', 'd'], { x: 1, y: 2 }),
    's:a,b:c,d:x=1;y=2;*'
  );
  t.expectToString(
    new TestQuerySingleCase('s', ['a', 'b'], ['c', 'd'], { x: 1, y: 2 }),
    's:a,b:c,d:x=1;y=2'
  );
  t.expectToString(new TestQuerySingleCase('s', ['a', 'b'], ['c', 'd'], {}), 's:a,b:c,d:');
});
