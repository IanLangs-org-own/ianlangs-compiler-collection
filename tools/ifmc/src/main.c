#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "utils.h"
#include "transpile.h"

int compile_C(int argc, char** argv, const char* out) {
    create_dir("dist", true);
    create_dir("dist/c", true);
    char buffer[4096];
    FILE* file = fopen(argv[2], "r");

    if(file == NULL) {
        printf("Error: No se pudo abrir el archivo %s.\n", argv[2]);
        return 1;
    }

    char* code;
    size_t len = 0UL;
    while (fgets(buffer, 4096, file) != NULL) {
        len += strlen(buffer);
    }

    code = string_dump("", len);
    
    fclose(file);

    file = fopen(argv[2],"r");

    while (fgets(buffer, 4096, file) != NULL) {
        strcat(code, buffer);
    }

    char* path = malloc(strlen(out)+strlen("dist/c/")+1);
    if (path == NULL) {
        fprintf(stderr, "can't alloc the path var");
        return 1;
    }
    strcpy(path, "dist/c/");
    strcat(path, out);

    char* cCode = transpile_fm2C(code);
    FILE* outf = fopen(path, "w");
    if (!outf) {
        fprintf(stderr, "Error al crear/escribir archivo %s\n", path);
        return 1;
    }
    fputs(cCode, outf);
    fclose(outf);
    fclose(file);
    free(cCode);
    free(code);
    free(path);
    return 0;
}

int compile_Obj(int argc, char** argv, const char* out, const char* cc) {
    create_dir("dist", true);
    create_dir("dist/c", true);
    create_dir("dist/obj", true);
    char* path = malloc(strlen(out)+1);
    if (path == NULL) {
        fprintf(stderr, "can't alloc the path var");
        return 1;
    }
    strcpy(path, out);
    path[strlen(path)-1] = 'c';
    int err = compile_C(argc, argv, "a.c");
    if (err) return err;
    char* cmd = malloc(strlen(cc)+strlen(out)+30);
    if (cmd == NULL) {
        fprintf(stderr, "can't alloc the command var");
        return 1;
    }
    cmd[0] = 0;
    strcat(cmd, cc);
    strcat(cmd, " -c -o dist/obj/");
    strcat(cmd, out);
    strcat(cmd, " dist/c/a.c");
    int exitCode = system(cmd);

    free(cmd);

    return exitCode % 256;
}

int compile_executable(int argc, char** argv, const char* out, const char* cc) {
    int exitCode = compile_Obj(argc, argv, "a.o", cc);
    if (exitCode) return exitCode;

    char* cmd = malloc(strlen(cc) + strlen(" dist/obj/a.o -o dist/") + strlen(out)+ 1);
    if (cmd == NULL) {
        fprintf(stderr, "can't alloc the command var");
        return 1;
    }
    cmd[0] = 0;
    strcat(cmd, cc);
    strcat(cmd, " dist/obj/a.o -o dist/");
    strcat(cmd, out);
    printf("%s\n", cmd);
    system(cmd);
    free(cmd);
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 1 || argc == 2 && strcmp(argv[1], "help") == 0) {
        FILE* out = argc == 1 ? stderr : stdout ;
        fprintf(out, "Uso:\n\t%s C file.fm", argv[0]);
        return argc == 1;
    }
    if (argc >= 3 && strcmp(argv[1], "C") == 0) {
        char* out = "a.c";
        bool need_free = false;
        if (argc >= 4 && strncmp(argv[3], "-o:", strlen("-o:")) == 0) {
            out = malloc(strlen(argv[3]+3)+1);
            memcpy(out, argv[3]+3, strlen(argv[3]+3)+1);
            need_free = true;
        }
        int exitCode = compile_C(argc, argv, out);
        if (need_free) free(out);
        return exitCode;
    } else if (argc >= 3 && strcmp(argv[1], "Obj") == 0) {
        char* out = "a.o";
        bool need_free = false;
        char* cc = "clang";
        bool need_free_cc = false;
        for (int i = 4; i <= 5; i++) {
            if (argc >= i && strncmp(argv[i-1], "-o:", strlen("-o:")) == 0) {
                out = malloc(strlen(argv[i-1]+3)+2);
                strcpy(out, argv[i-1]+3);
                need_free = true;
            } else if (argc >= i && strncmp(argv[i-1], "-cc:", strlen("-cc:")) == 0) {
                cc = malloc(strlen(argv[i-1]+4)+2);
                strcpy(cc, argv[i-1]+4);
                need_free_cc = true;
            }
        }
        int exitCode = compile_Obj(argc, argv, out, cc);
        if (need_free) free(out);
        if (need_free_cc) free(cc);
        return exitCode;
    } else if (argc >= 3 && strcmp(argv[1], "Build") == 0) {
        #ifdef _WIN32
        char* out = "a.exe";
        #elif defined(__linux__) || defined(__APPLE__)
        char* out = "a.out";
        #else
        #error "Unsupported platform"
        #endif
        bool need_free = false;
        char* cc = "clang";
        bool need_free_cc = false;
        for (int i = 4; i <= 5; i++) {
            if (argc >= i && strncmp(argv[i-1], "-o:", strlen("-o:")) == 0) {
                out = malloc(strlen(argv[i-1]+3)+2);
                strcpy(out, argv[i-1]+3);
                need_free = true;
            } else if (argc >= i && strncmp(argv[i-1], "-cc:", strlen("-cc:")) == 0) {
                cc = malloc(strlen(argv[i-1]+4)+2);
                strcpy(cc, argv[i-1]+4);
                need_free_cc = true;
            }
        }
        int exitCode = compile_executable(argc, argv, out, cc);
        if (need_free) free(out);
        if (need_free_cc) free(cc);
        return exitCode;
    }
    
}