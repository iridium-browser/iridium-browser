/**
 * @license
 * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
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
const {ScannedInlineDocument} = require('polymer-analyzer/lib/model/model.js');

const {expressionToValue} = require('polymer-analyzer/lib/javascript/ast-value.js');

/**
 * Finds assignments to innerHTML with HTML strings
 *
 * e.g.
 *     template.innerHTML = '<div></div>';
 */
class InnerHtmlAssignmentScanner {
  async scan(document, visit) {
    const features = [];

    const myVisitor = {
      enterAssignmentExpression(node) {
        const leftNode = node.left;
        if (leftNode.type !== 'MemberExpression') {
          return 'skip';
        }
        if (!leftNode.property || leftNode.property.name !== 'innerHTML'){
          return 'skip';
        }
        const inlineDocument = getInlineDocument(node.right, document);
        if (inlineDocument !== undefined) {
          features.push(inlineDocument);
        }
      }
    };

    await visit(myVisitor);

    return {features};
  }
}

/**
 * Parses the given string as an inline HTML document.
 */
function getInlineDocument(node, parsedDocument) {
  const sourceRangeForLiteral = parsedDocument.sourceRangeForNode(node);
  if (sourceRangeForLiteral === undefined) {
    return;
  }
  const sourceRangeForContents = {
    file: sourceRangeForLiteral.file,
    start: {
      line: sourceRangeForLiteral.start.line,
      column: sourceRangeForLiteral.start.column + 1
    },
    end: {
      line: sourceRangeForLiteral.end.line,
      column: sourceRangeForLiteral.end.column - 1
    }
  };

  let contents = expressionToValue(node);
  if (typeof contents !== 'string') {
    return;
  }
  let commentText;
  if (node.leadingComments != null) {
    commentText = node.leadingComments.map((c) => c.value).join('\n');
  } else {
    commentText = '';
  }

  return new ScannedInlineDocument(
      'html',
      contents,
      {
        filename: sourceRangeForContents.file,
        col: sourceRangeForContents.start.column,
        line: sourceRangeForContents.start.line
      },
      commentText,
      sourceRangeForContents,
      {language: 'js', node, containingDocument: parsedDocument});
}

exports.InnerHtmlAssignmentScanner = InnerHtmlAssignmentScanner;