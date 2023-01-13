/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '../icon/icon';

import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';
import {customElement, property, query, state} from 'lit/decorators';
import {ifDefined} from 'lit/directives/if-defined';
import {styleMap} from 'lit/directives/style-map';

import {Sidenav} from './sidenav';
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

/** The number of pixels to indent per level. */
export const TREE_ITEM_INDENT_PX = 20;

/** The width and height of the expand and label icons. */
const ICON_SIZE = css`20px`;

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
   * Indicate the level of this tree item, we use it to calculate the padding
   * indentation. Note: "aria-level" can be calculated by DOM structure so
   * no need to provide it explicitly.
   */
  @state() level = 1;

  @query('li') private readonly treeItemElement!: HTMLLIElement;
  @query('.rename') private readonly renameInputElement?: HTMLInputElement;
  @query('slot:not([name])')
  private readonly childrenSlotElement!: HTMLSlotElement;

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

    .tree-row {
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

    :host(:not([enabled]):not([disabled]):not([editing]))
        li:not(:focus-visible) .tree-row:hover {
      background-color: var(--cros-sys-hover_on_subtle);
    }

    :host([enabled]) .tree-row {
      background-color: var(--cros-sys-primary);
      color: var(--cros-sys-on_primary);
    }

    :host-context(.pointer-active) .tree-row:not(:active) {
      cursor: default;
    }

    :host-context(.pointer-active):host(:not([enabled]):not([disabled]):not([editing]))
        li:not(:focus-visible) .tree-row:not(:active):hover {
      background-color: unset;
    }

    :host-context(.pointer-active):host(:not([enabled]):not([disabled]):not([editing]))
        li:not(:focus-visible) .tree-row:not(:hover):active {
      background-color: var(--cros-sys-hover_on_subtle);
    }

    .expand-icon {
      flex: none;
      height: ${ICON_SIZE};
      margin-inline-start: 28px;
      -webkit-mask-position: center;
      -webkit-mask-repeat: no-repeat;
      position: relative;
      transform: rotate(-90deg);
      transition: all 150ms;
      visibility: hidden;
      width: ${ICON_SIZE};
    }

    li[aria-expanded] .expand-icon {
      visibility: visible;
    }

    :host-context(html[dir=rtl]) .expand-icon {
      transform: rotate(90deg);
    }

    :host([expanded]) .expand-icon {
      transform: rotate(0);
    }

    slot[name="icon"]::slotted(*){
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
      margin-inline-start: 8px;
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
  `;

  static get events() {
    return {
      /** Triggers when a sidenav item has been expanded. */
      SIDENAV_ITEM_EXPANDED: 'sidenav_item_expanded',
      /** Triggers when a sidenav item has been collapsed. */
      SIDENAV_ITEM_COLLAPSED: 'sidenav_item_collapsed',
      /** Triggers when a sidenav item's label has been renamed. */
      SIDENAV_ITEM_RENAMED: 'sidenav_item_renamed',
    } as const;
  }

  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  /** The child tree items which can be tabbed to. */
  get tabbableItems(): SidenavItem[] {
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

  get tree(): Sidenav|null {
    let t = this.parentElement;
    while (t && !isSidenav(t)) {
      t = t.parentElement;
    }
    return t;
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
    const showExpandIcon = this.hasChildren() && !this.disabled;
    const treeRowStyles = {
      paddingInlineStart:
          `max(0px, calc(${TREE_ITEM_INDENT_PX} * ${this.level - 1}px))`,
    };

    return html`
      <li
        class="tree-item"
        role="treeitem"
        tabindex=${this.tabIndex}
        aria-labelledby="tree-label"
        aria-selected=${this.enabled}
        aria-expanded=${ifDefined(showExpandIcon ? this.expanded : undefined)}
        aria-disabled=${this.disabled}
      >
        <div
            class="tree-row"
            style=${styleMap(treeRowStyles)}>
          <!-- TODO(b/262453851): Implement ripple -->
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

  override connectedCallback() {
    super.connectedCallback();
    if (!this.tree) {
      throw new Error(
          '<cros-sidenav-item> can not be used without a parent <cros-sidenav>');
    }
  }

  override firstUpdated() {
    this.updateLevel();
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
    const oldItems = new Set(this.items);
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

    if (this.tree?.enabledItem) {
      const newItems = new Set(this.items);
      if (oldItems.has(this.tree.enabledItem) &&
          !newItems.has(this.tree.enabledItem)) {
        // If the currently enabled item exists in `oldItems` but not in
        // `newItems`, it means it's being removed from the children slot,
        // we need to enable the parent node of the removed item (i.e. `this`).
        this.enabled = true;
        updateScheduled = true;
      }
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
    const tree = this.tree;
    if (this.enabled) {
      this.reveal();
      if (tree) {
        tree.enabledItem = this;
      }
    } else {
      if (tree && tree.enabledItem === this) {
        tree.enabledItem = null;
      }
    }
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

  /** Update the level of the tree item by traversing upwards. */
  private updateLevel() {
    // Traverse upwards to determine the level.
    let level = 0;
    let current: SidenavItem|null = this;
    while (current) {
      current = current.parentItem;
      level++;
    }
    this.level = level;
  }
}

declare global {
  interface HTMLElementEventMap {
    [SidenavItem.events.SIDENAV_ITEM_EXPANDED]: TreeItemExpandedEvent;
    [SidenavItem.events.SIDENAV_ITEM_COLLAPSED]: TreeItemCollapsedEvent;
    [SidenavItem.events.SIDENAV_ITEM_RENAMED]: TreeItemRenamedEvent;
  }

  interface HTMLElementTagNameMap {
    'cros-sidenav-item': SidenavItem;
  }
}
