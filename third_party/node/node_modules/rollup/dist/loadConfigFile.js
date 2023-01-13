/*
  @license
	Rollup.js v2.58.0
	Fri, 01 Oct 2021 06:54:03 GMT - commit 3a404a07f41a10d10b7af536f05b90ea46d8ad3d


	https://github.com/rollup/rollup

	Released under the MIT License.
*/
'use strict';

require('fs');
require('path');
require('url');
const loadConfigFile_js = require('./shared/loadConfigFile.js');
require('./shared/rollup.js');
require('./shared/mergeOptions.js');
require('tty');
require('crypto');
require('events');



module.exports = loadConfigFile_js.loadAndParseConfigFile;
//# sourceMappingURL=loadConfigFile.js.map
