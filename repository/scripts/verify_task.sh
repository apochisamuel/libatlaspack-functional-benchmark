#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 6 ]]; then
  echo "usage: $0 BASELINE_ZIP PATCH POC HARNESS SANITIZER_CLASS EVIDENCE_DIR" >&2
  exit 2
fi
baseline="$1"; patch_file="$2"; poc="$3"; harness="$4"; sanitizer="$5"; evidence="$6"
mkdir -p "$evidence"
sha256sum "$baseline" > "$evidence/baseline.sha256"
sha256sum "$patch_file" > "$evidence/patch.sha256"
sha256sum "$poc" > "$evidence/poc.sha256"
cat > "$evidence/verification-status.txt" <<STATUS
status=BLOCKED
reason=Safe foundation does not generate or verify intentionally vulnerable tasks.
requested_harness=$harness
requested_sanitizer=$sanitizer
STATUS
exit 3
