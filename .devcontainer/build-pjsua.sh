#!/bin/bash
set -e

apt-get update -qq && apt-get install -y --no-install-recommends ca-certificates curl libgsm1-dev  \
	libspeex-dev libspeexdsp-dev libssl-dev  portaudio19-dev libsrtp0-dev \
	&& apt-get purge -y --auto-remove && rm -rf /var/lib/apt/lists/*

git clone --depth 1 --branch 2.5.5 https://github.com/pjsip/pjproject /opt/pjproject \
	&& cd /opt/pjproject && ./configure && touch pjlib/include/pj/config_site.h && make dep && make \
	&& cp pjsip-apps/bin/pjsua* /usr/local/bin/pjsua