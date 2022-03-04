'use strict';

Object.defineProperty(exports, '__esModule', { value: true });

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

const nativeShadow = !(window['ShadyDOM'] && window['ShadyDOM']['inUse']);
let nativeCssVariables_;

/**
 * @param {(ShadyCSSOptions | ShadyCSSInterface)=} settings
 */
function calcCssVariables(settings) {
  if (settings && settings['shimcssproperties']) {
    nativeCssVariables_ = false;
  } else {
    // chrome 49 has semi-working css vars, check if box-shadow works
    // safari 9.1 has a recalc bug: https://bugs.webkit.org/show_bug.cgi?id=155782
    // However, shim css custom properties are only supported with ShadyDOM enabled,
    // so fall back on native if we do not detect ShadyDOM
    // Edge 15: custom properties used in ::before and ::after will also be used in the parent element
    // https://developer.microsoft.com/en-us/microsoft-edge/platform/issues/12414257/
    nativeCssVariables_ = nativeShadow || Boolean(!navigator.userAgent.match(/AppleWebKit\/601|Edge\/15/) &&
      window.CSS && CSS.supports && CSS.supports('box-shadow', '0 0 0 var(--foo)'));
  }
}

/** @type {string | undefined} */
let cssBuild;
if (window.ShadyCSS && window.ShadyCSS.cssBuild !== undefined) {
  cssBuild = window.ShadyCSS.cssBuild;
}

/** @type {boolean} */
const disableRuntime = Boolean(window.ShadyCSS && window.ShadyCSS.disableRuntime);

if (window.ShadyCSS && window.ShadyCSS.nativeCss !== undefined) {
  nativeCssVariables_ = window.ShadyCSS.nativeCss;
} else if (window.ShadyCSS) {
  calcCssVariables(window.ShadyCSS);
  // reset window variable to let ShadyCSS API take its place
  window.ShadyCSS = undefined;
} else {
  calcCssVariables(window['WebComponents'] && window['WebComponents']['flags']);
}

// Hack for type error under new type inference which doesn't like that
// nativeCssVariables is updated in a function and assigns the type
// `function(): ?` instead of `boolean`.
const nativeCssVariables = /** @type {boolean} */(nativeCssVariables_);

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

/** @unrestricted */
class StyleNode {
  constructor() {
    /** @type {number} */
    this['start'] = 0;
    /** @type {number} */
    this['end'] = 0;
    /** @type {StyleNode} */
    this['previous'] = null;
    /** @type {StyleNode} */
    this['parent'] = null;
    /** @type {Array<StyleNode>} */
    this['rules'] = null;
    /** @type {string} */
    this['parsedCssText'] = '';
    /** @type {string} */
    this['cssText'] = '';
    /** @type {boolean} */
    this['atRule'] = false;
    /** @type {number} */
    this['type'] = 0;
    /** @type {string} */
    this['keyframesName'] = '';
    /** @type {string} */
    this['selector'] = '';
    /** @type {string} */
    this['parsedSelector'] = '';
  }
}

// given a string of css, return a simple rule tree
/**
 * @param {string} text
 * @return {StyleNode}
 */
function parse(text) {
  text = clean(text);
  return parseCss(lex(text), text);
}

// remove stuff we don't care about that may hinder parsing
/**
 * @param {string} cssText
 * @return {string}
 */
function clean(cssText) {
  return cssText.replace(RX.comments, '').replace(RX.port, '');
}

// super simple {...} lexer that returns a node tree
/**
 * @param {string} text
 * @return {StyleNode}
 */
function lex(text) {
  let root = new StyleNode();
  root['start'] = 0;
  root['end'] = text.length;
  let n = root;
  for (let i = 0, l = text.length; i < l; i++) {
    if (text[i] === OPEN_BRACE) {
      if (!n['rules']) {
        n['rules'] = [];
      }
      let p = n;
      let previous = p['rules'][p['rules'].length - 1] || null;
      n = new StyleNode();
      n['start'] = i + 1;
      n['parent'] = p;
      n['previous'] = previous;
      p['rules'].push(n);
    } else if (text[i] === CLOSE_BRACE) {
      n['end'] = i + 1;
      n = n['parent'] || root;
    }
  }
  return root;
}

// add selectors/cssText to node tree
/**
 * @param {StyleNode} node
 * @param {string} text
 * @return {StyleNode}
 */
function parseCss(node, text) {
  let t = text.substring(node['start'], node['end'] - 1);
  node['parsedCssText'] = node['cssText'] = t.trim();
  if (node['parent']) {
    let ss = node['previous'] ? node['previous']['end'] : node['parent']['start'];
    t = text.substring(ss, node['start'] - 1);
    t = _expandUnicodeEscapes(t);
    t = t.replace(RX.multipleSpaces, ' ');
    // TODO(sorvell): ad hoc; make selector include only after last ;
    // helps with mixin syntax
    t = t.substring(t.lastIndexOf(';') + 1);
    let s = node['parsedSelector'] = node['selector'] = t.trim();
    node['atRule'] = (s.indexOf(AT_START) === 0);
    // note, support a subset of rule types...
    if (node['atRule']) {
      if (s.indexOf(MEDIA_START) === 0) {
        node['type'] = types.MEDIA_RULE;
      } else if (s.match(RX.keyframesRule)) {
        node['type'] = types.KEYFRAMES_RULE;
        node['keyframesName'] =
          node['selector'].split(RX.multipleSpaces).pop();
      }
    } else {
      if (s.indexOf(VAR_START) === 0) {
        node['type'] = types.MIXIN_RULE;
      } else {
        node['type'] = types.STYLE_RULE;
      }
    }
  }
  let r$ = node['rules'];
  if (r$) {
    for (let i = 0, l = r$.length, r;
      (i < l) && (r = r$[i]); i++) {
      parseCss(r, text);
    }
  }
  return node;
}

/**
 * conversion of sort unicode escapes with spaces like `\33 ` (and longer) into
 * expanded form that doesn't require trailing space `\000033`
 * @param {string} s
 * @return {string}
 */
function _expandUnicodeEscapes(s) {
  return s.replace(/\\([0-9a-f]{1,6})\s/gi, function() {
    let code = arguments[1],
      repeat = 6 - code.length;
    while (repeat--) {
      code = '0' + code;
    }
    return '\\' + code;
  });
}

/**
 * stringify parsed css.
 * @param {StyleNode} node
 * @param {boolean=} preserveProperties
 * @param {string=} text
 * @return {string}
 */
function stringify(node, preserveProperties, text = '') {
  // calc rule cssText
  let cssText = '';
  if (node['cssText'] || node['rules']) {
    let r$ = node['rules'];
    if (r$ && !_hasMixinRules(r$)) {
      for (let i = 0, l = r$.length, r;
        (i < l) && (r = r$[i]); i++) {
        cssText = stringify(r, preserveProperties, cssText);
      }
    } else {
      cssText = preserveProperties ? node['cssText'] :
        removeCustomProps(node['cssText']);
      cssText = cssText.trim();
      if (cssText) {
        cssText = '  ' + cssText + '\n';
      }
    }
  }
  // emit rule if there is cssText
  if (cssText) {
    if (node['selector']) {
      text += node['selector'] + ' ' + OPEN_BRACE + '\n';
    }
    text += cssText;
    if (node['selector']) {
      text += CLOSE_BRACE + '\n\n';
    }
  }
  return text;
}

/**
 * @param {Array<StyleNode>} rules
 * @return {boolean}
 */
function _hasMixinRules(rules) {
  let r = rules[0];
  return Boolean(r) && Boolean(r['selector']) && r['selector'].indexOf(VAR_START) === 0;
}

/**
 * @param {string} cssText
 * @return {string}
 */
function removeCustomProps(cssText) {
  cssText = removeCustomPropAssignment(cssText);
  return removeCustomPropApply(cssText);
}

/**
 * @param {string} cssText
 * @return {string}
 */
function removeCustomPropAssignment(cssText) {
  return cssText
    .replace(RX.customProp, '')
    .replace(RX.mixinProp, '');
}

/**
 * @param {string} cssText
 * @return {string}
 */
function removeCustomPropApply(cssText) {
  return cssText
    .replace(RX.mixinApply, '')
    .replace(RX.varApply, '');
}

/** @enum {number} */
const types = {
  STYLE_RULE: 1,
  KEYFRAMES_RULE: 7,
  MEDIA_RULE: 4,
  MIXIN_RULE: 1000
};

const OPEN_BRACE = '{';
const CLOSE_BRACE = '}';

// helper regexp's
const RX = {
  comments: /\/\*[^*]*\*+([^/*][^*]*\*+)*\//gim,
  port: /@import[^;]*;/gim,
  customProp: /(?:^[^;\-\s}]+)?--[^;{}]*?:[^{};]*?(?:[;\n]|$)/gim,
  mixinProp: /(?:^[^;\-\s}]+)?--[^;{}]*?:[^{};]*?{[^}]*?}(?:[;\n]|$)?/gim,
  mixinApply: /@apply\s*\(?[^);]*\)?\s*(?:[;\n]|$)?/gim,
  varApply: /[^;:]*?:[^;]*?var\([^;]*\)(?:[;\n]|$)?/gim,
  keyframesRule: /^@[^\s]*keyframes/,
  multipleSpaces: /\s+/g
};

const VAR_START = '--';
const MEDIA_START = '@media';
const AT_START = '@';

var cssParse = /*#__PURE__*/Object.freeze({
  StyleNode: StyleNode,
  parse: parse,
  stringify: stringify,
  removeCustomPropAssignment: removeCustomPropAssignment,
  types: types
});

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

const VAR_ASSIGN = /(?:^|[;\s{]\s*)(--[\w-]*?)\s*:\s*(?:((?:'(?:\\'|.)*?'|"(?:\\"|.)*?"|\([^)]*?\)|[^};{])+)|\{([^}]*)\}(?:(?=[;\s}])|$))/gi;
const MIXIN_MATCH = /(?:^|\W+)@apply\s*\(?([^);\n]*)\)?/gi;
const MEDIA_MATCH = /@media\s(.*)/;

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

/** @type {!Set<string>} */
const styleTextSet = new Set();

const scopingAttribute = 'shady-unscoped';

/**
 * Add a specifically-marked style to the document directly, and only one copy of that style.
 *
 * @param {!HTMLStyleElement} style
 * @return {undefined}
 */
function processUnscopedStyle(style) {
  const text = style.textContent;
  if (!styleTextSet.has(text)) {
    styleTextSet.add(text);
    const newStyle = style.cloneNode(true);
    document.head.appendChild(newStyle);
  }
}

/**
 * Check if a style is supposed to be unscoped
 * @param {!HTMLStyleElement} style
 * @return {boolean} true if the style has the unscoping attribute
 */
function isUnscopedStyle(style) {
  return style.hasAttribute(scopingAttribute);
}

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

/**
 * @param {string|StyleNode} rules
 * @param {function(StyleNode)=} callback
 * @return {string}
 */
function toCssText (rules, callback) {
  if (!rules) {
    return '';
  }
  if (typeof rules === 'string') {
    rules = parse(rules);
  }
  if (callback) {
    forEachRule(rules, callback);
  }
  return stringify(rules, nativeCssVariables);
}

/**
 * @param {HTMLStyleElement} style
 * @return {StyleNode}
 */
function rulesForStyle(style) {
  if (!style['__cssRules'] && style.textContent) {
    style['__cssRules'] = parse(style.textContent);
  }
  return style['__cssRules'] || null;
}

// Tests if a rule is a keyframes selector, which looks almost exactly
// like a normal selector but is not (it has nothing to do with scoping
// for example).
/**
 * @param {StyleNode} rule
 * @return {boolean}
 */
function isKeyframesSelector(rule) {
  return Boolean(rule['parent']) &&
  rule['parent']['type'] === types.KEYFRAMES_RULE;
}

/**
 * @param {StyleNode} node
 * @param {Function=} styleRuleCallback
 * @param {Function=} keyframesRuleCallback
 * @param {boolean=} onlyActiveRules
 */
function forEachRule(node, styleRuleCallback, keyframesRuleCallback, onlyActiveRules) {
  if (!node) {
    return;
  }
  let skipRules = false;
  let type = node['type'];
  if (onlyActiveRules) {
    if (type === types.MEDIA_RULE) {
      let matchMedia = node['selector'].match(MEDIA_MATCH);
      if (matchMedia) {
        // if rule is a non matching @media rule, skip subrules
        if (!window.matchMedia(matchMedia[1]).matches) {
          skipRules = true;
        }
      }
    }
  }
  if (type === types.STYLE_RULE) {
    styleRuleCallback(node);
  } else if (keyframesRuleCallback &&
    type === types.KEYFRAMES_RULE) {
    keyframesRuleCallback(node);
  } else if (type === types.MIXIN_RULE) {
    skipRules = true;
  }
  let r$ = node['rules'];
  if (r$ && !skipRules) {
    for (let i=0, l=r$.length, r; (i<l) && (r=r$[i]); i++) {
      forEachRule(r, styleRuleCallback, keyframesRuleCallback, onlyActiveRules);
    }
  }
}

// add a string of cssText to the document.
/**
 * @param {string} cssText
 * @param {string} moniker
 * @param {Node} target
 * @param {Node} contextNode
 * @return {HTMLStyleElement}
 */
function applyCss(cssText, moniker, target, contextNode) {
  let style = createScopeStyle(cssText, moniker);
  applyStyle(style, target, contextNode);
  return style;
}

/**
 * @param {string} cssText
 * @param {string} moniker
 * @return {HTMLStyleElement}
 */
function createScopeStyle(cssText, moniker) {
  let style = /** @type {HTMLStyleElement} */(document.createElement('style'));
  if (moniker) {
    style.setAttribute('scope', moniker);
  }
  style.textContent = cssText;
  return style;
}

/**
 * Track the position of the last added style for placing placeholders
 * @type {Node}
 */
let lastHeadApplyNode = null;

// insert a comment node as a styling position placeholder.
/**
 * @param {string} moniker
 * @return {!Comment}
 */
function applyStylePlaceHolder(moniker) {
  let placeHolder = document.createComment(' Shady DOM styles for ' +
    moniker + ' ');
  let after = lastHeadApplyNode ?
    lastHeadApplyNode['nextSibling'] : null;
  let scope = document.head;
  scope.insertBefore(placeHolder, after || scope.firstChild);
  lastHeadApplyNode = placeHolder;
  return placeHolder;
}

/**
 * @param {HTMLStyleElement} style
 * @param {?Node} target
 * @param {?Node} contextNode
 */
function applyStyle(style, target, contextNode) {
  target = target || document.head;
  let after = (contextNode && contextNode.nextSibling) ||
    target.firstChild;
  target.insertBefore(style, after);
  if (!lastHeadApplyNode) {
    lastHeadApplyNode = style;
  } else {
    // only update lastHeadApplyNode if the new style is inserted after the old lastHeadApplyNode
    let position = style.compareDocumentPosition(lastHeadApplyNode);
    if (position === Node.DOCUMENT_POSITION_PRECEDING) {
      lastHeadApplyNode = style;
    }
  }
}

/**
 * @param {string} buildType
 * @return {boolean}
 */
function isTargetedBuild(buildType) {
  return nativeShadow ? buildType === 'shadow' : buildType === 'shady';
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

/**
 * @param {string} str
 * @param {function(string, string, string, string)} callback
 */
function processVariableAndFallback(str, callback) {
  // find 'var('
  let start = str.indexOf('var(');
  if (start === -1) {
    // no var?, everything is prefix
    return callback(str, '', '', '');
  }
  //${prefix}var(${inner})${suffix}
  let end = findMatchingParen(str, start + 3);
  let inner = str.substring(start + 4, end);
  let prefix = str.substring(0, start);
  // suffix may have other variables
  let suffix = processVariableAndFallback(str.substring(end + 1), callback);
  let comma = inner.indexOf(',');
  // value and fallback args should be trimmed to match in property lookup
  if (comma === -1) {
    // variable, no fallback
    return callback(prefix, inner.trim(), '', suffix);
  }
  // var(${value},${fallback})
  let value = inner.substring(0, comma).trim();
  let fallback = inner.substring(comma + 1).trim();
  return callback(prefix, value, fallback, suffix);
}

/**
 * @param {Element} element
 * @param {string} value
 */
function setElementClassRaw(element, value) {
  // use native setAttribute provided by ShadyDOM when setAttribute is patched
  if (nativeShadow) {
    element.setAttribute('class', value);
  } else {
    window['ShadyDOM']['nativeMethods']['setAttribute'].call(element, 'class', value);
  }
}

/**
 * @type {function(*):*}
 */
const wrap = window['ShadyDOM'] && window['ShadyDOM']['wrap'] || ((node) => node);

/**
 * @param {Element | {is: string, extends: string}} element
 * @return {{is: string, typeExtension: string}}
 */
function getIsExtends(element) {
  let localName = element['localName'];
  let is = '', typeExtension = '';
  /*
  NOTE: technically, this can be wrong for certain svg elements
  with `-` in the name like `<font-face>`
  */
  if (localName) {
    if (localName.indexOf('-') > -1) {
      is = localName;
    } else {
      typeExtension = localName;
      is = (element.getAttribute && element.getAttribute('is')) || '';
    }
  } else {
    is = /** @type {?} */(element).is;
    typeExtension = /** @type {?} */(element).extends;
  }
  return {is, typeExtension};
}

/**
 * @param {Element|DocumentFragment} element
 * @return {string}
 */
function gatherStyleText(element) {
  /** @type {!Array<string>} */
  const styleTextParts = [];
  const styles = /** @type {!NodeList<!HTMLStyleElement>} */(element.querySelectorAll('style'));
  for (let i = 0; i < styles.length; i++) {
    const style = styles[i];
    if (isUnscopedStyle(style)) {
      if (!nativeShadow) {
        processUnscopedStyle(style);
        style.parentNode.removeChild(style);
      }
    } else {
      styleTextParts.push(style.textContent);
      style.parentNode.removeChild(style);
    }
  }
  return styleTextParts.join('').trim();
}

/**
 * Split a selector separated by commas into an array in a smart way
 * @param {string} selector
 * @return {!Array<string>}
 */
function splitSelectorList(selector) {
  const parts = [];
  let part = '';
  for (let i = 0; i >= 0 && i < selector.length; i++) {
    // A selector with parentheses will be one complete part
    if (selector[i] === '(') {
      // find the matching paren
      const end = findMatchingParen(selector, i);
      // push the paren block into the part
      part += selector.slice(i, end + 1);
      // move the index to after the paren block
      i = end;
    } else if (selector[i] === ',') {
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
  return parts;
}

const CSS_BUILD_ATTR = 'css-build';

/**
 * Return the polymer-css-build "build type" applied to this element
 *
 * @param {!HTMLElement} element
 * @return {string} Can be "", "shady", or "shadow"
 */
function getCssBuild(element) {
  if (cssBuild !== undefined) {
    return /** @type {string} */(cssBuild);
  }
  if (element.__cssBuild === undefined) {
    // try attribute first, as it is the common case
    const attrValue = element.getAttribute(CSS_BUILD_ATTR);
    if (attrValue) {
      element.__cssBuild = attrValue;
    } else {
      const buildComment = getBuildComment(element);
      if (buildComment !== '') {
        // remove build comment so it is not needlessly copied into every element instance
        removeBuildComment(element);
      }
      element.__cssBuild = buildComment;
    }
  }
  return element.__cssBuild || '';
}

/**
 * Check if the given element, either a <template> or <style>, has been processed
 * by polymer-css-build.
 *
 * If so, then we can make a number of optimizations:
 * - polymer-css-build will decompose mixins into individual CSS Custom Properties,
 * so the ApplyShim can be skipped entirely.
 * - Under native ShadowDOM, the style text can just be copied into each instance
 * without modification
 * - If the build is "shady" and ShadyDOM is in use, the styling does not need
 * scoping beyond the shimming of CSS Custom Properties
 *
 * @param {!HTMLElement} element
 * @return {boolean}
 */
function elementHasBuiltCss(element) {
  return getCssBuild(element) !== '';
}

/**
 * For templates made with tagged template literals, polymer-css-build will
 * insert a comment of the form `<!--css-build:shadow-->`
 *
 * @param {!HTMLElement} element
 * @return {string}
 */
function getBuildComment(element) {
  const buildComment = element.localName === 'template' ?
      /** @type {!HTMLTemplateElement} */ (element).content.firstChild :
      element.firstChild;
  if (buildComment instanceof Comment) {
    const commentParts = buildComment.textContent.trim().split(':');
    if (commentParts[0] === CSS_BUILD_ATTR) {
      return commentParts[1];
    }
  }
  return '';
}

/**
 * Check if the css build status is optimal, and do no unneeded work.
 *
 * @param {string=} cssBuild CSS build status
 * @return {boolean} css build is optimal or not
 */
function isOptimalCssBuild(cssBuild$$1 = '') {
  // CSS custom property shim always requires work
  if (cssBuild$$1 === '' || !nativeCssVariables) {
    return false;
  }
  return nativeShadow ? cssBuild$$1 === 'shadow' : cssBuild$$1 === 'shady';
}

/**
 * @param {!HTMLElement} element
 */
function removeBuildComment(element) {
  const buildComment = element.localName === 'template' ?
      /** @type {!HTMLTemplateElement} */ (element).content.firstChild :
      element.firstChild;
  buildComment.parentNode.removeChild(buildComment);
}

var styleUtil = /*#__PURE__*/Object.freeze({
  toCssText: toCssText,
  rulesForStyle: rulesForStyle,
  isKeyframesSelector: isKeyframesSelector,
  forEachRule: forEachRule,
  applyCss: applyCss,
  createScopeStyle: createScopeStyle,
  applyStylePlaceHolder: applyStylePlaceHolder,
  applyStyle: applyStyle,
  isTargetedBuild: isTargetedBuild,
  findMatchingParen: findMatchingParen,
  processVariableAndFallback: processVariableAndFallback,
  setElementClassRaw: setElementClassRaw,
  wrap: wrap,
  getIsExtends: getIsExtends,
  gatherStyleText: gatherStyleText,
  splitSelectorList: splitSelectorList,
  getCssBuild: getCssBuild,
  elementHasBuiltCss: elementHasBuiltCss,
  getBuildComment: getBuildComment,
  isOptimalCssBuild: isOptimalCssBuild
});

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

/* Transforms ShadowDOM styling into ShadyDOM styling

* scoping:

  * elements in scope get scoping selector class="x-foo-scope"
  * selectors re-written as follows:

    div button -> div.x-foo-scope button.x-foo-scope

* :host -> scopeName

* :host(...) -> scopeName...

* ::slotted(...) -> scopeName > ...

* ...:dir(ltr|rtl) -> [dir="ltr|rtl"] ..., ...[dir="ltr|rtl"]

* :host(:dir[rtl]) -> scopeName:dir(rtl) -> [dir="rtl"] scopeName, scopeName[dir="rtl"]

*/
const SCOPE_NAME = 'style-scope';

class StyleTransformer {
  get SCOPE_NAME() {
    return SCOPE_NAME;
  }
  /**
   * Given a node and scope name, add a scoping class to each node
   * in the tree. This facilitates transforming css into scoped rules.
   * @param {!Node} node
   * @param {string} scope
   * @param {boolean=} shouldRemoveScope
   * @deprecated
   */
  dom(node, scope, shouldRemoveScope) {
    const fn = (node) => {
      this.element(node, scope || '', shouldRemoveScope);
    };
    this._transformDom(node, fn);
  }

  /**
   * Given a node and scope name, add a scoping class to each node in the tree.
   * @param {!Node} node
   * @param {string} scope
   */
  domAddScope(node, scope) {
    const fn = (node) => {
      this.element(node, scope || '');
    };
    this._transformDom(node, fn);
  }

  /**
   * @param {!Node} startNode
   * @param {!function(!Node)} transformer
   */
  _transformDom(startNode, transformer) {
    if (startNode.nodeType === Node.ELEMENT_NODE) {
      transformer(startNode);
    }
    let c$;
    if (startNode.localName === 'template') {
      const template = /** @type {!HTMLTemplateElement} */ (startNode);
      // In case the template is in svg context, fall back to the node
      // since it won't be an HTMLTemplateElement with a .content property
      c$ = (template.content || template._content || template).childNodes;
    } else {
      c$ = /** @type {!ParentNode} */ (startNode).children ||
          startNode.childNodes;
    }
    if (c$) {
      for (let i = 0; i < c$.length; i++) {
        this._transformDom(c$[i], transformer);
      }
    }
  }

  /**
   * @param {?} element
   * @param {?} scope
   * @param {?=} shouldRemoveScope
   */
  element(element, scope, shouldRemoveScope) {
    // note: if using classes, we add both the general 'style-scope' class
    // as well as the specific scope. This enables easy filtering of all
    // `style-scope` elements
    if (scope) {
      // note: svg on IE does not have classList so fallback to class
      if (element.classList) {
        if (shouldRemoveScope) {
          element.classList.remove(SCOPE_NAME);
          element.classList.remove(scope);
        } else {
          element.classList.add(SCOPE_NAME);
          element.classList.add(scope);
        }
      } else if (element.getAttribute) {
        let c = element.getAttribute(CLASS);
        if (shouldRemoveScope) {
          if (c) {
            let newValue = c.replace(SCOPE_NAME, '').replace(scope, '');
            setElementClassRaw(element, newValue);
          }
        } else {
          let newValue = (c ? c + ' ' : '') + SCOPE_NAME + ' ' + scope;
          setElementClassRaw(element, newValue);
        }
      }
    }
  }

  /**
   * Given a node, replace the scoping class to each subnode in the tree.
   * @param {!Node} node
   * @param {string} oldScope
   * @param {string} newScope
   */
  domReplaceScope(node, oldScope, newScope) {
    const fn = (node) => {
      this.element(node, oldScope, true);
      this.element(node, newScope);
    };
    this._transformDom(node, fn);
  }
  /**
   * Given a node, remove the scoping class to each subnode in the tree.
   * @param {!Node} node
   * @param {string} oldScope
   */
  domRemoveScope(node, oldScope) {
    const fn = (node) => {
      this.element(node, oldScope || '', true);
    };
    this._transformDom(node, fn);
  }

  /**
   * @param {?} element
   * @param {?} styleRules
   * @param {?=} callback
   * @param {string=} cssBuild
   * @param {string=} cssText
   * @return {string}
   */
  elementStyles(element, styleRules, callback, cssBuild$$1 = '', cssText = '') {
    // no need to shim selectors if settings.useNativeShadow, also
    // a shady css build will already have transformed selectors
    // NOTE: This method may be called as part of static or property shimming.
    // When there is a targeted build it will not be called for static shimming,
    // but when the property shim is used it is called and should opt out of
    // static shimming work when a proper build exists.
    if (cssText === '') {
      if (nativeShadow || cssBuild$$1 === 'shady') {
        cssText = toCssText(styleRules, callback);
      } else {
        let {is, typeExtension} = getIsExtends(element);
        cssText = this.css(styleRules, is, typeExtension, callback) + '\n\n';
      }
    }
    return cssText.trim();
  }

  // Given a string of cssText and a scoping string (scope), returns
  // a string of scoped css where each selector is transformed to include
  // a class created from the scope. ShadowDOM selectors are also transformed
  // (e.g. :host) to use the scoping selector.
  css(rules, scope, ext, callback) {
    let hostScope = this._calcHostScope(scope, ext);
    scope = this._calcElementScope(scope);
    let self = this;
    return toCssText(rules, function(/** StyleNode */rule) {
      if (!rule.isScoped) {
        self.rule(rule, scope, hostScope);
        rule.isScoped = true;
      }
      if (callback) {
        callback(rule, scope, hostScope);
      }
    });
  }

  _calcElementScope(scope) {
    if (scope) {
      return CSS_CLASS_PREFIX + scope;
    } else {
      return '';
    }
  }

  _calcHostScope(scope, ext) {
    return ext ? `[is=${scope}]` : scope;
  }

  rule(rule, scope, hostScope) {
    this._transformRule(rule, this._transformComplexSelector,
      scope, hostScope);
  }

  /**
   * transforms a css rule to a scoped rule.
   *
   * @param {StyleNode} rule
   * @param {Function} transformer
   * @param {string=} scope
   * @param {string=} hostScope
   */
  _transformRule(rule, transformer, scope, hostScope) {
    // NOTE: save transformedSelector for subsequent matching of elements
    // against selectors (e.g. when calculating style properties)
    rule['selector'] = rule.transformedSelector =
      this._transformRuleCss(rule, transformer, scope, hostScope);
  }

  /**
   * @param {StyleNode} rule
   * @param {Function} transformer
   * @param {string=} scope
   * @param {string=} hostScope
   */
  _transformRuleCss(rule, transformer, scope, hostScope) {
    let p$ = splitSelectorList(rule['selector']);
    // we want to skip transformation of rules that appear in keyframes,
    // because they are keyframe selectors, not element selectors.
    if (!isKeyframesSelector(rule)) {
      for (let i=0, l=p$.length, p; (i<l) && (p=p$[i]); i++) {
        p$[i] = transformer.call(this, p, scope, hostScope);
      }
    }
    return p$.filter((part) => Boolean(part)).join(COMPLEX_SELECTOR_SEP);
  }

  /**
   * @param {string} selector
   * @return {string}
   */
  _twiddleNthPlus(selector) {
    return selector.replace(NTH, (m, type, inside) => {
      if (inside.indexOf('+') > -1) {
        inside = inside.replace(/\+/g, '___');
      } else if (inside.indexOf('___') > -1) {
        inside = inside.replace(/___/g, '+');
      }
      return `:${type}(${inside})`;
    });
  }

  /**
   * Preserve `:matches()` selectors by replacing them with MATCHES_REPLACMENT
   * and returning an array of `:matches()` selectors.
   * Use `_replacesMatchesPseudo` to replace the `:matches()` parts
   *
   * @param {string} selector
   * @return {{selector: string, matches: !Array<string>}}
   */
  _preserveMatchesPseudo(selector) {
    /** @type {!Array<string>} */
    const matches = [];
    let match;
    while ((match = selector.match(MATCHES))) {
      const start = match.index;
      const end = findMatchingParen(selector, start);
      if (end === -1) {
        throw new Error(`${match.input} selector missing ')'`)
      }
      const part = selector.slice(start, end + 1);
      selector = selector.replace(part, MATCHES_REPLACEMENT);
      matches.push(part);
    }
    return {selector, matches};
  }

  /**
   * Replace MATCHES_REPLACMENT character with the given set of `:matches()`
   * selectors.
   *
   * @param {string} selector
   * @param {!Array<string>} matches
   * @return {string}
   */
  _replaceMatchesPseudo(selector, matches) {
    const parts = selector.split(MATCHES_REPLACEMENT);
    return matches.reduce((acc, cur, idx) => acc + cur + parts[idx + 1], parts[0]);
  }

/**
 * @param {string} selector
 * @param {string} scope
 * @param {string=} hostScope
 */
  _transformComplexSelector(selector, scope, hostScope) {
    let stop = false;
    selector = selector.trim();
    // Remove spaces inside of selectors like `:nth-of-type` because it confuses SIMPLE_SELECTOR_SEP
    let isNth = NTH.test(selector);
    if (isNth) {
      selector = selector.replace(NTH, (m, type, inner) => `:${type}(${inner.replace(/\s/g, '')})`);
      selector = this._twiddleNthPlus(selector);
    }
    // Preserve selectors like `:-webkit-any` so that SIMPLE_SELECTOR_SEP does
    // not get confused by spaces inside the pseudo selector
    const isMatches = MATCHES.test(selector);
    /** @type {!Array<string>} */
    let matches;
    if (isMatches) {
      ({selector, matches} = this._preserveMatchesPseudo(selector));
    }
    selector = selector.replace(SLOTTED_START, `${HOST} $1`);
    selector = selector.replace(SIMPLE_SELECTOR_SEP, (m, c, s) => {
      if (!stop) {
        let info = this._transformCompoundSelector(s, c, scope, hostScope);
        stop = stop || info.stop;
        c = info.combinator;
        s = info.value;
      }
      return c + s;
    });
    // replace `:matches()` selectors
    if (isMatches) {
      selector = this._replaceMatchesPseudo(selector, matches);
    }
    if (isNth) {
      selector = this._twiddleNthPlus(selector);
    }
    selector = selector.replace(DIR_PAREN, (m, before, dir, after) =>
      `[dir="${dir}"] ${before}${after}, ${before}[dir="${dir}"]${after}`);
    return selector;
  }

  _transformCompoundSelector(selector, combinator, scope, hostScope) {
    // replace :host with host scoping class
    let slottedIndex = selector.indexOf(SLOTTED);
    if (selector.indexOf(HOST) >= 0) {
      selector = this._transformHostSelector(selector, hostScope);
    // replace other selectors with scoping class
    } else if (slottedIndex !== 0) {
      selector = scope ? this._transformSimpleSelector(selector, scope) :
        selector;
    }
    // mark ::slotted() scope jump to replace with descendant selector + arg
    // also ignore left-side combinator
    let slotted = false;
    if (slottedIndex >= 0) {
      combinator = '';
      slotted = true;
    }
    // process scope jumping selectors up to the scope jump and then stop
    let stop;
    if (slotted) {
      stop = true;
      if (slotted) {
        // .zonk ::slotted(.foo) -> .zonk.scope > .foo
        selector = selector.replace(SLOTTED_PAREN, (m, paren) => ` > ${paren}`);
      }
    }
    return {value: selector, combinator, stop};
  }

  _transformSimpleSelector(selector, scope) {
    const attributes = selector.split(/(\[.+?\])/);

    const output = [];
    for (let i = 0; i < attributes.length; i++) {
      // Do not attempt to transform any attribute selector content
      if ((i % 2) === 1) {
        output.push(attributes[i]);
      } else {
        const part = attributes[i];

        if (!(part === '' && i === attributes.length - 1)) {
          let p$ = part.split(PSEUDO_PREFIX);
          p$[0] += scope;
          output.push(p$.join(PSEUDO_PREFIX));
        }
      }
    }

    return output.join('');
  }

  // :host(...) -> scopeName...
  _transformHostSelector(selector, hostScope) {
    let m = selector.match(HOST_PAREN);
    let paren = m && m[2].trim() || '';
    if (paren) {
      if (!paren[0].match(SIMPLE_SELECTOR_PREFIX)) {
        // paren starts with a type selector
        let typeSelector = paren.split(SIMPLE_SELECTOR_PREFIX)[0];
        // if the type selector is our hostScope then avoid pre-pending it
        if (typeSelector === hostScope) {
          return paren;
        // otherwise, this selector should not match in this scope so
        // output a bogus selector.
        } else {
          return SELECTOR_NO_MATCH;
        }
      } else {
        // make sure to do a replace here to catch selectors like:
        // `:host(.foo)::before`
        return selector.replace(HOST_PAREN, function(m, host, paren) {
          return hostScope + paren;
        });
      }
    // if no paren, do a straight :host replacement.
    // TODO(sorvell): this should not strictly be necessary but
    // it's needed to maintain support for `:host[foo]` type selectors
    // which have been improperly used under Shady DOM. This should be
    // deprecated.
    } else {
      return selector.replace(HOST, hostScope);
    }
  }

  /**
   * @param {StyleNode} rule
   */
  documentRule(rule) {
    // reset selector in case this is redone.
    rule['selector'] = rule['parsedSelector'];
    this.normalizeRootSelector(rule);
    this._transformRule(rule, this._transformDocumentSelector);
  }

  /**
   * @param {StyleNode} rule
   */
  normalizeRootSelector(rule) {
    if (rule['selector'] === ROOT) {
      rule['selector'] = 'html';
    }
  }

/**
 * @param {string} selector
 */
  _transformDocumentSelector(selector) {
    if (selector.match(HOST)) {
      // remove ':host' type selectors in document rules
      return '';
    } else if (selector.match(SLOTTED)) {
      return this._transformComplexSelector(selector, SCOPE_DOC_SELECTOR)
    } else {
      return this._transformSimpleSelector(selector.trim(), SCOPE_DOC_SELECTOR);
    }
  }
}

const NTH = /:(nth[-\w]+)\(([^)]+)\)/;
const SCOPE_DOC_SELECTOR = `:not(.${SCOPE_NAME})`;
const COMPLEX_SELECTOR_SEP = ',';
const SIMPLE_SELECTOR_SEP = /(^|[\s>+~]+)((?:\[.+?\]|[^\s>+~=[])+)/g;
const SIMPLE_SELECTOR_PREFIX = /[[.:#*]/;
const HOST = ':host';
const ROOT = ':root';
const SLOTTED = '::slotted';
const SLOTTED_START = new RegExp(`^(${SLOTTED})`);
// NOTE: this supports 1 nested () pair for things like
// :host(:not([selected]), more general support requires
// parsing which seems like overkill
const HOST_PAREN = /(:host)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))/;
// similar to HOST_PAREN
const SLOTTED_PAREN = /(?:::slotted)(?:\(((?:\([^)(]*\)|[^)(]*)+?)\))/;
const DIR_PAREN = /(.*):dir\((?:(ltr|rtl))\)(.*)/;
const CSS_CLASS_PREFIX = '.';
const PSEUDO_PREFIX = ':';
const CLASS = 'class';
const SELECTOR_NO_MATCH = 'should_not_match';
const MATCHES = /:(?:matches|any|-(?:webkit|moz)-any)/;
const MATCHES_REPLACEMENT = '\u{e000}';

var styleTransformer = new StyleTransformer();

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

/**
 * return true if `cssText` contains a mixin definition or consumption
 * @param {string} cssText
 * @return {boolean}
 */
function detectMixin(cssText) {
  const has = MIXIN_MATCH.test(cssText) || VAR_ASSIGN.test(cssText);
  // reset state of the regexes
  MIXIN_MATCH.lastIndex = 0;
  VAR_ASSIGN.lastIndex = 0;
  return has;
}

/**
@license
Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

const APPLY_NAME_CLEAN = /;\s*/m;
const INITIAL_INHERIT = /^\s*(initial)|(inherit)\s*$/;
const IMPORTANT = /\s*!important/;

// separator used between mixin-name and mixin-property-name when producing properties
// NOTE: plain '-' may cause collisions in user styles
const MIXIN_VAR_SEP = '_-_';

// map of mixin to property names
// --foo: {border: 2px} -> {properties: {(--foo, ['border'])}, dependants: {'element-name': proto}}
class MixinMap {
  constructor() {
    /** @type {!Object<string, !MixinMapEntry>} */
    this._map = {};
  }
  /**
   * @param {string} name
   * @param {!PropertyEntry} props
   */
  set(name, props) {
    name = name.trim();
    this._map[name] = {
      properties: props,
      dependants: {}
    };
  }
  /**
   * @param {string} name
   * @return {MixinMapEntry}
   */
  get(name) {
    name = name.trim();
    return this._map[name] || null;
  }
}

/**
 * Callback for when an element is marked invalid
 * @type {?function(string)}
 */
let invalidCallback = null;

/** @unrestricted */
class ApplyShim {
  constructor() {
    /** @type {?string} */
    this._currentElement = null;
    /** @type {HTMLMetaElement} */
    this._measureElement = null;
    this._map = new MixinMap();
  }
  /**
   * return true if `cssText` contains a mixin definition or consumption
   * @param {string} cssText
   * @return {boolean}
   */
  detectMixin(cssText) {
    return detectMixin(cssText);
  }

  /**
   * Gather styles into one style for easier processing
   * @param {!HTMLTemplateElement} template
   * @return {HTMLStyleElement}
   */
  gatherStyles(template) {
    const styleText = gatherStyleText(template.content);
    if (styleText) {
      const style = /** @type {!HTMLStyleElement} */(document.createElement('style'));
      style.textContent = styleText;
      template.content.insertBefore(style, template.content.firstChild);
      return style;
    }
    return null;
  }
  /**
   * @param {!HTMLTemplateElement} template
   * @param {string} elementName
   * @return {StyleNode}
   */
  transformTemplate(template, elementName) {
    if (template._gatheredStyle === undefined) {
      template._gatheredStyle = this.gatherStyles(template);
    }
    /** @type {HTMLStyleElement} */
    const style = template._gatheredStyle;
    return style ? this.transformStyle(style, elementName) : null;
  }
  /**
   * @param {!HTMLStyleElement} style
   * @param {string} elementName
   * @return {StyleNode}
   */
  transformStyle(style, elementName = '') {
    let ast = rulesForStyle(style);
    this.transformRules(ast, elementName);
    style.textContent = toCssText(ast);
    return ast;
  }
  /**
   * @param {!HTMLStyleElement} style
   * @return {StyleNode}
   */
  transformCustomStyle(style) {
    let ast = rulesForStyle(style);
    forEachRule(ast, (rule) => {
      if (rule['selector'] === ':root') {
        rule['selector'] = 'html';
      }
      this.transformRule(rule);
    });
    style.textContent = toCssText(ast);
    return ast;
  }
  /**
   * @param {StyleNode} rules
   * @param {string} elementName
   */
  transformRules(rules, elementName) {
    this._currentElement = elementName;
    forEachRule(rules, (r) => {
      this.transformRule(r);
    });
    this._currentElement = null;
  }
  /**
   * @param {!StyleNode} rule
   */
  transformRule(rule) {
    rule['cssText'] = this.transformCssText(rule['parsedCssText'], rule);
    // :root was only used for variable assignment in property shim,
    // but generates invalid selectors with real properties.
    // replace with `:host > *`, which serves the same effect
    if (rule['selector'] === ':root') {
      rule['selector'] = ':host > *';
    }
  }
  /**
   * @param {string} cssText
   * @param {!StyleNode} rule
   * @return {string}
   */
  transformCssText(cssText, rule) {
    // produce variables
    cssText = cssText.replace(VAR_ASSIGN, (matchText, propertyName, valueProperty, valueMixin) =>
      this._produceCssProperties(matchText, propertyName, valueProperty, valueMixin, rule));
    // consume mixins
    return this._consumeCssProperties(cssText, rule);
  }
  /**
   * @param {string} property
   * @return {string}
   */
  _getInitialValueForProperty(property) {
    if (!this._measureElement) {
      this._measureElement = /** @type {HTMLMetaElement} */(document.createElement('meta'));
      this._measureElement.setAttribute('apply-shim-measure', '');
      this._measureElement.style.all = 'initial';
      document.head.appendChild(this._measureElement);
    }
    return window.getComputedStyle(this._measureElement).getPropertyValue(property);
  }
  /**
   * Walk over all rules before this rule to find fallbacks for mixins
   *
   * @param {!StyleNode} startRule
   * @return {!Object}
   */
  _fallbacksFromPreviousRules(startRule) {
    // find the "top" rule
    let topRule = startRule;
    while (topRule['parent']) {
      topRule = topRule['parent'];
    }
    const fallbacks = {};
    let seenStartRule = false;
    forEachRule(topRule, (r) => {
      // stop when we hit the input rule
      seenStartRule = seenStartRule || r === startRule;
      if (seenStartRule) {
        return;
      }
      // NOTE: Only matching selectors are "safe" for this fallback processing
      // It would be prohibitive to run `matchesSelector()` on each selector,
      // so we cheat and only check if the same selector string is used, which
      // guarantees things like specificity matching
      if (r['selector'] === startRule['selector']) {
        Object.assign(fallbacks, this._cssTextToMap(r['parsedCssText']));
      }
    });
    return fallbacks;
  }
  /**
   * replace mixin consumption with variable consumption
   * @param {string} text
   * @param {!StyleNode=} rule
   * @return {string}
   */
  _consumeCssProperties(text, rule) {
    /** @type {Array} */
    let m = null;
    // loop over text until all mixins with defintions have been applied
    while((m = MIXIN_MATCH.exec(text))) {
      let matchText = m[0];
      let mixinName = m[1];
      let idx = m.index;
      // collect properties before apply to be "defaults" if mixin might override them
      // match includes a "prefix", so find the start and end positions of @apply
      let applyPos = idx + matchText.indexOf('@apply');
      let afterApplyPos = idx + matchText.length;
      // find props defined before this @apply
      let textBeforeApply = text.slice(0, applyPos);
      let textAfterApply = text.slice(afterApplyPos);
      let defaults = rule ? this._fallbacksFromPreviousRules(rule) : {};
      Object.assign(defaults, this._cssTextToMap(textBeforeApply));
      let replacement = this._atApplyToCssProperties(mixinName, defaults);
      // use regex match position to replace mixin, keep linear processing time
      text = `${textBeforeApply}${replacement}${textAfterApply}`;
      // move regex search to _after_ replacement
      MIXIN_MATCH.lastIndex = idx + replacement.length;
    }
    return text;
  }
  /**
   * produce variable consumption at the site of mixin consumption
   * `@apply` --foo; -> for all props (${propname}: var(--foo_-_${propname}, ${fallback[propname]}}))
   * Example:
   *  border: var(--foo_-_border); padding: var(--foo_-_padding, 2px)
   *
   * @param {string} mixinName
   * @param {Object} fallbacks
   * @return {string}
   */
  _atApplyToCssProperties(mixinName, fallbacks) {
    mixinName = mixinName.replace(APPLY_NAME_CLEAN, '');
    let vars = [];
    let mixinEntry = this._map.get(mixinName);
    // if we depend on a mixin before it is created
    // make a sentinel entry in the map to add this element as a dependency for when it is defined.
    if (!mixinEntry) {
      this._map.set(mixinName, {});
      mixinEntry = this._map.get(mixinName);
    }
    if (mixinEntry) {
      if (this._currentElement) {
        mixinEntry.dependants[this._currentElement] = true;
      }
      let p, parts, f;
      const properties = mixinEntry.properties;
      for (p in properties) {
        f = fallbacks && fallbacks[p];
        parts = [p, ': var(', mixinName, MIXIN_VAR_SEP, p];
        if (f) {
          parts.push(',', f.replace(IMPORTANT, ''));
        }
        parts.push(')');
        if (IMPORTANT.test(properties[p])) {
          parts.push(' !important');
        }
        vars.push(parts.join(''));
      }
    }
    return vars.join('; ');
  }

  /**
   * @param {string} property
   * @param {string} value
   * @return {string}
   */
  _replaceInitialOrInherit(property, value) {
    let match = INITIAL_INHERIT.exec(value);
    if (match) {
      if (match[1]) {
        // initial
        // replace `initial` with the concrete initial value for this property
        value = this._getInitialValueForProperty(property);
      } else {
        // inherit
        // with this purposfully illegal value, the variable will be invalid at
        // compute time (https://www.w3.org/TR/css-variables/#invalid-at-computed-value-time)
        // and for inheriting values, will behave similarly
        // we cannot support the same behavior for non inheriting values like 'border'
        value = 'apply-shim-inherit';
      }
    }
    return value;
  }

  /**
   * "parse" a mixin definition into a map of properties and values
   * cssTextToMap('border: 2px solid black') -> ('border', '2px solid black')
   * @param {string} text
   * @param {boolean=} replaceInitialOrInherit
   * @return {!Object<string, string>}
   */
  _cssTextToMap(text, replaceInitialOrInherit = false) {
    let props = text.split(';');
    let property, value;
    let out = {};
    for (let i = 0, p, sp; i < props.length; i++) {
      p = props[i];
      if (p) {
        sp = p.split(':');
        // ignore lines that aren't definitions like @media
        if (sp.length > 1) {
          property = sp[0].trim();
          // some properties may have ':' in the value, like data urls
          value = sp.slice(1).join(':');
          if (replaceInitialOrInherit) {
            value = this._replaceInitialOrInherit(property, value);
          }
          out[property] = value;
        }
      }
    }
    return out;
  }

  /**
   * @param {MixinMapEntry} mixinEntry
   */
  _invalidateMixinEntry(mixinEntry) {
    if (!invalidCallback) {
      return;
    }
    for (let elementName in mixinEntry.dependants) {
      if (elementName !== this._currentElement) {
        invalidCallback(elementName);
      }
    }
  }

  /**
   * @param {string} matchText
   * @param {string} propertyName
   * @param {?string} valueProperty
   * @param {?string} valueMixin
   * @param {!StyleNode} rule
   * @return {string}
   */
  _produceCssProperties(matchText, propertyName, valueProperty, valueMixin, rule) {
    // handle case where property value is a mixin
    if (valueProperty) {
      // form: --mixin2: var(--mixin1), where --mixin1 is in the map
      processVariableAndFallback(valueProperty, (prefix, value) => {
        if (value && this._map.get(value)) {
          valueMixin = `@apply ${value};`;
        }
      });
    }
    if (!valueMixin) {
      return matchText;
    }
    let mixinAsProperties = this._consumeCssProperties('' + valueMixin, rule);
    let prefix = matchText.slice(0, matchText.indexOf('--'));
    // `initial` and `inherit` as properties in a map should be replaced because
    // these keywords are eagerly evaluated when the mixin becomes CSS Custom Properties,
    // and would set the variable value, rather than carry the keyword to the `var()` usage.
    let mixinValues = this._cssTextToMap(mixinAsProperties, true);
    let combinedProps = mixinValues;
    let mixinEntry = this._map.get(propertyName);
    let oldProps = mixinEntry && mixinEntry.properties;
    if (oldProps) {
      // NOTE: since we use mixin, the map of properties is updated here
      // and this is what we want.
      combinedProps = Object.assign(Object.create(oldProps), mixinValues);
    } else {
      this._map.set(propertyName, combinedProps);
    }
    let out = [];
    let p, v;
    // set variables defined by current mixin
    let needToInvalidate = false;
    for (p in combinedProps) {
      v = mixinValues[p];
      // if property not defined by current mixin, set initial
      if (v === undefined) {
        v = 'initial';
      }
      if (oldProps && !(p in oldProps)) {
        needToInvalidate = true;
      }
      out.push(`${propertyName}${MIXIN_VAR_SEP}${p}: ${v}`);
    }
    if (needToInvalidate) {
      this._invalidateMixinEntry(mixinEntry);
    }
    if (mixinEntry) {
      mixinEntry.properties = combinedProps;
    }
    // because the mixinMap is global, the mixin might conflict with
    // a different scope's simple variable definition:
    // Example:
    // some style somewhere:
    // --mixin1:{ ... }
    // --mixin2: var(--mixin1);
    // some other element:
    // --mixin1: 10px solid red;
    // --foo: var(--mixin1);
    // In this case, we leave the original variable definition in place.
    if (valueProperty) {
      prefix = `${matchText};${prefix}`;
    }
    return `${prefix}${out.join('; ')};`;
  }
}

/* exports */
/* eslint-disable no-self-assign */
ApplyShim.prototype['detectMixin'] = ApplyShim.prototype.detectMixin;
ApplyShim.prototype['transformStyle'] = ApplyShim.prototype.transformStyle;
ApplyShim.prototype['transformCustomStyle'] = ApplyShim.prototype.transformCustomStyle;
ApplyShim.prototype['transformRules'] = ApplyShim.prototype.transformRules;
ApplyShim.prototype['transformRule'] = ApplyShim.prototype.transformRule;
ApplyShim.prototype['transformTemplate'] = ApplyShim.prototype.transformTemplate;
ApplyShim.prototype['_separator'] = MIXIN_VAR_SEP;
/* eslint-enable no-self-assign */
Object.defineProperty(ApplyShim.prototype, 'invalidCallback', {
  /** @return {?function(string)} */
  get() {
    return invalidCallback;
  },
  /** @param {?function(string)} cb */
  set(cb) {
    invalidCallback = cb;
  }
});

/**!
 * @license
 * Copyright (c) 2018 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at
 * http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at
 * http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at
 * http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at
 * http://polymer.github.io/PATENTS.txt
 */

exports.StyleUtil = styleUtil;
exports.CssParse = cssParse;
exports.StyleTransformer = styleTransformer;
exports.ApplyShim = ApplyShim;
exports.ShadyUnscopedAttribute = scopingAttribute;
