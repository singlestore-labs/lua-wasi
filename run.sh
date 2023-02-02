#!/bin/bash

#set -x

# Version of Lua to build
LUA_VERSION="${LUA_VERSION:-5.4.4}"

# Location of WASI SDK tree
WASI_SDK_PATH="${WASI_SDK_PATH:-/opt/wasi-sdk}"

# Location of other wasi includes and libs such as wasi-vfs and wasix
WASI_ROOT="${WASI_ROOT:-/opt}"

PROJECT_DIR=$(pwd)

if [[ ! -d "lua-${LUA_VERSION}" ]]; then
    curl "https://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz" | tar -zxv
fi

cd "lua-${LUA_VERSION}"

cat "${PROJECT_DIR}/patches/src.Makefile.patch" | \
    sed "s#{{WASI_SDK_PATH}}#${WASI_SDK_PATH}#g" | \
    sed "s#{{WASI_ROOT}}#${WASI_ROOT}#g" | \
    sed "s#{{PROJECT_DIR}}#${PROJECT_DIR}#g" | \
    patch -p1 -N -r-

make clean && make generic
