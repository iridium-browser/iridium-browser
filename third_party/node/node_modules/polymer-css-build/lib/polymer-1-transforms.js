/**
@license
Copyright (c) 2018 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

function dirShadowTransform(selector) {
  if (!selector.match(/:dir\(/)) {
    return selector;
  }
  return splitSelectorList(selector).map(function(s) {
    s = ensureScopedDir(s);
    s = transformDir(s);
    let m = HOST_CONTEXT_PAREN.exec(s);
    if (m) {
      s += additionalDirSelectors(m[2], m[3], '');
    }
    return s;
  }, this).join(COMPLEX_SELECTOR_SEP);
}

// Split a selector separated by commas into an array in a smart way
function splitSelectorList(selector) {
  let parts = [];
  let part = '';
  for (let i = 0; i >= 0 && i < selector.length; i++) {
    // A selector with parentheses will be one complete part
    if (selector[i] === '(') {
      // find the matching paren
      let end = findMatchingParen(selector, i);
      // push the paren block into the part
      part += selector.slice(i, end + 1);
      // move the index to after the paren block
      i = end;
    } else if (selector[i] === COMPLEX_SELECTOR_SEP) {
      parts.push(part);
      part = '';
    } else {
      part += selector[i];
    }
  }
  // catch any pieces after the last comma
  if (part) {
    parts.push(part);
  }
  // if there were no commas, just push the whole selector as a "part"
  if (parts.length === 0) {
    parts.push(selector);
  }
  return parts;
}

// make sure `:dir() {` acts as `*:dir() {`
// otherwise, it would transform to `:host-context([dir="rtl"])` and apply incorrectly to the host
// so make it `:host-context([dir="rtl"]) *`
function ensureScopedDir(s) {
  let m = s.match(DIR_PAREN);
  if (m && m[1] === '' && m[0].length === s.length) {
    s = '*' + s;
  }
  return s;
}

// given a match from HOST_SCOPE_PAREN regex, add selectors for more :dir cases
// nested: [dir] is set on an ancestor node that is inside the host element
function additionalDirSelectors(dir, after, prefix) {
  if (!dir || !after) {
    return '';
  }
  prefix = prefix || '';
  return `${COMPLEX_SELECTOR_SEP}${prefix} ${dir} ${after}`;
}

function transformDir(s) {
  // replaces :host(:dir(rtl)) with :host-context([dir="rtl"])
  s = s.replace(HOST_DIR, HOST_DIR_REPLACE);
  // replaces `.foo :dir(rtl)` with `:host-context([dir="rtl") .foo`
  s = s.replace(DIR_PAREN, DIR_REPLACE);
  return s;
}

/**
 * Walk from text[start] matching parens and
 * returns position of the outer end paren
 * @param {string} text
 * @param {number} start
 * @return {number}
 */
function findMatchingParen(text, start) {
  let level = 0;
  for (let i=start, l=text.length; i < l; i++) {
    if (text[i] === '(') {
      level++;
    } else if (text[i] === ')') {
      if (--level === 0) {
        return i;
      }
    }
  }
  return -1;
}

// For forward compatibility with ShadowDOM v1 and Polymer 2.x,
// replace ::slotted(${inner}) with ::content > ${inner}
function slottedToContent(cssText) {
  return cssText.replace(SLOTTED_PAREN, `${CONTENT} > $1`);
}

function shadyReplaceContent(selector, scope) {
  if (selector.indexOf(CONTENT) === -1) {
    return selector;
  }
  const contentForm = `.${scope}::content`;
  const replaceContent = new RegExp(`[+~>]? ${contentForm}`);
  const end = new RegExp('.' + scope + '$');
  const beforePseudo = new RegExp('.' + scope + ':');
  return splitSelectorList(selector).map((sel) => {
    if (sel.indexOf(contentForm) > -1) {
      return sel.replace(replaceContent, '')
          .replace(beforePseudo, ':')
          .replace(end, '');
    } else {
      return sel;
    }
  }).join(',');
}

function fixBadVars(rule) {
  const cssText = rule.cssText;
  // fix shim variables
  rule.cssText = cssText.replace(BAD_VAR, fixVars);
}

// fix shim'd var syntax
// var(--a, --b) -> var(--a,var(--b))
function fixVars(matchText, varA, varB) { // eslint-disable-line no-unused-vars
  // if fallback doesn't exist, or isn't a broken variable, abort
  return `var(${varA},var(${varB}))`;
}

const COMPLEX_SELECTOR_SEP = ',';
const CONTENT = '::content';
const DIR_PAREN = /(.*):dir\((ltr|rtl)\)/;
const DIR_REPLACE = ':host-context([dir="$2"]) $1';
const HOST_CONTEXT_PAREN = /(.*)(?::host-context)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))(.*)/;
const HOST_DIR = /:host\(:dir\((rtl|ltr)\)\)/g;
const HOST_DIR_REPLACE = ':host-context([dir="$1"])';
const SLOTTED_PAREN = /(?:::slotted)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))/g;
// match var(--a, --b) to make var(--a, var(--b));
const BAD_VAR = /var\(\s*(--[^,]*),\s*(--[^)]*)\)/g;

module.exports = {
  dirShadowTransform,
  slottedToContent,
  shadyReplaceContent,
  fixBadVars
};
