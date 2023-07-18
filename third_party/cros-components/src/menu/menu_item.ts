/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/menu-item';
import '../icon/icon';
import '../switch/switch';

import {MenuItem as MenuItemType} from '@material/web/menu/menu-item';
import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * A cros compliant menu-item component for use in cros-menu.
 */
export class MenuItem extends LitElement implements MenuItemType {
  /** @nocollapse */
  // TODO(b/198759625): Remove negative margin-inline-end when mwc token
  // available (padding 16px)
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    md-menu-item {
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      --md-list-item-list-item-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-list-item-list-item-hover-state-layer-opacity: 1;
      --md-list-item-list-item-label-text-type: var(--cros-typography-button2);
      --md-list-item-list-item-leading-icon-color: var(--cros-sys-on_surface);
      --md-list-item-list-item-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-list-item-list-item-pressed-state-layer-opacity: 1;
    }
    :host {
      display: inline block;
    }
    cros-icon {
      width: 20px;
      height: 20px;
      color: var(--cros-sys-on_surface);
    }
    cros-switch {
      width: 32px;
      height: 18px;
    }
    .slot-end {
      font: var(--cros-typography-button2);
    }
    #shortcut {
      color: var(--cros-sys-secondary);
    }
  `;

  static override shadowRootOptions = {
    mode: 'open' as const,
    delegatesFocus: true,
  };

  /** @nocollapse */
  static override properties = {
    headline: {type: String},
    itemStart: {type: String},
    itemEnd: {type: String},
    active: {type: Boolean, reflect: true},
    disabled: {type: Boolean},
    isMenuItem: {type: Boolean, reflect: true, attribute: 'md-menu-item'},
    isListItem: {type: Boolean, reflect: true, attribute: 'md-list-item'},
  };

  /**
   * Headline is the primary text of the list item, name follows from
   * md-menu-item.
   */
  headline: string;
  itemStart: 'icon'|'';
  itemEnd: 'shortcut'|'icon'|'switch'|null;

  // The following properties are necessary for wrapping and keyboard
  // navigation
  /**
   * Whether or not the menu-item is disabled.
   */
  disabled: boolean;
  readonly isMenuItem: boolean;
  readonly isListItem: boolean;
  /**
   * Whether or not the element is actively being interacted with by md-list.
   * When active, tabindex is set to 0, and focuses the underlying item.
   */
  active: boolean;

  constructor() {
    super();

    this.headline = '';
    this.itemStart = '';
    this.itemEnd = null;

    this.disabled = false;
    this.isMenuItem = true;
    this.isListItem = true;
    // NOTE: unlike md-sub-menu-item md-menu-item does not internally set
    // `.active`, so we do not require accessors to synchronize `this.active`
    // with `md-menu-item.active`
    this.active = false;
  }

  override render() {
    let slotEnd;
    switch (this.itemEnd) {
      case 'shortcut':
        // TODO(heidichan): expose shortcut text.
        slotEnd =
            html`<span slot="end" class="slot-end" id="shortcut">Shortcut</span>`;
        break;
      case 'icon':
        slotEnd = html`<cros-icon slot="end" class="slot-end"></cros-icon>`;
        break;
      case 'switch':
        slotEnd = html`<cros-switch slot="end" class="slot-end"></cros-switch>`;
        break;
      default:
        // md-list-item has a ::slotted(*) selector which will never select the
        // children of a <slot> or a <slot> itself which is why default content
        // is not set as children of slot.
        // i.e. <slot slot="end">${slotEnd}</slot>
        slotEnd = html`<slot slot="end"></slot>`;
        break;
    }

    const keepOpen = this.itemEnd === 'switch';
    let startItem = null;
    if (this.itemStart === 'icon') {
      startItem =
          html`<cros-icon slot="start" data-variant="icon"></cros-icon>`;
    }
    // keep-open property needs to be true in switch case so cros-switch can be
    // selected/unselected without menu closing.
    return html`
      <md-menu-item
          headline=${this.headline}
          .keepOpen=${keepOpen}
          .disabled=${this.disabled}
          .active=${this.active}>
        ${startItem}
        ${slotEnd}
      </md-menu-item>
      `;
  }

  // This is just in case `delegatesFocus` doesn't do what is intended (which is
  // unfortunately often across browser updates).
  override focus() {
    this.renderRoot.querySelector('md-menu-item')?.focus();
  }
}

customElements.define('cros-menu-item', MenuItem);

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu-item': MenuItem;
  }
}
