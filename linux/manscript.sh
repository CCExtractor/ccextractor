# !/bin/bash
echo "Executing script"

# Check if help2man installed
if [[ $(dpkg -l | grep help2man) ]]; then
    echo "help2man found."
else
    echo "help2man does not exist."
    echo "Please install help2man. (for Ubuntu use: sudo apt-get install help2man)"
    exit 1
fi

exechelp2man="help2man --section 7 --include=replace.h2m ./ccextractor > ccextractor.7" 
echo $exechelp2man
eval $exechelp2man

mkdir -p /usr/local/man/man7
sudo cp ccextractor.7 /usr/local/man/man7/ 
sudo gzip /usr/local/man/man7/ccextractor.7
sudo mandb
rm ccextractor.7

echo "Man page generated"
