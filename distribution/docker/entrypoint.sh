#!/bin/bash
set -e

INF=${INTERFACE:-eth0}

sed -i "s/__INTERFACE__/$INF/g" /etc/orkaudio/config.xml

cat  /etc/orkaudio/config.xml

exec "$@"

