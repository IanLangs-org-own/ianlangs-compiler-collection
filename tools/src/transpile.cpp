#include "transpiler.hpp"
#include <string>
#include <unordered_map>
#include <cctype>
#include <array>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
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

#if defined(IFC)
    std::string cppCode;
    size_t i = 0, n = code.size();
    bool inString = false, inChar = false, inInclude = false, flowUsed = false;
    int pendingUntil = 0, parenDepth = 0;

    std::unordered_map<std::string, std::string> typeMap = {
        {"str", "flow::str"},
        {"wstr", "flow::wstr"},
        {"any", "flow::any"}
    };

    while (i < n) {
        char c = code[i];

        // ---------------- Strings / Chars ----------------
        if (c == '"' && !inChar && !isEscaped(code, i)) inString = !inString;
        else if (c == '\'' && !inString && !isEscaped(code, i)) inChar = !inChar;

        // ---------------- Includes ----------------
        if (!(inString || inChar)) {
            if (i + 8 <= n && code.substr(i, 8) == "#include") inInclude = true;
        }
        if (inInclude && c == '\n') inInclude = false;

        // ---------------- Transformaciones Flow ----------------
        if (!(inString || inChar || inInclude)) {
            // [expr]:Type: o [expr]:Type:?
            if (c == '[') {
                size_t j = i + 1, depth = 1;
                bool localString = false, localChar = false;
                while (j < n && depth > 0) {
                    char cj = code[j];
                    if (cj == '"' && !localChar && !isEscaped(code, j)) localString = !localString;
                    else if (cj == '\'' && !localString && !isEscaped(code, j)) localChar = !localChar;
                    else if (!localString && !localChar) {
                        if (cj == '[') depth++; else if (cj == ']') depth--;
                    }
                    j++;
                }
                if (depth == 0 && j < n && code[j] == ':') {
                    size_t typeStart = j + 1, k = typeStart;
                    while (k < n && code[k] != ':') k++;
                    if (k < n && code[k] == ':') {
                        bool verify = (k + 1 < n && code[k + 1] == '?');
                        std::string expr = code.substr(i + 1, j - i - 2);
                        std::string rawType = code.substr(typeStart, k - typeStart);
                        std::string typ = typeMap.count(rawType) ? typeMap[rawType] : rawType;
                        cppCode += verify ? "flow::any_comprobate<" + typ + ">(" + expr + ")"
                                          : "flow::any_cast<" + typ + ">(" + expr + ")";
                        i = verify ? k + 2 : k + 1;
                        flowUsed = true;
                        continue;
                    }
                }
            }

            // ---------------- #import ----------------
            if (i + 7 <= n && code.substr(i, 7) == "#import") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 7 < n) ? code[i + 7] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "#include \"";
                    size_t j = i + 7;
                    std::string name;
                    while (j < n && code[j] != '\n') { name += code[j]; ++j; }
                    cppCode += name + ".hpp\"\n";
                    i = j;
                    continue;
                }
            }

            // ---------------- until / unless ----------------
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

            // ---------------- fn / type simples (sin <>) ----------------
            // ---------------- fn ----------------
            if (i+2 <= n && code.substr(i, 2) == "fn") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextChar = (i + 2 < n) ? code[i + 2] : '\0';

                if (isBoundary(prev, nextChar)) {
                    size_t j = i + 3;
                    j = i+3;
                    bool arrowFound = false;
                    
                    std::string name;
                    std::string type;
                    std::string templateArgs ="<";
                    char now = '\0';

                    while (j < n && (now = code[j]) != '{') {
                        char nextLook = ((j + 1) < n) ? code[j+1] : '\0';

                        if (!arrowFound && now == '-' && nextLook == '>') {
                            arrowFound = true;
                            j += 2; // saltamos '->'
                            while (j < n && isspace(code[j])) j++; // salta espacios antes del tipo
                            continue;
                        }
                        if (!arrowFound)
                            name += now;
                        else
                            type += now;

                        j++;
                    }
                    // Limpiar espacios
                    templateArgs = trim(templateArgs);
                    name = trim(name);
                    type = trim(type);

                    if (((i + 2 < n) ? code[i + 2] : '\0') == '<')
                        cppCode += type.empty() ? "template " + templateArgs + "void " + name :"template " + templateArgs + type + " " + name;
                    else
                        cppCode += type.empty() ? "void " + name : type + " " + name;
                    i = j;
                    goto continue_main;
                }
            }

            if (i+2 <= n && code.substr(i, 2) == "*{") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextChar = (i + 2 < n) ? code[i + 2] : '\0';
                if (isBoundary(prev, nextChar)) {
                    std::string name;
                    cppCode += "(*(";
                    while (code[i] != '{') {
                        name += code[i];
                        i++;
                    }
                    cppCode += name;
                    cppCode += "))";
                    continue;
                }
            }

            // ---------------- tagged union ----------------
            if (i + 12 <= n && code.substr(i, 12) == "tagged union") {
                size_t j = i + 12;

                // --- leer [N] opcional ---
                int expectedCount = -1;
                while (j < n && isSpace(code[j])) j++;

                if (j < n && code[j] == '[') {
                    size_t k = j + 1;
                    std::string num;
                    while (k < n && std::isdigit(code[k])) {
                        num += code[k++];
                    }
                    if (k < n && code[k] == ']') {
                        expectedCount = std::stoi(num);
                        j = k + 1;
                    }
                }

                while (j < n && isSpace(code[j])) j++;

                // --- leer nombre ---
                std::string name;
                while (j < n && isIdentChar(code[j])) {
                    name += code[j++];
                }

                // --- leer <types> ---
                while (j < n && isSpace(code[j])) j++;
                if (j >= n || code[j] != '<') {
                    std::cerr << "error: expected <types>" << std::endl;
                    std::exit(1);
                }

                size_t tStart = ++j;
                while (j < n && code[j] != '>') j++;
                std::string typesStr = code.substr(tStart, j - tStart);
                j++; // saltar '>'

                // split tipos
                std::vector<std::string> types;

                {
                    std::stringstream ss(typesStr);
                    std::string item;
                    while (std::getline(ss, item, ',')) types.push_back(trim(item));
                }

                // --- leer {variants} ---
                while (j < n && isSpace(code[j])) j++;
                if (j >= n || code[j] != '{') {
                    std::cerr << "error: expected {variants}" << std::endl;
                    std::exit(1);
                }

                size_t vStart = ++j;
                while (j < n && code[j] != '}') j++;
                std::string varsStr = code.substr(vStart, j - vStart);
                j++; // saltar '}'

                std::vector<std::string> vars;
                {
                    std::stringstream ss(varsStr);
                    std::string item;
                    while (std::getline(ss, item, ',')) vars.push_back(trim(item));
                }

                // --- checks ---
                if (expectedCount != -1 && expectedCount != (int)vars.size()) {
                    std::cerr << "variant count mismatch" << std::endl;
                    std::exit(1);
                }

                if (types.size() != vars.size()) {
                    std::cerr << "types/variants count mismatch" << std::endl;
                    std::exit(1);
                }

                // --- generar struct ---
                cppCode += "struct " + name + " {\n";
                cppCode += "    enum tags { ";

                for (size_t x = 0; x < vars.size(); ++x) {
                    if (x) cppCode += ", ";
                    cppCode += vars[x];
                }
                cppCode += " };\n";

                cppCode += "    tags tag;\n";
                cppCode += "    union {\n";

                for (size_t x = 0; x < vars.size(); ++x) {
                    cppCode += "        " + types[x] + " " + vars[x] + ";\n";
                }

                cppCode += "    } value;\n";
                cppCode += "    constexpr operator tags() const noexcept { return tag; }\n";
                cppCode += "    " + name + "& operator=(tags t) noexcept { tag = t; return *this; }\n";
                cppCode += "    template <typename T>\n";
                cppCode += "    " + name + "& operator=(T&& val) noexcept {\n";
                cppCode += "        using U = std::decay_t<T>;\n";
                for (size_t x = 0; x < vars.size(); ++x) {
                    cppCode += "        if constexpr (std::is_same_v<U, " + types[x] + ">) { value." + vars[x] + " = std::forward<T>(val); }\n";
                }
                cppCode += "        return *this;\n";
                cppCode += "    }\n";
                cppCode += "};\n";

                i = j;
                continue;
            }

            // ---------------- Reemplazo de tipos simples ----------------
            const std::pair<std::string, std::string> kws[] = {
                {"any", "flow::any"}, {"anyP", "flow::anyP"}, {"str", "flow::str"}, {"wstr", "flow::wstr"},
                {"loop", "while(true)"}, {"type", "using"}, {"pub class", "struct"}
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

            // ---------------- Cfn ----------------
            if (i + 3 <= n && code.substr(i, 3) == "Cfn") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 3 < n) ? code[i + 3] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "extern \"C\"";
                    i += 3;
                    continue;
                }
            }

            // ---------------- defer ----------------
            if (i + 5 <= n && code.substr(i, 5) == "defer") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 5 < n) ? code[i + 5] : '\0';
                if (isBoundary(prev, nextc)) {
                    size_t exprBegin = i + 5;
                    while (exprBegin < n && isSpace(code[exprBegin])) exprBegin++;
                    size_t exprEnd = exprBegin;
                    while (exprEnd < n && code[exprEnd] != ';') exprEnd++;
                    if (exprBegin < exprEnd) {
                        cppCode += "flow::Defer([&](){"+code.substr(exprBegin, exprEnd-exprBegin)+";})";
                        i = exprEnd + 1;
                    } else {
                        cppCode += "flow::Defer()";
                        i += 5;
                    }
                    flowUsed = true;
                    continue;
                }
            }
        }

        // ---------------- Paréntesis ----------------
        if (c == '(') parenDepth++;
        else if (c == ')') {
            parenDepth--;
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

    if (flowUsed && cppCode.find("#include <flow/types>") == std::string::npos)
        cppCode = "#include <flow/types>\n" + cppCode;

    return cppCode;
#else
    return code;
#endif
}


}
