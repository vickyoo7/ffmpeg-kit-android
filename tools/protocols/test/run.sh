#!/usr/bin/env bash
#
# Builds and runs the saf protocol test (saf_protocol_test.c).
#
# CC selects the compiler. With an NDK compiler, for example
#   CC=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi21-clang NO_RUN=1
# it builds an Android executable to run on a device: adb push it to /data/local/tmp and run it
# there with TMPDIR=/data/local/tmp.
#
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${OUT_DIR:-${TEST_DIR}/../../../.tmp/protocols-test}"

mkdir -p "${OUT_DIR}"
"${CC:-cc}" -std=gnu11 -Wall -Wno-unused-parameter -Werror -I"${TEST_DIR}/stub" -o "${OUT_DIR}/saf_protocol_test" "${TEST_DIR}/saf_protocol_test.c"

if [[ -z ${NO_RUN:-} ]]; then
  "${OUT_DIR}/saf_protocol_test"
fi
