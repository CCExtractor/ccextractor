#!/usr/bin/env bash

set -e
set -u
set -o pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

REPO_ROOT="$(git rev-parse --show-toplevel)"
CONFIG_FILE="${REPO_ROOT}/scripts/uncrustify/uncrustify.cfg"
TARGET_DIR="${REPO_ROOT}/OnigiriAlert ${REPO_ROOT}/OnigiriAlertTests"

cd "${REPO_ROOT}"

while IFS= read -rd '' FILEPATH
do
	uncrustify -l oc -c "$CONFIG_FILE" --no-backup --mtime "$FILEPATH" 2>&1 || true
	rm "${FILEPATH}.uncrustify" >/dev/null 2>&1 || true
done < <(find ${TARGET_DIR} -iregex '.*\.[hm]$' -print0)

echo "Done"
