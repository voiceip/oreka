#!/bin/bash
set -e

INF=${INTERFACE:-eth0}

#check if /etc/orkaudio  is mounted externally

if grep -qs '/etc/orkaudio' /proc/mounts; then
    echo "Config File exists externally."
else
    echo "Config File does not exist. Applying default template"
    sed "s/__INTERFACE__/$INF/g" /etc/orkaudio/config_default.xml  >  /etc/orkaudio/config.xml
fi

echo "---- Using Configuration -----"
cat  /etc/orkaudio/config.xml
echo "---------"

exec "$@"

