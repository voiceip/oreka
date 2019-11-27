#!/bin/bash
set -ex

echo $GITHUB_WORKSPACE

if [ -d "$GITHUB_WORKSPACE" ] 
then
    echo "Directory $GITHUB_WORKSPACE exists." 
    ls $GITHUB_WORKSPACE

	# && git clone --depth 1 https://github.com/voiceip/oreka.git /opt/oreka `
	# && cd /opt/oreka/orkbasecxx `
	# && autoreconf -i `
	# && ./configure CXX=g++ `
	# && make `
	# && make install

	# && autoreconf -i `
	# && ./configure CXX=g++ `
	# && make `
	# && make install

fi



