const playwright = require('playwright');
// TODO: Re-enable when working on CI
// process.env.WEBKIT_HEADLESS_BIN = playwright.webkit.executablePath();
process.env.FIREFOX_HEADLESS_BIN = playwright.firefox.executablePath();
process.env.CHROMIUM_BIN = playwright.chromium.executablePath();

module.exports = function (config) {
  config.set({
    // base path that will be used to resolve all patterns (eg. files, exclude)
    basePath: '.',

    // frameworks to use
    // available frameworks: https://npmjs.org/browse/keyword/karma-adapter
    frameworks: ['jasmine'],

    // list of files / patterns to load in the browser
    files: [
      { pattern: 'src/*.js', type: 'module' },
      { pattern: 'test/**/*.js', type: 'module' },
      'test/**/*.html',
      { pattern: 'test/**/*.json', included: false },
      { pattern: 'test/**/*.out', included: false }
    ],

    // list of files / patterns to exclude
    exclude: [
      'src/text-fragments.js', // so that the polyfill doesn't run in test
    ],

    // preprocess matching files before serving them to the browser
    // available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
    preprocessors: {
      '**/*.html': ['html2js'],
    },

    // test results reporter to use
    // possible values: 'dots', 'progress'
    // available reporters: https://npmjs.org/browse/keyword/karma-reporter
    reporters: ['progress'],

    // web server port
    port: 9876,

    // enable / disable colors in the output (reporters and logs)
    colors: true,

    // level of logging
    // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
    logLevel: config.LOG_INFO,

    // enable / disable watching file and executing tests whenever any file changes
    autoWatch: false,

    // start these browsers
    // TODO: WebkitHeadless would be great to add, but currently is causing issues on CI
    browsers: ['ChromiumHeadlessNoSandbox', 'FirefoxHeadless'],
    customLaunchers: {
      ChromiumHeadlessNoSandbox: {
          base: 'ChromiumHeadless',
          flags: ['--no-sandbox']
      },
      Chrome_with_debugging: {
        base: 'Chrome',
        flags: ['--remote-debugging-port=9222'],
        debug: true
      },
      Firefox_with_debugging: {
        base: 'Firefox',
        flags: ['--remote-debugging-port=9222'],
        debug: true
      }
    },

    client: {
      // We can't run in an iframe because some of the code under test is
      // checking if it's inside an iframe.
      useIframe: false,
      debug: config.debug
    },

    // Continuous Integration mode
    // if true, Karma captures browsers, runs the tests and exits
    singleRun: false,

    // Concurrency level
    // how many browser should be started simultaneous
    concurrency: Infinity,

    html2JsPreprocessor: {
      // strip this from the file path
      stripPrefix: 'test/unit/',
    },
  });
};
