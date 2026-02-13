#include "transpiler.hpp"
#include <string>
#include <unordered_map>
#include <cctype>
#include <array>
#include <iostream>
namespace flowcpp {

// ---------------------------------
// Utilidades léxicas
// ---------------------------------

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(s[start])) start++;

    if (start == s.size()) return ""; // todo era espacio

    size_t end = s.size() - 1;
    while (end > start && std::isspace(s[end])) end--;

    return s.substr(start, end - start + 1);
}

static bool isIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

static bool isBoundary(char c) {
    return !isIdentChar(c);
}

static bool isBoundary(char prev, char next) {
    return !isIdentChar(prev) && !isIdentChar(next);
}

static bool isEscaped(const std::string& code, size_t pos) {
    if (pos == 0) return false;

    int count = 0;
    int p = static_cast<int>(pos) - 1;

    while (p >= 0 && code[p] == '\\') {
        ++count;
        --p;
    }

    return (count % 2) == 1;
}

static bool isSpace(char c) {
    return c == ' ' || c == '\n' || c == '\t' ||
           c == '\v' || c == '\r' || c == '\f';
}

// ---------------------------------
// Eliminación de comentarios
// ---------------------------------

static std::string delete_comments(const std::string& code) {
#if defined(IFC)
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inChar = false;
    bool inString = false;

    std::string result;
    result.reserve(code.size());

    for (size_t i = 0; i < code.size(); ++i) {

        char c = code[i];
        char next = (i + 1 < code.size()) ? code[i + 1] : '\0';

        if (inLineComment) {
            if (c == '\n')
                inLineComment = false;
            continue;
        }

        if (inBlockComment) {
            if (c == '*' && next == '/') {
                ++i;
                inBlockComment = false;
            }
            continue;
        }

        if (!(inChar || inString)) {
            if (c == '/' && next == '/') {
                inLineComment = true;
                ++i;
                continue;
            }
            if (c == '/' && next == '*') {
                inBlockComment = true;
                ++i;
                continue;
            }
        }

        if (c == '"' && !inChar && !isEscaped(code, i))
            inString = !inString;
        else if (c == '\'' && !inString && !isEscaped(code, i))
            inChar = !inChar;

        result += c;
    }
    return result;
#else
    return code;
#endif
}

// ---------------------------------
// Transpiler principal
// ---------------------------------

std::string transpile(const std::string& rawCode) {

    std::string code = delete_comments(rawCode);
    size_t line = 1;
    // ifn: herramienta para que VSCode marque errores dentro de #if 
    // y que no compile si se olvida de reemplazarlo correctamente por #if

    // Uso típico:
    // #ifn defined(IFC)    <-- VSCode resalta, no compila si no se cambia
    // ... código ...
    // #endif
#if defined(IFC)
    ; //esto para que detecte a cppCode y de error en esta linea y no en la declaraccion de cppCode
    std::string cppCode;

    size_t i = 0;
    size_t n = code.size();

    bool inString = false;
    bool inChar = false;
    bool inInclude = false;
    bool flowUsed = false;

    int pendingUntil = 0;
    int parenDepth = 0;

    std::unordered_map<std::string, std::string> typeMap = {
        {"str",  "flow::str"},
        {"wstr", "flow::wstr"},
        {"any",  "flow::any"}
    };

    while (i < n) {

        char c = code[i];
        if (c == '\n') line++;
        // ---------------- Strings / chars ----------------

        if (c == '"' && !inChar && !isEscaped(code, i))
            inString = !inString;
        else if (c == '\'' && !inString && !isEscaped(code, i))
            inChar = !inChar;

        // ---------------- Includes ----------------

        if (!(inString || inChar)) {
            if (i + 8 <= n && code.substr(i, 8) == "#include")
                inInclude = true;
        }

        if (inInclude && c == '\n')
            inInclude = false;

        // ---------------- Transformaciones Flow ----------------

        if (!(inString || inChar || inInclude)) {

            // [expr]:Type: o ?
            if (c == '[') {

                size_t j = i + 1;
                int depth = 1;
                bool localString = false;
                bool localChar = false;

                while (j < n && depth > 0) {
                    char cj = code[j];

                    if (cj == '"' && !localChar && !isEscaped(code, j))
                        localString = !localString;
                    else if (cj == '\'' && !localString && !isEscaped(code, j))
                        localChar = !localChar;
                    else if (!(localString || localChar)) {
                        if (cj == '[') ++depth;
                        else if (cj == ']') --depth;
                    }

                    ++j;
                }

                if (depth == 0 && j < n && code[j] == ':') {

                    size_t typeStart = j + 1;
                    size_t k = typeStart;

                    while (k < n && code[k] != ':' && code[k] != '\n')
                        ++k;

                    if (k < n && code[k] == ':') {

                        bool verify = (k + 1 < n && code[k + 1] == '?');

                        std::string expr =
                            code.substr(i + 1, j - i - 2);

                        std::string rawType =
                            code.substr(typeStart, k - typeStart);

                        std::string typ =
                            typeMap.count(rawType)
                            ? typeMap[rawType]
                            : rawType;

                        if (verify) {
                            cppCode += "flow::any_comprobate<" + typ + ">(" + expr + ")";
                            i = k + 2;
                        } else {
                            cppCode += "flow::any_cast<" + typ + ">(" + expr + ")";
                            i = k + 1;
                        }

                        flowUsed = true;
                        continue;
                    }
                }
            }

            // #import
            if (i + 7 <= n && code.substr(i, 7) == "#import") {

                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 7 < n) ? code[i + 7] : '\0';

                if (isBoundary(prev, nextc)) {

                    cppCode += "#include \"";

                    size_t j = i + 7;
                    std::string name;

                    while (j < n && code[j] != '\n') {
                        name += code[j];
                        ++j;
                    }

                    cppCode += name + ".hpp\"\n";
                    i = j;
                    continue;
                }
            }

            // until
            if (i + 5 <= n && code.substr(i, 5) == "until") {

                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 5 < n) ? code[i + 5] : '\0';

                if (isBoundary(prev, nextc)) {
                    cppCode += "while (!";
                    ++pendingUntil;
                    i += 5;
                    continue;
                }
            }

            // unless
            if (i + 6 <= n && code.substr(i, 6) == "unless") {

                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 6 < n) ? code[i + 6] : '\0';

                if (isBoundary(prev, nextc)) {
                    cppCode += "if (!";
                    ++pendingUntil;
                    i += 6;
                    continue;
                }
            }
            
            // fn / fn<...>
            if (i+2 <= n && code.substr(i, 2) == "fn") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextChar = (i + 2 < n) ? code[i + 2] : '\0';

                if (isBoundary(prev, nextChar)) {
                    bool closeTemplFound = true;
                    size_t j = i + 3;
                    while(isBoundary(code[j])) {
                        if (code[j] == '<') {
                            closeTemplFound = false;
                            break;
                        }
                        j++;
                    }
                    j = i+3;
                    int ltGtSymbols = 1;
                    bool arrowFound = false;
                    
                    std::string name;
                    std::string type;
                    std::string templateArgs ="<";
                    char now = '\0';

                    while (j < n && (now = code[j]) != '{') {
                        char nextLook = ((j + 1) < n) ? code[j+1] : '\0';
                        
                        if (!closeTemplFound) {
                            if (now == '<')  ltGtSymbols++;
                            else if (now == '>')  ltGtSymbols--;

                            if (ltGtSymbols == 0) closeTemplFound = true;
                        }

                        if (closeTemplFound && !arrowFound && now == '-' && nextLook == '>') {
                            arrowFound = true;
                            j += 2; // saltamos '->'
                            while (j < n && isspace(code[j])) j++; // salta espacios antes del tipo
                            continue;
                        }

                        if (!arrowFound && !closeTemplFound)
                            templateArgs += now;
                        else if (!arrowFound)
                            name += now;
                        else
                            type += now;

                        j++;
                    }
                    // Limpiar espacios
                    templateArgs = trim(templateArgs);
                    name = trim(name);
                    type = trim(type);

                    if (!closeTemplFound) {
                        std::cerr << "template function not closed in line " << line;
                    }
                    if (((i + 2 < n) ? code[i + 2] : '\0') == '<')
                        cppCode += type.empty() ? "template " + templateArgs + "void " + name :"template " + templateArgs + type + " " + name;
                    else
                        cppCode += type.empty() ? "void " + name : type + " " + name;
                    i = j;
                }
            }
            // type / type<...>

            if (i+4 <= n && code.substr(i, 4) == "type") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextChar = (i + 4 < n) ? code[i + 4] : '\0';
                if (isBoundary(prev, nextChar)) {
                    bool isTemplate = true;
                    size_t j = i + 4;
                    while(isBoundary(code[j])) {
                        if (code[j] == '<') {
                            isTemplate = false;
                            break;
                        }
                        j++;
                    }
                    j = i+4;
                    if (isTemplate) {
                        char now;
                        size_t ltGtSymbols = 1;
                        std::string templateArgs = "<";
                        while (j<n && ltGtSymbols != 0) {
                            now = code[j];
                            if (now == '<') ltGtSymbols++;
                            else if (now == '>') ltGtSymbols--;
                            templateArgs += now;
                            j++;
                        }
                        cppCode += "template " + templateArgs + " using";
                    } else {
                        cppCode += "using";
                    }
                }
            }

            // reemplazos tipo
            const std::pair<std::string, std::string> kws[] = {
                {"any",  "flow::any"},
                {"anyP", "flow::anyP"},
                {"str",  "flow::str"},
                {"wstr", "flow::wstr"}
            };

            for (auto& [kw, repl] : kws) {

                size_t len = kw.size();

                if (i + len <= n && code.substr(i, len) == kw) {

                    char prev = (i > 0) ? code[i - 1] : '\0';
                    char nextc = (i + len < n) ? code[i + len] : '\0';

                    if (isBoundary(prev, nextc)) {
                        cppCode += repl;
                        flowUsed = true;
                        i += len;
                        goto continue_main;
                    }
                }
            }

            // Cfn
            if (i + 3 <= n && code.substr(i, 3) == "Cfn") {

                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 3 < n) ? code[i + 3] : '\0';

                if (isBoundary(prev, nextc)) {
                    cppCode += "extern \"C\"";
                    i += 3;
                    continue;
                }
            }

            // defer
            if (i + 5 <= n && code.substr(i, 5) == "defer") {

                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 5 < n) ? code[i + 5] : '\0';

                if (isBoundary(prev, nextc)) {

                    size_t exprBegin = i + 5;

                    while (exprBegin < n && isSpace(code[exprBegin]))
                        ++exprBegin;

                    size_t exprEnd = exprBegin;

                    while (exprEnd < n && code[exprEnd] != ';')
                        ++exprEnd;
                    if (exprBegin < exprEnd) {
                        cppCode += "flow::Defer([&](){" +
                                   code.substr(exprBegin, (exprEnd+1) - exprBegin) +
                                   "})";
                        i = exprEnd + 1;
                    } else {
                        cppCode += "flow::Defer()";
                        i += 5;
                    } [[maybe_unused]]  size_t line = 1;
                    continue;
                }
            }
        }

        // ---------------- Paréntesis ----------------

        if (c == '(')
            ++parenDepth;
        else if (c == ')') {
            --parenDepth;
            if (pendingUntil > 0 && parenDepth == 0) {
                cppCode += "))";
                --pendingUntil;
                ++i;
                continue;
            }
        }

        cppCode += c;
        ++i;

    continue_main:
        ;
    }

    if (flowUsed &&
        cppCode.find("#include <flow/types>") == std::string::npos)
    {
        cppCode = "#include <flow/types>\n" + cppCode;
    }

    return cppCode;
#else
    return code;
#endif
}

}
