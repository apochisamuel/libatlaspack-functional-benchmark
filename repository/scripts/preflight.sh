#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

required=(
  build.sh
  .clusterfuzzlite/build.sh
  .clusterfuzzlite/project.yaml
  Makefile
  README.md
  ARCHITECTURE.md
  LICENSE
  CHANGELOG.md
  FENRIR_MASTER_SPEC.md
  docs/requirement-ledger.md
  docs/difficulty-gate.md
  docs/poc-verification.md
  docs/patch-verification.md
  docs/format.md
  docs/robustness-gate.md
  scripts/verify_task.sh
  scripts/release_gate.sh
  scripts/coverage.sh
  scripts/archive_audit.sh
  scripts/robustness_gate.sh
  tests/test_malformed.c
  tests/test_mutation_sweep.c
)
for path in "${required[@]}"; do
  [[ -e "$path" ]] || {
    echo "missing: $path" >&2
    exit 1
  }
done

command -v clang >/dev/null
command -v clang++ >/dev/null
[[ "$(grep -c '^language: c$' .clusterfuzzlite/project.yaml)" -eq 1 ]] || {
  echo "project language mismatch" >&2
  exit 1
}
! grep -q '^main_repo:' .clusterfuzzlite/project.yaml || {
  echo "main_repo is forbidden" >&2
  exit 1
}

grep -q '^set -euo pipefail$' build.sh
grep -q 'cd "$SRC"' build.sh
grep -q -- '-Isrc' build.sh
grep -q '^set -euo pipefail$' .clusterfuzzlite/build.sh
grep -q 'cd "$SRC"' .clusterfuzzlite/build.sh
grep -q -- '-Isrc' .clusterfuzzlite/build.sh

mapfile -t harnesses < <(find fuzz -maxdepth 1 -name '*_fuzzer.cc' -type f | sort)
[[ ${#harnesses[@]} -ge 3 ]] || {
  echo "fewer than three harnesses" >&2
  exit 1
}
for harness in "${harnesses[@]}"; do
  grep -Fq 'if (size < 64 || size > 16 * 1024 * 1024)' "$harness" || {
    echo "guard missing: $harness" >&2
    exit 1
  }
  corpus="fuzz/corpus/$(basename "${harness%.cc}")"
  [[ -d "$corpus" ]] || {
    echo "corpus missing: $corpus" >&2
    exit 1
  }
  find "$corpus" -maxdepth 1 -type f -name 'seed_*.bin' -print -quit | grep -q . || {
    echo "neutral seed missing: $corpus" >&2
    exit 1
  }
done

for forbidden in private pocs patches generators crash-logs; do
  [[ ! -e "$forbidden" ]] || {
    echo "private artifact path present: $forbidden" >&2
    exit 1
  }
done

if find . \( -path './.git' -o -path './.git/*' \) -prune -o \
  \( -name '*.o' -o -name '*.a' -o -name 'test_atlas' -o -name 'test_malformed' -o -name 'test_mutation_sweep' -o -name 'atlas_dump' \) \
  -type f -print | grep -q .; then
  echo "build artifacts present" >&2
  exit 1
fi

loc="$(cat src/*.c src/*.h | wc -l | tr -d ' ')"
echo "source_loc=$loc"
if (( loc < 10000 )); then
  echo "source LOC gate not met" >&2
  exit 1
fi

make clean >/dev/null
make CC=clang \
  CFLAGS='-std=c11 -O0 -g -Wall -Wextra -Wpedantic -Werror -Isrc' \
  test tools
make clean >/dev/null

work="$(mktemp -d)"
trap 'rm -rf "$work"; make clean >/dev/null 2>&1 || true' EXIT
mkdir -p "$work/direct" "$work/cfl"
SRC="$root" \
OUT="$work/direct" \
CC=clang \
CXX=clang++ \
LIB_FUZZING_ENGINE='-fsanitize=fuzzer' \
./build.sh
SRC="$root" \
OUT="$work/cfl" \
CC=clang \
CXX=clang++ \
LIB_FUZZING_ENGINE='-fsanitize=fuzzer' \
./.clusterfuzzlite/build.sh

mkdir -p "$work/corpora"
for harness in "${harnesses[@]}"; do
  name="$(basename "${harness%.cc}")"
  corpus="$work/corpora/$name"
  mkdir -p "$corpus"
  cp "fuzz/corpus/$name"/seed_*.bin "$corpus/"
  "$work/direct/$name" -runs=1 "$corpus" >/dev/null 2>&1
  "$work/cfl/$name" -runs=1 "$corpus" >/dev/null 2>&1
done

echo "harness_count=${#harnesses[@]}"
echo "preflight=PASS"
