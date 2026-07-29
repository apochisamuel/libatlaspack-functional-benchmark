#!/usr/bin/env bash
set -euo pipefail
cd "$SRC"
: "${OUT:?OUT is required}"
: "${LIB_FUZZING_ENGINE:?LIB_FUZZING_ENGINE is required}"
CC_BIN="${CC:-clang}"
CXX_BIN="${CXX:-clang++}"
rm -rf .build-fuzz
mkdir -p .build-fuzz "$OUT"
objects=()
for source in src/*.c; do
  object=".build-fuzz/$(basename "${source%.c}").o"
  "$CC_BIN" -std=c11 -O1 -g -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer -c "$source" -o "$object"
  objects+=("$object")
done
ar rcs .build-fuzz/libatlaspack.a "${objects[@]}"
for harness in fuzz/*_fuzzer.cc; do
  name="$(basename "${harness%.cc}")"
  "$CXX_BIN" -std=c++17 -O1 -g -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$harness" .build-fuzz/libatlaspack.a $LIB_FUZZING_ENGINE -o "$OUT/$name"
done
