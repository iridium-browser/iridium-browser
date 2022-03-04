/**
 * Reifies a `GPUExtent3D` into a `Required<GPUExtent3DDict>`.
 */
export function reifyExtent3D(
  val: Readonly<GPUExtent3DDict> | Iterable<number>
): Required<GPUExtent3DDict> {
  // TypeScript doesn't seem to want to narrow the types here properly, so hack around it.
  if (typeof (val as Iterable<number>)[Symbol.iterator] === 'function') {
    const v = Array.from(val as Iterable<number>);
    return { width: v[0] ?? 1, height: v[1] ?? 1, depthOrArrayLayers: v[2] ?? 1 };
  } else {
    const v = val as Readonly<GPUExtent3DDict>;
    return {
      width: v.width ?? 1,
      height: v.height ?? 1,
      depthOrArrayLayers: v.depthOrArrayLayers ?? 1,
    };
  }
}
