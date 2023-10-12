# Text Fragments Polyfill

This is a polyfill for the
[Text Fragments](https://wicg.github.io/scroll-to-text-fragment/) feature for
browsers that don't support it directly.

<div align="center">
  <img width="400" src="https://user-images.githubusercontent.com/145676/79250513-02bb5800-7e7f-11ea-8e56-bd63edd31f5b.jpeg">
  <p>
    <sup>
      The text fragment link
      <a href="https://en.wikipedia.org/wiki/Cat#Size:~:text=This%20article%20is,kept%20as%20a%20pet">
        #:~:text=This%20article%20is,kept%20as%20a%20pet
      </a>
      rendered in Safari on an iOS device, with
      <a href="https://github.com/GoogleChromeLabs/text-fragments-polyfill/blob/main/src/text-fragments.js">
        <code>text-fragments.js</code>
      </a>
      injected via Web Inspector.
    </sup>
  </p>
</div>

The implementation broadly follows the text-finding algorithms, with a few small deviations
and reorganizations, and should perform similarly to renderers which have implemented this
natively. It is used in Chromium for iOS as well as the
[Link to Text Fragment Browser Extension](https://github.com/GoogleChromeLabs/link-to-text-fragment).

The `src` directory contains three files:

* `text-fragments.js`, containing the polyfilling mechanism.

* `text-fragment-utils.js`, a module of util functions related to parsing, finding, and highlighting text fragments within the DOM. Most of the logic used by the polyfill for finding fragments in a page lives here.

* `fragment-generation-utils.js`, a module of util functions for generating URLs with a text fragment. These utils are not used by the polyfill itself, but they are likely to be useful for related projects.

The `tools` directory contains a util script used for generating a regex used in the utils
module.

The `test` directory contains unit and data-driven tests, based on Karma and Jasmine, as well as HTML files
which can be loaded during those tests. Unit tests go in the `unit` subfolder and data-driven tests go in
the `data-driven` subfolder.

## Installation

From npm:

```bash
npm install text-fragments-polyfill
```

From unpkg:

```html
<script type="module">
  if (!('fragmentDirective' in Location.prototype) &&
      !('fragmentDirective' in document)) {
    import('https://unpkg.com/text-fragments-polyfill');
  }
</script>
```

## Usage

For simple usage as a polyfill, it is sufficient to import the `text-fragments.js` file:

```js
// Only load the polyfill in browsers that need it.
if (
  !('fragmentDirective' in Location.prototype) &&
  !('fragmentDirective' in document)
) {
  import('text-fragments.js');
}
```

Users who wish to take a more hands-on approach can reuse chunks of the logic by importing the `text-fragment-utils.js` and `fragment-generation-utils.js` modules; support is provided for inclusion either as an ES6 module or using the Closure compiler.

## Demo

Try the [demo](https://text-fragments-polyfill.glitch.me/) on a browser that
does not support Text Fragments.

## Development

1. Hack in `/src`.
1. Run `npm run start` and open
   [http://localhost:8080/demo/](http://localhost:8080/demo/`) in a browser that
   does not support Text Fragments URLs directly, for example, Safari.
1. Hack, reload, hack,…
1. You can modify the Text Fragments URLs directly in
   [`/demo/index.html`](https://github.com/GoogleChromeLabs/text-fragments-polyfill/blob/main/demo/index.html)
   (look for `location.hash`).

## Data-Driven Tests
Data-Driven tests are round trip tests checking both fragment generation and highlighting.
Each test case is defined by an html document and a selection range inside the document.
The tests try to generate a text fragment targeting the selection range in the document
and use it for highlighting, comparing the highlighted text against the expected highlighted text.

To add more data-driven tests, put your files in the following subfolders of `test/data-driven`:
  - `input/test-params`: Json file with test case definition. See `TextFragmentDataDrivenTestInput` in `text-fragments-data-driven-test.js` for a detailed schema description.
  - `input/html`: Html file referenced in the test case's input file.
  - `output`: Plain text file with the test case's expected results. Must have extension `.out` and the same file name
  as the test case's input file.
  See the documentation of `TextFragmentsDataDrivenTest` in `text-fragments-data-driven-test.js` for more details.

The example test case `basic-test` can be used for reference. Files: `input/test-params/basic-test.json`,
`input/html/basic-test.html` and `output/basic-test.out`.

## Debugging tests
1. Run `npm run debug <debug browser>` with `debug browser = Firefox_with_debugging | Chrome_with_debugging`
2. Go to the browser window and click on "Debug" to run the tests
3. Add breakpoints in the browser's developer tool debugger
4. Refresh the page so the tests run again and your breakpoints are hit

## License

Apache 2.0. This is not an official Google product.
