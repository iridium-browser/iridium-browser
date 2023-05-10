/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {assertExists} from 'google3/javascript/common/asserts/asserts';
import {waitForEvent} from 'google3/third_party/javascript/cros_components/async_helpers/async_helpers';
import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement, property, query} from 'lit/decorators';
import {trustedResourceUrl} from 'safevalues';
import {safeWorker} from 'safevalues/dom';

const WORKER_BINARY = trustedResourceUrl`/js/lottie_worker.js`;

interface MessageData {
  animationData: JSON;
  drawSize: {width: number, height: number};
  params: {loop: boolean, autoplay: boolean};
  canvas?: OffscreenCanvas;
}

/** The CustomEvent names that LottieRenderer can fire. */
export enum CrosLottieEvent {
  INITIALIZED = `cros-lottie-initialized`,
  PAUSED = `cros-lottie-paused`,
  PLAYING = `cros-lottie-playing`,
  RESIZED = `cros-lottie-resized`,
  STOPPED = `cros-lottie-stopped`,
}

interface ResizeDetail {
  width: number;
  height: number;
}

/**
 * Lottie renderer with correct hooks to handle dynamic color. This component
 * currently only supports static illustrations, but will eventually support
 * animations via an offscreen canvas.
 */
@customElement('cros-lottie-renderer')
export class LottieRenderer extends LitElement {
  /** The path to the lottie asset JSON file. */
  @property({attribute: true}) assetUrl!: string;
  /** When true, animation will begin to play as soon as it's loaded. */
  @property({attribute: true, type: Boolean}) autoplay = true;

  /** When true, animation will loop continuously. */
  @property({attribute: true, type: Boolean}) loop = true;

  /** The onscreen canvas controlled by lottie_worker.js. */
  @query('#onscreen-canvas') onscreenCanvas!: HTMLCanvasElement;

  /** The worker thread running the lottie library. */
  private worker: Worker|null = null;
  private offscreenCanvas: OffscreenCanvas|null = null;

  /** If the offscreen canvas has been transferred to the Worker thread. */
  private hasTransferredCanvas = false;

  /**
   * Animations are loaded asynchronously, the worker will send an event when
   * it has finished loading.
   */
  private animationIsLoaded = false;

  /**
   * If the canvas is resized before an animation has finished initializing,
   * we wait and send the size once it's loaded fully into the worker.
   */
  private workerNeedsSizeUpdate = false;

  /**
   * If there is a control command (play, pause, stop) before the animation has
   * finished initializing, it should be queued and completed after successful
   * initializaton.
   */
  private workerNeedsControlUpdate = false;
  private playStateInternal = false;

  /**
   * Propagates resize events from the onscreen canvas to the offscreen one.
   */
  private resizeObserver: ResizeObserver|null = null;

  override connectedCallback() {
    super.connectedCallback();
    if (!this.worker) {
      this.worker = safeWorker.create(WORKER_BINARY);
      this.worker.onmessage = (e) => void this.onMessage(e);
    }
  }

  override firstUpdated() {
    assertExists(this.onscreenCanvas);
    this.resizeObserver =
        new ResizeObserver(this.onCanvasElementResized.bind(this));
    this.resizeObserver.observe(this.onscreenCanvas);
    this.offscreenCanvas = this.onscreenCanvas.transferControlToOffscreen();
    this.initAnimation();
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    if (this.worker) {
      this.worker.terminate();
      this.worker = null;
    }
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
    }
  }

  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }

    canvas {
      height: 100%;
      width: 100%;
    }
  `;

  override render() {
    return html`
        <canvas id='onscreen-canvas'></canvas>
      `;
  }

  /**
   * Play the current animation, and wait for a successful response from the
   * worker thread. This promise will also wait until animation initialization
   * has completed before resolving.
   */
  async play(): Promise<Event> {
    return this.setPlayState(true, CrosLottieEvent.PLAYING);
  }

  /**
   * Pause the current animation, and wait for a successful response from the
   * worker thread. This promise will also wait until animation initialization
   * has completed before resolving.
   */
  async pause(): Promise<Event> {
    return this.setPlayState(false, CrosLottieEvent.PAUSED);
  }

  /**
   * Stop the current animation, and wait for a successful response from the
   * worker thread. Stopping the animation resets it to it's first frame, and
   * pauses it, so there's no need to wait for initialization.
   */
  async stop(): Promise<Event> {
    assertExists(this.worker);
    this.worker.postMessage({control: {stop: true}});
    return waitForEvent(this, CrosLottieEvent.STOPPED);
  }

  private async initAnimation() {
    assertExists(this.worker);
    const animationData = await fetch(this.assetUrl).then(r => r.json());
    const animationConfig: MessageData = ({
      animationData,
      drawSize: this.getCanvasDrawBufferSize(),
      params: {
        loop: this.loop,
        autoplay: this.autoplay,
      },
    });

    // The offscreen canvas can only be transferred across to the WebWorker
    // once, when we first initialize an animation. On subsequent
    // initializations (such as when the asset URL has changed), we avoid
    // re-transferring the canvas and just send across the animation data.
    // TODO(b/268419751): Add hooks for updating when the animation URL has
    // changed.
    if (this.hasTransferredCanvas) {
      this.worker.postMessage(animationConfig);
      return;
    }

    this.worker.postMessage(
        {...animationConfig, canvas: this.offscreenCanvas},
        [this.offscreenCanvas as unknown as Transferable]);
    this.hasTransferredCanvas = true;
  }

  /**
   * Computes the draw buffer size for the canvas. This ensures that the
   * rasterization is crisp and sharp rather than blurry.
   */
  private getCanvasDrawBufferSize() {
    const canvasElement = this.onscreenCanvas;
    const devicePixelRatio = window.devicePixelRatio;
    const clientRect = canvasElement.getBoundingClientRect();
    const drawSize = {
      width: clientRect.width * devicePixelRatio,
      height: clientRect.height * devicePixelRatio,
    };
    return drawSize;
  }

  /**
   * Handles the canvas element resize event. If the animation isn't fully
   * loaded, the canvas size is sent later, once the loading is done.
   */
  private onCanvasElementResized() {
    if (!this.animationIsLoaded) {
      this.workerNeedsSizeUpdate = true;
      return;
    }
    this.sendCanvasSizeToWorker();
  }

  private sendCanvasSizeToWorker() {
    assertExists(this.worker);
    this.worker.postMessage({drawSize: this.getCanvasDrawBufferSize()});
  }

  private setPlayState(play: boolean, expectedEvent: CrosLottieEvent) {
    this.playStateInternal = play;
    if (!this.animationIsLoaded) {
      this.workerNeedsControlUpdate = true;
    } else {
      this.sendPlayControlToWorker();
    }
    return waitForEvent(this, expectedEvent);
  }

  private sendPlayControlToWorker() {
    assertExists(this.worker);
    this.worker.postMessage({control: {play: this.playStateInternal}});
  }

  private onMessage(event: MessageEvent) {
    const message = event.data.name;
    switch (message) {
      case 'initialized':
        this.animationIsLoaded = true;
        this.fire(CrosLottieEvent.INITIALIZED);
        this.sendPendingInfo();
        break;
      case 'playing':
        this.fire(CrosLottieEvent.PLAYING);
        break;
      case 'paused':
        this.fire(CrosLottieEvent.PAUSED);
        break;
      case 'stopped':
        this.fire(CrosLottieEvent.STOPPED);
        break;
      case 'resized':
        this.fire(CrosLottieEvent.RESIZED, event.data.size);
        break;
      default:
        console.log(`unknown message type received: ${message}`);
        break;
    }
  }

  private fire(event: CrosLottieEvent, detail: ResizeDetail|null = null) {
    this.dispatchEvent(
        new CustomEvent(event, {bubbles: true, composed: true, detail}));
  }

  /**
   * Called once the animation is fully loaded into the worker. Sends any
   * size or control information that may have arrived while the animation
   * was not yet fully loaded.
   */
  private sendPendingInfo() {
    if (this.workerNeedsSizeUpdate) {
      this.workerNeedsSizeUpdate = false;
      this.sendCanvasSizeToWorker();
    }
    if (this.workerNeedsControlUpdate) {
      this.workerNeedsControlUpdate = false;
      this.sendPlayControlToWorker();
    }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-lottie-renderer': LottieRenderer;
  }
}
