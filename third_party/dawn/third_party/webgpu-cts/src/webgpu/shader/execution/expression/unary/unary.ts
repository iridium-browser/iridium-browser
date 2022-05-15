import { ExpressionBuilder } from '../expression.js';

/* @returns an ExpressionBuilder that evaluates the prefix unary operation */
export function unary(op: string): ExpressionBuilder {
  return value => `${op}${value}`;
}
