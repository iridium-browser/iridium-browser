/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import './sidenav_item';

import {css, CSSResultGroup, html, LitElement} from 'lit';

import {castExists} from '../helpers/helpers';

import {SidenavItem, SidenavItemCollapsedEvent, SidenavItemEnabledChangedEvent, SidenavItemExpandedEvent} from './sidenav_item';
import {isRTL, isSidenavItem, shadowPiercingActiveItem} from './sidenav_util';

/**
 * Allow calling `scrollIntoViewIfNeeded`.
 * go/typescript-reopening
 */
declare interface ExtendedElement extends Element {
  // TODO(b/262453851): Investigate if we can use a standard approved method
  // instead of this. Perhaps a mix of intersection observer and scrollTo.
  scrollIntoViewIfNeeded: (centerIfNeeded: boolean) => void;
}

const ATTRIBUTE_OBSERVER_CONFIG = {
  attributes: true,
  childList: true,
  subtree: true,
  attributeFilter: ['may-have-children']
};

/**
 * True iff `item` is not disabled and every parent of it allows children to be
 * selected.
 */
function isSelectable(item: SidenavItem): boolean {
  if (item.disabled) return false;

  let parent = item.parentItem;
  while (parent) {
    if (!parent.expanded || parent.disabled) return false;
    parent = parent.parentItem;
  }
  return true;
}

/**
 * <cros-sidenav> is the container of the <cros-sidenav-item> elements. An
 * example DOM structure is like this:
 *
 * <cros-sidenav>
 *   <cros-sidenav-item>
 *     <cros-sidenav-item></cros-sidenav-item>
 *   </cros-sidenav-item>
 *   <cros-sidenav-item></cros-sidenav-item>
 * </cros-sidenav>
 *
 * The enabling and focus of <cros-sidenav-item> is controlled in
 * <cros-sidenav>, this is because we need to make sure only one item is being
 * enabled or focused.
 */
export class Sidenav extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
      :host {
        display: block;
      }

      ul {
        list-style: none;
        margin: 0;
        padding: 0;
      }
  `;
  /** @nocollapse */
  static override properties = {
    ariaSetSize: {type: String},
    doubleclickExpands:
        {type: Boolean, reflect: true, attribute: 'doubleclick-expands'},
  };

  /** @nocollapse */
  static get events() {
    return {
      /** Triggers when a tree item has been enabled. */
      SIDENAV_ENABLED_CHANGED: 'sidenav_enabled_changed',
    } as const;
  }

  /**
   * Return the enabled tree item. If there are items, an enabled item always
   * exists.
   */
  get enabledItem(): SidenavItem|null {
    return this.items.find(item => item.enabled) || null;
  }
  /** Setting this to null is a noop. */
  set enabledItem(item: SidenavItem|null) {
    if (item) this.enableItem(item);
  }

  /** Get all the SidenavItems in the Sidenav. */
  get items(): SidenavItem[] {
    return this.childrenSlot.assignedElements()
        .filter(isSidenavItem)
        .flatMap(
            // The Sidenav's slot contains the declarations of all SidenavItems,
            // regardless of how nested. However, `assignedElements()` returns
            // an array of the top elements, so we have to expand each one
            // individually.
            e => [e].concat(
                Array.from(e.querySelectorAll('cros-sidenav-item'))));
  }

  /** The child tree items which can be selected, e.g. using the arrow keys. */
  get selectableItems(): SidenavItem[] {
    return this.items.filter(isSelectable);
  }

  /** The default unnamed slot to let consumer pass children tree items. */
  private get childrenSlot(): HTMLSlotElement {
    return castExists(this.renderRoot.querySelector('slot'));
  }

  /**
   * Value to set aria-setsize, which is the number of the top level child tree
   * items.
   * @export
   */
  override ariaSetSize = '0';

  /**
   * Whether double-clicking expands the tree.
   * @export
   */
  doubleclickExpands: boolean;

  /**
   * The `selectedItem` is the item that the user navigated to. If the user
   * hasn't navigated, this is the enabled item. The `selectedItem` is the item
   * that should receive focus when the sidenav receives focus. There should
   * always be an item to focus, unless there are no items.
   */
  private get selectedItem(): SidenavItem|null {
    return this.items.find(item => item.tabIndex === 0) || null;
  }

  /**
   * Listens for changes on sidenav children and recalculates the sidenav's
   * state if necessary.
   */
  private readonly itemAttributeObserver =
      new MutationObserver((mutationList: MutationRecord[]) => {
        for (const mutation of mutationList) {
          if (mutation.type === 'attributes' &&
              (mutation.target as Element).tagName === 'CROS-SIDENAV-ITEM' &&
              mutation.attributeName === 'may-have-children') {
            this.updateLayered();
            return;
          }
          if (mutation.type === 'childList') {
            this.updateLayered();
            return;
          }
        }
      });

  constructor() {
    super();
    this.doubleclickExpands = false;

    // Listen for changes to children, to update the Sidenav's state if
    // necessary.
    this.itemAttributeObserver.observe(this, ATTRIBUTE_OBSERVER_CONFIG);
  }

  override render() {
    return html`
      <ul
        class="tree"
        role="tree"
        aria-setsize=${this.ariaSetSize}
        @dblclick=${this.onTreeDblClicked}
        @keydown=${this.onTreeKeyDown}
        @sidenav_item_expanded=${this.onSidenavItemExpanded}
        @sidenav_item_collapsed=${this.onSidenavItemCollapsed}
        @sidenav_item_enabled_changed=${this.onSidenavItemEnabledChanged}
      >
        <slot @slotchange=${this.onSlotChanged}></slot>
      </ul>
    `;
  }

  /**
   * If the sidenav receives focus, proxy focus down to the selected sidenav
   * item.
   */
  override focus() {
    // If there are items, there's always a selected item.
    if (this.selectedItem) {
      this.selectedItem.focus();
    }
  }

  /**
   * When the children change, make sure the sidenav state is passed on to the
   * new tree structure.
   */
  private onSlotChanged() {
    this.ariaSetSize = this.selectableItems.length.toString();

    // The sidenav must always have an enabled item. This also ensures that it
    // has a selected/tabbable item.
    if (!this.items.some(item => item.enabled) &&
        this.selectableItems.length > 0) {
      this.enableItem(castExists(this.selectableItems[0]));
    }

    this.updateLayered();
  }

  /**
   * Infers whether this sidenav has nested children (i.e. is layered), and
   * sets `inLayered` on all the children.
   */
  private updateLayered() {
    const layered = this.querySelectorAll(
                            'cros-sidenav-item cros-sidenav-item, ' +
                            'cros-sidenav-item[may-have-children]')
                        .length > 0;
    for (const item of this.querySelectorAll('cros-sidenav-item')) {
      item.inLayered = layered;
    }
  }

  /** Handles the expanded event of the tree item. */
  private onSidenavItemExpanded(e: SidenavItemExpandedEvent) {
    const treeItem = e.detail.item;
    (treeItem as unknown as ExtendedElement).scrollIntoViewIfNeeded(false);
  }

  /** Handles the collapse event of the tree item. */
  private onSidenavItemCollapsed(e: SidenavItemCollapsedEvent) {
    const collapsedItem = e.detail.item;
    // If the currently selected tree item (`oldSelectedItem`) is a descent of
    // another tree item (`treeItem`) which is going to be collapsed, we need to
    // mark the ancestor tree item (`this`) as selected.
    if (this.selectedItem !== collapsedItem) {
      const oldSelectedItem = this.selectedItem;
      if (oldSelectedItem && collapsedItem.contains(oldSelectedItem)) {
        this.selectItem(collapsedItem);
      }
    }
  }

  /** Handles when that enabled status of an item has changed. */
  private onSidenavItemEnabledChanged(e: SidenavItemEnabledChangedEvent) {
    const item = e.detail.item;

    if (item.enabled) {
      item.reveal();
      this.enableItem(item);
    }
  }

  /** Called when the user double clicks on a tree item. */
  private async onTreeDblClicked(e: MouseEvent) {
    if (!this.doubleclickExpands) return;

    // Stop if the the click target is not a tree item.
    const treeItem = e.target as SidenavItem;
    if (treeItem && !isSidenavItem(treeItem)) {
      return;
    }

    if (treeItem.disabled) {
      e.stopImmediatePropagation();
      e.preventDefault();
      return;
    }

    // Use composed path to know which element inside the shadow root
    // has been clicked.
    const innerClickTarget = e.composedPath()[0] as HTMLElement;
    if (innerClickTarget.className !== 'expand-icon' &&
        treeItem.hasChildren()) {
      treeItem.expanded = !treeItem.expanded;
    }
  }

  /**
   * Handle the keydown within the tree, this mainly handles the navigation
   * and the selection with the keyboard.
   */
  private onTreeKeyDown(e: KeyboardEvent) {
    const selectedItem = this.selectedItem;
    const selectableItems = this.selectableItems;
    const currentIndex = selectableItems.findIndex(i => i === selectedItem);

    if (e.ctrlKey || e.repeat) {
      return;
    }

    if (!selectedItem || currentIndex === -1) {
      return;
    }

    let itemToSelect: SidenavItem|null|undefined = null;
    switch (e.key) {
      case 'Enter':
      case 'Space':
        this.enableItem(selectedItem);
        break;
      case 'ArrowUp':
        if (currentIndex > 0) {
          itemToSelect = selectableItems[currentIndex - 1];
        }
        break;
      case 'ArrowDown':
        if (currentIndex < selectableItems.length - 1) {
          itemToSelect = selectableItems[currentIndex + 1];
        }
        break;
      case 'ArrowLeft':
      case 'ArrowRight':
        // Don't let back/forward keyboard shortcuts be used.
        if (e.altKey) {
          break;
        }

        const expandKey = isRTL() ? 'ArrowLeft' : 'ArrowRight';
        if (e.key === expandKey) {
          if (selectedItem.hasChildren() && !selectedItem.expanded) {
            selectedItem.expanded = true;
          } else {
            itemToSelect = selectedItem.selectableItems[0];
          }
        } else {
          if (selectedItem.expanded) {
            selectedItem.expanded = false;
          } else {
            itemToSelect = selectedItem.parentItem;
          }
        }
        break;
      case 'Home':
        itemToSelect = this.selectableItems[0];
        break;
      case 'End':
        itemToSelect = this.selectableItems[this.selectableItems.length - 1];
        break;
      default:
        break;
    }

    if (itemToSelect) {
      this.selectItem(itemToSelect);
      e.preventDefault();
    }
  }

  /**
   * Make `itemToEnable` become the enabled item in the tree, this will
   * also un-enable the previously enabled tree item to make sure at most
   * one tree item is enabled in the tree.
   */
  private enableItem(itemToEnable: SidenavItem) {
    const previousEnabledItem =
        this.items.find(item => item.enabled && item !== itemToEnable) || null;

    if (previousEnabledItem) {
      previousEnabledItem.enabled = false;
    }

    itemToEnable.enabled = true;
    this.selectItem(itemToEnable);
    (itemToEnable as unknown as ExtendedElement).scrollIntoViewIfNeeded(false);

    const enabledChangeEvent: SidenavEnabledChangedEvent =
        new CustomEvent(Sidenav.events.SIDENAV_ENABLED_CHANGED, {
          bubbles: true,
          composed: true,
          detail: {
            previousEnabledItem,
            enabledItem: itemToEnable,
          },
        });
    this.dispatchEvent(enabledChangeEvent);
  }

  /**
   * Make `itemToSelect` become the selected item in the tree. The previously
   * selected item is unselected. The selected item is the only tabbable item,
   * so that if the user tabs away and returns, they return to the last selected
   * item.
   */
  private selectItem(itemToSelect: SidenavItem) {
    const oldSelectedItem =
        this.items.find(item => item.tabIndex === 0 && item !== itemToSelect);

    if (oldSelectedItem) oldSelectedItem.tabIndex = -1;
    itemToSelect.tabIndex = 0;

    // If the sidenav has focus, move focus to the newly selected item.
    if (this.contains(shadowPiercingActiveItem())) {
      if (oldSelectedItem) oldSelectedItem.blur();
      // Schedule the focusing, because we may need to wait for the item to
      // become visible, so that it can receive focus.
      itemToSelect.updateComplete.then(() => {
        itemToSelect.focus();
      });
    }
  }
}

customElements.define('cros-sidenav', Sidenav);

/** Type of the tree item enabling custom event. */
export type SidenavEnabledChangedEvent = CustomEvent<{
  /**
   * The tree item which has been enabled previously.
   * TODO(b/262453851): Figure out if we need this event at all, and if we do if
   * we need the prev selected field. In files I don't think this is ever used:
   * https://source.chromium.org/search?q=previousSelectedItem%20f:file_manager&sq=
   */
  previousEnabledItem: SidenavItem | null,
  /** The tree item which has been enabled now. */
  enabledItem: SidenavItem | null,
}>;

declare global {
  interface HTMLElementEventMap {
    [Sidenav.events.SIDENAV_ENABLED_CHANGED]: SidenavEnabledChangedEvent;
  }

  interface HTMLElementTagNameMap {
    'cros-sidenav': Sidenav;
  }
}
