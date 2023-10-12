// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const kName = "crossbench";
const view = await chrome.devtools.recorder.createView(kName, 'devtools-recorder/crossbench.html');

let latestRecording;

view.onShown.addListener(() => {
  chrome.runtime.sendMessage(JSON.stringify(latestRecording));
});

view.onHidden.addListener(() => {
  chrome.runtime.sendMessage("stop");
});

export class RecorderPlugin {
  replay(recording) {
    latestRecording = recording;
    view.show();
  }
}

chrome.devtools.recorder.registerRecorderExtensionPlugin(new RecorderPlugin(), kName);
