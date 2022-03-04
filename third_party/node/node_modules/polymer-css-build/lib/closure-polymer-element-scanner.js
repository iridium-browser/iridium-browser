/**
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

const babel = require('@babel/types');

const {getIdentifierName, expressionToValue} = require('polymer-analyzer/lib/javascript/ast-value.js');
const esutil = require('polymer-analyzer/lib/javascript/esutil.js');
const jsdoc = require('polymer-analyzer/lib/javascript/jsdoc.js');
const {Severity, Warning} = require('polymer-analyzer/lib/model/model.js');

const {declarationPropertyHandlers} = require('polymer-analyzer/lib/polymer/declaration-property-handlers.js');
const {ScannedPolymerElement} = require('polymer-analyzer/lib/polymer/polymer-element.js');

class ClosurePolymerElementScanner {
  async scan(
      document,
      visit) {
    const visitor = new ElementVisitor(document);
    await visit(visitor);
    return {features: visitor.features, warnings: visitor.warnings};
  }
}

/**
 * Handles Polymer({}) calls.
 */
class ElementVisitor {
  constructor(document) {
    this.document = document;
    this.features = [];
    this.warnings = [];
  }

  enterCallExpression(
      node, parent, path) {
    const callee = node.callee;
    if (!babel.isIdentifier(callee)) {
      return;
    }
    if (node.arguments.length !== 1) {
      return;
    }
    const argument = node.arguments[0];
    if (!babel.isObjectExpression(argument)) {
      return;
    }
    const argAsObject = {};
    for (const prop of esutil.getSimpleObjectProperties(argument)) {
      const name = esutil.getPropertyName(prop);
      argAsObject[name] = expressionToValue(prop.value);
    }
    if (!argAsObject.is) {
      return;
    }
    const rawDescription = esutil.getAttachedComment(parent);
    let className = undefined;
    if (babel.isAssignmentExpression(parent)) {
      className = getIdentifierName(parent.left);
    } else if (babel.isVariableDeclarator(parent)) {
      className = getIdentifierName(parent.id);
    }
    const jsDoc = jsdoc.parseJsdoc(rawDescription || '');
    const element = new ScannedPolymerElement({
      className,
      astNode: node,
      statementAst: esutil.getCanonicalStatement(path),
      description: jsDoc.description,
      events: esutil.getEventComments(parent),
      sourceRange: this.document.sourceRangeForNode(node.arguments[0]),
      privacy: esutil.getOrInferPrivacy('', jsDoc),
      abstract: jsdoc.hasTag(jsDoc, 'abstract'),
      attributes: new Map(),
      properties: [],
      behaviors: [],
      extends: undefined,
      jsdoc: jsDoc,
      listeners: [],
      methods: new Map(),
      staticMethods: new Map(),
      mixins: [],
      observers: [],
      superClass: undefined,
      tagName: undefined
    });
    element.description = (element.description || '').trim();
    const propertyHandlers =
        declarationPropertyHandlers(element, this.document, path);

    if (babel.isObjectExpression(argument)) {
      this.handleObjectExpression(argument, propertyHandlers, element);
    }

    this.features.push(element);
  }

  handleObjectExpression(
      node, propertyHandlers,
      element) {
    for (const prop of esutil.getSimpleObjectProperties(node)) {
      const name = esutil.getPropertyName(prop);
      if (!name) {
        element.warnings.push(new Warning({
          message: `Can't determine name for property key from expression ` +
              `with type ${prop.key.type}.`,
          code: 'cant-determine-property-name',
          severity: Severity.WARNING,
          sourceRange: this.document.sourceRangeForNode(prop.key),
          parsedDocument: this.document
        }));
        continue;
      }
      if (name in propertyHandlers) {
        propertyHandlers[name](prop.value);
      } else if (
          (babel.isMethod(prop) && prop.kind === 'method') ||
          babel.isFunction(prop.value)) {
        const method = esutil.toScannedMethod(
            prop, this.document.sourceRangeForNode(prop), this.document);
        element.addMethod(method);
      }
    }

    for (const prop of esutil
             .extractPropertiesFromClassOrObjectBody(node, this.document)
             .values()) {
      if (prop.name in propertyHandlers) {
        continue;
      }
      element.addProperty({
        ...prop,
        isConfiguration: esutil.configurationProperties.has(prop.name),
      });
    }
  }
}

exports.ClosurePolymerElementScanner = ClosurePolymerElementScanner;