/usr/local/bin/ccextractor $1

function display() {
  osascript <<EOT
    tell app "System Events"
      display dialog "$1" buttons {"OK"} default button 1 with title "CCExtractor"
      return
    end tell
EOT
}
echo "Done"
display "Process Complete. Check the source file's folder for the subtitles."