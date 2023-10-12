/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';

// TODO(b/295990177): Make these absolute.
import {waitForEvent} from '../async_helpers/async_helpers';
import {assertExists, hexToRgb} from '../helpers/helpers';

import {addColorSchemeChangeListener, removeColorSchemeChangeListener} from './event_binders';
import {getWorker} from './worker';

/**
 * The list of tokens that are used to identify shapes and colors in Lottie
 * animation data. If token names change, we will need to update this set.
 * Existing token names are very unlikely to change, it's more likely that new
 * tokens may be added if illustration palettes become more complex. There
 * is currently no easy way to generate this list in Google3, and as a small
 * set it can be manually maintained, but in future a more robust solution may
 * be needed. See go/cros-tokens (internal).
 */
const CROS_TOKENS = new Set<string>([
  'cros.sys.illo.color1',
  'cros.sys.illo.color1.1',
  'cros.sys.illo.color1.2',
  'cros.sys.illo.color2',
  'cros.sys.illo.color3',
  'cros.sys.illo.color4',
  'cros.sys.illo.color5',
  'cros.sys.illo.color6',
  'cros.sys.illo.base',
  'cros.sys.illo.secondary',

  /**
   * These are colors outside of the standard illo palette. Some animations
   * need to modify the base background color depending on the surface so
   * contain tokens for app base, card color etc,
   */
  'cros.sys.app_base',
  'cros.sys.app_base_shaded',
  'cros.sys.app_base_elevated',
  'cros.sys.illo.card.color4',
  'cros.sys.illo.card.on_color4',
  'cros.sys.illo.card.color3',
  'cros.sys.illo.card.on_color3',
  'cros.sys.illo.card.color2',
  'cros.sys.illo.card.on_color2',
  'cros.sys.illo.card.color1',
  'cros.sys.illo.card.on_color1',
]);

interface MessageData {
  animationData: JSON;
  drawSize: {width: number, height: number};
  params: {loop: boolean, autoplay: boolean};
  canvas?: OffscreenCanvas;
}

/**
 * This definition of this type is taken from
 * https://lottiefiles.github.io/lottie-docs/concepts/#colors
 */
interface LottieShape {
  /**
   * The name of a shape. For Material3 compliant animations, this will be the
   * token name of the color to apply.
   */
  nm: string;
  /**
   * Colors can be represented in one of two ways. The first and most often used
   * is as an array of four floats between 0 and 1 for RGB components and alpha.
   * The second and much more rarely used is with a hex string.
   */
  c?: {k: number[]};
  sc?: string;
}

/** String variant of the name field used for comparison during parsing. */
const LOTTIE_NAME_KEY = 'nm';

/**
 * A type for storing a css variable and a list of known shapes within the
 * currently loaded animation that have the css variable color applied. There
 * will be one of these structures per token in CROS_TOKENS that appears in the
 * animation data.
 */
interface TokenColor {
  cssVar: string;
  locations: LottieShape[];
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
 * Helper function for converting between the hexadecimal string we get from the
 * computed style to the format that lottie expects, which is an array of four
 * floats. Since these come directly from the computed style and color pipeline,
 * we can be confident that we are only going to be parsing 8 digit hexadecimal
 * strings.
 */
function convertHexToLottieRGBA(hexString: string): number[] {
  let r;
  let g;
  let b;
  let alpha;
  if (hexString.length === 9) {
    // Assume #rrggbbaa format.
    const hexRgb = hexString.slice(0, -2);
    const alphaString = hexString.slice(-2);
    [r, g, b] = hexToRgb(hexRgb);
    alpha = Number(`0x${alphaString}`);
  } else {
    // Assume #rrggbb format.
    [r, g, b] = hexToRgb(hexString);
    alpha = 255;
  }

  return [r / 255, g / 255, b / 255, alpha / 255];
}

/**
 * Takes in a cros token and returns the corresponding css variable.
 * Eg: cros.sys.illo.base -> --cros-sys-illo-base.
 */
function convertTokenToCssVariable(token: string) {
  return `--${(token).replaceAll('.', '-')}`;
}

/**
 * Traverses through a jsonObject, looking for known keys and tokens, and
 * saving them in the `shapes` map.
 */
function traverse(jsonObj: object, shapes: Map<string, TokenColor>) {
  if (jsonObj === null || typeof jsonObj !== 'object') return;

  for (const [key, value] of Object.entries(jsonObj)) {
    // If we are looking at something that contains a "nm" field that is set to
    // one of the known illustration tokens, this is a LottieShape that needs to
    // have its color value set.
    if (key === LOTTIE_NAME_KEY && CROS_TOKENS.has(value)) {
      const tokenName = value as string;
      if (!shapes.has(tokenName)) {
        shapes.set(
            tokenName,
            {cssVar: convertTokenToCssVariable(tokenName), locations: []});
      }
      const color = shapes.get(tokenName) as TokenColor;
      const shape = jsonObj as LottieShape;
      color.locations.push(shape);
    }

    traverse(value, shapes);
  }
}

/**
 * Lottie renderer with correct hooks to handle dynamic color. This component
 * should only be used for dynamic illustrations, as it involves spinning up an
 * instance of the Lottie library on a worker thread. For static illustrations,
 * it is more performant to use <cros-static-illustration>.
 */
export class LottieRenderer extends LitElement {
  /**
   * The path to the lottie asset JSON file.
   * @export
   */
  assetUrl: string;

  /**
   * When true, animation will begin to play as soon as it's loaded.
   * @export
   */
  autoplay: boolean;

  /**
   * When true, animation will loop continuously.
   * @export
   */
  loop: boolean;

  /**
   * Whether or not the illustration should render using a dynamic palette
   * taken from the computed style, or the default GM2 palette each asset comes
   * with.
   * @export
   */
  dynamic: boolean;

  /** The onscreen canvas controlled by lottie_worker.js. */
  get onscreenCanvas(): HTMLCanvasElement|null {
    return this.renderRoot.querySelector('#onscreen-canvas');
  }

  /**
   * Temporary public API to ensure component color resolution works.
   * TODO(b/290864323): Remove legacy usages of this function and then make
   * private.
   */
  onColorSchemeChanged = () => {
    if (!this.dynamic) return;
    this.refreshAnimationColors();
  };

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

  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: block;
    }

    canvas {
      height: 100%;
      width: 100%;
    }
  `;

  /** @nocollapse */
  static override properties = {
    assetUrl: {type: String, attribute: 'asset-url'},
    autoplay: {type: Boolean, attribute: true},
    loop: {type: Boolean, attribute: true},
    dynamic: {type: Boolean, attribute: true},
  };

  constructor() {
    super();
    this.assetUrl = '';
    this.autoplay = true;
    this.loop = true;

    // Once all apps are launching with dynamic colors, we can default this to
    // true, or remove this attribute altogether.
    this.dynamic = false;
  }

  /**
   * A copy of this.assetUrl that has the color token CSS variables replaced
   * with the hardcoded color strings based on the current color scheme.
   */
  private animationData: JSON|null = null;

  /**
   * Map from token name to a TokenColor. The TokenColor contains the CSS
   * variable name for the token, as well as a list of references to Lottie
   * animation sub objects, which need to be updated every time the color scheme
   * changes to avoid retraversing the JSON.
   */
  private readonly colors = new Map<string, TokenColor>();

  override connectedCallback() {
    super.connectedCallback();
    if (!this.worker) {
      this.worker = getWorker();
      this.worker.onmessage = (e) => void this.onMessage(e);
    }

    addColorSchemeChangeListener(this.onColorSchemeChanged);
  }

  override firstUpdated() {
    assertExists(
        this.onscreenCanvas,
        'Could not find <cavas> element in lottie-renderer');
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
    removeColorSchemeChangeListener(this.onColorSchemeChanged);
  }

  override updated(changedProperties: PropertyValues<LottieRenderer>) {
    super.updated(changedProperties);
    const prop = changedProperties.get('assetUrl');
    if (prop && prop !== undefined && this.worker) {
      this.assetUrlChanged();
    }
  }

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
    assertExists(this.worker, 'lottie-renderer has no web worker.');
    this.worker.postMessage({control: {stop: true}});
    return waitForEvent(this, CrosLottieEvent.STOPPED);
  }

  private async refreshAnimationColors() {
    // We must await update here to ensure the computed style will be up to date
    // with the new color scheme.
    // TODO(b/276079531): Investigate if this can be removed when using events.
    this.requestUpdate();
    await this.updateComplete;
    if (!this.animationData) {
      console.info('Refresh animation colors failed: No animation data.');
      return;
    }
    this.updateColorsInAnimationData();
    this.sendAnimationToWorker(this.animationData);
  }

  private sendAnimationToWorker(animationData: JSON) {
    // There are some edge cases where the renderer has been removed from the
    // DOM in between initialization and this function running, which causes
    // the worker to have been terminated. In these cases, we can just early
    // exit without attempting to send anything to the worker.
    if (!this.worker) {
      console.info('lottie-renderer has no web worker.');
      return;
    }

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
   * Load the animation data from `assetUrl` and begin the color token
   * resolution work. If no `assetUrl` was provided, we don't need to try and
   * fetch or send anything to the worker.
   */
  private async initAnimation() {
    if (!this.assetUrl) {
      console.info('No assetUrl provided.');
      return;
    }
    try {
      // In chromium you are not allowed to use `fetch` with `chrome://` URLs,
      // as such we use XMLHttpRequest here.
      this.animationData = await new Promise((resolve, reject) => {
        const req = new XMLHttpRequest();
        req.responseType = 'json';
        req.open('GET', this.assetUrl, true);
        req.onload = () => {
          resolve(req.response);
        };
        req.onerror = (err) => {
          reject(err);
        };
        req.send(null);
      });
    } catch (error) {
      console.info(`Unable to load JSON from ${this.assetUrl}`, error);
    }

    if (!this.animationData) {
      console.info('cros-lottie-renderer fetched null animation data');
      return;
    }

    // For apps using this component without the Jelly flag, they should ignore
    // the dynamic palette and just use the GM2 colors that are inside the
    // JSON file itself. This means we can skip populating the color map with
    // values, which means `updateColorsInAnimationData` will be a no-op.
    if (this.dynamic) {
      this.colors.clear();
      // When the animation is first loaded, it needs to be traversed via DFS to
      // find all shapes that are mapped to tokens, as these will need to
      // receive color updates. `colors` is the internal list of all known
      // shapes, and will be modified by this function.
      traverse(this.animationData, this.colors);

      if (this.colors.size === 0) {
        console.warn(
            `Unable to find cros.sys.illo tokens in ${this.assetUrl}. Please ` +
            `ensure this animation file has been run through the VisD token ` +
            `resolution script.`);
      }
    }

    this.updateColorsInAnimationData();
    this.sendAnimationToWorker(this.animationData);
  }

  /**
   * Go through the known list of shapes in the animation data that need to have
   * their colors updated, and use the current computed style to ensure they
   * match the current color scheme.
   */
  private updateColorsInAnimationData() {
    const computedStyle = getComputedStyle(this);
    for (const color of this.colors.values()) {
      const computedColor = computedStyle.getPropertyValue(color.cssVar).trim();
      const colorArray = convertHexToLottieRGBA(computedColor);
      for (const location of color.locations) {
        if (location.c) {
          location.c.k = colorArray;
        } else if (location.sc) {
          location.sc = computedColor;
        } else {
          console.info(
              `Unable to assign color to shape: ${JSON.stringify(location)}`);
        }
      }
    }
  }

  private async assetUrlChanged() {
    assertExists(this.worker, 'lottie-renderer has no web worker.');

    // If we have an already loaded animation, stop it from playing and
    // reinitialize.
    if (this.animationIsLoaded) {
      await this.stop();
    }
    await this.initAnimation();
  }

  /**
   * Computes the draw buffer size for the canvas. This ensures that the
   * rasterization is crisp and sharp rather than blurry.
   */
  private getCanvasDrawBufferSize() {
    const devicePixelRatio = window.devicePixelRatio;
    const clientRect = this.onscreenCanvas!.getBoundingClientRect();
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
    assertExists(this.worker, 'lottie-renderer has no web worker.');
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
    assertExists(this.worker, 'lottie-renderer has no web worker.');
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
        console.warn(`unknown message type received: ${message}`);
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

customElements.define('cros-lottie-renderer', LottieRenderer);

declare global {
  interface HTMLElementTagNameMap {
    'cros-lottie-renderer': LottieRenderer;
  }
}
