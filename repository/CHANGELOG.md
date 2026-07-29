# Changelog

## 3.1.0-safe-contracts — 2026-07-29

- Defined the reusable `atlas_context` allocation-accounting epoch: parse and
  builder serialization reset automatically, while independent allocating
  analysis calls should be preceded by `atlas_context_reset`.
- Defined unknown-section serialization behavior: directory metadata is
  inspectable when unknown sections are allowed, but semantic serialization
  intentionally emits only the five standard ATPK sections.
- Added a regression test covering accepted unknown-section inspection and
  canonical serialization without unknown payload preservation.

## 3.0.0-safe-hardened — 2026-07-29

- Added a malformed-input regression executable covering header, directory,
  section, reference, duplicate-key, UTF-8, checksum, and allocation-limit
  behavior through the public parser API.
- Added a deterministic mutation-sweep executable with all truncation lengths,
  one-bit mutations at every byte, and 10,000 seeded compound mutations.
- Added LLVM coverage generation with a fail-closed 75% source-line threshold.
- Added a combined robustness gate for strict compilation, ASan/UBSan tests,
  coverage, dual-build fuzzer smoke runs, archive auditing, and source hashes.
- Isolated writable libFuzzer corpora from repository seed directories in both
  preflight and robustness workflows.
- Added public ATPK format and robustness-gate documentation.

This revision remains a safe functional baseline. It does not contain private
benchmark tasks, deliberately vulnerable variants, crash inputs, or reference
repairs.
