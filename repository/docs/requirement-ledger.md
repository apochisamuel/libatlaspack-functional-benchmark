# Requirement Ledger

Statuses in this ledger are conservative. `VERIFIED` means the named command was
executed for this revision and its output is retained in the external evidence
package. Task-level rows remain blocked because this safe repository contains no
private benchmark task implementation or task evidence.

| Rule ID | Requirement | Implementation | Verification command | Evidence path | Status |
|---|---|---|---|---|---|
| CTRL-01 | Required public control files | Present in repository | `./scripts/preflight.sh` | `verification/preflight.log` | VERIFIED |
| REPO-01 | Genuine C binary-format library | ATPK parser, builder, graph analysis, transforms, reports | `make CC=clang ... test` | `verification/strict-clang-test.log` | VERIFIED |
| REPO-02 | At least 10,000 genuine source lines | Functional `src/*.c` and `src/*.h` modules | `cat src/*.c src/*.h \| wc -l` | `verification/source-loc.txt` | VERIFIED |
| REPO-03 | Meaningful functionality beyond fuzzing | Public builder, serializer, graph/query/export APIs and CLI | regression suite and `make tools` | `verification/strict-clang-test.log` | VERIFIED |
| BUILD-01 | Both scripts use strict mode and `$SRC` | `build.sh` and `.clusterfuzzlite/build.sh` | structural checks in preflight | `verification/preflight.log` | VERIFIED |
| BUILD-02 | Compile every C source into a static library | wildcard source loops in both scripts | execute both scripts | `verification/build-sh.log`, `verification/cfl-build-sh.log` | VERIFIED |
| BUILD-03 | Build every harness with `-Isrc` and fuzz engine | both scripts enumerate `fuzz/*_fuzzer.cc` | execute both scripts | `verification/build-sh.log`, `verification/cfl-build-sh.log` | VERIFIED |
| BASE-01 | Strict Clang normal tests pass | comprehensive public API tests | `make CC=clang CFLAGS='... -Werror ...' test` | `verification/strict-clang-test.log` | VERIFIED |
| BASE-02 | ASan and UBSan normal tests pass | all three test executables use sanitizer flags | `./scripts/robustness_gate.sh ...` | `verification/sanitizer-tests.log` | VERIFIED |
| BASE-03 | Malformed inputs produce bounded, classified failures | header, directory, section, reference, duplicate, and resource-limit matrix | `tests/test_malformed` | `verification/strict-tests.log` | VERIFIED |
| BASE-04 | Deterministic mutation sweep completes safely | truncations, one-bit mutations, and 10,000 seeded compound mutations | `tests/test_mutation_sweep` | `verification/strict-tests.log`, `verification/sanitizer-tests.log` | VERIFIED |
| COVER-01 | Source line coverage meets the local hardening threshold | LLVM instrumentation with 75% minimum | `./scripts/coverage.sh ...` | `verification/coverage-summary.txt` | VERIFIED |
| HARNESS-01 | At least three distinct harnesses | parse, graph-plan, and query/report surfaces | preflight and builds | `verification/preflight.log` | VERIFIED |
| HARNESS-02 | Every harness has size guard and corpus | guards and neutral seed files | preflight and isolated corpus checks | `verification/preflight.log` | VERIFIED |
| HARNESS-03 | Verification does not mutate frozen seed corpora | copied writable corpora under evidence directory | `./scripts/robustness_gate.sh ...` | `verification/fuzz-smoke.status` | VERIFIED |
| HARNESS-04 | Both build variants pass deterministic smoke runs | 2,000 iterations for three harnesses under each build script | `./scripts/robustness_gate.sh ...` | `verification/fuzz-*.log` | VERIFIED |
| API-01 | Every public declaration has a definition | archive symbol audit | declaration-to-`nm` comparison | `verification/public-symbols.txt` | VERIFIED |
| API-02 | Context allocation-accounting epoch is explicit | parse/serialization auto-reset; callers reset before independent allocating analysis operations | functional and documentation review | `verification/strict-clang-test.log`, `verification/contract-audit.txt` | VERIFIED |
| FORMAT-01 | Unknown-section serialization policy is explicit and tested | accepted unknown directory metadata is inspectable; canonical serialization emits standard sections only | `tests/test_malformed` | `verification/strict-clang-test.log`, `verification/contract-audit.txt` | VERIFIED |
| LEAK-01 | Agent-facing tree excludes private evidence | evidence stored in separate archive | archive content audit | `verification/archive-audit.log` | VERIFIED |
| LEAK-02 | Public tree contains no build or coverage artifacts | explicit archive scan | `./scripts/archive_audit.sh` | `verification/archive-audit.log` | VERIFIED |
| ROBUST-01 | Combined safe hardening workflow is fail-closed | strict, sanitizer, coverage, fuzzer, and archive stages | `./scripts/robustness_gate.sh ...` | `verification/summary.txt` | VERIFIED |
| TASK-01 | 12–14 candidate difficulty entries | Not included in safe scope | private review | not produced | BLOCKED |
| POC-01 | Exact-base deterministic raw task inputs | Not included in safe scope | `scripts/verify_task.sh` | not produced | BLOCKED |
| PATCH-01 | Complete task patches and anti-shortcut tests | Not included in safe scope | exact-base task workflow | not produced | BLOCKED |
| RELEASE-01 | At least 10 fully evidenced tasks | Not met | `scripts/release_gate.sh` | `verification/release-gate.log` | BLOCKED |
