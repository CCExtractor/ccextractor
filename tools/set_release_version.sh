#!/usr/bin/env bash
set -euo pipefail

tag=${1:-}
version=${tag#v}
if [[ ! $version =~ ^[0-9][0-9A-Za-z.+_-]*$ ]]; then
    printf 'Invalid CCExtractor release tag: %s\n' "$tag" >&2
    exit 1
fi

header=src/lib_ccx/lib_ccx.h
if ! grep -q '^#define VERSION "[^"]*"' "$header"; then
    printf 'Missing VERSION definition in %s\n' "$header" >&2
    exit 1
fi

tmp=$(mktemp "$header.XXXXXX")
trap 'rm -f "$tmp"' EXIT
cp -p "$header" "$tmp"
sed "s/^#define VERSION \"[^\"]*\"/#define VERSION \"$version\"/" "$header" > "$tmp"
mv -f "$tmp" "$header"
printf 'CCExtractor release version %s\n' "$version"
