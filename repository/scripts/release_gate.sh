#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

./scripts/preflight.sh

count=0
if [[ -d private/evidence ]]; then
  count="$(
    find private/evidence \
      -mindepth 2 \
      -maxdepth 2 \
      -name metadata.json \
      -type f \
      -print0 \
      | xargs -0 -r grep -l \
          '"difficulty_gate"[[:space:]]*:[[:space:]]*"PASS"' \
      | wc -l \
      | tr -d ' '
  )"
fi

if (( count < 10 )); then
  echo "release blocked: only $count complete task evidence bundles" >&2
  exit 1
fi

echo "release evidence count=$count"
echo "release_gate=PASS"
