/**
@license
Copyright (c) 2018 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

/**
 * Load ShadyCSS
 */
function loadShadyCSS(nativeShadow) {
  /**
   * Boilerplate for using ShadyCSS in a Node context
   */
  global.window = {
    ShadyDOM: {
      inUse: !nativeShadow
    },
    ShadyCSS: {
      nativeCss: true
    }
  };

  const {StyleTransformer, StyleUtil, ApplyShim, CssParse, ShadyUnscopedAttribute} = require('./bundled-shadycss.js');

  /**
   * Apply Shim uses a dynamically create DOM element
   * to lookup initial property values at runtime.
   *
   * Override with a fixed table for building
   */
  const {initialValues} = require('./initial-values.js');
  ApplyShim.prototype._getInitialValueForProperty = (property) =>
    initialValues[property] || '';

  return {
    StyleTransformer,
    StyleUtil,
    ApplyShim: new ApplyShim(),
    CssParse,
    ShadyUnscopedAttribute
  };
}

module.exports = loadShadyCSS;