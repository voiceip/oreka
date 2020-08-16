#!/bin/bash
set -e

#make orkbase
cd /oreka-src/orkbasecxx
autoreconf -i
./configure CXX=g++
make
make install

#orkaudio
cd /oreka-src/orkaudio
autoreconf -i
./configure CXX=g++
make
make install

#package orkaudio deb
git clone https://github.com/voiceip/oreka-build.git /oreka-build
cd /oreka-build/distribution
chmod +x make-orkaudio-deb.sh
./make-orkaudio-deb.sh

exit_code=$?

ls -ltrah *.deb
set +e
mv /oreka-src/.megarc ~/.megarc
hash=$(git --git-dir /oreka-src/.git rev-parse --short @)
echo "$hash"
mv orkaudio.deb "orkaudio-$hash.deb"
ls -ltrah *.deb
megaput --path "/Root/voiceip-oreka/" "orkaudio-$hash.deb"

exit $exit_code
