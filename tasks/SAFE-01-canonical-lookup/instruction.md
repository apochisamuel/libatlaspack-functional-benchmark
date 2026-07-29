# Canonical lookup normalization

Normalized documents must resolve lookup queries after applying the same ASCII case and whitespace canonicalization used for stored node names.

## Required behavior

- Preserve all existing public behavior and normal tests.
- Implement the contract through the public API; do not special-case the verifier input.
- Keep output deterministic for equivalent inputs.
- Do not disable validation, parsing, cleanup, or unrelated functionality.

## Success

The supplied hidden regression must fail on the exact R6 baseline, pass after the patch, and the complete existing test suite must remain green.
