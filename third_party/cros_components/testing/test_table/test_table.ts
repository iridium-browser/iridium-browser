/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, html, LitElement} from 'lit';
import {customElement, property} from 'lit/decorators';


/**
 * A matrix containing arbitrary elements, keyed by column and row titles. To
 * place a child in a particular position in the grid, use
 * `column-title-row-title` as the slot name. Column and row names must be
 * alphanumeric with hyphens.
 */
@customElement('cros-test-table')
export class CrosTestTable extends LitElement {
  static override styles = [css`
      table, th, td {
        border: 1px solid black;
        border-collapse: collapse;
      }
      th, td {
        padding: 10px;
      }
      th, td:nth-of-type(1) {
        font-weight: bold;
        background-color: slategray;
      }
      td {
        background-color: whitesmoke;
      }
    `];

  @property({type: Array}) columns: string[] = [];
  @property({type: Array}) rows: string[] = [];


  override render() {
    return html`
          <table>
            <thead>
              <tr>
                <th></th>
                ${this.columns.map(column => html`<th>${column}</th>`)}
              </tr>
            </thead>
            <tbody>
              ${
        this.rows.map(
            row => html`
                <tr>
                  <td>
                    ${row}
                  </td>
                  ${
                this.columns.map(
                    column => html`<td><slot name=${
                        column + '-' + row}>∅</slot></td>`)}
                </tr>`)}
            </tbody>
          </table>
      `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-test-table': CrosTestTable;
  }
}
