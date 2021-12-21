/* eslint no-console: "off" */

import { DefaultTestFileLoader } from '../internal/file_loader.js';
import { prettyPrintLog } from '../internal/logging/log_message.js';
import { Logger } from '../internal/logging/logger.js';
import { LiveTestCaseResult } from '../internal/logging/result.js';
import { parseQuery } from '../internal/query/parseQuery.js';
import { parseExpectationsForTestQuery } from '../internal/query/query.js';
import { assert, unreachable } from '../util/util.js';

import sys from './helper/sys.js';

function usage(rc: number): never {
  console.log('Usage:');
  console.log(`  tools/run_${sys.type} [OPTIONS...] QUERIES...`);
  console.log(`  tools/run_${sys.type} 'unittests:*' 'webgpu:buffers,*'`);
  console.log('Options:');
  console.log('  --verbose       Print result/log of every test as it runs.');
  console.log('  --debug         Include debug messages in logging.');
  console.log('  --print-json    Print the complete result JSON in the output.');
  console.log('  --expectations  Path to expectations file.');
  return sys.exit(rc);
}

if (!sys.existsSync('src/common/runtime/cmdline.ts')) {
  console.log('Must be run from repository root');
  usage(1);
}

let verbose = false;
let debug = false;
let printJSON = false;
let loadWebGPUExpectations: Promise<unknown> | undefined = undefined;

const queries: string[] = [];
for (let i = 0; i < sys.args.length; ++i) {
  const a = sys.args[i];
  if (a.startsWith('-')) {
    if (a === '--verbose') {
      verbose = true;
    } else if (a === '--debug') {
      debug = true;
    } else if (a === '--print-json') {
      printJSON = true;
    } else if (a === '--expectations') {
      const expectationsFile = new URL(sys.args[++i], `file://${sys.cwd()}`).pathname;
      loadWebGPUExpectations = import(expectationsFile).then(m => m.expectations);
    } else {
      usage(1);
    }
  } else {
    queries.push(a);
  }
}

if (queries.length === 0) {
  usage(0);
}

(async () => {
  const loader = new DefaultTestFileLoader();
  assert(queries.length === 1, 'currently, there must be exactly one query on the cmd line');
  const filterQuery = parseQuery(queries[0]);
  const testcases = await loader.loadCases(filterQuery);
  const expectations = parseExpectationsForTestQuery(
    await (loadWebGPUExpectations ?? []),
    filterQuery
  );

  Logger.globalDebugMode = debug;
  const log = new Logger();

  const failed: Array<[string, LiveTestCaseResult]> = [];
  const warned: Array<[string, LiveTestCaseResult]> = [];
  const skipped: Array<[string, LiveTestCaseResult]> = [];

  let total = 0;

  for (const testcase of testcases) {
    const name = testcase.query.toString();
    const [rec, res] = log.record(name);
    await testcase.run(rec, expectations);

    if (verbose) {
      printResults([[name, res]]);
    }

    total++;
    switch (res.status) {
      case 'pass':
        break;
      case 'fail':
        failed.push([name, res]);
        break;
      case 'warn':
        warned.push([name, res]);
        break;
      case 'skip':
        skipped.push([name, res]);
        break;
      default:
        unreachable('unrecognized status');
    }
  }

  assert(total > 0, 'found no tests!');

  // TODO: write results out somewhere (a file?)
  if (printJSON) {
    console.log(log.asJSON(2));
  }

  if (skipped.length) {
    console.log('');
    console.log('** Skipped **');
    printResults(skipped);
  }
  if (warned.length) {
    console.log('');
    console.log('** Warnings **');
    printResults(warned);
  }
  if (failed.length) {
    console.log('');
    console.log('** Failures **');
    printResults(failed);
  }

  const passed = total - warned.length - failed.length - skipped.length;
  const pct = (x: number) => ((100 * x) / total).toFixed(2);
  const rpt = (x: number) => {
    const xs = x.toString().padStart(1 + Math.log10(total), ' ');
    return `${xs} / ${total} = ${pct(x).padStart(6, ' ')}%`;
  };
  console.log('');
  console.log(`** Summary **
Passed  w/o warnings = ${rpt(passed)}
Passed with warnings = ${rpt(warned.length)}
Skipped              = ${rpt(skipped.length)}
Failed               = ${rpt(failed.length)}`);

  if (failed.length || warned.length) {
    sys.exit(1);
  }
})().catch(ex => {
  console.log(ex.stack ?? ex.toString());
  sys.exit(1);
});

function printResults(results: Array<[string, LiveTestCaseResult]>): void {
  for (const [name, r] of results) {
    console.log(`[${r.status}] ${name} (${r.timems}ms). Log:`);
    if (r.logs) {
      for (const l of r.logs) {
        console.log(prettyPrintLog(l));
      }
    }
  }
}
