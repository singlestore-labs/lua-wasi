#!/usr/bin/env python3

import msgpack
import wasmtime
from bindings import Udf

store = wasmtime.Store()

module = wasmtime.Module(store.engine, open('s2-udf-lua5.4.wasm', 'rb').read())

linker = wasmtime.Linker(store.engine)
linker.define_wasi()

wasi = wasmtime.WasiConfig()
wasi.inherit_stdin()
wasi.inherit_stdout()
wasi.inherit_stderr()
store.set_wasi(wasi)

udf = Udf(store, linker, module)

# You *must* call _initialize for wasi to work
udf.instance.exports(store)['_initialize'](store)

#
# Lua
#
out = udf.exec(store, '''function foo(x, y)
  return x, y
end''')
print('Result of exec:', out)

out = udf.call(store, 'foo', msgpack.packb(["xyz", 2.4]))
print('Result of foo("xyz", 2.4):', out)
print('Unpacked result of foo("xyz", 2.4):', msgpack.unpackb(out))

out = udf.call(store, 'math.sqrt', msgpack.packb([2]))
print('Result of math.sqrt(2):', out)
print('Unpacked result of math.sqrt(2):', msgpack.unpackb(out))
