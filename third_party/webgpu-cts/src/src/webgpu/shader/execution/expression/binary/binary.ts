import { ShaderBuilder, basicExpressionBuilder, compoundAssignmentBuilder } from '../expression.js';

/* @returns a ShaderBuilder that evaluates a binary operation */
export function binary(op: string): ShaderBuilder {
  return basicExpressionBuilder(values => `(${values.map(v => `(${v})`).join(op)})`);
}

/* @returns a ShaderBuilder that evaluates a compound binary operation */
export function compoundBinary(op: string): ShaderBuilder {
  return compoundAssignmentBuilder(op);
}
