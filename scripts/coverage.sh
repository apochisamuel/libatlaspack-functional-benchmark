#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

out_dir="${1:-$root/.coverage-build}"
threshold="${ATLAS_COVERAGE_MIN:-75}"
rm -rf "$out_dir"
mkdir -p "$out_dir/obj" "$out_dir/profiles"

cc="${CC:-clang}"
common=(
  -std=c11
  -O0
  -g
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Isrc
  -fprofile-instr-generate
  -fcoverage-mapping
)

objects=()
for source in src/*.c; do
  object="$out_dir/obj/$(basename "${source%.c}").o"
  "$cc" "${common[@]}" -c "$source" -o "$object"
  objects+=("$object")
done
ar rcs "$out_dir/libatlaspack.a" "${objects[@]}"

binaries=()
for source in tests/test_atlas.c tests/test_malformed.c tests/test_mutation_sweep.c; do
  name="$(basename "${source%.c}")"
  binary="$out_dir/$name"
  "$cc" "${common[@]}" "$source" "$out_dir/libatlaspack.a" -o "$binary"
  LLVM_PROFILE_FILE="$out_dir/profiles/$name.profraw" "$binary"
  binaries+=("$binary")
done

llvm-profdata merge -sparse "$out_dir"/profiles/*.profraw -o "$out_dir/coverage.profdata"
objects_args=()
for binary in "${binaries[@]:1}"; do
  objects_args+=( -object "$binary" )
done
llvm-cov report "${binaries[0]}" \
  "${objects_args[@]}" \
  -instr-profile="$out_dir/coverage.profdata" \
  src > "$out_dir/coverage-summary.txt"
llvm-cov export "${binaries[0]}" \
  "${objects_args[@]}" \
  -instr-profile="$out_dir/coverage.profdata" \
  -summary-only \
  src > "$out_dir/coverage-summary.json"

line_percent="$(python3 - "$out_dir/coverage-summary.json" <<'PY'
import json
import sys
with open(sys.argv[1], 'r', encoding='utf-8') as handle:
    report = json.load(handle)
print(f"{report['data'][0]['totals']['lines']['percent']:.2f}")
PY
)"

echo "line_coverage_percent=$line_percent"
echo "line_coverage_minimum=$threshold"
python3 - "$line_percent" "$threshold" <<'PY'
import sys
actual = float(sys.argv[1])
minimum = float(sys.argv[2])
if actual < minimum:
    raise SystemExit(f"coverage gate failed: {actual:.2f}% < {minimum:.2f}%")
PY

echo "coverage_gate=PASS"
