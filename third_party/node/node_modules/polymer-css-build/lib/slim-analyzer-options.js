/**
@license
Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/
const {HtmlCustomElementReferenceScanner} = require('polymer-analyzer/lib/html/html-element-reference-scanner.js');
const {HtmlImportScanner} = require('polymer-analyzer/lib/html/html-import-scanner.js');
const {HtmlParser} = require('polymer-analyzer/lib/html/html-parser.js');
const {HtmlScriptScanner} = require('polymer-analyzer/lib/html/html-script-scanner.js');
const {ClassScanner} = require('polymer-analyzer/lib/javascript/class-scanner.js');
const {FunctionScanner} = require('polymer-analyzer/lib/javascript/function-scanner.js');
const {InlineHtmlDocumentScanner} = require('polymer-analyzer/lib/javascript/html-template-literal-scanner.js');
const {JavaScriptImportScanner} = require('polymer-analyzer/lib/javascript/javascript-import-scanner.js');
const {JavaScriptParser} = require('polymer-analyzer/lib/javascript/javascript-parser.js');
const {DomModuleScanner} = require('polymer-analyzer/lib/polymer/dom-module-scanner.js');
// const {PolymerElementScanner} = require('polymer-analyzer/lib/polymer/polymer-element-scanner.js');

const {InnerHtmlAssignmentScanner} = require('./innerhtml-assignment-scanner.js');
const {ClosureHtmlTemplateScanner} = require('./closure-html-template-scanner.js');
const {ClosurePolymerElementScanner} = require('./closure-polymer-element-scanner.js');

exports.createScannerMap = function createScannerMap(scanInlineTemplates = true) {
  const map = new Map([
    [
      'html',
      [
        new HtmlImportScanner(),
        new HtmlScriptScanner(),
        new DomModuleScanner(),
        new HtmlCustomElementReferenceScanner(),
      ]
    ],
    [
      'js',
      [
        // new PolymerElementScanner(),
        new JavaScriptImportScanner(),
        new ClosurePolymerElementScanner()
      ]
    ]
  ]);
  if (scanInlineTemplates) {
    const jsScanners = map.get('js');
    jsScanners.push(
        new FunctionScanner(),
        new ClassScanner(),
        new InlineHtmlDocumentScanner(),
        new InnerHtmlAssignmentScanner(),
        new ClosureHtmlTemplateScanner(),
    );
  }
  return map;
};

exports.createParserMap = function createParserMap() {
  return new Map([
    ['html', new HtmlParser()],
    ['js', new JavaScriptParser()]
  ]);
};
