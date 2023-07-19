/**
 * Base class for implementing data-driven browser tests using Jasmine and
 * Karma.
 *
 * Subclass this type to implement data-driven tests by providing an
 * implementation for |generateResults|.
 *
 * Data-driven testing is the practice of separating the testing logic (scripts)
 * from the inputs they supply to the tested code and from the expected outputs
 * of the test. In other words, data-driven tests are parametrized tests that
 * given an input and an expected output, verify that the code to be tested
 * generates the expected output for the given input. This way the testing logic
 * can be reused and adding more test cases just requires adding more
 * input/output sets.
 *
 * When defining tests using DataDrivenTest, create a subclass providing the
 * testing logic and for each test case you want to run, define an input file
 * with the test case input data and an output file with the expected test
 * output.
 *
 * Your testing logic goes in |generateResults|. When running your tests,
 * DataDrivenTest will go through each of your input files and pass their
 * contents to |generateResults|. Your implementation will be in charge of
 * running the tested code against the provided input and generate some output
 * text. DataDrivenTest will then compare the output you returned against the
 * expected output in the test case's output file. The test fill pass only if
 * the text returned by your implementation of |generateResults| is the same as
 * the content of the test case's output file.
 *
 * To run your test cases first create an instance of your DataDrivenTest
 * subclass and then call |runDataDrivenTests|.
 */
export class DataDrivenTest {
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
   * @param {String} inputExtension - File extension of input files. e.g: 'json'
   * @param {String} outputDirectory - Path to directory containing output
   *     files. Relative to your Karma base path. e.g:
   * 'test/data-driven/output/'
   * @param {String} outputExtension - File extension of output files. e.g:
   *     'out'
   */
  constructor(
      testSuiteName, inputDirectory, inputExtension, outputDirectory,
      outputExtension) {
    this.testSuiteName = testSuiteName;
    this.inputDirectory = this.appendKarmaBasePath(inputDirectory);
    this.inputExtension = inputExtension;
    // Regex used to match an input file name from its path.
    this.inputFilesRegex =
        new RegExp(`${this.inputDirectory}(.+?)\.${this.inputExtension}`);
    this.outputDirectory = this.appendKarmaBasePath(outputDirectory);
    this.outputExtension = outputExtension;
  }

  /**
   * Runs data-driven test cases defined by each input/output file.
   */
  runDataDrivenTests() {
    // Keeping track of any actual input files found to trigger a warning if no
    // files found.
    let testRun = false;
    describe(this.testSuiteName, () => {
      for (const testFile of this.getInputFileNames()) {
        this.runDataDrivenTest(testFile);
        testRun = true;
      }
    });

    if (!testRun) {
      console.warn(
          `No input files found in ${this.inputDirectory} ` +
          `with extension *.${this.inputExtension}.\n` +
          `Verify that the input directory and extensions are correct.`);
    }
  }

  /**
   * Runs a single data-driven test case.
   * @param {String} inputFile - Name of the test case's input file without
   *     extension.
   */
  runDataDrivenTest(inputFile) {
    it(inputFile, async () => {
      const inputFilePath = this.getPathForInputFile(inputFile);
      const inputText = await this.readFileContent(inputFilePath);
      expect(inputText).withContext('Input file content').not.toBeNull();

      const outputFilePath = this.getPathForOutputFile(inputFile);
      const expectedOutput = await this.readFileContent(outputFilePath)
      expect(expectedOutput).withContext('Output file content').not.toBeNull();

      const output = this.generateResults(inputText);

      expect(output).withContext('Test output').toEqual(expectedOutput);
    });
  }

  /**
   * Generate test results for a given input.
   * This function is called for each input file in |inputDirectory|, passing
   * their contents in |inputText|.
   *
   * Subclasses must provide their own implementation.
   * @abstract
   * @param {String} inputText - Content of a input file.
   * @return {String} - Output text that will be compared against the output
   *     file corresponding to the input file containing |inputText|.
   * @throws {Error} - Will throw an error if not implemented by subclasses.
   */
  generateResults(inputText) {
    throw new Error('Method generateResults must be implemented by subclasses');
  }

  /**
   * Builds the path to an input file using |inputDirectory| and
   * |inputExtension|. The resulting path can be used to get the file contents
   * via |readFileContent|.
   * @param {String} fileName - Input file name without extension.
   * @return {String} Path to input file.
   */
  getPathForInputFile(fileName) {
    return `${this.inputDirectory}${fileName}.${this.inputExtension}`;
  }

  /**
   * Builds the path to an output file using |inputDirectory| and
   * |inputExtension|. The resulting path can be used to get the file contents
   * via |readFileContent|.
   * @param {String} fileName - Output file name without extension.
   * @return {String} Path to output file.
   */
  getPathForOutputFile(fileName) {
    return `${this.outputDirectory}${fileName}.${this.outputExtension}`;
  }

  /**
   * Appends Karma's base path to a given path. The resulting path can be used
   * to get the file contents via |readFileContent|.
   * @param {String} path - Relative path to Karma's base path.
   * @return {String} - Original path including Karma's base path.
   */
  appendKarmaBasePath(path) {
    return `/base/${path}`;
  }

  /**
   * Generator method that yields the set of input file names found in
   * |inputDirectory|.
   *
   * @yields {String} Test file name without the extension.
   */
  * getInputFileNames() {
    for (const file in __karma__.files) {
      if (!__karma__.files.hasOwnProperty(file)) continue;
      // Match file name without the extension.
      const match = file.match(this.inputFilesRegex);

      if (match) {
        yield match[1];
      }
    }
  }

  /**
   * Gets the contents of a file served by Karma's server.
   * @param {String} filePath - Path to the file relative to Karma's base path.
   * @return {Promise<String|null>} String content of the file or null if file
   *     could not be found.
   */
  async readFileContent(filePath) {
    const response = await fetch(filePath);

    if (response.status != 200) {
      return null;
    }

    return response.text();
  }
}
