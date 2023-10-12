/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {StoriesRenderer} from 'google3/javascript/lit/stories/components/stories_renderer/stories-renderer';
import {css} from 'lit';
import {customElement} from 'lit/decorators';

/** Stories renderer that automatically applies cros styles. */
@customElement('cros-stories-renderer')
export class JellybeanStoriesRenderer extends StoriesRenderer {
  static override styles = [
    ...StoriesRenderer.styles,
    css`
      :host {
        background: var(--cros-sys-app_base);
        color: var(--cros-sys-primary);
        padding: 20px;
      }
      // TODO(b/296797338): Override knob panel styles.
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-stories-renderer': JellybeanStoriesRenderer;
  }
}
