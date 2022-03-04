/**
@license
Copyright (c) 2018 The Polymer Project Authors. All rights reserved.
This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
Code distributed by Google as part of the polymer project is also
subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
*/

// copied from polymer DirMixin
const HOST_DIR = /:host\(:dir\((ltr|rtl)\)\)/g;
const HOST_DIR_REPLACMENT = '$&, :host([dir="$1"])';

const EL_DIR = /(?<!:host\()([\s\w-#.[\]*]*):dir\((ltr|rtl)\)/g;
const EL_DIR_REPLACMENT = '$&, :host([dir="$2"]) $1';

const DIR_CHECK = /:dir\((?:ltr|rtl)\)/;

function polymer2DirShadowTransform(selector) {
  if (!DIR_CHECK.test(selector)) {
    return selector;
  }
  selector = selector.replace(HOST_DIR, HOST_DIR_REPLACMENT);
  return selector.replace(EL_DIR, EL_DIR_REPLACMENT);
}

module.exports = {
  polymer2DirShadowTransform
}