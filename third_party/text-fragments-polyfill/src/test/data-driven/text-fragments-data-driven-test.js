import * as generationUtils from '../../src/fragment-generation-utils.js';
import * as fragmentUtils from '../../src/text-fragment-utils.js';
import {marksArrayToString} from '../utils/marksArrayToString.js';

import {DataDrivenTest} from './data-driven-test.js';

/**
 * Test output success constant.
 */
const OUTPUT_SUCCESS = 'Success';
/**
 * Test output failure constant.
 */
const OUTPUT_FAILURE = 'Failure';

/**
 * Data Driven Tests For Text Fragment Generation and Highlighting.
 *
 * This class tests the main features of the polyfill: text fragment generation
 * and fragment highlighting. {@see TextFragmentDataDrivenTestInput} for the
 * test case definition. Each test case tries to generate a text fragment for a
 * range in an html DOM, then highlights the generated fragment to verify that
 * the expected text is highlighted.
 *
 * Adding Test Cases:
 *
 * To add more test cases, include one input file per test case in
 * |inputDirectory|, the corresponding output file in |outputDirectory| and in
 * |htmlDirectory| put the html document referenced in the test case's input
 * file.
 *
 * Each input file
 * should be a {@see TextFragmentDataDrivenTestInput} json serialized. Each
 * output file should be a plain text file with the following format:
 *
 * GENERATION: [Success | Failure]
 * HIGHLIGHTING: [Success | Failure]
 *
 * The GENERATION line states if fragment generation should succeed for the test
 * case. The HIGHLIGHTING line states if the test case's highlighted text should
 * match the expected highlighted text.
 *
 * If at some point the expectations for a test case change due to a bug fix or
 * change in the requirements, update the corresponding output file with the new
 * expectations.
 */
class TextFragmentsDataDrivenTest extends DataDrivenTest {
  /**
   * Constructor of {@link DataDrivenTest}. Configure your tests here by
   * providing the source of input and output files. To be able differentiate
   * input and output files, either |inputDirectory| must be different of
   * |outputDirectory|, or |inputExtension| must be different to
   * |outputExtension|
   * @param {String} testSuiteName - Name of the Jasmine test suite that
   *     contains the tests run by this instance. e.g: 'My Beautiful Feature
   * Data Driven Tests'
   * @param {String} inputDirectory - Path to directory containing input files.
   *     Relative to your Karma base path. e.g:
   * 'test/data-driven/input/test-params/'
   *
   * Each input file must be a {@see TextFragmentDataDrivenTestInput} json
   * serialized.
   * @param {String} inputExtension - File extension of input files. e.g: 'json'
   * @param {String} outputDirectory - Path to directory containing output
   *     files. Relative to your Karma base path. e.g:
   * 'test/data-driven/output/'
   * @param {String} outputExtension - File extension of output files. e.g:
   *     'out'
   * @param {String} htmlDirectory - Path to directory containing the test
   *     case's html pages. Relative to your Karma base path. e.g:
   * 'test/data-driven/input/html/'
   */
  constructor(
      testSuiteName, inputDirectory, inputExtension, outputDirectory,
      outputExtension, htmlDirectory) {
    super(
        testSuiteName, inputDirectory, inputExtension, outputDirectory,
        outputExtension);
    this.htmlDirectory = htmlDirectory
  }

  /**
   * Runs a Text Fragment data-driven test case.
   *
   * This method takes a test case definition in |inputText|, tries to generate
   * and highlight a text fragment, checking if the highlighted text matches the
   * test case's expected highlighted text.
   * @param {String} inputText - Test case input. Expected to be a
   * {@see TextFragmentDataDrivenTestInput} json serialized.
   * @return {String} - The test results, having the following format:
   *
   * GENERATION: [Success | Failure]
   * HIGHLIGHTING: [Success | Failure]
   *
   * The GENERATION line indicates if a fragment was successfully generated for
   * the test case input. The HIGHLIGHTING line indicates if the highlighted
   * text matched the test case's expected highlighted text.
   */
  generateResults(inputText) {
    const input = this.parseInput(inputText);
    document.body.innerHTML = __html__[this.getPathForHtml(input.htmlFileName)];

    const generatedFragment = this.generateFragment(input);
    const generationResult = generatedFragment ? true : false;

    const highlightingResult =
        generationResult && this.testHighlighting(generatedFragment, input);

    const outputText =
        this.generateOutput(generationResult, highlightingResult);
    return outputText;
  }

  /**
   * Generates a text fragment for a test case input.
   * @param {TextFragmentDataDrivenTestInput} input - Test case input.
   * @return {TextFragment|null} Generated text fragment or null if generation
   *     failed.
   */
  generateFragment(input) {
    const startParent = input.startParentId ?
        document.getElementById(input.startParentId) :
        document.body;
    const start = startParent.childNodes[input.startOffsetInParent];
    const startOffset = input.startTextOffset ?? 0;

    const endParent = input.endParentId ?
        document.getElementById(input.endParentId) :
        document.body;
    const end = endParent.childNodes[input.endOffsetInParent];
    const endOffset = input.endTextOffset ?? 0;

    const range = document.createRange();
    range.setStart(start, startOffset);
    range.setEnd(end, endOffset);

    const selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);

    const result = generationUtils.generateFragment(selection);
    return result.fragment;
  }

  /**
   * Verifies that the text highlighted for a given text fragment is correct.
   * @param {TextFragment} fragment - Fragment to highlight.
   * @param {TextFragmentDataDrivenTestInput} testInput - Test input object
   *     including the expected highlight text.
   * @return {Boolean} True iff the text highlighted for |fragment| is the same
   *     as |testInput.highlightText|.
   */
  testHighlighting(fragment, testInput) {
    const directive = {text: [fragment]};
    const processedDirectives = fragmentUtils.processFragmentDirectives(
        directive,
        )['text'];

    if (processedDirectives.count == 0) {
      return false;
    }
    const marks = processedDirectives[0];
    const highlightingTextMatch =
        marksArrayToString(marks) == testInput.highlightText;

    return highlightingTextMatch;
  }

  /**
   * Generates the output of a test case.
   * @param {Boolean} generationSuccess - True if fragment generation test was
   *     successful.
   * @param {Boolean} highlightingSuccess - True if highlighting test was
   *     successful.
   * @return {String} The test's output text that will be compared with the
   *     expected output in the test's output file.
   */
  generateOutput(generationSuccess, highlightingSuccess) {
    return `GENERATION: ${this.getTextForTestResult(generationSuccess)}\n` +
        `HIGHLIGHTING: ${this.getTextForTestResult(highlightingSuccess)}\n`
  }

  /**
   * Helper method to get the textual representation of a test result that is
   * included in the data driven test output.
   * @param {Boolean} testSuccess - True if test was successful.
   * @return {String} {@link OUTPUT_SUCCESS} when |testSuccess| is true
   *     or {@link OUTPUT_FAILURE} when false.
   */
  getTextForTestResult(testSuccess) {
    return testSuccess ? OUTPUT_SUCCESS : OUTPUT_FAILURE;
  }

  /**
   * Text Fragments Data Driven Test input object.
   * @typedef {Object} TextFragmentDataDrivenTestInput
   * @property {String} htmlFileName - Html file name for the test case. This
   * file should be in |htmlDirectory|.
   * @property {String} [startParentId] - Id of the parent of the node where the
   * test case’s selection starts. Optional: when null then the parent node is
   * the document's body.
   * @property {Number} startOffsetInParent - Offset of the selection’s starting
   * node in its parent node.
   * @property {Number} [startTextOffset] - When the selection starts in a text
   * node, this value indicates the offset inside the node’s text where the
   * selection starts.
   * @property {String} [endParentId] - Id of the parent of the node where the
   * test case’s selection ends. Optional: when null then the parent node is the
   * document's body.
   * @property {Number} endOffsetInParent - Offset of the selection’s ending
   * node in its parent node.
   * @property {Number} [endTextOffset] - When the selection ends in a text
   * node, this value indicates the offset inside the node’s text where the
   * selection ends.
   * @property {String} selectedText - Expected text content inside the
   * selection.
   * @property {String} highlightText - Expected text content inside the range
   * highlighted.
   */

  /**
   * Parses the content of a test case input file.
   * @param {String} input - Content of a test case input file.
   * @return {TextFragmentDataDrivenTestInput} Test Case input object.
   */
  parseInput(input) {
    try {
      return JSON.parse(input);
    } catch (error) {
      throw new Error(
          'Invalid input file. Content must be a valid json object.\n' +
          `${error}.`);
    }
  }

  /**
   * Builds the path to an html file using |htmlDirectory|.
   * The resulting path can be used to get the html contents
   * via karma's __html__[path].
   * @param {String} fileName - Html file name including extension.
   * @return {String} Path to html file.
   */
  getPathForHtml(fileName) {
    return `${this.htmlDirectory}${fileName}`;
  }
}

// Instantiate and run the tests.
const test = new TextFragmentsDataDrivenTest(
    'Text Fragments Data Driven Tests', 'test/data-driven/input/test-params/',
    'json', 'test/data-driven/output/', 'out', 'test/data-driven/input/html/');

test.runDataDrivenTests();
