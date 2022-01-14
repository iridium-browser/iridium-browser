// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  let {page, session, dp} = await testRunner.startWithFrameControl(
      'Tests renderer: redirect invalid url.');

  let RendererTestHelper =
      await testRunner.loadScript('../helpers/renderer-test-helper.js');
  let {httpInterceptor, frameNavigationHelper, virtualTimeController} =
      await (new RendererTestHelper(testRunner, dp, page)).init();

  httpInterceptor.addResponse('http://www.example.com/',
      `<!DOCTYPE html><p>Pass</p>`,
      ['HTTP/1.1 302 Found', 'Location: http://']);

  await virtualTimeController.grantInitialTime(1000, 1000,
    null,
    async () => {
      testRunner.completeTest();
    }
  );

  await frameNavigationHelper.navigate('http://www.example.com/');
})
