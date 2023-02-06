#!/bin/bash

# Lua version
export LUA_VERSION=5.4.4

# Path to wasi-sdk
export WASI_SDK_PATH=/opt/wasi-sdk

# Additional wasi libraries such as wasi-vfs and wasix
export WASI_ROOT=/opt

export LIBS="-Wl,--stack-first -Wl,-z,stack-size=83886080"
export CC="${WASI_SDK_PATH}/bin/clang --target=wasm32-wasi"
export CFLAGS="-g -D_WASI_EMULATED_GETPID -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_PROCESS_CLOCKS -I/opt/include -I${WASI_ROOT}/include -isystem ${WASI_ROOT}/include -I${WASI_SDK_PATH}/share/wasi-sysroot/include --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot"
export CPPFLAGS="${CFLAGS}"
export LIBS="${LIBS} -L${WASI_ROOT}/lib -lwasix -lwasi_vfs -L${WASI_SDK_PATH}/share/wasi-sysroot/lib/wasm32-wasi -lwasi-emulated-signal --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot"

export LUA_SHORT_VERSION=$(echo "$LUA_VERSION" | cut -d. -f1,2)

$CC \
    -mexec-model=reactor \
    -I. \
    -I../lua-${LUA_VERSION}/src \
    $CFLAGS \
    $LIBS \
    -lc -lpthread -lwasi-emulated-signal -lgcc \
    -g \
    ../lua-${LUA_VERSION}/src/liblua.a \
    udf.c udf_impl.c \
    ../lua-${LUA_VERSION}/src/lua.c \
    -o udf-lua${LUA_SHORT_VERSION}.wasm

wasi-vfs pack udf-lua${LUA_SHORT_VERSION}.wasm \
    --mapdir "/usr/local/share/lua/${LUA_SHORT_VERSION}::./lib" \
    --mapdir "/app::./app" \
    --output s2-udf-lua${LUA_SHORT_VERSION}.wasm
