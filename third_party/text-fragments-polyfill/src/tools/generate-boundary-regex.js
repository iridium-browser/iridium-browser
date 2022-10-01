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

/**
 * Helper script which outputs an ES5-compatible regex for identifying boundary
 * characters. Ideally this would be done in the affected file automatically at
 * build-time, but doing so would break integrations that rely on including that
 * file uncompiled. Better still would be to use the Unicode Character
 * Properties regexes that were added in ES 2018, but it will be a long time
 * before those have wide support.
 */

const regenerate = require('regenerate');
const set =
    regenerate()
        .add(require(
            'unicode-9.0.0/General_Category/Punctuation/code-points.js'))
        .add(require(
            'unicode-9.0.0/Binary_Property/White_Space/code-points.js'));
console.log('/' + set.toString() + '/u');
