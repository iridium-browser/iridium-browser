/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import 'google3/third_party/javascript/cros_components/icon/icon';

import {ripple} from '@material/web/ripple/directive.js';
import {MdRipple} from '@material/web/ripple/ripple.js';
import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';
import {customElement, property, query, queryAsync} from 'lit/decorators';
import {ifDefined} from 'lit/directives/if-defined';
import {styleMap} from 'lit/directives/style-map';

import {isSidenav, isSidenavItem} from './sidenav_util';

/** Type of the tree item expanded custom event. */
export type TreeItemExpandedEvent = CustomEvent<{
  /** The tree item which has been expanded. */
  item: SidenavItem,
}>;
/** Type of the tree item collapsed custom event. */
export type TreeItemCollapsedEvent = CustomEvent<{
  /** The tree item which has been collapsed. */
  item: SidenavItem,
}>;
/** Type of the tree item collapsed custom event. */
export type TreeItemRenamedEvent = CustomEvent<{
  /** The tree item which has been renamed. */
  item: SidenavItem,
  /** The label before rename. */
  oldLabel: string,
  /** The label after rename. */
  newLabel: string,
}>;
/** Type of the tree item enabled changed custom event. */
export type TreeItemEnabledChangedEvent = CustomEvent<{
  /** The tree item which has been (un)enabled. */
  item: SidenavItem,
  /** The new enabled value. */
  enabled: boolean,
}>;

/** The number of pixels to indent per level. */
export const TREE_ITEM_INDENT_PX = 20;

/** The width and height of the expand and label icons. */
const ICON_SIZE = css`20px`;

/**
 * Spacing between items in the a row (in LTR):
 *   1. [ICON_LEADING_MARGIN]<icon...>[ICON_LABEL_GAP]<label>[LABEL_ONLY_MARGIN]
 *   2. [LABEL_ONLY_MARGIN]<label>[LABEL_ONLY_MARGIN]
 *
 *   1. With any icon:
 *     * 28px from left
 *     * 0px between icons
 *     * 8px from icon to label
 *   2. Without any icon:
 *     * 24px from left
 */
const ICON_LABEL_GAP = css`8px`;
const ICON_LEADING_MARGIN = css`28px`;
const LABEL_ONLY_MARGIN = css`24px`;

/** An item for a ChromeOS compliant sidenav element. */
@customElement('cros-sidenav-item')
export class SidenavItem extends LitElement {
  /**
   * The `separator` attribute will show a top border for the tree item. It's
   * mainly used to identify this tree item is a start of the new section.
   */
  @property({type: Boolean, reflect: true}) separator = false;

  /**
   * Indicate if a tree item is disabled or not. Disabled tree item will have
   * a greyed out color, can't be enabled, can't get focus. It can still have
   * children, but it can't be expanded, and the expand icon will be hidden.
   */
  @property({type: Boolean, reflect: true}) disabled = false;

  /** Indicate if a tree item has been enabled or not. */
  @property({type: Boolean, reflect: true}) enabled = false;

  /** Indicate if a tree item has been expanded or not. */
  @property({type: Boolean, reflect: true}) expanded = false;

  /** Indicate if a tree item is in editing mode (rename) or not. */
  @property({type: Boolean, reflect: true}) editing = false;

  /**
   * A tree item will have children if the child tree items have been inserted
   * into its default slot. Only use `mayHaveChildren` if we want the tree item
   * to appear as having children even without the actual child tree items
   * (e.g. no DOM children). This is mainly used when we asynchronously load
   * child tree items.
   */
  @property({type: Boolean, reflect: true, attribute: 'may-have-children'})
  mayHaveChildren = false;

  /** The label text of the tree item. */
  @property({type: String, reflect: true}) label = '';

  /**
   * If the user navigates to this item, tabs away from the sidenav and then
   * tabs back into the sidenav, the item that receives focus should be the one
   * they had navigated to. `tabIndex` is forwarded to the ` < li > ` element.
   */
  @property({attribute: false}) override tabIndex: number = -1;

  /**
   * Indicate the depth of this tree item, we use it to calculate the padding
   * indentation. Note: "aria-level" can be calculated by DOM structure so
   * no need to provide it explicitly.
   */
  @property({type: Number, reflect: true}) layer = 0;

  @query('li') private readonly treeItemElement!: HTMLLIElement;
  @query('.rename') private readonly renameInputElement?: HTMLInputElement;
  @query('slot:not([name])')
  private readonly childrenSlotElement!: HTMLSlotElement;
  @queryAsync('md-ripple') protected ripple!: Promise<MdRipple|null>;

  static override styles: CSSResultGroup = css`
    ul {
      list-style: none;
      margin: 0;
      outline: none;
      padding: 0;
    }

    li {
      display: block;
    }

    li:focus-visible {
      outline: none;
    }

    :host([separator])::before {
      border-bottom: 1px solid var(--cros-separator-color);
      content: '';
      display: block;
      margin: 8px 0;
      width: 100%;
    }

    .tree-row,
    .md-ripple {
      align-items: center;
      border-inline-start-width: 0 !important;
      border-radius: 20px;
      box-sizing: border-box;
      color: var(--cros-sys-on_surface);
      cursor: pointer;
      display: flex;
      height: 40px;
      margin: 8px 0;
      position: relative;
      user-select: none;
      white-space: nowrap;
    }

    li:focus-visible .tree-row {
      outline: 2px solid var(--cros-sys-focus_ring);
      outline-offset: 2px;
      z-index: 2;
    }

    :host([disabled]) .tree-row {
      pointer-events: none;
      color: var(--cros-sys-disabled);
    }

    :host([enabled]) .tree-row {
      background-color: var(--cros-sys-primary);
      color: var(--cros-sys-on_primary);
    }

    :host-context(.pointer-active) .tree-row:not(:active) {
      cursor: default;
    }

    .expand-icon {
      fill: currentcolor;
      flex: none;
      height: 0px;
      margin-inline-start: calc(${LABEL_ONLY_MARGIN} - ${ICON_LABEL_GAP});
      -webkit-mask-position: center;
      -webkit-mask-repeat: no-repeat;
      position: relative;
      transform: rotate(-90deg);
      transition: all 150ms;
      visibility: hidden;
      width: 0px;
    }

    li[aria-expanded] .expand-icon {
      height: ${ICON_SIZE};
      margin-inline-start: ${ICON_LEADING_MARGIN};
      visibility: visible;
      width: ${ICON_SIZE};
    }

    .expand-icon + slot[name="icon"]::slotted(*) {
      margin-inline-start: calc(${ICON_LEADING_MARGIN} - (${
      LABEL_ONLY_MARGIN} - ${ICON_LABEL_GAP}));
    }

    li[aria-expanded] .expand-icon + slot[name="icon"]::slotted(*) {
      margin-inline-start: 0px;
    }

    :host-context(html[dir=rtl]) .expand-icon {
      transform: rotate(90deg);
    }

    :host([expanded]) .expand-icon {
      transform: rotate(0);
    }

    slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-on_surface);
      flex: none;
      height: ${ICON_SIZE};
      width: ${ICON_SIZE};
    }

    :host([enabled]) slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-on_primary)
    }

    :host([disabled]) slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-disabled);
    }

    .tree-label {
      display: block;
      flex: auto;
      font-weight: 500;
      margin-inline-end: ${LABEL_ONLY_MARGIN};
      margin-inline-start: ${ICON_LABEL_GAP};
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: pre;
    }

    .rename {
      background-color: var(--cros-sys-app_base);
      border: none;
      border-radius: 4px;
      color: var(--cros-sys-on_surface);
      height: 20px;
      margin: 0 10px;
      outline: 2px solid var(--cros-sys-focus_ring);
      overflow: hidden;
      padding: 1px 8px;
    }

    /* We need to ensure that even empty labels take up space */
    .tree-label:empty::after {
      content: ' ';
      white-space: pre;
    }

    .tree-children {
      display: none;
    }

    :host([expanded]) .tree-children {
      display: block;
    }

    :host([enabled]) .rename {
      outline: 2px solid var(--cros-sys-inverse_primary);
    }

    .rename::selection {
      background-color: var(--cros-sys-highlight_text)
    }

    md-ripple {
      border-radius: 20px;
    }

    :host(:not([editing])) md-ripple {
      color: var(--cros-sys-ripple_primary);
      --md-ripple-hover-color: var(--cros-sys-hover_on_subtle);
      --md-ripple-hover-opacity: 100%;
      --md-ripple-pressed-color: var(--cros-sys-ripple_primary);
      --md-ripple-pressed-opacity: 100%;
    }
  `;

  static get events() {
    return {
      /** Triggers when a sidenav item has been expanded. */
      SIDENAV_ITEM_EXPANDED: 'sidenav_item_expanded',
      /** Triggers when a sidenav item has been collapsed. */
      SIDENAV_ITEM_COLLAPSED: 'sidenav_item_collapsed',
      /** Triggers when a sidenav item's label has been renamed. */
      SIDENAV_ITEM_RENAMED: 'sidenav_item_renamed',
      /** Triggers when a sidenav item's changed status changed. */
      SIDENAV_ITEM_ENABLED_CHANGED: 'sidenav_item_enabled_changed',
    } as const;
  }

  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  /** The child tree items which can be tabbed to. */
  get selectableItems(): SidenavItem[] {
    return this.items.filter(item => !item.disabled);
  }

  hasChildren(): boolean {
    return this.mayHaveChildren || this.items.length > 0;
  }

  /**
   * Return the parent SidenavItem, if there is one. For top level SidenavItems
   * with no parent, return null.
   */
  get parentItem(): SidenavItem|null {
    let p = this.parentElement;
    while (p) {
      if (isSidenavItem(p)) {
        return p;
      }
      if (isSidenav(p)) {
        return null;
      }
      p = p.parentElement;
    }
    return p;
  }

  /** Expands all parent items. */
  reveal() {
    let pi = this.parentItem;
    while (pi) {
      pi.expanded = true;
      pi = pi.parentItem;
    }
  }

  /**
   * The child tree items.
   * TODO(b/262453851): Should be private, and accessible to tests.
   */
  items: SidenavItem[] = [];

  /** Indicate if we should commit the rename on input blur or not. */
  private shouldRenameOnBlur = true;

  override render() {
    const showExpandIcon = this.hasChildren();
    const treeRowStyles = {
      paddingInlineStart:
          `max(0px, calc(${TREE_ITEM_INDENT_PX} * ${this.layer}px))`,
    };

    return html`
      <li
          class="tree-item"
          role="treeitem"
          tabindex=${this.tabIndex}
          aria-labelledby="tree-label"
          aria-selected=${this.enabled}
          aria-expanded=${ifDefined(showExpandIcon ? this.expanded : undefined)}
          aria-disabled=${this.disabled}>
        <div
            class="tree-row"
            style=${styleMap(treeRowStyles)}
            ${ripple(this.ripple)}>
          <md-ripple></md-ripple>
          <!-- TODO(b/262453851): Implement icon from spec -->
          <span class="expand-icon">
            <svg  xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 20 20">
              <polyline points="6 8 10 12 14 8 6 8"/>
            </svg>
          </span>
          <slot name="icon"></slot>
          ${this.renderTreeLabel()}
        </div>
        <ul
            class="tree-children"
            role="group">
          <slot @slotchange=${this.onSlotChanged}></slot>
        </ul>
       </li>
     `;
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties);
    if (changedProperties.has('expanded')) {
      this.onExpandChanged();
    }
    if (changedProperties.has('enabled')) {
      this.onEnabledChanged();
    }
    if (changedProperties.has('editing')) {
      this.onEditingChanged();
    }
  }

  private renderTreeLabel() {
    if (this.editing) {
      // Stop propagation of some events to prevent them being captured by
      // tree when the tree item is in editing mode.
      const interceptPropagation = (e: MouseEvent) => {
        e.stopPropagation();
      };
      return html`
        <input
          class="rename"
          type="text"
          spellcheck="false"
          .value=${this.label}
          @click=${interceptPropagation}
          @dblclick=${interceptPropagation}
          @mouseup=${interceptPropagation}
          @mousedown=${interceptPropagation}
          @blur=${this.onRenameInputBlur}
          @keydown=${this.onRenameInputKeydown}
        />
      `;
    }
    return html`
    <span
      class="tree-label"
      id="tree-label"
    >${this.label || ''}</span>`;
  }

  // TODO(b/262453851): There's probably a nicer way to implement this.
  private onSlotChanged() {
    // Whether the old set of children contained the enabled item.
    const oldItemsContainEnabledItem =
        this.items.some((item: SidenavItem) => item.enabled);
    // Update `items` every time when the children slot changes (e.g.
    // add/remove).
    this.items =
        this.childrenSlotElement.assignedElements().filter(isSidenavItem);

    let updateScheduled = false;

    // If an expanded item's last children is deleted, update expanded property.
    if (this.items.length === 0 && this.expanded) {
      this.expanded = false;
      updateScheduled = true;
    }

    if (oldItemsContainEnabledItem &&
        !this.items.some((item: SidenavItem) => item.enabled)) {
      // If the currently enabled item was in the old set of children but not in
      // the new set, it means it's being removed from the children slot. The
      // parent becomes the enabled item.
      this.enabled = true;
      updateScheduled = true;
    }

    for (const item of this.items) {
      item.layer = this.layer + 1;
    }

    // Explicitly trigger an update because render() relies on hasChildren().
    if (!updateScheduled) {
      this.requestUpdate();
    }
  }

  private onExpandChanged() {
    if (this.expanded) {
      const expandedEvent: TreeItemExpandedEvent =
          new CustomEvent(SidenavItem.events.SIDENAV_ITEM_EXPANDED, {
            bubbles: true,
            composed: true,
            detail: {item: this},
          });
      this.dispatchEvent(expandedEvent);
    } else {
      const collapseEvent: TreeItemCollapsedEvent =
          new CustomEvent(SidenavItem.events.SIDENAV_ITEM_COLLAPSED, {
            bubbles: true,
            composed: true,
            detail: {item: this},
          });
      this.dispatchEvent(collapseEvent);
    }
  }

  private onEnabledChanged() {
    const event: TreeItemEnabledChangedEvent =
        new CustomEvent(SidenavItem.events.SIDENAV_ITEM_ENABLED_CHANGED, {
          bubbles: true,
          composed: true,
          detail: {item: this, enabled: this.enabled},
        });
    this.dispatchEvent(event);
  }

  private onEditingChanged() {
    if (this.editing) {
      this.renameInputElement?.focus();
      this.renameInputElement?.select();
    }
  }

  private onRenameInputKeydown(e: KeyboardEvent) {
    // Make sure that the tree does not handle the key.
    e.stopPropagation();

    if (e.repeat) {
      return;
    }

    // Calling this.focus() blurs the input which will make the tree item
    // non editable.
    switch (e.key) {
      case 'Escape':
        // By default blur() will trigger the rename, but when ESC is pressed
        // we don't want the blur() (triggered by treeItemElement.focus() below)
        // to commit the rename.
        this.shouldRenameOnBlur = false;
        this.treeItemElement.focus();
        e.preventDefault();
        break;
      case 'Enter':
        // treeItemElement.focus() will trigger blur() for the rename input
        // which will commit the rename.
        this.treeItemElement.focus();
        e.preventDefault();
        break;
      default:
        break;
    }
  }

  private onRenameInputBlur() {
    this.editing = false;
    if (this.shouldRenameOnBlur) {
      this.commitRename(this.renameInputElement?.value || '');
    } else {
      this.shouldRenameOnBlur = true;
    }
  }

  private commitRename(newName: string) {
    const isEmpty = newName.trim() === '';
    const isChanged = newName !== this.label;
    if (isEmpty || !isChanged) {
      return;
    }
    const oldLabel = this.label;
    this.label = newName;
    const renameEvent: TreeItemRenamedEvent =
        new CustomEvent(SidenavItem.events.SIDENAV_ITEM_RENAMED, {
          bubbles: true,
          composed: true,
          detail: {item: this, oldLabel, newLabel: newName},
        });
    this.dispatchEvent(renameEvent);
  }
}

declare global {
  interface HTMLElementEventMap {
    [SidenavItem.events.SIDENAV_ITEM_EXPANDED]: TreeItemExpandedEvent;
    [SidenavItem.events.SIDENAV_ITEM_COLLAPSED]: TreeItemCollapsedEvent;
    [SidenavItem.events.SIDENAV_ITEM_RENAMED]: TreeItemRenamedEvent;
    [SidenavItem.events.SIDENAV_ITEM_ENABLED_CHANGED]:
        TreeItemEnabledChangedEvent;
  }

  interface HTMLElementTagNameMap {
    'cros-sidenav-item': SidenavItem;
  }
}
