#pragma once
#include <stdio.h>
#include <stdlib.h>
char* readFile(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Reservar memoria
    char *buffer = (char*)malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, f);
        buffer[length] = '\0';
    }

    fclose(f);
    return buffer;
}

char* readFileWithOffset(const char* filename, size_t offset, size_t* lenght_of_read) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Reservar memoria
    char *buffer = (char*)malloc(length + offset + 1);
    if (buffer) {
        *lenght_of_read = fread(buffer, 1, length, f);
        buffer[length] = '\0';
    }

    fclose(f);
    return buffer;
}