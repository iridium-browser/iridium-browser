import {
  basicExpressionBuilder,
  basicExpressionWithPredeclarationBuilder,
  ShaderBuilder,
} from '../../expression.js';

/* @returns a ShaderBuilder that calls the builtin with the given name */
export function builtin(name: string): ShaderBuilder {
  return basicExpressionBuilder(values => `${name}(${values.join(', ')})`);
}

/* @returns a ShaderBuilder that calls the builtin with the given name and has given predeclaration */
export function builtinWithPredeclaration(name: string, predeclaration: string): ShaderBuilder {
  return basicExpressionWithPredeclarationBuilder(
    values => `${name}(${values.join(', ')})`,
    predeclaration
  );
}
