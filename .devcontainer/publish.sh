#!/bin/bash

source /etc/os-release
mv /oreka-src/.megarc ~/.megarc
hash=$(git --git-dir /oreka-src/.git rev-parse --short @)
echo "$hash"
mv orkaudio.deb "orkaudio-$VERSION_CODENAME-$hash.deb"
ls -ltrah *.deb
megaput --path "/Root/voiceip-oreka/" "orkaudio-$VERSION_CODENAME-$hash.deb"
