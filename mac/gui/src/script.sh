#!/bin/bash

CCEXTRACTOR="/usr/local/bin/ccextractor"

function display() {
  osascript <<EOT
    tell app "System Events"
      display dialog "$1" buttons {"OK"} default button 1 with title "CCExtractor"
      return
    end tell
EOT
}

if [ ! -x "$CCEXTRACTOR" ]; then
  display "The CCExtractor command line tool was not found at $CCEXTRACTOR. Please install it first."
  exit 1
fi

if [ $# -eq 0 ]; then
  display "No input file received. Drop a video file onto the app to extract its subtitles."
  exit 1
fi

"$CCEXTRACTOR" "$1"
status=$?
echo "Done"

# Exit codes from src/lib_ccx/ccx_common_common.h (10 = EXIT_NO_CAPTIONS)
case $status in
  0)
    display "Process Complete. Check the source file's folder for the subtitles."
    ;;
  10)
    display "No captions were found in this file, so no subtitle file was created."
    ;;
  *)
    display "CCExtractor failed (exit code $status). Run it from the Terminal to see the detailed error output."
    ;;
esac
exit $status
