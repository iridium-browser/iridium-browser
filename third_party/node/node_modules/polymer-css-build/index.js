/**
@license
Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

'use strict';
const dom5 = require('dom5');

const pathResolver = require('./lib/pathresolver.js');

let ApplyShim, CssParse, StyleTransformer, StyleUtil, ShadyUnscopedAttribute;
const loadShadyCSS = require('./lib/shadycss-entrypoint.js');

const {Analyzer, InMemoryOverlayUrlLoader} = require('polymer-analyzer');

// Use types from polymer-analyzer
/* eslint-disable no-unused-vars */
const {Analysis} = require('polymer-analyzer/lib/model/model.js');
const {ParsedHtmlDocument} = require('polymer-analyzer');
/* eslint-enable */

const {traverse} = require('polymer-analyzer/lib/javascript/estraverse-shim.js');

const {dirShadowTransform, slottedToContent, shadyReplaceContent, fixBadVars} = require('./lib/polymer-1-transforms.js');

const {polymer2DirShadowTransform} = require('./lib/polymer-2-dir-transform.js');

const {createScannerMap, createParserMap} = require('./lib/slim-analyzer-options.js');

const {IdentifierIsDocumentVariable} = require('./lib/closure-html-template-scanner.js');

const pred = dom5.predicates;

/**
 * Map of dom-module id to <dom-module> element in the HTML AST
 * @type {!Map<string, !Object>}
 */
const domModuleMap = Object.create(null);

// TODO: upstream to dom5
const styleMatch = pred.AND(
  pred.hasTagName('style'),
  pred.OR(
    pred.NOT(
      pred.hasAttr('type')
    ),
    pred.hasAttrValue('type', 'text/css')
  )
);

const notStyleMatch = pred.NOT(styleMatch);

const customStyleMatchV1 = pred.AND(
  styleMatch,
  pred.hasAttrValue('is', 'custom-style')
);

const customStyleMatchV2 = pred.AND(
  styleMatch,
  pred.parentMatches(
    pred.hasTagName('custom-style')
  )
);

const styleIncludeMatch = pred.AND(styleMatch, pred.hasAttr('include'));

/**
 * Map of <style> ast node to element name that defines it
 * @type {!WeakMap<!Object, string>}
 */
const scopeMap = new WeakMap();

/**
 * Map of <style> ast node to inline HTML documents from analyzer
 * @type {!WeakMap<!Object, !ParsedHtmlDocument>}
 */
const inlineHTMLDocumentMap = new WeakMap();

function prepend(parent, node) {
  if (parent.childNodes.length > 0) {
    dom5.insertBefore(parent, parent.childNodes[0], node);
  } else {
    dom5.append(parent, node);
  }
}

/*
 * Collect styles from dom-module
 * In addition, make sure those styles are inside a template
 */
function getAndFixDomModuleStyles(domModule) {
  // TODO: support `.styleModules = ['module-id', ...]` ?
  const styles = getStyles(domModule);
  if (!styles.length) {
    return [];
  }
  let template = dom5.query(domModule, pred.hasTagName('template'));
  if (!template) {
    template = dom5.constructors.element('template');
    const content = dom5.constructors.fragment();
    styles.forEach(s => dom5.append(content, s));
    dom5.append(template, content);
    dom5.append(domModule, template);
  } else {
    styles.forEach((s) => {
      let templateContent = template.content;
      // ensure element styles are inside the template element
      const parent = dom5.nodeWalkAncestors(s, (n) =>
        n === templateContent || n === domModule
      );
      if (parent !== templateContent) {
        prepend(templateContent, s);
      }
    })
  }
  return styles;
}

function getStyles(astNode) {
  return dom5.queryAll(astNode, styleMatch, undefined, dom5.childNodesIncludeTemplate)
}

// TODO: consider upstreaming to dom5
function getAttributeArray(node, attribute) {
  const attr = dom5.getAttribute(node, attribute);
  let array;
  if (!attr) {
    array = [];
  } else {
    array = attr.split(' ');
  }
  return array;
}

function isUnscopedStyle(style) {
  return dom5.hasAttribute(style, ShadyUnscopedAttribute);
}

function inlineStyleIncludes(style, useNativeShadow) {
  if (!styleIncludeMatch(style)) {
    return;
  }
  const styleText = [];
  const includes = getAttributeArray(style, 'include');
  const leftover = [];
  const baseDocument = style.__ownerDocument;
  includes.forEach((id) => {
    const domModule = domModuleMap[id];
    if (!domModule) {
      // we missed this one, put it back on later
      leftover.push(id);
      return;
    }
    const includedStyles = getAndFixDomModuleStyles(domModule);
    // gather included styles
    includedStyles.forEach((ism) => {
      // do not inline styles that need to be unscoped in ShadyDOM
      if (!useNativeShadow && isUnscopedStyle(ism)) {
        leftover.push(id);
        return;
      }
      // this style may also have includes
      inlineStyleIncludes(ism);
      const inlineDocument = domModule.__ownerDocument;
      let includeText = dom5.getTextContent(ism);
      // adjust paths
      includeText = pathResolver.rewriteURL(inlineDocument, baseDocument, includeText);
      styleText.push(includeText);
    });
  });
  // remove inlined includes
  if (leftover.length) {
    dom5.setAttribute(style, 'include', leftover.join(' '));
  } else {
    dom5.removeAttribute(style, 'include');
  }
  // prepend included styles
  if (styleText.length) {
    let text = dom5.getTextContent(style);
    text = styleText.join('') + text;
    dom5.setTextContent(style, text);
  }
}

function applyShim(ast) {
  /*
   * `transform` expects an array of decorated <style> elements
   *
   * Decorated <style> elements are ones with `__cssRules` property
   * with a value of the CSS ast
   */
  StyleUtil.forEachRule(ast, (rule) => ApplyShim.transformRule(rule));
}

function getModuleDefinition(moduleName, elementDescriptors) {
  for (let ed of elementDescriptors) {
    if (ed.tagName && ed.tagName.toLowerCase() === moduleName) {
      return ed;
    }
  }
  return null;
}

function shadyShim(ast, style, analysis) {
  const scope = scopeMap.get(style);
  const moduleDefinition = getModuleDefinition(scope, analysis.getFeatures({kind: 'element'}));
  // only shim if module is a full polymer element, not just a style module
  if (!scope || !moduleDefinition) {
    return;
  }
  const ext = moduleDefinition.extends;
  StyleTransformer.css(ast, scope, ext);
}

function addClass(node, className) {
  const classList = getAttributeArray(node, 'class');
  if (classList.indexOf('style-scope') === -1) {
    classList.push('style-scope');
  }
  if (classList.indexOf(className) === -1) {
    classList.push(className);
  }
  dom5.setAttribute(node, 'class', classList.join(' '));
}

function markElement(element, useNativeShadow) {
  dom5.setAttribute(element, 'css-build', useNativeShadow ? 'shadow' : 'shady');
}

function markDomModule(domModule, scope, useNativeShadow, markTemplate = true) {
  // apply scoping to dom-module
  markElement(domModule, useNativeShadow);
  // apply scoping to template
  const template = dom5.query(domModule, pred.hasTagName('template'));
  if (template) {
    if (markTemplate) {
      markElement(template, useNativeShadow);
    }
    // mark elements' subtree under shady build
    if (!useNativeShadow && scope) {
      shadyScopeElementsInTemplate(template, scope);
    }
  }
}

function shadyScopeElementsInTemplate(template, scope) {
  const elements = dom5.queryAll(template, notStyleMatch, undefined, dom5.childNodesIncludeTemplate);
  elements.forEach((el) => addClass(el, scope));
}

// For forward compatibility with ShadowDOM v1 and Polymer 2.x,
// replace ::slotted(${inner}) with ::content > ${inner}
function slottedTransform(ast) {
  StyleUtil.forEachRule(ast, (rule) => {
    rule.selector = slottedToContent(rule.selector);
  });
}

function dirTransform(ast, polymerVersion) {
  StyleUtil.forEachRule(ast, (rule) => {
    if (polymerVersion === 1) {
      rule.selector = dirShadowTransform(rule.selector);
    } else {
      rule.selector = polymer2DirShadowTransform(rule.selector);
    }
  });
}

function contentTransform(ast, scope) {
  StyleUtil.forEachRule(ast, (rule) => {
    rule.selector = shadyReplaceContent(rule.selector, scope);
  });
}

function setUpLibraries(useNativeShadow) {
  ({
    ApplyShim,
    CssParse,
    StyleTransformer,
    StyleUtil,
    ShadyUnscopedAttribute
  } = loadShadyCSS(useNativeShadow));
}

function setNodeFileLocation(node, analysisKind) {
  if (!node.__ownerDocument) {
    node.__ownerDocument = analysisKind.sourceRange.file;
  }
}

/**
 *
 * @param {Analysis} analysis
 * @param {Function} query
 * @param {Function=} queryOptions
 * @return {Array}
 */
function nodeWalkAllDocuments(analysis, query, queryOptions = undefined) {
  const results = [];
  for (const document of analysis.getFeatures({kind: 'html-document'})) {
    const matches = dom5.nodeWalkAll(document.parsedDocument.ast, query, undefined, queryOptions);
    matches.forEach((match) => {
      setNodeFileLocation(match, document);
      results.push(match)
    });
  }
  return results;
}

/**
 * Handle the difference between analyzer 2 and 3 Analyzer::getDocument
 * @param {!Analysis} analysis
 * @param {string} url
 * @return {!Document}
 */
function getDocument(analysis, url) {
  const res = analysis.getDocument(url);
  if (res.error) {
    throw res.error;
  }
  return res.value;
}

function getAstNode(feature) {
  let astNode = feature.astNode;
  if (astNode.node) {
    return astNode.node;
  } else {
    return astNode;
  }
}

function getContainingDocument(feature, analysis) {
  return feature.astNode.containingDocument ||
    analysis.getDocumentContaining(feature.sourceRange).parsedDocument;
}

function getOrderedPolymerElements(analysis) {
  const polymerElements = new Set();
  for (const document of analysis.getFeatures({kind: 'document'})) {
    for (const element of document.getFeatures({kind: 'polymer-element'})) {
      polymerElements.add(element);
    }
  }
  return Array.from(polymerElements);
}

function updateInlineDocument(inlineDocument) {
  if (!inlineDocument || !inlineDocument.isInline) {
    return;
  }
  // this is a hack and bad
  // get the containing JavascriptDocument for the inline document
  const jsNode = getAstNode(inlineDocument);
  let node = jsNode;
  /* eslint-disable no-fallthrough */
  switch(jsNode.type) {
    case 'TaggedTemplateExpression':
      // Get the to the node representing the inside of the TemplateLiteral
      node = node.quasi;
    case 'TemplateLiteral':
      // set the value of the TemplateLiteral interior to the stringified document
      // the AST has both "cooked" and "raw" versions of the string, but they should be the same in practice
      node = node.quasis[0];
      node.value = {
        cooked: inlineDocument.stringify(),
        raw: inlineDocument.stringify()
      };
      break;
    case 'ArrayExpression':
      // get to the node representing the closurified output of html``
      node = node.elements[0];
    case 'StringLiteral':
      // set the value of the array element to the stringified document
      node.value = inlineDocument.stringify();
      break;
  }
  /* eslint-enable */
}

function searchAst(polymerElement, templateDocument, analysis) {
  const polymerElementNode = getAstNode(polymerElement, analysis);
  const templateNode = getAstNode(templateDocument, analysis);
  let match = false;
  // Note: in visitor, return 'skip' to skip subtree, and 'break' to exit early
  const visitor = {
    enterCallExpression(current) {
      const args = current.arguments;
      if (args.length !== 1) {
        return 'skip';
      }
      const [arg, ] = args;
      if (arg.type !== 'Identifier') {
        return 'skip';
      }
      if (IdentifierIsDocumentVariable(arg, templateNode)) {
        match = templateNode;
        return 'break';
      }
    },
    enterObjectProperty(current, _parent, path) {
      if (path.parentPath.parent !== polymerElementNode) {
        return 'skip';
      }
      // this is a Polymer({}) call with `_template`
      const keyName = current.key.name;
      if (keyName === '_template' || keyName === 'template') {
        match = current.value === templateNode;
        if (match) {
          return 'break';
        }
      }
    },
    enterClassMethod(current) {
      // this is a class element with `static get template() {}`
      if (!current.static || current.kind !== 'get' || current.key.name !== 'template') {
        return 'skip';
      }
    },
    enterTaggedTemplateExpression(current) {
      match = current === templateNode;
      if (match) {
        return 'break';
      }
    }
  };
  // walk the AST from the `Polymer({})` or `class {}` definition
  traverse(polymerElementNode, visitor);
  return match;
}

function getInlinedTemplateDocument(polymerElement, analysis) {
  // lookup from cache
  if (inlineHTMLDocumentMap.has(polymerElement)) {
    return inlineHTMLDocumentMap.get(polymerElement);
  }
  // find InlinedHTMLDocuments inside of the "parsed" js document containing this PolymerElement
  const jsDocument = getContainingDocument(polymerElement, analysis);
  const parsedJsDocument = getDocument(analysis, jsDocument.url);
  const inlineHTMLDocumentSet = parsedJsDocument.getFeatures({kind: 'html-document'});
  let inlinedDocument = null;
  // search all inlined html-documents in this js document for the one that is
  // inside the class definition for this polymer element scanning the AST
  for (const htmlDocument of inlineHTMLDocumentSet) {
    if (searchAst(polymerElement, htmlDocument, analysis)) {
      inlinedDocument = htmlDocument;
      break;
    }
  }
  if (!inlinedDocument) {
    return null;
  }
  // cache template lookup
  inlineHTMLDocumentMap.set(polymerElement, inlinedDocument.parsedDocument);
  return inlinedDocument.parsedDocument;
}

function getInlinedStyles(polymerElement, analysis) {
  const document = getInlinedTemplateDocument(polymerElement, analysis);
  if (!document) {
    return [];
  }
  return getStyles(document.ast);
}

function findDisconnectedDomModule(polymerElement, analysis) {
  // search analysis for a dom-module with the same id as the polymerElement's
  const [domModuleFeature, ] = analysis.getFeatures({kind: 'dom-module', id: polymerElement.tagName});
  if (domModuleFeature) {
    // polymerElement.domModule is assumed to the the HTML node
    const domModuleNode = getAstNode(domModuleFeature);
    polymerElement.domModule = domModuleNode;
  }
}

function markPolymerElement(polymerElement, useNativeShadow, analysis) {
  const document = getInlinedTemplateDocument(polymerElement, analysis);
  if (!document) {
    return;
  }
  const template = document.ast;
  // add a comment of the form `<!--css-build:shadow-->` to the template as the first child
  const buildComment = dom5.constructors.comment(`css-build:${useNativeShadow ? 'shadow' : 'shady'}`);
  prepend(template, buildComment);
  if (!useNativeShadow) {
    shadyScopeElementsInTemplate(template, polymerElement.tagName);
  }
}

function buildInlineDocumentSet(analysis) {
  const inlineHTMLDocumentSet = new Set();

  for (const doc of analysis.getFeatures({kind: 'html-document', externalPackages: true})) {
    if (doc.isInline) {
      inlineHTMLDocumentSet.add(doc);
    }
  }

  return inlineHTMLDocumentSet;
}

async function polymerCssBuild(paths, options = {}) {
  const nativeShadow = options ? !options['build-for-shady'] : true;
  const polymerVersion = options['polymer-version'] || 2;
  const customStyleMatch = polymerVersion === 2 ? customStyleMatchV2 : customStyleMatchV1;
  setUpLibraries(nativeShadow);
  // build analyzer loader
  const loader = new InMemoryOverlayUrlLoader();
  const analyzer = new Analyzer({
    urlLoader: loader,
    parsers: createParserMap(),
    scanners: createScannerMap(polymerVersion === 2)
  });
  // load given files as strings
  paths.forEach((p) => {
    loader.urlContentsMap.set(analyzer.resolveUrl(p.url), p.content);
  });
  // run analyzer on all given files
  /** @type {Analysis} */
  const analysis = await analyzer.analyze(paths.map((p) => p.url));
  // populate the dom module map
  const domModuleSet = analysis.getFeatures({kind: 'dom-module'})
  for (const domModule of domModuleSet) {
    const scope = domModule.id.toLowerCase();
    const astNode = getAstNode(domModule);
    domModuleMap[scope] = astNode;
    setNodeFileLocation(astNode, domModule);
  }
  // grap a set of all inline html documents to update at the end
  const inlineHTMLDocumentSet = buildInlineDocumentSet(analysis);
  // map polymer elements to styles
  const moduleStyles = [];
  for (const polymerElement of getOrderedPolymerElements(analysis)) {
    const scope = polymerElement.tagName;
    let styles = [];
    if (!polymerElement.domModule) {
      // there can be cases where a polymerElement is defined in a way that
      // analyzer can't get associate it with the <dom-module>, so try to find
      // it before assuming the polymerElement has an inline template
      findDisconnectedDomModule(polymerElement, analysis);
    }
    if (polymerElement.domModule) {
      const domModule = polymerElement.domModule;
      markDomModule(domModule, scope, nativeShadow, polymerVersion > 1);
      styles = getAndFixDomModuleStyles(domModule);
    } else {
      markPolymerElement(polymerElement, nativeShadow, analysis);
      styles = getInlinedStyles(polymerElement, analysis);
    }
    styles.forEach((s) => {
      scopeMap.set(s, scope);
      setNodeFileLocation(s, polymerElement);
    });
    moduleStyles.push(styles);
  }
  // also process dom modules used only for style sharing
  for (const domModule of domModuleSet) {
    const {id} = domModule;
    const domModuleIsElementTemplate = analysis.getFeatures({kind: 'polymer-element', id}).size > 0;
    if (!domModuleIsElementTemplate) {
      const styles = getAndFixDomModuleStyles(domModuleMap[id]);
      styles.forEach((s) => {
        scopeMap.set(s, id);
        setNodeFileLocation(s, domModule);
      });
      moduleStyles.push(styles);
    }
  }
  // inline and flatten styles into a single list
  const flatStyles = [];
  moduleStyles.forEach((styles) => {
    if (!styles.length) {
      return;
    }
    // do style includes
    if (options ? !options['no-inline-includes'] : true) {
      styles.forEach((s) => inlineStyleIncludes(s, nativeShadow));
    }
    // reduce styles to one
    const finalStyle = styles[styles.length - 1];
    dom5.setAttribute(finalStyle, 'scope', scopeMap.get(finalStyle));
    if (styles.length > 1) {
      const consumed = styles.slice(0, -1);
      const text = styles.map((s) => dom5.getTextContent(s));
      const includes = styles.map((s) => getAttributeArray(s, 'include')).reduce((acc, inc) => acc.concat(inc));
      consumed.forEach((c) => dom5.remove(c));
      dom5.setTextContent(finalStyle, text.join(''));
      const oldInclude = getAttributeArray(finalStyle, 'include');
      const newInclude = Array.from(new Set(oldInclude.concat(includes))).join(' ');
      if (newInclude) {
        dom5.setAttribute(finalStyle, 'include', newInclude);
      }
    }
    flatStyles.push(finalStyle);
  });
  // find custom styles
  const customStyles = nodeWalkAllDocuments(analysis, customStyleMatch);
  // inline custom styles with includes
  if (options ? !options['no-inline-includes'] : true) {
    customStyles.forEach((s) => inlineStyleIncludes(s, nativeShadow));
  }
  // add custom styles to the front
  // custom styles may define mixins for the whole tree
  flatStyles.unshift(...customStyles);
  // fix old `var(--a, --b)` syntax in polymer v1 builds
  if (polymerVersion === 1) {
    flatStyles.forEach((s) => {
      const text = dom5.getTextContent(s);
      const ast = CssParse.parse(text);
      StyleUtil.forEachRule(ast, (rule) => {
        fixBadVars(rule);
      });
      dom5.setTextContent(s, CssParse.stringify(ast, true));
    });
  }
  // populate mixin map
  flatStyles.forEach((s) => {
    const text = dom5.getTextContent(s);
    const ast = CssParse.parse(text);
    applyShim(ast);
  });
  // parse, transform, emit
  flatStyles.forEach((s) => {
    let text = dom5.getTextContent(s);
    const ast = CssParse.parse(text);
    if (customStyleMatch(s)) {
      // custom-style `:root` selectors need to be processed to `html`
      StyleUtil.forEachRule(ast, (rule) => {
        if (options && options['build-for-shady']) {
          StyleTransformer.documentRule(rule);
        } else {
          StyleTransformer.normalizeRootSelector(rule);
        }
      });
      // mark the style as built
      markElement(s, nativeShadow);
    }
    applyShim(ast);
    if (nativeShadow) {
      dirTransform(ast, polymerVersion);
      if (polymerVersion === 1) {
        slottedTransform(ast);
      }
    } else {
      shadyShim(ast, s, analysis);
      if (polymerVersion === 1) {
        contentTransform(ast, scopeMap.get(s));
      }
    }
    text = CssParse.stringify(ast, true);
    dom5.setTextContent(s, text);
  });
  // update inline HTML documents
  for (const inlineDocument of inlineHTMLDocumentSet) {
    updateInlineDocument(inlineDocument);
  }
  return paths.map((p) => {
    const doc = getDocument(analysis, p.url);
    return {
      url: p.url,
      content: doc.stringify()
    };
  });
}

exports.polymerCssBuild = polymerCssBuild;
