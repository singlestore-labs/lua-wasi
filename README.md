# Lua WASI

This project consists of utilities and libraries for building
Lua sources for the [WebAssembly](https://webassembly.org)
platform using the [WASI SDK](https://github.com/WebAssembly/wasi-sdk) (v15+).
The scripts here should work for any version of Lua.


## Building Lua

The `run.sh` script applies the appropriate patches for compiling with
`clang` and the `wasm32-wasi` target. The build does depend on 
[wasi_vfs](https://github.com/kateinoigakukun/wasi-vfs) and 
[wasix](https://github.com/singlestore-labs/wasix) being installed (see
the `run.sh` script for specifying the locations).

After the libraries are installed, simply run the following:
```
./run.sh
```

This will build Lua 5.4.4. You can specify alternate versions in the
variables at the top of `run.sh` or pass `LUA_VERSION` as an environment
variable.


## Running Lua

To run Lua, use a Wasm runtime such as wasmtime:
```
wasmtime run lua-5.4.4/src/lua 
```


## Caveats

Lua uses `setjmp` / `longjmp` for raising and catching exceptions. These functions
are not implemented in Wasi yet and are simply stubbed out to do nothing.
This does allow Lua to run, but means that any errors that occur in the code
or any attempts to throw an exception will end in the process aborting.


## Resources

[Lua](https://lua.org) Lua programming language

[WASI SDK](https://github.com/WebAssembly/wasi-sdk) WASI SDK source repository

[wasix](https://github.com/singlestore-labs/wasix) wasix library source repository

[WASI VFS](https://github.com/kateinoigakukun/wasi-vfs) Virtual file system
