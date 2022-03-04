// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  let VirtualTimeController =
      await testRunner.loadScript('virtual-time-controller.js');

  let {page, session, dp} = await testRunner.startWithFrameControl(
    `Tests virtual time controller operation.`);

  await dp.Runtime.enable();
  await dp.HeadlessExperimental.enable();

  dp.Runtime.onConsoleAPICalled(data => {
    const text = data.params.args[0].value;
    testRunner.log(text);
  });

  let expirationCount = 0;
  const vtc = new VirtualTimeController(testRunner, dp, 25);
  await vtc.grantInitialTime(100, 1000, onInstalled, onExpired);

  async function onInstalled(virtualTimeBase){
    testRunner.log(`onInstalled:`);
  }

  async function onExpired(totalElapsedTime) {
    testRunner.log(`onExpired: ${totalElapsedTime}`);
    if (expirationCount === 0)
      await session.evaluate('startRAF()');

    if (++expirationCount < 3) {
      await vtc.grantTime(50, onExpired);
    } else {
      testRunner.completeTest();
    }
  }

  dp.Page.navigate({url: testRunner.url(
      'resources/virtual-time-controller-test.html')});
})
