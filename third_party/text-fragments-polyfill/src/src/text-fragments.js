/**
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as utils from './text-fragment-utils.js';

(async () => {
  // Return early if Text Fragments are supported by the browser.
  if ('fragmentDirective' in document ||
      'fragmentDirective' in Location.prototype) {
    return;
  }
  const hash = document.location.hash;

  // Pass feature detection (https://web.dev/text-fragments/#feature-detection)
  document.fragmentDirective = {};

  // Return early when there is no hash.
  // This is fine, since later added hashes are not relevant for
  // Text Fragments.
  if (!hash) {
    return;
  }

  const init = () => {
    const fragmentDirectives = utils.getFragmentDirectives(hash);
    const parsedFragmentDirectives = utils.parseFragmentDirectives(
        fragmentDirectives,
    );
    const processedFragmentDirectives = utils.processFragmentDirectives(
        parsedFragmentDirectives,
    );
    const createdMarks = processedFragmentDirectives['text'];
    utils.applyTargetTextStyle();
    const firstFoundMatch = createdMarks.find((marks) => marks.length)[0];
    if (firstFoundMatch) {
      window.setTimeout(() => utils.scrollElementIntoView(firstFoundMatch));
    }
  };
  if (document.readyState !== 'complete') {
    document.addEventListener('readystatechange', event => {
      if (event.target.readyState === 'complete') {
        init();
      }
    });
  } else {
    init();
  }
})();
