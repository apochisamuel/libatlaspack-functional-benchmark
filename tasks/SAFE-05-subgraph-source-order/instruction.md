# Stable subgraph source order

Node-selection permutations must produce byte-identical subgraphs by publishing nodes, edges, attributes, and metadata in source-document order.

## Required behavior

- Preserve all existing public behavior and normal tests.
- Implement the contract through the public API; do not special-case the verifier input.
- Keep output deterministic for equivalent inputs.
- Do not disable validation, parsing, cleanup, or unrelated functionality.

## Success

The supplied hidden regression must fail on the exact R6 baseline, pass after the patch, and the complete existing test suite must remain green.
