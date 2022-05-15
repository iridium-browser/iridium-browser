import { Fixture, SkipTestCase, TestParams, UnexpectedPassError } from '../framework/fixture.js';
import {
  CaseParamsBuilder,
  builderIterateCasesWithSubcases,
  kUnitCaseParamsBuilder,
  ParamsBuilderBase,
  SubcaseParamsBuilder,
} from '../framework/params_builder.js';
import { Expectation } from '../internal/logging/result.js';
import { TestCaseRecorder } from '../internal/logging/test_case_recorder.js';
import { extractPublicParams, Merged, mergeParams } from '../internal/params_utils.js';
import { compareQueries, Ordering } from '../internal/query/compare.js';
import { TestQuerySingleCase, TestQueryWithExpectation } from '../internal/query/query.js';
import { kPathSeparator } from '../internal/query/separators.js';
import {
  stringifyPublicParams,
  stringifyPublicParamsUniquely,
} from '../internal/query/stringify_params.js';
import { validQueryPart } from '../internal/query/validQueryPart.js';
import { assert, unreachable } from '../util/util.js';

export type RunFn = (
  rec: TestCaseRecorder,
  expectations?: TestQueryWithExpectation[]
) => Promise<void>;

export interface TestCaseID {
  readonly test: readonly string[];
  readonly params: TestParams;
}

export interface RunCase {
  readonly id: TestCaseID;
  run(
    rec: TestCaseRecorder,
    selfQuery: TestQuerySingleCase,
    expectations: TestQueryWithExpectation[]
  ): Promise<void>;
}

// Interface for defining tests
export interface TestGroupBuilder<F extends Fixture> {
  test(name: string): TestBuilderWithName<F>;
}
export function makeTestGroup<F extends Fixture>(fixture: FixtureClass<F>): TestGroupBuilder<F> {
  return new TestGroup(fixture);
}

// Interfaces for running tests
export interface IterableTestGroup {
  iterate(): Iterable<IterableTest>;
  validate(): void;
}
export interface IterableTest {
  testPath: string[];
  description: string | undefined;
  readonly testCreationStack: Error;
  iterate(): Iterable<RunCase>;
}

export function makeTestGroupForUnitTesting<F extends Fixture>(
  fixture: FixtureClass<F>
): TestGroup<F> {
  return new TestGroup(fixture);
}

export type FixtureClass<F extends Fixture = Fixture> = new (
  log: TestCaseRecorder,
  params: TestParams
) => F;
type TestFn<F extends Fixture, P extends {}> = (t: F & { params: P }) => Promise<void> | void;

export class TestGroup<F extends Fixture> implements TestGroupBuilder<F> {
  private fixture: FixtureClass;
  private seen: Set<string> = new Set();
  private tests: Array<TestBuilder> = [];

  constructor(fixture: FixtureClass) {
    this.fixture = fixture;
  }

  iterate(): Iterable<IterableTest> {
    return this.tests;
  }

  private checkName(name: string): void {
    assert(
      // Shouldn't happen due to the rule above. Just makes sure that treated
      // unencoded strings as encoded strings is OK.
      name === decodeURIComponent(name),
      `Not decodeURIComponent-idempotent: ${name} !== ${decodeURIComponent(name)}`
    );
    assert(!this.seen.has(name), `Duplicate test name: ${name}`);

    this.seen.add(name);
  }

  test(name: string): TestBuilderWithName<F> {
    const testCreationStack = new Error(`Test created: ${name}`);

    this.checkName(name);

    const parts = name.split(kPathSeparator);
    for (const p of parts) {
      assert(validQueryPart.test(p), `Invalid test name part ${p}; must match ${validQueryPart}`);
    }

    const test = new TestBuilder(parts, this.fixture, testCreationStack);
    this.tests.push(test);
    return (test as unknown) as TestBuilderWithName<F>;
  }

  validate(): void {
    for (const test of this.tests) {
      test.validate();
    }
  }
}

interface TestBuilderWithName<F extends Fixture> extends TestBuilderWithParams<F, {}> {
  desc(description: string): this;
  /**
   * A noop function with the purpose of highlighting value `id`.
   *
   * @param id a token uniquely assigned to this test.
   */
  uniqueId(id: string): this;
  /**
   * A noop function to associate a test with the relevant part of the specification.
   *
   * @param url a link to the spec where test is extracted from.
   */
  specURL(url: string): this;
  /**
   * Parameterize the test, generating multiple cases, each possibly having subcases.
   *
   * The `unit` value passed to the `cases` callback is an immutable constant
   * `CaseParamsBuilder<{}>` representing the "unit" builder `[ {} ]`,
   * provided for convenience. The non-callback overload can be used if `unit` is not needed.
   */
  params<CaseP extends {}, SubcaseP extends {}>(
    cases: (unit: CaseParamsBuilder<{}>) => ParamsBuilderBase<CaseP, SubcaseP>
  ): TestBuilderWithParams<F, Merged<CaseP, SubcaseP>>;
  /**
   * Parameterize the test, generating multiple cases, each possibly having subcases.
   *
   * Use the callback overload of this method if a "unit" builder is needed.
   */
  params<CaseP extends {}, SubcaseP extends {}>(
    cases: ParamsBuilderBase<CaseP, SubcaseP>
  ): TestBuilderWithParams<F, Merged<CaseP, SubcaseP>>;

  /**
   * Parameterize the test, generating multiple cases, without subcases.
   */
  paramsSimple<P extends {}>(cases: Iterable<P>): TestBuilderWithParams<F, P>;

  /**
   * Parameterize the test, generating one case with multiple subcases.
   */
  paramsSubcasesOnly<P extends {}>(subcases: Iterable<P>): TestBuilderWithParams<F, P>;
  /**
   * Parameterize the test, generating one case with multiple subcases.
   *
   * The `unit` value passed to the `subcases` callback is an immutable constant
   * `SubcaseParamsBuilder<{}>`, with one empty case `{}` and one empty subcase `{}`.
   */
  paramsSubcasesOnly<P extends {}>(
    subcases: (unit: SubcaseParamsBuilder<{}, {}>) => SubcaseParamsBuilder<{}, P>
  ): TestBuilderWithParams<F, P>;
}

interface TestBuilderWithParams<F extends Fixture, P extends {}> {
  batch(b: number): this;
  fn(fn: TestFn<F, P>): void;
  unimplemented(): void;
}

class TestBuilder {
  readonly testPath: string[];
  description: string | undefined;
  readonly testCreationStack: Error;

  private readonly fixture: FixtureClass;
  private testFn: TestFn<Fixture, {}> | undefined;
  private testCases?: ParamsBuilderBase<{}, {}> = undefined;
  private batchSize: number = 0;

  constructor(testPath: string[], fixture: FixtureClass, testCreationStack: Error) {
    this.testPath = testPath;
    this.fixture = fixture;
    this.testCreationStack = testCreationStack;
  }

  desc(description: string): this {
    this.description = description.trim();
    return this;
  }

  uniqueId(id: string): this {
    return this;
  }

  specURL(url: string): this {
    return this;
  }

  fn(fn: TestFn<Fixture, {}>): void {
    // eslint-disable-next-line no-warning-comments
    // MAINTENANCE_TODO: add "TODO" if there's no description? (and make sure it only ends up on
    // actual tests, not on test parents in the tree, which is what happens if you do it here, not
    // sure why)
    assert(this.testFn === undefined);
    this.testFn = fn;
  }

  batch(b: number): this {
    this.batchSize = b;
    return this;
  }

  unimplemented(): void {
    assert(this.testFn === undefined);

    this.description =
      (this.description ? this.description + '\n\n' : '') + 'TODO: .unimplemented()';

    this.testFn = () => {
      throw new SkipTestCase('test unimplemented');
    };
  }

  validate(): void {
    const testPathString = this.testPath.join(kPathSeparator);
    assert(this.testFn !== undefined, () => {
      let s = `Test is missing .fn(): ${testPathString}`;
      if (this.testCreationStack.stack) {
        s += `\n-> test created at:\n${this.testCreationStack.stack}`;
      }
      return s;
    });

    if (this.testCases === undefined) {
      return;
    }

    const seen = new Set<string>();
    for (const [caseParams, subcases] of builderIterateCasesWithSubcases(this.testCases)) {
      for (const subcaseParams of subcases ?? [{}]) {
        const params = mergeParams(caseParams, subcaseParams);
        assert(this.batchSize === 0 || !('batch__' in params));

        // stringifyPublicParams also checks for invalid params values
        const testcaseString = stringifyPublicParams(params);

        // A (hopefully) unique representation of a params value.
        const testcaseStringUnique = stringifyPublicParamsUniquely(params);
        assert(
          !seen.has(testcaseStringUnique),
          `Duplicate public test case params for test ${testPathString}: ${testcaseString}`
        );
        seen.add(testcaseStringUnique);
      }
    }
  }

  params(
    cases: ((unit: CaseParamsBuilder<{}>) => ParamsBuilderBase<{}, {}>) | ParamsBuilderBase<{}, {}>
  ): TestBuilder {
    assert(this.testCases === undefined, 'test case is already parameterized');
    if (cases instanceof Function) {
      this.testCases = cases(kUnitCaseParamsBuilder);
    } else {
      this.testCases = cases;
    }
    return this;
  }

  paramsSimple(cases: Iterable<{}>): TestBuilder {
    assert(this.testCases === undefined, 'test case is already parameterized');
    this.testCases = kUnitCaseParamsBuilder.combineWithParams(cases);
    return this;
  }

  paramsSubcasesOnly(
    subcases: Iterable<{}> | ((unit: SubcaseParamsBuilder<{}, {}>) => SubcaseParamsBuilder<{}, {}>)
  ): TestBuilder {
    if (subcases instanceof Function) {
      return this.params(subcases(kUnitCaseParamsBuilder.beginSubcases()));
    } else {
      return this.params(kUnitCaseParamsBuilder.beginSubcases().combineWithParams(subcases));
    }
  }

  *iterate(): IterableIterator<RunCase> {
    assert(this.testFn !== undefined, 'No test function (.fn()) for test');
    this.testCases ??= kUnitCaseParamsBuilder;
    for (const [caseParams, subcases] of builderIterateCasesWithSubcases(this.testCases)) {
      if (this.batchSize === 0 || subcases === undefined) {
        yield new RunCaseSpecific(
          this.testPath,
          caseParams,
          subcases,
          this.fixture,
          this.testFn,
          this.testCreationStack
        );
      } else {
        const subcaseArray = Array.from(subcases);
        if (subcaseArray.length <= this.batchSize) {
          yield new RunCaseSpecific(
            this.testPath,
            caseParams,
            subcaseArray,
            this.fixture,
            this.testFn,
            this.testCreationStack
          );
        } else {
          for (let i = 0; i < subcaseArray.length; i = i + this.batchSize) {
            yield new RunCaseSpecific(
              this.testPath,
              { ...caseParams, batch__: i / this.batchSize },
              subcaseArray.slice(i, Math.min(subcaseArray.length, i + this.batchSize)),
              this.fixture,
              this.testFn,
              this.testCreationStack
            );
          }
        }
      }
    }
  }
}

class RunCaseSpecific implements RunCase {
  readonly id: TestCaseID;

  private readonly params: {};
  private readonly subcases: Iterable<{}> | undefined;
  private readonly fixture: FixtureClass;
  private readonly fn: TestFn<Fixture, {}>;
  private readonly testCreationStack: Error;

  constructor(
    testPath: string[],
    params: {},
    subcases: Iterable<{}> | undefined,
    fixture: FixtureClass,
    fn: TestFn<Fixture, {}>,
    testCreationStack: Error
  ) {
    this.id = { test: testPath, params: extractPublicParams(params) };
    this.params = params;
    this.subcases = subcases;
    this.fixture = fixture;
    this.fn = fn;
    this.testCreationStack = testCreationStack;
  }

  async runTest(
    rec: TestCaseRecorder,
    params: {},
    throwSkip: boolean,
    expectedStatus: Expectation
  ): Promise<void> {
    try {
      rec.beginSubCase();
      if (expectedStatus === 'skip') {
        throw new SkipTestCase('Skipped by expectations');
      }
      const inst = new this.fixture(rec, params);

      try {
        await inst.doInit();
        await this.fn(inst as Fixture & { params: {} });
      } finally {
        // Runs as long as constructor succeeded, even if initialization or the test failed.
        await inst.doFinalize();
      }
    } catch (ex) {
      // There was an exception from constructor, init, test, or finalize.
      // An error from init or test may have been a SkipTestCase.
      // An error from finalize may have been an eventualAsyncExpectation failure
      // or unexpected validation/OOM error from the GPUDevice.
      if (throwSkip && ex instanceof SkipTestCase) {
        throw ex;
      }
      rec.threw(ex);
    } finally {
      try {
        rec.endSubCase(expectedStatus);
      } catch (ex) {
        assert(ex instanceof UnexpectedPassError);
        ex.message = `Testcase passed unexpectedly.`;
        ex.stack = this.testCreationStack.stack;
        rec.warn(ex);
      }
    }
  }

  async run(
    rec: TestCaseRecorder,
    selfQuery: TestQuerySingleCase,
    expectations: TestQueryWithExpectation[]
  ): Promise<void> {
    const getExpectedStatus = (selfQueryWithSubParams: TestQuerySingleCase) => {
      let didSeeFail = false;
      for (const exp of expectations) {
        const ordering = compareQueries(exp.query, selfQueryWithSubParams);
        if (ordering === Ordering.Unordered || ordering === Ordering.StrictSubset) {
          continue;
        }

        switch (exp.expectation) {
          // Skip takes precedence. If there is any expectation indicating a skip,
          // signal it immediately.
          case 'skip':
            return 'skip';
          case 'fail':
            // Otherwise, indicate that we might expect a failure.
            didSeeFail = true;
            break;
          default:
            unreachable();
        }
      }
      return didSeeFail ? 'fail' : 'pass';
    };

    rec.start();
    if (this.subcases) {
      let totalCount = 0;
      let skipCount = 0;
      for (const subParams of this.subcases) {
        rec.info(new Error('subcase: ' + stringifyPublicParams(subParams)));
        try {
          const params = mergeParams(this.params, subParams);
          const subcaseQuery = new TestQuerySingleCase(
            selfQuery.suite,
            selfQuery.filePathParts,
            selfQuery.testPathParts,
            params
          );
          await this.runTest(rec, params, true, getExpectedStatus(subcaseQuery));
        } catch (ex) {
          if (ex instanceof SkipTestCase) {
            // Convert SkipTestCase to info messages
            ex.message = 'subcase skipped: ' + ex.message;
            rec.info(ex);
            ++skipCount;
          } else {
            // Since we are catching all error inside runTest(), this should never happen
            rec.threw(ex);
          }
        }
        ++totalCount;
      }
      if (skipCount === totalCount) {
        rec.skipped(new SkipTestCase('all subcases were skipped'));
      }
    } else {
      await this.runTest(rec, this.params, false, getExpectedStatus(selfQuery));
    }
    rec.finish();
  }
}
