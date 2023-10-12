/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @fileoverview The worker insantiation code used when lottie-renderer is
 * running in chromium. NOTE this is in a sub folder so automatic build tools
 * don't keep trying to add cr_worker into ts_library rules. This file must
 * never actually get compiled in google3 because it creates a worker in a
 * unsafe way. This is fine in chromium however.
 */


let workerLoaderPolicy: TrustedTypePolicy|null = null;

function getLottieWorkerURL(): TrustedScriptURL {
  if (workerLoaderPolicy === null) {
    workerLoaderPolicy =
        window.trustedTypes!.createPolicy('lottie-worker-script-loader', {
          createScriptURL: (_ignore: string) => {
            const script =
                `import 'chrome://resources/cros_components/lottie_renderer/lottie_worker.js';`;
            // CORS blocks loading worker script from a different origin, even
            // if chrome://resources/ is added in the 'worker-src' CSP header.
            // (see https://crbug.com/1385477). Loading scripts as blob and then
            // instantiating it as web worker is possible.
            const blob = new Blob([script], {type: 'text/javascript'});
            return URL.createObjectURL(blob);
          },
          createHTML: () => {
            throw new Error('Not Allowed.');
          },
          createScript: () => {
            throw new Error('Not Allowed.');
          },
        });
  }

  return workerLoaderPolicy.createScriptURL('');
}

/** Construct a lottie web worker and return it. */
export function getWorker() {
  return new Worker(getLottieWorkerURL() as unknown as URL, {type: 'module'});
}
