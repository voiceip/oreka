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
cd /oreka-build/distribution
bash make-orkaudio-deb.sh

ls -ltrah *.deb
