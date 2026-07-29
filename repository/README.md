# libatlaspack

`libatlaspack` is a C library for reading, validating, creating, transforming,
and inspecting ATPK binary asset-graph packages. ATPK stores a bounded section
directory followed by string, node, edge, attribute, and metadata sections.
The parser resolves identifiers into immutable graph indexes while retaining the
original public IDs and section information.

This revision is a safe, hardened functional repository baseline. It contains no
private benchmark task materials, generated crash cases, or reference task
solutions.

## Capabilities

The public API supports:

- bounded parsing from memory or files, checksum verification, UTF-8 checks,
  canonical name normalization, reference resolution, and semantic validation;
- document builders, binary serialization, round-trip parsing, cloning,
  structural comparison, fingerprints, and file output;
- outgoing and incoming adjacency, breadth-first and depth-first traversal,
  shortest paths, topological plans, dependency layers, weak and strong
  components, reachability, distance maps, cycle discovery, longest paths,
  degree histograms, and iterative graph ranking;
- node and edge selections, attribute and metadata lookup, name search,
  subgraphs, catalogs, policy diagnostics, and graph statistics;
- graph reversal, edge-label filtering, reachability pruning, ID compaction,
  document merging, and transitive reduction for acyclic graphs;
- JSON, CSV, adjacency-matrix CSV, reachability CSV, DOT, GraphML, Mermaid,
  Markdown, edge-list, node, topology, component, attribute, schema, integrity,
  and manifest reports.

## Build and test

```sh
make
make test
make tools
make sanitize
make coverage
make robustness
./scripts/preflight.sh
```

The normal test target runs the functional suite, a malformed-input status matrix,
and a deterministic 10,000-case mutation sweep. `make coverage` enforces at
least 75% source-line coverage, while `make robustness` runs strict tests,
sanitisers, coverage, both fuzzer builds, isolated deterministic fuzzer smoke
runs, and the archive audit.

The two fuzz-build scripts compile every `src/*.c` file into a static library
and build every `fuzz/*_fuzzer.cc` target with `-Isrc`. They expect the standard
ClusterFuzz environment variables:

```sh
SRC="$PWD" OUT=/tmp/atlas-out LIB_FUZZING_ENGINE=-fsanitize=fuzzer ./build.sh
```

## Command-line inspection

```sh
make tools
./tools/atlas_dump package.atpk
```

The tool parses a package with the same public API used by applications and
prints a structured document report.

## Context and serialization contracts

`atlas_parse` and `atlas_builder_serialize` reset the supplied context's
diagnostic and allocation-accounting epoch automatically. Before a separate
allocating analysis operation, callers should use `atlas_context_reset`; doing so
does not invalidate previously returned documents or result objects.

When unknown sections are allowed, their directory metadata can be inspected,
but their payload is not part of the semantic graph model. Document serialization
produces a canonical ATPK package containing the five standard section types and
intentionally omits accepted unknown sections.

## Repository controls

`docs/format.md` documents the public ATPK layout, and
`docs/robustness-gate.md` defines the deterministic hardening workflow.
`docs/requirement-ledger.md` records only checks supported by executed evidence.
The task verification and release scripts remain fail-closed when task-level
evidence is absent. The agent-facing tree must remain separate from any private
development evidence package.
