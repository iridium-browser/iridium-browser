import { ExpressionBuilder } from '../expression.js';

/* @returns an ExpressionBuilder that evaluates the binary operation */
export function binary(op: string): ExpressionBuilder {
  return values => `(${values.join(op)})`;
}
