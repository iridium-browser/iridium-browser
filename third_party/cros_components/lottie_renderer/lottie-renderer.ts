/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {html, LitElement, PropertyValues} from 'lit';
import {customElement, property, query} from 'lit/decorators';
import * as lottie from 'lottie_full';  // from //third_party/javascript/lottie:lottie_full

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
  @property({attribute: true, type: Boolean}) autoplay: boolean = false;

  /** When true, animation will loop continuously. */
  @property({attribute: true, type: Boolean}) loop: boolean = false;

  /**
   * When true, this indicates that the lottie file is animated, and hence
   * should use the offscreen canvas, rather than the SVGRenderer.
   */
  @property({attribute: true, type: Boolean}) animated: boolean = false;

  /** The container that lottie attaches an illustration to. */
  @query('#animation-container') animationContainer!: HTMLElement;

  /** The currently loaded lottie animation. */
  private currentAnimation: LottieAnimationItem|null = null;

  override render() {
    return html`
      <div id="animation-container">
        ${this.animated ? html`<canvas id="onscreen-canvas"></canvas>` : html``}
      </div>`;
  }

  override firstUpdated(changedProperties: PropertyValues) {
    super.firstUpdated(changedProperties);
    this.initAnimation();
  }

  override connectedCallback() {
    super.connectedCallback();
    if (!this.currentAnimation) return;
    this.currentAnimation.stop();
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (!this.currentAnimation) return;
    this.currentAnimation.stop();
  }

  /**
   * Initializes Lottie with animation at `this.assetURL`. For static
   * illustrations we use the SVGRenderer.
   * TODO(b/236660267): Add canvas rendering web worker for animations.
   */
  private initAnimation() {
    if (!this.animationContainer) return;
    if (this.animated) {
      throw new Error('Animations not yet implemented.');
    }
    this.currentAnimation = lottie.loadAnimation({
      container: this.animationContainer,
      renderer: 'svg',
      loop: this.loop,
      autoplay: this.autoplay,
      path: this.assetUrl,
    });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-lottie-renderer': LottieRenderer;
  }
}
