#!/bin/sh
# Download the experiments' large data files (GitHub release experiments-data-v1) and
# unpack them into experiments/*/data/. Run from anywhere inside the repo.
#   experiments/fetch-data.sh                 download, unpack, verify
#   experiments/fetch-data.sh ARCHIVE.tar.zst use an already downloaded archive
# Needs curl, tar and zstd. File list and checksums: experiments/data-v1.sha256.
set -eu

URL=https://github.com/mb6611/esp32-d8rx/releases/download/experiments-data-v1/esp32-d8rx-experiments-data-v1.tar.zst
ARCHIVE_SHA256=7967196c5f6e25b45d2dc6868b19e87bfa44a87daf1878dc5715db391ab4ca05

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

sha256() { if command -v sha256sum >/dev/null 2>&1; then sha256sum "$@"; else shasum -a 256 "$@"; fi; }

if [ $# -ge 1 ]; then
  ARCHIVE=$1
else
  ARCHIVE=$(mktemp -d)/esp32-d8rx-experiments-data-v1.tar.zst
  echo "downloading $URL"
  curl -fL --retry 3 -o "$ARCHIVE" "$URL"
fi

got=$(sha256 "$ARCHIVE" | cut -d' ' -f1)
if [ "$got" != "$ARCHIVE_SHA256" ]; then
  echo "archive sha256 mismatch: got $got, want $ARCHIVE_SHA256" >&2
  exit 1
fi

echo "unpacking into $ROOT/experiments"
zstd -dc "$ARCHIVE" | tar -xf - -C "$ROOT"

sha256 -c --quiet experiments/data-v1.sha256 && echo "all $(wc -l < experiments/data-v1.sha256 | tr -d ' ') files OK"
