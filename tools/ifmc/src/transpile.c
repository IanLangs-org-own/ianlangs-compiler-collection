#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include "types.h"
int isspace_cstr(const char *str) {
    // Si el puntero es NULL, podemos considerarlo vacío
    if (str == NULL) return 1;

    // Recorrer la cadena hasta encontrar el final '\0'
    while (*str != '\0') {
        // Si encontramos un caracter que NO es espacio, la cadena no está vacía
        if (!isspace((unsigned char)*str)) {
            return 0; // No está vacía
        }
        str++;
    }
    return 1; // Solo contenía espacios
}

/* ─── String ─────────────────────────────────────────────────────────────── */

typedef struct {
    char*  ptr;
    size_t len;
    size_t capacity;
} String;

String* new_str(const char* data) {
    if (!data) return NULL;
    size_t len = strlen(data);
    String* self = malloc(sizeof(*self));
    if (!self) return NULL;
    self->ptr = malloc(len);
    if (!self->ptr) { free(self); return NULL; }
    memcpy(self->ptr, data, len);
    self->len = self->capacity = len;
    return self;
}

String* str_alloc(size_t size) {
    String* self = malloc(sizeof(*self));
    if (!self) return NULL;
    self->ptr = calloc(size, 1);
    if (!self->ptr) { free(self); return NULL; }
    self->len = 0;
    self->capacity = size;
    return self;
}

/* Retorna copia null-terminated. El caller es dueño. */
char* cstr(const String* self) {
    char* str = malloc(self->len + 1);
    if (!str) return NULL;
    memcpy(str, self->ptr, self->len);
    str[self->len] = '\0';
    return str;
}

int cstr_concat(String* self, const char* s) {
    size_t slen    = strlen(s);
    size_t new_len = self->len + slen;

    if (new_len > self->capacity) {
        size_t new_cap = self->capacity ? self->capacity : 1;
        while (new_cap < new_len) new_cap <<= 1;
        void* tmp = realloc(self->ptr, new_cap);
        if (!tmp) return -1;
        self->ptr      = tmp;
        self->capacity = new_cap;
    }

    memcpy(self->ptr + self->len, s, slen);
    self->len = new_len;
    return 0;
}

int str_concat(String* self, const String* other) {
    char* tmp = cstr(other);
    if (!tmp) return -1;
    int r = cstr_concat(self, tmp);
    free(tmp);
    return r;
}

void free_str(String** self) {
    if (!self || !*self) return;
    free((**self).ptr);
    free(*self);
    *self = NULL;
}

#define scaped(str, index) ({ \
    size_t count = 0; \
    for (int _i = (index) - 1; _i >= 0; _i--) { \
        if ((str)[_i] == '\\') count++; \
        else break; \
    } \
    (count % 2) == 1; \
})

/* ─── Helpers ────────────────────────────────────────────────────────────── */

static void skip_spaces(const char* s, size_t* i) {
    while (s[*i] == ' ' || s[*i] == '\t') (*i)++;
}

static size_t read_ident(const char* s, size_t* i, char* buf, size_t cap) {
    size_t n = 0;
    while ((isalnum((unsigned char)s[*i]) || s[*i] == '_') && n < cap - 1)
        buf[n++] = s[(*i)++];
    buf[n] = '\0';
    return n;
}

static size_t read_ident_extra(const char* s, size_t* i, char* buf, size_t cap, char extras_chars[]) {
    size_t n = 0;
    while ((isalnum((unsigned char)s[*i]) || s[*i] == '_' || strchr(extras_chars, s[*i]) != NULL) && n < cap - 1)
        buf[n++] = s[(*i)++];
    buf[n] = '\0';
    return n;
}

/*
 * TypeInfo: resultado de parsear un tipo iformath.
 * is_array = true  → [T]  — al emitir: "ctype name[]"
 * is_array = false → T    — al emitir: "ctype name"
 */
typedef struct {
    char ctype[256]; /* tipo C base, ej "long double", "char*", "int*" */
    bool is_array;   /* [T] → emite T name[] */
} TypeInfo;

/*
 * Lee un tipo iformath desde s[*i] y llena ti.
 * Formatos:
 *   number | bool | char | str | <ident>   → tipo simple
 *   [T]                                    → array de T
 *   {cType}                                → raw C type
 */
static void read_type(const char* s, size_t* i, TypeInfo* ti) {
    ti->is_array = false;

    /* {cType} */
    if (s[*i] == '{') {
        (*i)++;
        size_t n = 0;
        while (s[*i] && s[*i] != '}' && n < sizeof(ti->ctype) - 1)
            ti->ctype[n++] = s[(*i)++];
        ti->ctype[n] = '\0';
        if (s[*i] == '}') (*i)++;
        return;
    }

    /* [T] */
    if (s[*i] == '[') {
        (*i)++;
        char inner[128] = {0};
        size_t n = 0;
        while (s[*i] && s[*i] != ']' && n < sizeof(inner) - 1)
            inner[n++] = s[(*i)++];
        inner[n] = '\0';
        if (s[*i] == ']') (*i)++;
        ti->is_array = true;
        /* mapea el tipo interno */
        TypeInfo inner_ti;
        size_t j = 0;
        read_type(inner, &j, &inner_ti);
        snprintf(ti->ctype, sizeof(ti->ctype), "%s", inner_ti.ctype);
        return;
    }

    /* tipo simple */
    char tok[128];
    read_ident(s, i, tok, sizeof(tok));

    if      (strcmp(tok, "float") == 0)  snprintf(ti->ctype, sizeof(ti->ctype), "long double");
    else if (strcmp(tok, "int") == 0)    snprintf(ti->ctype, sizeof(ti->ctype), "long long");
    else if (strcmp(tok, "uint") == 0)   snprintf(ti->ctype, sizeof(ti->ctype), "unsigned long long");
    else if (strcmp(tok, "bool")   == 0) snprintf(ti->ctype, sizeof(ti->ctype), "bool");
    else if (strcmp(tok, "char")   == 0) snprintf(ti->ctype, sizeof(ti->ctype), "char");
    else if (strcmp(tok, "str")    == 0) snprintf(ti->ctype, sizeof(ti->ctype), "char*"); 
    else if (isspace_cstr(tok)) ;
    else {
        fprintf(stderr, "invalid type '%s'\n", tok);
        fflush(stderr);
        exit(1);
    }
}

/*
 * Emite la declaración de una variable C dado tipo y nombre.
 * Si is_array: "ctype name[]"
 * Si no:       "ctype name"
 */
static void emit_var_decl(const TypeInfo* ti, const char* name, String* out) {
    if (ti->is_array) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s %s[]", ti->ctype, name);
        cstr_concat(out, buf);
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s %s", ti->ctype, name);
        cstr_concat(out, buf);
    }
}

/*
 * Parsea lista de parámetros iformath (s[*i] justo después de '(').
 * Cada param: [Type] name
 * Sin tipo → long double por defecto.
 * Emite en out_params la lista C lista para poner en firma de función.
 * Para arrays: "long double name[]"
 */
static void parse_params(const char* s, size_t* i, String* out_params) {
    bool first = true;

    while (s[*i] && s[*i] != ')') {
        skip_spaces(s, i);
        if (s[*i] == ')') break;

        // ── Si empieza con " o ', es un argumento literal, copiarlo directo ──
        if (s[*i] == '"' || s[*i] == '\'') {
            char delim = s[(*i)++];
            if (!first) cstr_concat(out_params, ", ");
            first = false;
            char pbuf[1024]; size_t n = 0;
            pbuf[n++] = delim;
            while (s[*i] && !(s[*i] == delim && !scaped(s, *i))) {
                if (n < sizeof(pbuf) - 2) pbuf[n++] = s[(*i)++];
                else (*i)++;
            }
            if (s[*i] == delim) pbuf[n++] = s[(*i)++];
            pbuf[n] = '\0';
            cstr_concat(out_params, pbuf);
            skip_spaces(s, i);
            if (s[*i] == ',') (*i)++;
            continue;
        }
        skip_spaces(s, i);
        if (s[*i] == ')') break;

        /* Intentar leer tipo + nombre, o solo nombre */
        size_t saved = *i;
        char tok1[128];
        read_ident(s, i, tok1, sizeof(tok1));
        skip_spaces(s, i);

        TypeInfo ti;
        char name[128];

        /* ¿Hay un segundo token antes de ',' o ')'? */
        bool has_second = s[*i] && s[*i] != ',' && s[*i] != ')';

        if (has_second && (s[*i] == '[' || s[*i] == '{')) {
            /* tok1 es nombre, pero el tipo empieza con [ o { — inusual,
               tratar tok1 como nombre de tipo simple y leer tipo especial */
            size_t j = 0;
            read_type(tok1, &j, &ti);  /* parsea tok1 como tipo */
            read_type(s, i, &ti);       /* sobreescribe con el tipo especial */
            skip_spaces(s, i);
            read_ident(s, i, name, sizeof(name));
        } else if (has_second) {
            /* Podría ser "Type name": tok1 es tipo, siguiente token es nombre */
            size_t saved2 = *i;
            char tok2[128];
            read_ident(s, i, tok2, sizeof(tok2));
            skip_spaces(s, i);

            if (strlen(tok2) > 0 && (s[*i] == ',' || s[*i] == ')' ||
                                      s[*i] == ' ' || s[*i] == '\t' || s[*i] == '\0')) {
                /* "Type name" */
                size_t j = 0;
                read_type(tok1, &j, &ti);
                strncpy(name, tok2, sizeof(name) - 1);
                name[sizeof(name)-1] = '\0';
            } else {
                /* Solo nombre, retrocede */
                *i = saved2;
                ti.is_array = false;
                snprintf(ti.ctype, sizeof(ti.ctype), "long double");
                strncpy(name, tok1, sizeof(name) - 1);
                name[sizeof(name)-1] = '\0';
            }
        } else {
            /* Solo nombre */
            (void)saved;
            ti.is_array = false;
            snprintf(ti.ctype, sizeof(ti.ctype), "long double");
            strncpy(name, tok1, sizeof(name) - 1);
            name[sizeof(name)-1] = '\0';
        }

        if (!first) cstr_concat(out_params, ", ");
        first = false;

        /* Emite param */
        char pbuf[512];
        if (ti.is_array)
            snprintf(pbuf, sizeof(pbuf), "%s %s[]", ti.ctype, name);
        else
            snprintf(pbuf, sizeof(pbuf), "%s %s", ti.ctype, name);
        cstr_concat(out_params, pbuf);

        skip_spaces(s, i);
        if (s[*i] == ',') (*i)++;
    }
}

/* ─── Bloque C verbatim ──────────────────────────────────────────────────── */

/*
 * Consume "C [macro] do...done" o "C do...done" desde code[*i].
 * *i apunta justo después de 'C'.
 * Emite verbatim (con #ifdef si hay macro).
 */
static void handle_c_block(const char* code, size_t* i, String* out) {
    skip_spaces(code, i);

    char macro[128] = {0};
    bool has_macro = false;

    if (code[*i] == '[') {
        (*i)++;
        size_t n = 0;
        while (code[*i] && code[*i] != ']' && n < sizeof(macro) - 1)
            macro[n++] = code[(*i)++];
        macro[n] = '\0';
        if (code[*i] == ']') (*i)++;
        has_macro = true;
        skip_spaces(code, i);
    }

    /* consume "do" */
    if (strncmp(code + *i, "do", 2) == 0) {
        *i += 2;
        if (code[*i] == '\n') (*i)++;
    }

    if (has_macro) {
        char hdr[256];
        snprintf(hdr, sizeof(hdr), "#ifdef %s\n", macro);
        cstr_concat(out, hdr);
    }

    while (code[*i]) {
        if (strncmp(code + *i, "done", 4) == 0) {
            char after = code[*i + 4];
            if (!isalnum((unsigned char)after) && after != '_') {
                *i += 4;
                break;
            }
        }
        char ch[2] = { code[(*i)++], '\0' };
        cstr_concat(out, ch);
    }

    if (has_macro) cstr_concat(out, "#endif\n");
}

/* ─── Transpilación de línea ─────────────────────────────────────────────── */

/*
 * Reemplaza todas las ocurrencias de percentOf(expr) por ((expr)/100.0L)
 * en src, escribe resultado en dst (tamaño dst_cap).
 */

typedef struct {
    bool alloc;
    bool realloc;
    bool free;
} ptrflag;
static size_t expand_percentOf(const char* src, char* dst, size_t dst_cap, ptrflag flags) {
    size_t si = 0, di = 0;
    size_t src_len = strlen(src);

    while (si < src_len) {
        if (! (di < dst_cap - 1)) {
            if (flags.realloc) {
                for (size_t attemps = 0; attemps < 5; attemps++) {
                    void* tmp = realloc(dst, (size_t)(dst_cap * 1.5));
                    if (tmp) {
                        dst = tmp;
                        dst_cap = (size_t)(dst_cap * 1.5);
                        break;
                    } else if (attemps++ < 5) {
                        continue;
                    } else {
                        fprintf(stderr, "memory error\n");
                        exit(1);
                    }
                }
            } else  {
                fprintf(stderr, "buffer overflow in expand_percentOf\n");
                exit(1);
            }
        }
        /* ¿Empieza "percentOf(" acá? */
        if (strncmp(src + si, "percentOf(", 10) == 0) {
            si += 10; /* salta "percentOf(" */

            /* Copia el prefijo de reemplazo */
            const char* prefix = "((";
            for (size_t k = 0; prefix[k] && di < dst_cap - 1; k++)
                dst[di++] = prefix[k];

            /* Copia el contenido del paréntesis (anidado) */
            int depth = 1;
            while (si < src_len && depth > 0 && di < dst_cap - 1) {
                if (src[si] == '(') depth++;
                else if (src[si] == ')') {
                    depth--;
                    if (depth == 0) { si++; break; }
                }
                dst[di++] = src[si++];
            }

            /* Sufijo */
            const char* suffix = ")/100.0L)";
            for (size_t k = 0; suffix[k] && di < dst_cap - 1; k++)
                dst[di++] = suffix[k];
        } else {
            dst[di++] = src[si++];
        }
    }
    dst[di] = '\0';
    return strlen(dst);
}

static void transpile_line(const char* line, String* out) {
    size_t i = 0;
    skip_spaces(line, &i);
    if (!line[i]) { cstr_concat(out, "\n"); return; }

    char tok[256];
    size_t saved = i;

    read_ident(line, &i, tok, sizeof(tok));

    /* ── main do ──────────────────────────────────────────────────────────── */
    if (strcmp(tok, "main") == 0) {
        skip_spaces(line, &i);
        char kw[8] = {0};
        read_ident(line, &i, kw, sizeof(kw));
        if (strcmp(kw, "do") == 0) {
            cstr_concat(out, "int main(int argc, char** argv) {\n");
            return;
        }
        i = saved;
        goto fallback;
    }

    if (strcmp(tok, "cimport") == 0) {
        char header[256] = {0};
        skip_spaces(line, &i);
        read_ident(line, &i, header, sizeof header);
        cstr_concat(out, "#include \"");
        cstr_concat(out, header);
        cstr_concat(out, ".h\"\n");
        return;
    }

    /* ── var / const ──────────────────────────────────────────────────────── */
    if (strcmp(tok, "var") == 0 || strcmp(tok, "const") == 0) {
        bool is_const = tok[0] == 'c';
        skip_spaces(line, &i);
        TypeInfo ti;
        read_type(line, &i, &ti);
        skip_spaces(line, &i);
        char name[128];
        read_ident(line, &i, name, sizeof(name));
        skip_spaces(line, &i);

        if (is_const) cstr_concat(out, "const ");
        if (line[i] == '=') {
            i++; skip_spaces(line, &i);
            char expr[1024] = {0};
            size_t n = 0;
            while (line[i] && line[i] != '\n' && n < sizeof(expr) - 1)
                expr[n++] = line[i++];
            expr[n] = '\0';
            emit_var_decl(&ti, name, out);
            char tail[1100];
            snprintf(tail, sizeof(tail), " = %s;\n", expr);
            cstr_concat(out, tail);
        } else {
            emit_var_decl(&ti, name, out);
            cstr_concat(out, ";\n");
        }
        return;
    }

    /* ── multiline fn ─────────────────────────────────────────────────────── */
    i = saved;
    if (strncmp(line + i, "multiline", 9) == 0 &&
        !isalnum((unsigned char)line[i+9]) && line[i+9] != '_') {
        i += 9; skip_spaces(line, &i);
        if (strncmp(line + i, "fn", 2) == 0 &&
            !isalnum((unsigned char)line[i+2]) && line[i+2] != '_') {
            i += 2; skip_spaces(line, &i);
            char fname[128];
            read_ident(line, &i, fname, sizeof(fname));
            skip_spaces(line, &i);
            String* params = str_alloc(256);
            if (line[i] == '(') { i++; parse_params(line, &i, params); if (line[i] == ')') i++; }
            skip_spaces(line, &i);
            TypeInfo ret; snprintf(ret.ctype, sizeof(ret.ctype), "long double"); ret.is_array = false;
            if (line[i] == '-' && line[i+1] == '>') {
                i += 2; skip_spaces(line, &i);
                read_type(line, &i, &ret); skip_spaces(line, &i);
            }
            if (strncmp(line + i, "do", 2) == 0) {
                char hdr[768]; char* pstr = cstr(params);
                snprintf(hdr, sizeof(hdr), "%s %s(%s) {\n", ret.ctype, fname, pstr ? pstr : "");
                free(pstr); cstr_concat(out, hdr);
            }
            free_str(&params);
            return;
        }
    }

    /* ── done ─────────────────────────────────────────────────────────────── */
    i = saved; read_ident(line, &i, tok, sizeof(tok));
    if (strcmp(tok, "done") == 0) { cstr_concat(out, "}\n"); return; }

    /* ── if / elif ────────────────────────────────────────────────────────── */
    i = saved; read_ident(line, &i, tok, sizeof(tok));
    if (strcmp(tok, "if") == 0 || strcmp(tok, "elif") == 0) {
        bool is_elif = tok[1] == 'l';
        skip_spaces(line, &i);
        char cond[512] = {0}; size_t cn = 0;
        while (line[i]) {
            if (strncmp(line + i, "do", 2) == 0) {
                char after = line[i+2];
                if (!isalnum((unsigned char)after) && after != '_') { i += 2; break; }
            }
            if (cn < sizeof(cond) - 1) cond[cn++] = line[i];
            i++;
        }
        while (cn > 0 && cond[cn-1] == ' ') cn--;
        cond[cn] = '\0';
        char hdr[640];
        snprintf(hdr, sizeof(hdr), "%s (%s) {\n", is_elif ? "} else if" : "if", cond);
        cstr_concat(out, hdr);
        return;
    }

    /* ── else ─────────────────────────────────────────────────────────────── */
    i = saved; read_ident(line, &i, tok, sizeof(tok));
    if (strcmp(tok, "else") == 0) { cstr_concat(out, "} else {\n"); return; }

    /* ── return ───────────────────────────────────────────────────────────── */
    i = saved; read_ident(line, &i, tok, sizeof(tok));
    if (strcmp(tok, "return") == 0) {
        skip_spaces(line, &i);
        char expr[1024] = {0}; size_t n = 0;
        while (line[i] && line[i] != '\n' && n < sizeof(expr) - 1)
            expr[n++] = line[i++];
        expr[n] = '\0';
        /* strip trailing ';' si lo tiene */
        while (n > 0 && (expr[n-1] == ';' || expr[n-1] == ' ')) expr[--n] = '\0';
        char buf[1100];
        snprintf(buf, sizeof(buf), "return %s;\n", expr);
        cstr_concat(out, buf);
        return;
    }

    /* ── Función de una línea: name(params) [-> Type] = expr ─────────────── */
    i = saved;
    {
        char fname[128];
        read_ident(line, &i, fname, sizeof(fname));
        skip_spaces(line, &i);
        if (strlen(fname) > 0 && line[i] == '(') {
            i++;
            String* params = str_alloc(256);
            parse_params(line, &i, params);
            if (line[i] == ')') i++;
            skip_spaces(line, &i);
            TypeInfo ret; snprintf(ret.ctype, sizeof(ret.ctype), "long double"); ret.is_array = false;
            if (line[i] == '-' && line[i+1] == '>') {
                i += 2; skip_spaces(line, &i);
                read_type(line, &i, &ret); skip_spaces(line, &i);
            }
            if (line[i] == '=') {
                i++; skip_spaces(line, &i);
                char expr[1024] = {0}; size_t n = 0;
                while (line[i] && line[i] != '\n' && n < sizeof(expr) - 1)
                    expr[n++] = line[i++];
                expr[n] = '\0';
                char* pstr = cstr(params);
                char buf[1600];
                snprintf(buf, sizeof(buf), "%s %s(%s) { return %s; }\n",
                         ret.ctype, fname, pstr ? pstr : "", expr);
                free(pstr); free_str(&params);
                cstr_concat(out, buf);
                return;
            }
            free_str(&params);
            goto fallback;
        }
    }

fallback:
    i = saved;
    {
        // copiar la línea
        char fbuf[4096]; size_t fn = 0;
        while (line[i] && line[i] != '\n' && fn < sizeof(fbuf) - 1)
            fbuf[fn++] = line[i++];
        // strip espacios al final
        while (fn > 0 && fbuf[fn-1] == ' ') fn--;
        fbuf[fn] = '\0';

        if (fn == 0) { cstr_concat(out, "\n"); return; }

        // si ya termina en ; { } no agregar nada
        char last = fbuf[fn-1];
        cstr_concat(out, fbuf);
        if (last != ';' && last != '{' && last != '}')
            cstr_concat(out, ";");
        cstr_concat(out, "\n");
    }
}

/* ─── transpile_fm2C ─────────────────────────────────────────────────────── */

char* transpile_fm2C(const char* rcode) {
    char* code = malloc(strlen(rcode) + 1);
    if (!code) {
        fprintf(stderr, "memory error\n");
        exit(1);
    }
    
    size_t code_len = expand_percentOf(rcode, code, strlen(rcode) + 1, (ptrflag){.realloc=true, .alloc=false, .free=false});
    String* out;
    {
        size_t tmp = code_len >> 2;
        out = str_alloc(code_len * (tmp == 0 ? 1 : tmp));
    }
    if (!out) return NULL;

    bool inString  = false;
    bool inChar    = false;
    bool inComment = false;

    char line_buf[4096];
    size_t line_len = 0;
    /* offset en code[] donde empezó la línea actual */
    size_t line_start = 0;

    size_t i = 0;

    while (i <= code_len) {
        char c = (i < code_len) ? code[i] : '\n';

        /* ── tracking string/char/comment ─────────────────────────────────── */
        if (c == '"'  && !scaped(code, i) && !inChar    && !inComment) inString = !inString;
        if (c == '\'' && !scaped(code, i) && !inString  && !inComment) inChar   = !inChar;
        if (c == '#'  && !inString && !inChar) {
            inComment = !inComment;
            i++;
            continue; /* consume '#', no acumular */
        }

        /* ── flush de línea ───────────────────────────────────────────────── */
        if (c == '\n' || i == code_len) {
            line_buf[line_len] = '\0';

            if (!inComment) {
                /* Detectar "C do" o "C [macro] do" como bloque multilinea */
                size_t li = 0;
                while (line_buf[li] == ' ' || line_buf[li] == '\t') li++;

                bool is_c_block = false;
                if (line_buf[li] == 'C' &&
                    !isalnum((unsigned char)line_buf[li+1]) && line_buf[li+1] != '_') {
                    size_t lj = li + 1;
                    while (line_buf[lj] == ' ' || line_buf[lj] == '\t') lj++;
                    if (line_buf[lj] == '[') is_c_block = true;
                    else if (strncmp(line_buf + lj, "do", 2) == 0) {
                        char after = line_buf[lj+2];
                        if (!isalnum((unsigned char)after) && after != '_')
                            is_c_block = true;
                    }
                }

                if (is_c_block) {
                    /* Procesar desde el código original para capturar "done"
                       que puede estar en líneas posteriores */
                    size_t orig = line_start;
                    /* avanza hasta 'C' */
                    while (orig < code_len && (code[orig]==' '||code[orig]=='\t')) orig++;
                    orig++; /* salta 'C' */
                    handle_c_block(code, &orig, out);
                    i = orig;
                    if (i < code_len && code[i] == '\n') i++;
                    line_len = 0;
                    line_start = i;
                    continue;
                }

                transpile_line(line_buf, out);
            }

            line_len   = 0;
            line_start = i + 1;
            i++;
            continue;
        }

        /* ── acumular carácter ────────────────────────────────────────────── */
        if (!inComment) {
            if (line_len < sizeof(line_buf) - 1)
                line_buf[line_len++] = c;
        }
        i++;
    }

    char* result = cstr(out);
    free_str(&out);
    free(code);
    return result;
}

/* ─── main (test) ────────────────────────────────────────────────────────── */

#ifdef FM2C_TEST1
int main(void) {
    const char* src =
        "# esto es un comentario #\n"
        "var float x = 3.14\n"
        "const str msg = \"hola\"\n"
        "var [float] arr = {}\n"
        "var {int*} raw = NULL\n"
        "var float p = percentOf(50)\n"
        "var float q = percentOf(1 + 2 * (3 + 4))\n"
        "\n"
        "add(a, b) = a + b\n"
        "dot(float x, float y) -> float = x * y\n"
        "norm([float] v, float n) -> float = v[0] + n\n"
        "\n"
        "multiline fn factorial(float n) -> float do\n"
        "    if n <= 1 do\n"
        "        return 1\n"
        "    done\n"
        "    return n * factorial(n - 1)\n"
        "done\n"
        "main do\n"
        "\n"
        "C do\n"
        "    printf(\"verbatim C\\n\");\n"
        "done\n"
        "\n"
        "C [_WIN32] do\n"
        "    Sleep(1000);\n"
        "done\n"
        "return 0;\n"
        "done\n";

    char* result = transpile_fm2C(src);
    if (result) {
        printf("/* === C output === */\n%s", result);
        free(result);
    }
    return 0;
}
#endif