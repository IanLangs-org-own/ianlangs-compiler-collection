#include <stdio.h>
#include <string.h>
#include "../lua/lua.h"
#include "../lua/lauxlib.h"
#include "../lua/lualib.h"

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
"end"
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
"    io.output():write(res)\n"
"    if code ~= 0 then error('Build failed!') end\n"
"end\n"
"\n"
"-- ejemplo de uso\n"
"-- local myProject = Project:new()\n"
"-- myProject:add('src/main.fcpp')\n"
"-- myProject:build('ifc', '-oTest', '-O2')\n";

void printHelp() {
    printf("Uso: ilua [-b || --builder] <archivo.lua>\n");
    printf("Opciones:\n");
    printf("  -h, --help     Mostrar esta ayuda\n");
    printf("  -v, --version  Mostrar la versión de ilua\n");
}

static int isalnum(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int isIdent(char c) {
    return isalnum(c) || c == '_';
}

static int isBoundary(char c) {
    return !isIdent(c);
}

static int runLua(lua_State* L, const char* code, const char* filename) {
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

static int isScaped(const char* code, size_t pos) {
    if (pos == 0) return 0;

    int count = 0;
    size_t p = pos - 1;

    while (p >= 0 && code[p] == '\\') {
        count++;
        p--;
    }

    return (count % 2) == 1;
}

static char* readFile(const char *filename) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Reservar memoria
    char *buffer = malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, f);
        buffer[length] = '\0'; // Asegurar el terminador nulo
    }

    fclose(f);
    return buffer;
}

static const char* transpile(const char* code, int builder) { // null -> nil
    char* luaCode = (char*)malloc(strlen(code) + (builder ? 22 : 1));
    size_t i = 0, j = 0;
    int inLineComment = 0;
    int inBlockComment = 0;
    int inSString = 0;
    int inDString = 0;
    while (code[i]) {
        char c = code[i];
        if (c == '\'' && !inLineComment && !inBlockComment && !inDString && !isScaped(code, i)) {
            inSString = !inSString;
        }

        if (c == '"' && !inLineComment && !inBlockComment && !inSString && !isScaped(code, i)) {
            inDString = !inDString;
        }

        if (code[i+1] && c == '-' && code[i+1] == '-' && !inSString && !inDString) {
            if (code[i+2] && code[i+3] && code[i+2] == '[' && code[i+3] == '[') {
                inBlockComment = 1;
                i += 4;
                continue;
            }
            inLineComment = 1;
            i += 2;
            continue;
            
        }
        if (c == '\n' && inLineComment) {
            inLineComment = 0;
        }

        if (c == ']' && code[i+1] && code[i+1] == ']' && inBlockComment) {
            inBlockComment = 0;
            i += 2;
            continue;
        }

        if (!(inLineComment || inBlockComment || inSString || inDString)) {
            if (strncmp(code + i, "null", 4) == 0) {
                if ((i == 0 || isBoundary(code[i-1])) && (code[i+4] == '\0' || isBoundary(code[i+4]))) {
                    luaCode[j++] = 'n';
                    luaCode[j++] = 'i';
                    luaCode[j++] = 'l';
                    i += 4;
                    continue;
                }
            }
        }

        if (!inLineComment && !inBlockComment) {
            luaCode[j++] = c;
        }
        i++;
    }
    if (builder) {
        luaCode[j++] = '\n';
        luaCode[j++] = 'b';
        luaCode[j++] = 'u';
        luaCode[j++] = 'i';
        luaCode[j++] = 'l';
        luaCode[j++] = 'd';
        luaCode[j++] = '(';
        luaCode[j++] = 'P';
        luaCode[j++] = 'r';
        luaCode[j++] = 'o';
        luaCode[j++] = 'j';
        luaCode[j++] = 'e';
        luaCode[j++] = 'c';
        luaCode[j++] = 't';
        luaCode[j++] = ':';
        luaCode[j++] = 'n';
        luaCode[j++] = 'e';
        luaCode[j++] = 'w';
        luaCode[j++] = '(';
        luaCode[j++] = ')';
        luaCode[j++] = ')';
    }
    luaCode[j] = '\0';
    return luaCode;
}

int ilua_main(char* argv[]) {
    char* filename = argv[1];
    int builder = 0;
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

    char* code = readFile(filename);


    if (!code) {
        fprintf(stderr, "No se pudo leer el archivo: %s\n", filename);
        return 1;
    }
    char* luaCode = (char*)transpile(code, builder);
    free(code);

    // Crear nuevo estado Lua
    lua_State* L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "No se pudo crear el estado Lua.\n");
        return 1;
    }

    // Abrir librerías estándar
    luaL_openlibs(L);
    runLua(L,"function shell(cmd)\n"
                        "local f = io.popen(cmd, \"r\")\n"
                        "if not f then return nil, nil end\n"
                        "local out = f:read(\"*a\")\n"
                        "local ok, _, code = f:close()\n"
                        "return out, code\n"
             "end",
            "iluaExtraBuildings"
    );

    if (builder) {
        runLua(L, iccBuildLua, "iccBuilder");

    }
    // Ejecutar archivo Lua
    if (runLua(L, luaCode, filename) != LUA_OK) {
        const char* error_msg = lua_tostring(L, -1);
        fprintf(stderr, "%s: %s\n", filename, error_msg);
        lua_pop(L, 1);
        lua_close(L);
        return 1;
    }

    lua_close(L);
    free(luaCode);
    return 0;
}
