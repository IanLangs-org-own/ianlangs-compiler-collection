#include <stdio.h>
#include <string.h>
#include "../../../lua/lua.h"
#include "../../../lua/lauxlib.h"
#include "../../../lua/lualib.h"
#include "fileReader.h"
#include <stdio.h>
#include <stdlib.h>

char* iccBuildLua = 
    "Project = {}\n"
    "Project.__index = Project\n"
    "\n"
    "function Project:new()\n"
    "    return setmetatable({files = {}}, Project)\n"
    "end\n"
    "\n"
    "function Project:add(file)\n"
    "    table.insert(self.files, file)\n"
    "end\n"
    "\n"
    "function Project:clear()\n"
    "   self.files = {}\n"
    "end\n"
    "\n"
    "function Project:build(compiler, ...)\n"
    "    local args = table.concat({...}, ' ')\n"
    "    local cmd = compiler .. ' '\n"
    "    for _, f in ipairs(self.files) do\n"
    "        cmd = cmd .. f .. ' '\n"
    "    end\n"
    "    if args ~= '' then cmd = cmd .. args end\n"
    "    io.output():write('Running: ')\n"
    "    print(cmd)\n"
    "    local res, code = shell(cmd)\n"
    "    if code ~= 0 then error('Build failed!') end\n"
    "end\n"
    "\n"
    "-- ejemplo de uso\n"
    "-- local myProject = Project:new()\n"
    "-- myProject:add('src/main.fcpp')\n"
    "-- myProject:build('ifc', '-oTest', '-O2')";

const char* iluaExtraBuildings =
    "global null <const> = nil\n"
    "function shell(cmd)\n"
    "    local f = io.popen(cmd, \"r\")\n"
    "    if not f then return nil, nil end\n"
    "    local out = \"\"\n"
    "    while true do\n"
    "        local c = f:read(1)\n"
    "        if not c then break end\n"
    "        io.write(c)\n"
    "        io.flush()\n"
    "        out = out .. c\n"
    "    end\n"
    "    local ok, _, code = f:close()\n"
    "    return out, code\n"
    "end;\n";

int runLua(lua_State* L, const char* code, const char* filename) {
    size_t len = strlen(filename);
    char* chunkname = malloc(len + 2);
    if (!chunkname) return 1;

    chunkname[0] = '@';
    memcpy(chunkname + 1, filename, len + 1);

    int status = luaL_loadbuffer(L, code, strlen(code), chunkname);
    free(chunkname);

    if (status != LUA_OK) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return status;
    }

    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    return status;
}

void printHelp(void) {
    printf("%s\n%s\n\t%s\n",
        "ilua file.ilua [flags]",
        "flags:",
            "-b, --builder - modo builder, entry point build(b)"
    );
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printHelp();
        return 1;
    }
    char* filename = argv[1];
    _Bool builder = 0;
    if (strcmp(filename, "-h") == 0 || strcmp(filename, "--help") == 0) {
        printHelp();
        return 0;
    }
    else if (strcmp(filename, "-v") == 0 || strcmp(filename, "--version") == 0) {
        printf("ilua version 1.0\nbasado en %s\n", LUA_VERSION);
        return 0;
    }
    else if (strcmp(filename, "-b") == 0 || strcmp(filename, "--builder") == 0) {
        filename = argv[2];
        builder = 1;
    }

    char* code = NULL;

    if (builder) {
        size_t lenght = 0;
        code = readFileWithOffset(filename, strlen("\nbuild(Project:new())\n"), &lenght);
        if (!code) goto code_error;
        char* code_end = code + lenght;

        strcpy(code_end, "\nbuild(Project:new())\n");
    } else {
        code = readFile(filename);
    }

    if (!code) {
        code_error:
            fprintf(stderr, "No se pudo leer el archivo: %s\n", filename);
            return 1;
    }

    // Crear nuevo estado Lua
    lua_State* L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "No se pudo crear el estado Lua.\n");
        return 1;
    }

    // Abrir librerías estándar
    luaL_openlibs(L);
    runLua(L,iluaExtraBuildings,"iluaExtraBuildings.lua");

    if (builder) {
        runLua(L, iccBuildLua, "iccBuilder.lua");
    }
    // Ejecutar archivo Lua
    if (runLua(L, code, filename) != LUA_OK) {
        const char* error_msg = lua_tostring(L, -1);
        fprintf(stderr, "%s: %s\n", filename, error_msg);
        lua_pop(L, 1);
        lua_close(L);
        free(code);
        return 1;
    }
    free(code);
    lua_close(L);
    return 0;
}
