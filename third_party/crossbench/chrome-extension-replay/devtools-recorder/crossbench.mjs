// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

class CrossbenchRecorder {

  constructor(host, port, token, onmessage, onerror) {
    this._host = host;
    this._port = port;
    this._token = token;
    this._onMessageHandler = onmessage;
    this._onErrorHandler = onerror;
  }

  async start() {
    this._webSocket = new WebSocket(`ws://${this._host}:${this._port}`);
    this._isConnecting = true;
    this._webSocket.onmessage = this._onmessage.bind(this);
    this._webSocket.onerror = (e) => this._onErrorHandler(e);
    this._webSocket.onopen = (e) => this.status();
  }

  get isConnected() {
    return this._webSocket && this._webSocket.readyState == WebSocket.OPEN;
  }

  _onmessage(messageEvent) {
    const { success, type, payload, error } = JSON.parse(messageEvent.data);
    this._onMessageHandler(success, type, payload, error);
  }

  _command(command, args = undefined) {
    const message = { token: this._token, command: command, args: args };
    if (!this.isConnected) {
      throw Error("Invalid websocket state");
    }
    this._webSocket.send(JSON.stringify(message));
  }

  status() {
    this._command("status");
  }

  run(cmd, json) {
    this._command("run", { cmd, json });
  }

  readline() {
    this._command("readline");
  }

  stop() {
    if (this.isConnected) {
      this._command("stop");
    }
    this._webSocket.close();
  }
}

function $(query) {
  return document.querySelector(query);
}

class Status {
  static DISCONNECTED = "disconnected";
  static CONNECTING = "connecting";
  static CONNECTED = "connected";
  static RUNNING = "running";
}


class UI {
  _crossbench;
  _recorderJSON;
  _status = Status.DISCONNECTED;
  _pingIntervalID;

  constructor() {
    if (chrome?.runtime?.onMessage) {
      chrome.runtime.onMessage.addListener(this._onChromeMessage.bind(this));
    }
    $("#port").addEventListener("change", this._reconnect.bind(this));
    $("#token").addEventListener("change", this._reconnect.bind(this));
    $("#connectButton").onclick = this._reconnect.bind(this);
    $("#crossbenchCMD").addEventListener("change", this._updateCMD.bind(this));
    $("#helpButton").onclick = (e) => this._showHelp("loading");
    $("#probesHelpButton").onclick = (e) => this._showHelp("probes");
    $("#runButton").onclick = this._run.bind(this);
    $("#stopButton").onclick = this._stop.bind(this);
    $("#copyStdout").onclick = () => this._copyOutput("#outputStdout");
    $("#copyStderr").onclick = () => this._copyOutput("#outputStderr");
    $("#copyRecorderJSON").onclick = () => this._copyOutput("#copyRecorderJSON");


    $("#port").value = localStorage.getItem("crossbenchPort") | 44645;
    $("#token").value = localStorage.getItem("crossbenchToken");
    $("#crossbenchCMD").value = localStorage.getItem("crossbenchCMD");
    this._recorderJSON = JSON.parse(localStorage.getItem("recordingJSON") || "{}");
    this._reconnect();
  }

  _updateRecording() {
    $("#recording").value = JSON.stringify(this._recorderJSON, undefined, "  ");
  }

  _updateCMD() {
    const cmd = $("#crossbenchCMD").value;
    localStorage.setItem("crossbenchCMD", cmd);
  }

  _run() {
    if (!this._crossbench) return;
    if (!this._recorderJSON) return;
    const cmd = $("#crossbenchCMD").value;
    this._crossbench.run(cmd, this._recorderJSON);
    this._clearOutput();
  }

  _clearOutput() {
    $("#outputStdout").value = "";
    $("#outputStderr").value = "";
  }

  _showHelp(type) {
    if (this._status !== Status.CONNECTED) return;
    if (type === "loading") {
      this._crossbench.run("--help");
    } else if (type === "probes") {
      this._crossbench.run("describe probes");
    } else {
      console.error("Unknown help type: ", type);
    }
  }

  _stop() {
    this._crossbench.stop();
  }

  _copyOutput(selector) {
    navigator.clipboard.writeText($(selector).value);
  }

  _updateStatus(status) {
    if (this._status === status) return;
    document.body.className = status;
    $("#status").value = status;
    if (status === Status.DISCONNECTED) {
      this._stopPinger();
    } else if (status === Status.CONNECTING) {
      this._checkStatusTransition(Status.DISCONNECTED, status);
    } else if (status === Status.CONNECTED) {
      if (this._status !== Status.RUNNING) {
        this._checkStatusTransition(Status.CONNECTING, status);
        this._ensurePinger();
      }
    } else if (status === Status.RUNNING) {
      this._clearOutput();
      this._checkStatusTransition(Status.CONNECTED, status);
    } else {
      console.error("Unknown status: ", status);
    }
    this._status = status;
  }

  _checkStatusTransition(from, to) {
    if (this._status != from) {
      console.error(`Invalid status transition ${this._status} => ${to};`);
    }
  }

  _stopPinger() {
    clearInterval(this._pingIntervalID);
    this._pingIntervalID = undefined;
  }

  _ensurePinger() {
    if (!this._crossbench || this._pingIntervalID) return;
    this._pingIntervalID = setInterval(this._ping.bind(this), 1000);
  }

  _ping() {
    if (!this._crossbench || !this._crossbench.isConnected) {
      this._updateStatus(Status.DISCONNECTED);
    }
  }

  _appendOutput({ stdout = "", stderr = "" }) {
    const stdoutNode = $("#outputStdout");
    const stderrNode = $("#outputStderr");
    stdoutNode.value += stdout;
    stderrNode.value += stderr;
    stdoutNode.scrollTop = stdoutNode.scrollHeight;
    stderrNode.scrollTop = stderrNode.scrollHeight;
  }

  _reconnect() {
    if (this._crossbench) {
      this._crossbench.stop();
    }

    const portNode = $("#port");
    const tokenNode = $("#token");
    const port = portNode.value;
    const token = tokenNode.value;

    let success = true;
    if (port.length < 4 || !(parseInt(port) > 1024)) {
      success = false;
      portNode.className = "error";
    }
    if (token.length != 32) {
      success = false;
      tokenNode.className = "error";
    }

    // Try connecting to the local crossbench proxy server:
    this._clearOutput();
    this._stopPinger();
    try {
      this._crossbench = new CrossbenchRecorder(
        "localhost", parseInt(port), token,
        this._onCrossbenchMessage.bind(this),
        this._onConnectionFail.bind(this));
      this._crossbench.start();
      this._updateStatus(Status.CONNECTING);
    } catch (e) {
      success = false;
      portNode.className = "error";
      tokenNode.className = "error";
      this._crossbench = undefined;
      console.error(e);
      this._updateStatus(Status.DISCONNECTED);
    }
    localStorage.setItem("crossbenchPort", port);
    localStorage.setItem("crossbenchToken", token);
    if (!success) return;
    portNode.className = "";
    tokenNode.className = "";
  }

  _onConnectionFail(e) {
    const websocket = e.target;
    const errorMessage = [
      `Connection to WebSocket at ${websocket.url} failed.`,
      "Did you run `./cb.py devtools-recorder-proxy`?"
    ].join("\n");
    this._appendOutput({ stderr: errorMessage });
    this._updateStatus(Status.DISCONNECTED);
  }

  _onCrossbenchMessage(isSuccess, command, payload, error) {
    console.log("Response: ", { isSuccess, command, payload, error });
    if (!isSuccess) {
      console.error(error);
      this._appendOutput({ stderr: error });
      if (error == "AuthenticationError") {
        this._onAuthenticationError();
      }
      return;
    }
    if (command == "status") return this._updateStatus(payload);
    if (command == "output") return this._appendOutput(payload);
  }

  _onAuthenticationError() {
    this._updateStatus(Status.DISCONNECTED);
  }

  _onChromeMessage(request, sender, sendResponse) {
    if (request === "stop") return this._stop();
    this._recorderJSON = JSON.parse(request);
    localStorage.setItem("recordingJSON", request);
    this._updateRecording();
  }
}

globalThis.ui = new UI();

