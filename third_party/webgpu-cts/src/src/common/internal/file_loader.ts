import { IterableTestGroup } from '../internal/test_group.js';
import { assert } from '../util/util.js';

import { parseQuery } from './query/parseQuery.js';
import { TestQuery } from './query/query.js';
import { TestSuiteListing } from './test_suite_listing.js';
import { loadTreeForQuery, TestTree, TestTreeLeaf } from './tree.js';

// A listing file, e.g. either of:
// - `src/webgpu/listing.ts` (which is dynamically computed, has a Promise<TestSuiteListing>)
// - `out/webgpu/listing.js` (which is pre-baked, has a TestSuiteListing)
interface ListingFile {
  listing: Promise<TestSuiteListing> | TestSuiteListing;
}

// A .spec.ts file, as imported.
export interface SpecFile {
  readonly description: string;
  readonly g: IterableTestGroup;
}

// Base class for DefaultTestFileLoader and FakeTestFileLoader.
export abstract class TestFileLoader {
  abstract listing(suite: string): Promise<TestSuiteListing>;
  protected abstract import(path: string): Promise<SpecFile>;

  importSpecFile(suite: string, path: string[]): Promise<SpecFile> {
    return this.import(`${suite}/${path.join('/')}.spec.js`);
  }

  async loadTree(query: TestQuery, subqueriesToExpand: string[] = []): Promise<TestTree> {
    return loadTreeForQuery(
      this,
      query,
      subqueriesToExpand.map(s => {
        const q = parseQuery(s);
        assert(q.level >= 2, () => `subqueriesToExpand entries should not be multi-file:\n  ${q}`);
        return q;
      })
    );
  }

  async loadCases(query: TestQuery): Promise<IterableIterator<TestTreeLeaf>> {
    const tree = await this.loadTree(query);
    return tree.iterateLeaves();
  }
}

export class DefaultTestFileLoader extends TestFileLoader {
  async listing(suite: string): Promise<TestSuiteListing> {
    return ((await import(`../../${suite}/listing.js`)) as ListingFile).listing;
  }

  import(path: string): Promise<SpecFile> {
    return import(`../../${path}`);
  }
}
