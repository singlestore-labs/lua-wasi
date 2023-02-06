#define PY_SSIZE_T_CLEAN
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "udf.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

static lua_State *LuaState;

int find_func_name(char *path, int path_l)
//
// Find the last '.' in the given path
//
// Parameters
// ----------
// path : string
//     Period-delimited string containing the absolute path to
//     a Python function
// path_l : int
//     The length of `path`
//
// Returns
// -------
// int : the position of the last '.' in the string, or zero if
//       there is no '.'
//
{
    for (int i = path_l - 1; i > 0; i--)
    {
        if (path[i] == '.')
        {
            return i + 1;
        }
    }
    return 0;
}

static int initialize()
//
// Initialize the Python interpreter
//
// Returns
// -------
// int : 0 for success, -1 for error
//
{
    int rc = 0;

    if (LuaState) goto exit;
   
    LuaState = luaL_newstate();
    if (!LuaState) goto error;

    luaL_openlibs(LuaState);

    // Add library paths
    const char *c = "package.path = package.path .. ';/app/?.lua'";
    if (luaL_dostring(LuaState, c) == LUA_OK)
    {
        lua_pop(LuaState, lua_gettop(LuaState));
    }
    else goto error;

    // Load msgpack library
    c = "msgpack = require('msgpack')";
    if (luaL_dostring(LuaState, c) == LUA_OK)
    {
        lua_pop(LuaState, lua_gettop(LuaState));
    }
    else goto error;

exit:
    return rc;

error:
    fprintf(stderr, "Could not initialize Lua\n");
    rc = -1;
    goto exit;
}

int udf_exec(udf_string_t *code)
//
// Execute arbitrary code
//
// Parameters
// ----------
// code : string
//     The code to execute
//
// Returns
// -------
// int : 0 for success, -1 for error
//
{
    int rc = 0;
    char *c = malloc(code->len + 1);
    memcpy(c, code->ptr, code->len);
    c[code->len] = '\0';

    if (initialize()) goto error;

    if (luaL_dostring(LuaState, c) == LUA_OK)
    {
        lua_pop(LuaState, lua_gettop(LuaState));
    }
    else goto error;

exit:
    if (c) free(c);
    return rc;

error:
    fprintf(stderr, "Error occurred when running code\n");
    rc = -1;
    goto exit;
}

int load_function(char *path, uint64_t path_l)
//
// Search a library path for a specific function
//
// Parameters
// ----------
// path : string
//     The path to the function
// path_l : int
//     The length of the string in `path`
//
// Returns
// -------
// int : 0 for ok, -1 for error
//
{
    int rc = 0;
    uint64_t i = 0;
    char *func = NULL;
    uint64_t func_l = 0;
    char *obj = path;
    char *end = path + path_l;

    for (i = 0; i < path_l; i++)
    {
        if (path[i] == '.')
        {
            path[i] = '\0';
            if (obj == path) 
            {
                lua_getglobal(LuaState, obj);
            }
            else 
            {
                lua_getfield(LuaState, -1, obj);
                lua_remove(LuaState, -2);
            }
            path[i] = '.';
            obj = path + i + 1;
        }
    }

    func_l = end - obj;
    func = malloc(func_l + 1);
    if (!func) goto error;
    memcpy(func, obj, func_l);
    func[func_l] = '\0';

    if (obj == path)
    {
        lua_getglobal(LuaState, func);
    }
    else 
    {
        lua_getfield(LuaState, -1, func);
        lua_remove(LuaState, -2);
    }

exit:
    return rc;

error:
    rc = -1;
    goto exit;
}

void udf_call(udf_string_t *name, udf_list_u8_t *args, udf_list_u8_t *ret)
//
// Call a function with the given arguments
//
// Parameters
// ----------
// name : string
//     Absolute path to a function. For example, `urllib.parse.urlparse`.
// args : bytes
//     MessagePack blob of function arguments
//
// Returns
// -------
// ret : MessagePack blob of function return values
//
{
    int rc = 0;
    const char *msg = NULL;

    ret->ptr = NULL;
    ret->len = 0;

    if (initialize()) goto error;

    // Start with a fresh stack
    lua_pop(LuaState, lua_gettop(LuaState));

    // Get the pack function
    lua_getglobal(LuaState, "msgpack");
    lua_getfield(LuaState, -1, "encode");
    lua_remove(LuaState, -2);

    lua_getglobal(LuaState, "table");
    lua_getfield(LuaState, -1, "pack");
    lua_remove(LuaState, -2);

    // Get a function from the global namespace
    rc = load_function(name->ptr, name->len);
    if (rc != 0) goto error;

    lua_getglobal(LuaState, "table");
    lua_getfield(LuaState, -1, "unpack");
    lua_remove(LuaState, -2);

    // Unpack arguments
    lua_getglobal(LuaState, "msgpack");
    lua_getfield(LuaState, -1, "decode");
    lua_remove(LuaState, -2);
    lua_pushlstring(LuaState, (const char*)args->ptr, args->len);

    // Call msgpack.decode
    if (lua_pcall(LuaState, 1, LUA_MULTRET, 0) != LUA_OK) goto error;

    // Call table.unpack to place arguments
    if (lua_pcall(LuaState, 1, LUA_MULTRET, 0) != LUA_OK) goto error;

    // Call user-specified function
    if (lua_pcall(LuaState, lua_gettop(LuaState) - 3, LUA_MULTRET, 0) != LUA_OK) goto error;

    // Call table.pack to put return value into a list
    if (lua_pcall(LuaState, lua_gettop(LuaState) - 2, LUA_MULTRET, 0) != LUA_OK) goto error;

    // Call msgpack.encode
    if (lua_pcall(LuaState, lua_gettop(LuaState) - 1, LUA_MULTRET, 0) != LUA_OK) goto error;

    msg = lua_tolstring(LuaState, -1, &ret->len);
    if (ret->len)
    {
        ret->ptr = malloc(ret->len);
        if (!ret->ptr)
        {
           ret->len = 0;
           goto error;
        }
        memcpy(ret->ptr, msg, ret->len);
    }

exit:
    lua_pop(LuaState, lua_gettop(LuaState));
    return;

error:
    fprintf(stderr, "Calling function failed\n");
    ret->ptr = NULL;
    ret->len = 0;
    goto exit;
}
