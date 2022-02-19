// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      'Tests that virtual time fence does not block interrupting protocol' +
      ' commands.');

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending',
      budget: 1000, waitForNavigation: true});
  await dp.Performance.enable();
  dp.Page.navigate({url: testRunner.url('/resources/blank.html')});

  await dp.Emulation.onceVirtualTimeBudgetExpired();
  await dp.Performance.getMetrics();
  // Should pass.
  testRunner.log('Returned from the Performance.getMetrics call.');
  testRunner.completeTest();
})
