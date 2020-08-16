#!/bin/bash

mv /oreka-src/.megarc ~/.megarc
hash=$(git --git-dir /oreka-src/.git rev-parse --short @)
echo "$hash"
mv orkaudio.deb "orkaudio-$hash.deb"
ls -ltrah *.deb
megaput --path "/Root/voiceip-oreka/" "orkaudio-$hash.deb"
