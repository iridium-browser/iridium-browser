/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/menu.js';

import {Corner, DefaultFocusState} from '@material/web/menu/menu.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';

export {Corner, DefaultFocusState} from '@material/web/menu/menu.js';

/**
 * A chromeOS menu component.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2650%3A7994&t=01NOG3FTGuaigSvB-0
 */
export class Menu extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-menu {
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      --md-list-container-color: var(--cros-sys-on_primary);
      --md-menu-item-list-item-container-color: var(--cros-sys-on_primary);
      --md-list-item-list-item-label-text-type: var(--cros-typography-button2);
      --md-list-item-list-item-one-line-container-height: 36px;
      --md-menu-container-shape: 8px;
      --md-menu-container-surface-tint-layer-color: white;
    }
  `;
  /** @nocollapse */
  static override properties = {
    anchor: {type: Object, attribute: false},
    anchorCorner: {type: String, attribute: 'anchor-corner'},
    hasOverflow: {type: Boolean, attribute: 'has-overflow'},
    open: {type: Boolean},
    quick: {type: Boolean},
    menuCorner: {type: String, attribute: 'menu-corner'},
    defaultFocus: {type: String, attribute: 'default-focus'},
    skipRestoreFocus: {type: Boolean, attribute: 'skip-restore-focus'},
  };
  /** @nocollapse */
  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  anchor: HTMLElement|null;
  anchorCorner: Corner;
  hasOverflow: boolean;
  /**
   * Sets the initial state of the md-menu on first render. In general do not
   * set this and instead use show() and close().
   */
  open: boolean;

  // The following properties are necessary for wrapping and keyboard
  // navigation
  /**
   * Skips the opening and closing animations.
   */
  quick: boolean;
  /**
   * The corner of the menu which to align the anchor in the standard logical
   * property style of <block>_<inline>.
   */
  menuCorner: Corner;
  /**
   * The element that should be focused by default once opened.
   */
  defaultFocus: DefaultFocusState;
  /**
   * After closing, does not restore focus to the last focused element before
   * the menu was opened.
   */
  skipRestoreFocus: boolean;

  constructor() {
    super();
    this.anchor = null;
    this.anchorCorner = 'END_START';
    this.hasOverflow = false;
    this.open = false;
    this.quick = false;
    this.menuCorner = 'START_START';
    this.defaultFocus = 'LIST_ROOT';
    this.skipRestoreFocus = false;
  }

  /**
   * The menu items associated with this menu. The items must be `MenuItem`s and
   * have both the `md-menu-item` and `md-list-item` attributes.
   */
  get items() {
    // This accessor is required for keyboard navigation
    const menu = this.renderRoot.querySelector('md-menu');
    if (!menu) {
      return [];
    }

    return menu.items;
  }

  override render() {
    return html`
      <md-menu
          .anchor=${this.anchor}
          .open=${this.open}
          .hasOverflow=${this.hasOverflow}
          .anchorCorner=${this.anchorCorner}
          .menuCorner=${this.menuCorner}
          .quick=${this.quick}
          .defaultFocus=${this.defaultFocus}
          .skipRestoreFocus=${this.skipRestoreFocus}
          @closed=${this.close}
          @opened=${this.show}
          @closing=${this.onClosing}
          @opening=${this.onOpening}>
        <slot></slot>
      </md-menu>
    `;
  }

  show() {
    this.open = true;
    // these non-bubbling events need to be re-dispatched for keyboard
    // navigation to work on submenus
    this.dispatchEvent(new Event('opened'));
  }

  close() {
    this.open = false;
    this.dispatchEvent(new Event('closed'));
  }

  private onClosing() {
    this.dispatchEvent(new Event('closing'));
  }

  private onOpening() {
    this.dispatchEvent(new Event('opening'));
  }

  // This is just in case `delegatesFocus` doesn't do what is intended (which is
  // unfortunately often across browser updates).
  override focus() {
    this.renderRoot.querySelector('md-menu')?.focus();
  }
}

customElements.define('cros-menu', Menu);

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu': Menu;
  }
}
