import { basicExpressionBuilder, ShaderBuilder } from '../expression.js';

/* @returns a ShaderBuilder that evaluates a prefix unary operation */
export function unary(op: string): ShaderBuilder {
  return basicExpressionBuilder(value => `${op}(${value})`);
}

export function assignment(): ShaderBuilder {
  return basicExpressionBuilder(value => `${value}`);
}
