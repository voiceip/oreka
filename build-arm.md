# Oreka on Linux ARM 64 CPU

This works with Ubuntu on an Odroid C4 and Raspberry PI 4 with PiOS 64 bit.

## Install dependencies

```
sudo apt install daemontools daemontools-run build-essential \
                 autoconf shtool libtool libssl-dev libdw-dev \
                 libdw1 libunwind8 libunwind-dev libboost-dev \
                 libpcap-dev libsndfile1-dev libapr1-dev \
                 libspeex-dev libace-dev libopus-dev \
                 libboost-all-dev liblog4cxx-dev libxerces-c-dev \
                 libxerces-c3.2 libcap-dev libbcg729-dev \
                 libbackward-cpp-dev
```

## Compile OrecX dependencies

```
cd ~
git clone https://github.com/OrecX/dependencies.git
tar -xf dependencies/opus-1.2.1.tar.gz
cd opus-1.2.1
./configure --prefix=/opt/opus/
make CFLAGS="-fPIC"
sudo make install
sudo cp /opt/opus/lib/libopus.a /opt/opus/lib/libopusstatic.a
cd ~
sudo tar -xf dependencies/silk.tgz -C /opt/
sudo cd /opt/silk/SILKCodec/SILK_SDK_SRC_FIX/
sudo make clean lib
```

## Compile OrkbaseCXX

```
cd ~
git clone https://github.com/voiceip/oreka.git
cd oreka/orkbasecxx
autoreconf -i
./configure CXX=g++
make
sudo make install
```

## Compile Orkaudio

```
cd ~
cd oreka/orkaudio
autoreconf -i
./configure CXX=g++
make
sudo make install
```