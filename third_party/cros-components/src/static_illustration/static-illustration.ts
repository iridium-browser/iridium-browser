/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {html, LitElement} from 'lit';
import {customElement} from 'lit/decorators';

/** A static illustration for SVG illustrations. */
@customElement('cros-static-illustration')
export class StaticIllustration extends LitElement {
  override render() {
    return html`TBD`;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-static-illustration': StaticIllustration;
  }
}
