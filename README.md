# libatlaspack safe functional benchmark R8

This package contains ten independent C-library functional-correctness tasks built
against one immutable libatlaspack R6 baseline. Each task is applied independently;
tasks are not cumulative.

The public package contains the clean repository and task instructions only. Reference
patches, hidden regressions, and verification logs are stored in the separate private
evidence archive.

Baseline SHA-256:
`cecfc5eea5f7d8d8fa3490857ba4e92c5f4597f3e1ab9272748e87cb810e79ec`

These are safe functional and robustness tasks. They are not sanitizer-crash or
memory-corruption vulnerability tasks and must not be represented as satisfying the
Fenrir vulnerability-task requirement.
