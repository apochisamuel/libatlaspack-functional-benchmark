# Architecture

## ATPK container

An ATPK file begins with a fixed 32-byte little-endian header containing the
magic value, format version, flags, section count, directory location, declared
file size, and payload checksum. The directory contains fixed-size entries with
section type, flags, offset, size, and auxiliary data. Required semantic data is
split across strings, nodes, edges, attributes, and optional metadata.

All offsets, counts, multiplications, and additions are checked before memory is
allocated or bytes are consumed. Section ranges are validated against the file,
against the directory, and against one another. Decoders reject trailing bytes
for fixed-layout sections.

## Processing phases

The public parser runs these phases in order:

1. bounded header decoding and version validation;
2. directory range, overlap, type, and placement checks;
3. string decoding, UTF-8 validation, and hashing;
4. node, edge, attribute, and metadata decoding;
5. node-ID registration;
6. cross-section reference resolution;
7. optional recovery hooks;
8. canonical node-name normalization;
9. semantic and attribute-slice validation;
10. outgoing and incoming adjacency construction;
11. canonical-name index construction;
12. publication of an immutable document.

A failed phase destroys the partially built document. Published documents own
all decoded arrays, strings, indexes, and edge-order vectors. Analysis objects
borrow the immutable document and own only their result storage.

## Modules

The implementation is divided by responsibility rather than artificial call
depth:

- `reader`, `header`, and `directory` implement bounded binary access;
- `strings`, `nodes`, `edges`, `attributes`, and `metadata` decode sections;
- `registry`, `references`, `canonical`, `normalize`, and `graph` establish
  document invariants and indexes;
- `builder`, `writer`, and `serialize` create valid ATPK packages;
- `query`, `neighbors`, `search`, `selection`, and `catalog` expose views;
- `plan`, `layers`, `traversal`, `path`, `components`, `strong_components`,
  `distance`, `reachability`, `cycle`, `rank`, and `histogram` analyze graphs;
- `subgraph`, `transform`, `transform_extra`, `merge`, `clone_compare`, `diff`,
  and `fingerprint` implement document-level operations;
- the report and export modules produce independent textual representations;
- `policy` and `diagnostics` evaluate public structural requirements;
- `lifecycle` centralizes destruction of document-owned state.

## Harness surfaces

The three harnesses intentionally exercise different public workflows:

- `atlas_parse_fuzzer` runs the complete parse and destruction lifecycle;
- `atlas_graph_fuzzer` parses, builds a topological plan when possible, reads
  planned nodes, and cleans up both result and document state;
- `atlas_query_fuzzer` parses, performs canonical lookups, constructs a public
  report, and releases all returned buffers.

Each harness accepts only inputs from 64 bytes through 16 MiB, creates its own
context, disables checksum enforcement to increase structural exploration,
uses no private files, and always executes normal cleanup.

## Allocation and ownership

`atlas_context` provides a caller-controlled allocation-accounting epoch and a
diagnostic buffer. `atlas_parse` and `atlas_builder_serialize` begin a fresh
epoch automatically. Other allocating analysis APIs charge their allocations to
the current epoch; applications that treat those calls as independent operations
should call `atlas_context_reset` before each one. Resetting clears diagnostics
and accounting only—it never destroys returned objects. Returned documents,
plans, paths, traversals, component maps, selections, catalogs, diagnostics,
and text buffers have explicit destruction functions. Builders use independent
heap ownership so that a builder can outlive any one parse operation.

## Verification boundary

Normal tests construct packages through the public builder, parse exact binary
bytes, inspect graph behavior, serialize and parse again, compare documents,
exercise transformations and exports, and cover both acyclic and cyclic graphs.
The repository preflight separately checks mechanical layout, source size,
strict Clang compilation, both fuzz-build scripts, isolated seed-corpus
execution, and absence of compiled artifacts in the final tree. The robustness
gate adds malformed-input status assertions, a deterministic 10,000-case
mutation sweep, ASan/UBSan execution, a 75% LLVM line-coverage threshold,
2,000-run smoke sessions for every harness under both build scripts, and an
archive audit. Writable fuzz corpora are always copied outside the source tree
so verification cannot change the baseline.
