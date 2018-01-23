#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"
cd ../../

REPO_ROOT="$(git rev-parse --show-toplevel)"
CONFIG_FILE="${REPO_ROOT}/uncrustify.cfg"
TARGET_DIR="${REPO_ROOT}"

cd "${REPO_ROOT}"

while IFS= read -rd '' FILEPATH
do
	uncrustify -l oc -c "$CONFIG_FILE" --no-backup --mtime "$FILEPATH" 2>&1 || true
	rm "${FILEPATH}.uncrustify" >/dev/null 2>&1 || true
done < <(find ${TARGET_DIR} -iregex '.*\.[c]$' -print0)

echo "Done"
