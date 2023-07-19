import { basicExpressionBuilder, ShaderBuilder } from '../../expression.js';

/* @returns a ShaderBuilder that calls the builtin with the given name */
export function builtin(name: string): ShaderBuilder {
  return basicExpressionBuilder(values => `${name}(${values.join(', ')})`);
}
