#!/usr/bin/env bash
set -euo pipefail

root=${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
header="$root/include/uvaps_gbus_runtime.h"
library="$root/lib/libuvgbus.so"
test -s "$header" || { echo "missing GBus header: $header" >&2; exit 1; }
test -s "$library" || { echo "missing GBus library: $library" >&2; exit 1; }

header_sha=$(sha256sum "$header" | awk '{print $1}')
library_sha=$(sha256sum "$library" | awk '{print $1}')
test "$header_sha" = 6c321d54a5db25828d24ccf735c598d7e9a5c60c175b2083e30f40a4560db955 \
  || { echo "Gbus header SHA-256 mismatch: $header_sha" >&2; exit 1; }
test "$library_sha" = c34b59ed9760c8a27c3509dba0174d519c9df6a620d542a53ebd83dfd59e5c49 \
  || { echo "Gbus library SHA-256 mismatch: $library_sha" >&2; exit 1; }

file "$library"
readelf -d "$library" | sed -n '/SONAME/ p; /NEEDED/ p'
printf 'GBUS_RUNTIME_ROOT=%s\n' "$root"
