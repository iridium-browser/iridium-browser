/**
 * Boundary between the first operation, and the second operation.
 */
export const kOperationBoundaries = [
  'queue-op', // Operations are performed in different queue operations (submit, writeTexture).
  'command-buffer', // Operations are in different command buffers.
  'pass', // Operations are in different passes.
  'render-bundle', // Operations are in different render bundles.
  'dispatch', // Operations are in different dispatches.
  'draw', // Operations are in different draws.
] as const;
export type OperationBoundary = typeof kOperationBoundaries[number];

/**
 * Context a particular operation is permitted in.
 */
export const kOperationContexts = [
  'queue', // Operation occurs on the GPUQueue object
  'command-encoder', // Operation may be encoded in a GPUCommandEncoder.
  'compute-pass-encoder', // Operation may be encoded in a GPUComputePassEncoder.
  'render-pass-encoder', // Operation may be encoded in a GPURenderPassEncoder.
  'render-bundle-encoder', // Operation may be encoded in a GPURenderBundleEncoder.
] as const;
export type OperationContext = typeof kOperationContexts[number];

interface BoundaryInfo {
  readonly contexts: [OperationContext, OperationContext][];
  // Add fields as needed
}

function combineContexts(
  as: readonly OperationContext[],
  bs: readonly OperationContext[]
): [OperationContext, OperationContext][] {
  const result: [OperationContext, OperationContext][] = [];
  for (const a of as) {
    for (const b of bs) {
      result.push([a, b]);
    }
  }
  return result;
}

const queueContexts = combineContexts(kOperationContexts, kOperationContexts);
const commandBufferContexts = combineContexts(
  kOperationContexts.filter(c => c !== 'queue'),
  kOperationContexts.filter(c => c !== 'queue')
);

/**
 * Mapping of OperationBoundary => to a set of OperationContext pairs.
 * The boundary is capable of separating operations in those two contexts.
 */
export const kBoundaryInfo: {
  readonly [k in OperationBoundary]: BoundaryInfo;
} = /* prettier-ignore */ {
  'queue-op': {
    contexts: queueContexts,
  },
  'command-buffer': {
    contexts: commandBufferContexts,
  },
  'pass': {
    contexts: [
      ['compute-pass-encoder', 'compute-pass-encoder'],
      ['compute-pass-encoder', 'render-pass-encoder'],
      ['render-pass-encoder', 'compute-pass-encoder'],
      ['render-pass-encoder', 'render-pass-encoder'],
      ['render-bundle-encoder', 'render-pass-encoder'],
      ['render-pass-encoder', 'render-bundle-encoder'],
      ['render-bundle-encoder', 'render-bundle-encoder'],
    ],
  },
  'render-bundle': {
    contexts: [
      ['render-bundle-encoder', 'render-pass-encoder'],
      ['render-pass-encoder', 'render-bundle-encoder'],
      ['render-bundle-encoder', 'render-bundle-encoder'],
    ],
  },
  'dispatch': {
    contexts: [
      ['compute-pass-encoder', 'compute-pass-encoder'],
    ],
  },
  'draw': {
    contexts: [
      ['render-pass-encoder', 'render-pass-encoder'],
      ['render-bundle-encoder', 'render-pass-encoder'],
    ],
  },
};

export const kAllWriteOps = [
  'write-texture',
  'b2t-copy',
  't2t-copy',
  'storage',
  'attachment-store',
  'attachment-resolve',
] as const;
export type WriteOp = typeof kAllWriteOps[number];

export const kAllReadOps = [
  't2b-copy',
  't2t-copy',
  'attachment-load',
  'storage',
  'sample',
] as const;
export type ReadOp = typeof kAllReadOps[number];

export type Op = ReadOp | WriteOp;

interface OpInfo {
  readonly contexts: OperationContext[];
  // Add fields as needed
}

/**
 * Mapping of Op to the OperationContext(s) it is valid in
 */
const kOpInfo: {
  readonly [k in Op]: OpInfo;
} = /* prettier-ignore */ {
  'write-texture': { contexts: [ 'queue' ] },
  'b2t-copy': { contexts: [ 'command-encoder' ] },
  't2t-copy': { contexts: [ 'command-encoder' ] },
  't2b-copy': { contexts: [ 'command-encoder' ] },
  'storage': { contexts: [ 'compute-pass-encoder', 'render-pass-encoder', 'render-bundle-encoder' ] },
  'sample': { contexts: [ 'compute-pass-encoder', 'render-pass-encoder', 'render-bundle-encoder' ] },
  'attachment-store': { contexts: [ 'render-pass-encoder' ] },
  'attachment-resolve': { contexts: [ 'render-pass-encoder' ] },
  'attachment-load': { contexts: [ 'render-pass-encoder' ] },
};

export function checkOpsValidForContext(
  ops: [Op, Op],
  context: [OperationContext, OperationContext]
) {
  return (
    kOpInfo[ops[0]].contexts.indexOf(context[0]) !== -1 &&
    kOpInfo[ops[1]].contexts.indexOf(context[1]) !== -1
  );
}
