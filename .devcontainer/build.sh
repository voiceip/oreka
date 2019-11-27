#!/bin/bash
set -ex

#make orkbase
cd /oreka/orkbasecxx
autoreconf -i
./configure CXX=g++
make
make install

#orkaudio
cd /oreka/orkaudio
autoreconf -i
./configure CXX=g++
make
make install

