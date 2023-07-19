/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/slider/slider.js';

import {MdSlider} from '@material/web/slider/slider.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';

// tslint:disable:no-any needed for JS interface
const renderTrack = (MdSlider.prototype as any).renderTrack;
(MdSlider.prototype as any).renderTrack = function() {
  return html`<slot name="track">${renderTrack.call(this)}</slot>`;
};

/**
 * Styles needed to achieve transparent border around handle for disabled state.
 * Border radius for upper and lower ensures the track is rounded at the end but
 * not next to the handle.
 * Width is calculated given slider lower/upper fraction (the part of the
 * track that is currently active/inactive depending on value) minus the handle
 * gap (half of the handle width + width of transparent gap on one side).
 */
const DISABLED_STATE_OVERRIDES = css`
  md-slider[disabled] {
    --md-slider-pressed-handle-color: var(--disabled-color);
  }
  .disabledStyle > [slot="track"] {
    display: flex;
    align-items: center;
    position: absolute;
    inset: 0 calc(var(--md-slider-state-layer-size) / 2);
    --handle-margin: 2px;
    --handle-gap: calc(var(--handle-margin) + (var(--md-slider-handle-width) / 2));
  }
  .lower, .upper {
    position: absolute;
    height: 4px;
    background: var(--disabled-color);
  }
  .lower {
      border-radius: 9999px 0 0 9999px;
      left: 0;
      width: calc((var(--slider-upper-fraction)) * 100% - var(--handle-gap));
  }
  .upper {
      border-radius: 0 9999px 9999px 0;
      right: 0;
      width: calc((1 - var(--slider-upper-fraction)) * 100% - var(--handle-gap));
  }
`;
/**
 * A cros compliant slider component.
 * See spec:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2978%3A19626
 */
export class Slider extends LitElement {
  // TODO(b/279854770): check disabled styling.
  static override styles: CSSResultGroup = [
    css`
    :host {
      display: inline-block;
    }
    md-slider {
      display: block;
      --active-disabled: var(--cros-ref-neutral10);
      --inactive-disabled: var(--cros-ref-neutral10);
      --disabled-color: var(--inactive-disabled);
      --md-focus-ring-color: var(--cros-sys-primary);
      --md-slider-active-track-color: var(--cros-sys-primary);
      --md-slider-disabled-active-track-color: var(--disabled-color);
      --md-slider-disabled-active-track-opacity: var(--cros-sys-opacity-disabled);
      --md-slider-disabled-handle-color: var(--disabled-color);
      --md-slider-disabled-inactive-track-color: var(--disabled-color);
      --md-slider-disabled-inactive-track-opacity: var(--cros-sys-opacity-disabled);
      --md-slider-focus-handle-color: var(--cros-sys-primary);
      --md-slider-handle-color: var(--cros-sys-primary);
      --md-slider-handle-height: 12px;
      --md-slider-handle-width: 12px;
      --md-slider-hover-handle-color: var(--cros-sys-primary);
      --md-slider-hover-state-layer-opacity: 0;
      --md-slider-inactive-track-color: var(--cros-sys-primary_container);
      --md-slider-label-container-color: var(--cros-sys-primary);
      --md-slider-label-container-height: 18px;
      --md-slider-label-label-text-type: var(--cros-typography-label1);
      --md-slider-pressed-handle-color: var(--cros-sys-primary);
      --md-slider-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-slider-pressed-state-layer-opacity: 1;
      --md-slider-state-layer-size: 28px;
      --md-slider-with-tick-marks-active-container-color: var(--cros-sys-on_primary);
      --md-slider-with-tick-marks-disabled-container-color: var(--disabled-color);
      --md-slider-with-tick-marks-inactive-container-color: var(--cros-sys-primary);
    }
  `,
    DISABLED_STATE_OVERRIDES
  ];

  /** @nocollapse */
  static override properties = {
    value: {type: Number},
    disabled: {type: Boolean},
    withTickMarks: {type: Boolean},
    withLabel: {type: Boolean},
  };

  /** @export */
  value: number;
  /** @export */
  disabled: boolean;
  /** @export */
  withTickMarks: boolean;
  /** @export */
  withLabel: boolean;

  constructor() {
    super();
    this.value = 0;
    this.disabled = false;
    this.withTickMarks = false;
    this.withLabel = false;
  }

  override firstUpdated() {
    const slider = this.shadowRoot!.querySelector('md-slider')!;
    // Using unicode non-breaking space U-00A0, charCode 160. This is to add
    // padding on either side of the label text.
    const space = `  `;
    const updateValueLabel = () => slider.valueLabel =
        `${space}${slider.value}${space}`;
    updateValueLabel();
  }

  override render() {
    if (this.disabled) {
      return html`
        <md-slider class="disabledStyle" disabled value=${
          this.value} max=10 ?withTickMarks=${this.withTickMarks}>
          <div slot="track">
            <div class="lower"></div>
            <div class="upper"></div>
          </div>
        </md-slider>`;
    } else {
      return html`
        <md-slider value=${this.value} max=10 ?withTickMarks=${
          this.withTickMarks} ?withLabel=${this.withLabel}></md-slider>
      `;
    }
  }
}

customElements.define('cros-slider', Slider);

declare global {
  interface HTMLElementTagNameMap {
    'cros-slider': Slider;
  }
}
