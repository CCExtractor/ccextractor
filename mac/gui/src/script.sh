#!/bin/bash

function display() {
  osascript <<EOT
    tell app "System Events"
      display dialog "$1" buttons {"OK"} default button 1 with title "CCExtractor"
      return
    end tell
EOT
}

CCEXTRACTOR="$(command -v ccextractor)"
if [ -z "$CCEXTRACTOR" ]; then
  for candidate in /usr/local/bin/ccextractor /opt/homebrew/bin/ccextractor; do
    if [ -x "$candidate" ]; then
      CCEXTRACTOR="$candidate"
      break
    fi
  done
fi

if [ -z "$CCEXTRACTOR" ]; then
  display "The CCExtractor command line tool was not found in your PATH, /usr/local/bin, or /opt/homebrew/bin. Please install it first."
  exit 1
fi

if [ $# -eq 0 ]; then
  display "No input file received. Drop one or more video files onto the app to extract their subtitles."
  exit 1
fi

# Exit codes from src/lib_ccx/ccx_common_common.h (10 = EXIT_NO_CAPTIONS)
failures=""
no_captions=""
for file in "$@"; do
  "$CCEXTRACTOR" "$file"
  status=$?
  case $status in
    0)
      ;;
    10)
      no_captions="$no_captions
$(basename "$file")"
      ;;
    *)
      failures="$failures
$(basename "$file") (exit code $status)"
      ;;
  esac
done
echo "Done"

if [ -z "$failures" ] && [ -z "$no_captions" ]; then
  display "Process Complete. Check the source file's folder for the subtitles."
  exit 0
fi

message="Finished processing $# file(s)."
if [ -n "$no_captions" ]; then
  message="$message
No captions found in:$no_captions"
fi
if [ -n "$failures" ]; then
  message="$message
Failed:$failures
Run from the Terminal to see detailed error output."
fi
display "$message"
exit 1
