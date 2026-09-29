#!/bin/sh
set -e
cd "$(dirname "$0")"
# devkitARM image already ships libctru, citro2d/3d and all 3ds-portlibs (curl, mbedtls, libarchive, zlib ...)
docker run --rm -u "$(id -u):$(id -g)" -v "$PWD":/src -w /src devkitpro/devkitarm:20260610 make "$@"
echo "-> $(pwd)/sharkive-updater.3dsx"
