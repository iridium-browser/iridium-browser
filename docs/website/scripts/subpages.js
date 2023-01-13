// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file implements support for the "subpages" extension. If a page author
// inserts `{% subpages collections.all %}` into a document, this function will
// find all of the pages that are sub-pages of the specified page (sub-pages in
// the sense that /blink/design-documents is a sub-page of /blink) and display
// them in a hierarchical tree format. `pageUrl` should be the (path part of
// the) url of the current page (Eleventy's `page.url`) and
// `collectionOfAllPages` should be Eleventy's `collections.all`.
//
// TODO(crbug.com/1271672): Figure out how to make this cleaner so the syntax
// is less clunky.
function render(pageUrl, collectionOfAllPages) {
  let topPage = new Page('', pageUrl);

  let subPages = [];
  for (const item of collectionOfAllPages) {
    if (isDescendant(item, topPage)) {
      subPages.push(new Page(item.data.title, item.data.page.url));
    }
  }

  // Create a map from URLs to Pages for `pageUrl` and its sub-pages.
  const pageMap = new Map();
  pageMap.set(topPage.url, topPage);

  // Sorting the pages by url ensures that a parent will be added to the
  // map before any of its descendants when we populate the map.
  subPages.sort(byProperty('url'));

  for (const subPage of subPages) {
    pageMap.set(subPage.url, subPage);
    pageMap.get(subPage.parentUrl).subPages.push(subPage);
  }

  let html = ('<nav class="subpage-listing">\n' +
              '  <h4>Subpage Listing</h4>\n' +
              '  <ul>\n');

  topPage.subPages.sort(byProperty('title'));
  for (const subPage of topPage.subPages) {
    html += '    <li>\n' + subPage.walk(3);
  }
  html += ('  </ul>\n' +
           '</nav>\n');

  return html;
}

class Page {
  constructor(title, url) {
    this.title = title;
    this.url = trimSlash(url);
    this.parentUrl = dirname(this.url);

    // This holds only the immediate sub-pages of the page, not the
    // transitive closure of all sub-pages.
    this.subPages = [];
  }

  // walk over the transitive closure of all of the page's subpages,
  // and return an html fragment describing them as a tree of
  // links and <details> elements (when a page has subpages).
  // `indentDepth` is the number of levels to indent the HTML fragment.
  walk(indentDepth) {
    const indent = '  '.repeat(indentDepth);

    this.subPages.sort(byProperty('title'));

    if (this.subPages.length) {
      let html = (`${indent}<details open>\n` +
                  `${indent}  <summary><a href="${this.url}">${
                      this.title}</a></summary>\n` +
                  `${indent}  <ul>\n`);

      for (const subPage of this.subPages) {
        html += (`${indent}    <li>\n` +
                 `${subPage.walk(indentDepth + 3)}`);
      }
      html += (`${indent}  </ul>\n` +
               `${indent}</details>\n`);
      return html;
    } else {
      return `${indent}<a href="${this.url}">${this.title}</a>\n`;
    }
  }
}

// Returns the directory above the `path`, e.g.:
//  `dirname("/foo/bar")` returns "/foo".
// Note that `dirname("/foo/bar/") also returns "/foo").
function dirname(path) {
  comps = path.split('/');
  return comps.slice(0, comps.length - 1).join('/');
}

// Returns a copy of the string `s` with the rightmost `/` removed.
function trimSlash(s) {
  if (s.endsWith('/')) {
    return s.substr(0, s.length - 1);
  }
  return s;
}

function isDescendant(item, topPage) {
  let itemUrl = trimSlash(item.data.page.url);
  return itemUrl.startsWith(topPage.url + '/');
}

// Returns a comparison function that will compare two objects by
// the lower-cased values of the specified property.
function byProperty(prop) {
  return (x, y) => {
    a = x[prop].toLowerCase();
    b = y[prop].toLowerCase();
    return (a > b ? 1 : (a === b ? 0 : -1));
  }
}

exports.render = render;
