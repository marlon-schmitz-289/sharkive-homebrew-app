#!/bin/sh
# Usage: ./build.sh [make targets...]   e.g. ./build.sh cia
set -e
cd "$(dirname "$0")"
docker build -q -t sharkive-build . >/dev/null
docker run --rm -u "$(id -u):$(id -g)" -v "$PWD":/src sharkive-build make "$@"
