#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
target="${1:-$root}"
[[ -d "$target" ]] || {
  echo "audit target is not a directory: $target" >&2
  exit 2
}

for path in private pocs patches generators crash-logs; do
  if find "$target" -type d -name "$path" -print -quit | grep -q .; then
    echo "forbidden private directory present: $path" >&2
    exit 1
  fi
done

if find "$target" \
  \( -name '.git' -o -name '.coverage-build' -o -name '.build-fuzz' \) -prune -o \
  \( -name '*.o' -o -name '*.a' -o -name '*.profraw' -o -name '*.profdata' \
     -o -name 'test_atlas' -o -name 'test_malformed' -o -name 'test_mutation_sweep' \
     -o -name 'atlas_dump' \) -type f -print | grep -q .; then
  echo "compiled or coverage artifacts present" >&2
  exit 1
fi

if find "$target/fuzz/corpus" -type f ! -name 'seed_*.bin' -print 2>/dev/null | grep -q .; then
  echo "non-neutral corpus filename present" >&2
  exit 1
fi

for source_root in "$target/src" "$target/fuzz"; do
  [[ -d "$source_root" ]] || continue
  if grep -RInE --include='*.c' --include='*.h' --include='*.cc' \
      'expected[[:space:]]+crash|root[[:space:]_-]*cause|reference[[:space:]_-]*fix|specific[[:space:]]+CVE' \
      "$source_root"; then
    echo "revealing implementation comment found" >&2
    exit 1
  fi
done

file_count="$(find "$target" -type f | wc -l | tr -d ' ')"
echo "audited_files=$file_count"
echo "archive_audit=PASS"
