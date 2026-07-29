# Robustness Gate

The repository robustness gate is a deterministic, fail-closed verification
workflow for the safe functional baseline. It does not create benchmark task
inputs or intentionally vulnerable variants.

Run it with:

```sh
./scripts/robustness_gate.sh /absolute/path/to/evidence
```

The gate performs these stages in order:

1. strict Clang compilation with all warnings promoted to errors;
2. the functional, malformed-input, and deterministic mutation-sweep tests;
3. the same test matrix under AddressSanitizer and UndefinedBehaviorSanitizer;
4. LLVM source coverage with a 75 percent line-coverage minimum;
5. both required libFuzzer build scripts;
6. 2,000 deterministic smoke iterations per harness and build variant;
7. an archive-oriented scan for private paths, generated binaries, coverage
   artifacts, non-neutral corpus filenames, and revealing implementation text;
8. SHA-256 recording for public source, test, and harness files.

Fuzzer smoke runs copy neutral seeds into the evidence directory before use.
The harnesses never receive the repository corpus directories as writable
corpora, so verification cannot silently mutate the frozen source tree.

The run is successful only when `summary.txt` ends with:

```text
strict_tests=PASS
sanitizer_tests=PASS
coverage=PASS
fuzz_smoke=PASS
archive_audit=PASS
robustness_gate=PASS
```

The coverage report is stored as both human-readable text and JSON. The fuzz
logs retain the selected seed, run count, corpus statistics, and normal `DONE`
termination line for each harness variant.
