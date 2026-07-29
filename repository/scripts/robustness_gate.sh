#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
evidence="${1:-$root/.robustness-evidence}"
rm -rf "$evidence"
mkdir -p "$evidence"

cleanup() {
  make clean >/dev/null 2>&1 || true
  rm -rf .build-fuzz
}
trap cleanup EXIT

revision="$(sed -n 's/.*return "\([^"]*\)".*/\1/p' src/version.c | head -1)"
{
  echo "revision=$revision"
  echo "clang=$(clang --version | head -1)"
  echo "timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$evidence/environment.txt"

make clean >/dev/null
make CC=clang \
  CFLAGS='-std=c11 -O0 -g -Wall -Wextra -Wpedantic -Werror -Isrc' \
  test tools > "$evidence/strict-tests.log" 2>&1
echo "strict_tests=PASS" | tee "$evidence/strict-tests.status"

make clean >/dev/null
ASAN_OPTIONS='detect_leaks=1:abort_on_error=1:strict_string_checks=1' \
UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1' \
make CC=clang \
  CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer' \
  test > "$evidence/sanitizer-tests.log" 2>&1
echo "sanitizer_tests=PASS" | tee "$evidence/sanitizer-tests.status"

ATLAS_COVERAGE_MIN="${ATLAS_COVERAGE_MIN:-75}" \
  ./scripts/coverage.sh "$evidence/coverage" > "$evidence/coverage.log" 2>&1
cp "$evidence/coverage/coverage-summary.txt" "$evidence/coverage-summary.txt"
cp "$evidence/coverage/coverage-summary.json" "$evidence/coverage-summary.json"
echo "coverage=PASS" | tee "$evidence/coverage.status"

make clean >/dev/null
mkdir -p "$evidence/fuzz-direct" "$evidence/fuzz-cfl"
SRC="$root" OUT="$evidence/fuzz-direct" CC=clang CXX=clang++ \
LIB_FUZZING_ENGINE='-fsanitize=fuzzer' ./build.sh > "$evidence/build-direct.log" 2>&1
SRC="$root" OUT="$evidence/fuzz-cfl" CC=clang CXX=clang++ \
LIB_FUZZING_ENGINE='-fsanitize=fuzzer' ./.clusterfuzzlite/build.sh > "$evidence/build-cfl.log" 2>&1

runs="${ATLAS_FUZZ_SMOKE_RUNS:-2000}"
mkdir -p "$evidence/corpora"
for harness_source in fuzz/*_fuzzer.cc; do
  name="$(basename "${harness_source%.cc}")"
  corpus="$evidence/corpora/$name"
  mkdir -p "$corpus"
  cp "fuzz/corpus/$name"/seed_*.bin "$corpus/"
  for variant in fuzz-direct fuzz-cfl; do
    log="$evidence/${variant}-${name}.log"
    ASAN_OPTIONS='detect_leaks=1:abort_on_error=1:strict_string_checks=1' \
    UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1' \
      "$evidence/$variant/$name" \
      -seed=1337 \
      -runs="$runs" \
      -max_len=4096 \
      -timeout=5 \
      "$corpus" > "$log" 2>&1
    echo "$variant/$name=PASS runs=$runs" >> "$evidence/fuzz-smoke.status"
  done
done

make clean >/dev/null
./scripts/archive_audit.sh "$root" > "$evidence/archive-audit.log"
sha256sum src/*.c src/*.h tests/*.c fuzz/*_fuzzer.cc \
  > "$evidence/source-files.sha256"

cat > "$evidence/summary.txt" <<SUMMARY
strict_tests=PASS
sanitizer_tests=PASS
coverage=PASS
fuzz_smoke=PASS
archive_audit=PASS
robustness_gate=PASS
SUMMARY
cat "$evidence/summary.txt"
