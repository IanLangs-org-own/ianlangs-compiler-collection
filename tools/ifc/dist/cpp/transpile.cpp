#include <flow/collections>

#include <cctype>

#include <sstream>

#include <flow/errors>

using namespace flow;

namespace flowcpp {

static str trim(const str& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(s[start])) start++;

    if (start == s.size()) return ""; 
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

static bool isEscaped(const str code, size_t pos) {
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


str delete_comments(const str code) {
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inChar = false;
    bool inString = false;

    str result;
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
}


str transpile(const str rawCode, flow::set<str>* outHeaders) {
    str code = delete_comments(rawCode);
    str cppCode;
    size_t i = 0, n = code.size();
    bool inString = false, inChar = false, inInclude = false, flowUsed = false;
    int pendingUntil = 0, parenDepth = 0;
    long long defers = 0;

    flow::map<str, str> typeMap = {
        {"str", "flow::strstr"},
        {"wstr", "flow::wstr"},
        {"any", "flow::any"}
    };

    while (i < n) {
        char c = code[i];

                if (c == '"' && !inChar && !isEscaped(code, i)) inString = !inString;
        else if (c == '\'' && !inString && !isEscaped(code, i)) inChar = !inChar;

                if (!(inString || inChar)) {
            if (i + 8 <= n && code.substr(i, 8) == "#include") inInclude = true;
        }
        if (inInclude && c == '\n') inInclude = false;

                if (!(inString || inChar || inInclude)) {
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
                    else if (!localString && !localChar) {
                        if (cj == '[') depth++;
                        else if (cj == ']') depth--;
                    }

                    j++;
                }

                if (depth != 0) {
                    flow::raise("Unbalanced brackets");
                }

                                str inner = trim(code.substr(i + 1, j - i - 2));

                                size_t asPos = inner.find(" as ");
                if (asPos != str::npos) {
                    str expr = trim(inner.substr(0, asPos));
                    str ype = trim(inner.substr(asPos + 4));

                    cppCode += "flow::any_cast<" + ype + ">(" + expr + ")";
                    flowUsed = true;
                    i = j;
                    continue;
                }

                                size_t canPos = inner.find(" is ");
                if (canPos != str::npos) {
                    str expr = trim(inner.substr(0, canPos));
                    str ype = trim(inner.substr(canPos + 4));

                    cppCode += "flow::any_comprobate<" + ype + ">(" + expr + ")";
                    flowUsed = true;
                    i = j;
                    continue;
                }

                                if (inner.rfind("try ", 0) == 0) {
                    str rest = trim(inner.substr(4));
                    size_t exceptPos = rest.find(" except return ");

                    if (exceptPos != str::npos) {
                        str expr = trim(rest.substr(0, exceptPos));
                        str ret  = trim(rest.substr(exceptPos + 15));

                        cppCode += "{auto __flow_tmp = " + expr +
                                "; if(!__flow_tmp) return " + ret +
                                ";}";
                    } else {
                        cppCode += "{auto __flow_tmp = " + rest +
                                "; if(!__flow_tmp) return;}";
                    }

                    flowUsed = true;
                    i = j;
                    continue;
                }

                                cppCode += "[" + inner + "]";
                i = j;
                continue;
            }


                        if (i + 7 <= n && code.substr(i, 7) == "#import") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 7 < n) ? code[i + 7] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "#include \"";
                    size_t j = i + 7;
                    str name;
                    while (j < n && code[j] != '\n') { name += code[j]; ++j; }
                    cppCode += trim(name) + ".hpp\"\n";
                    if (outHeaders != nullptr) outHeaders->insert(trim(name) + ".fhpp");
                    i = j;
                    continue;
                }
            }
            if (i + 4 <= n && code.substr(i, 4) == "#std") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 4 < n) ? code[i + 4] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "#include <flow/";
                    size_t j = i + 4;
                    str name;
                    while (j < n && code[j] != '\n') { name += code[j]; ++j; }
                    cppCode += trim(name) + ">\n";
                    i = j;
                    continue;
                }
            }

            if (i + 5 <= n && code.substr(i, 5) == "#cstd") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 5 < n) ? code[i + 5] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "#include <";
                    size_t j = i + 5;
                    str name;
                    while (j < n && code[j] != '\n') { name += code[j]; ++j; }
                    cppCode += trim(name) + ">\n";
                    i = j;
                    continue;
                }
            }

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

                                    if (i+2 <= n && code.substr(i, 2) == "fn") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextChar = (i + 2 < n) ? code[i + 2] : '\0';

                if (isBoundary(prev, nextChar)) {
                    size_t j = i + 3;
                    j = i+3;
                    bool arrowFound = false;
                    
                    str name;
                    str ype;
                    str templateArgs ="<";
                    char now = '\0';

                    while (j < n && (now = code[j]) != '{' && code[j] != ';' && code[j] != '\n') {
                        char nextLook = ((j + 1) < n) ? code[j+1] : '\0';

                        if (!arrowFound && now == '-' && nextLook == '>') {
                            arrowFound = true;
                            j += 2;                             while (j < n && isspace(code[j])) j++;                             continue;
                        }
                        if (!arrowFound)
                            name += now;
                        else
                            ype += now;

                        j++;
                    }
                                        templateArgs = trim(templateArgs);
                    name = trim(name);
                    ype = trim(ype);

                    if (((i + 2 < n) ? code[i + 2] : '\0') == '<')
                        cppCode += ype.empty() ? "template " + templateArgs + "void " + name :"template " + templateArgs + ype + " " + name;
                    else
                        cppCode += ype.empty() ? "void " + name : ype + " " + name;
                    i = j;
                    goto continue_main;
                }
            }

            if (i+2 <= n && code.substr(i, 2) == "*{") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextChar = (i + 2 < n) ? code[i + 2] : '\0';
                if (isBoundary(prev, nextChar)) {
                    str name;
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

                        if (i + 12 <= n && code.substr(i, 12) == "tagged union") {
                size_t j = i + 12;

                                int expectedCount = -1;
                while (j < n && isSpace(code[j])) j++;

                if (j < n && code[j] == '[') {
                    size_t k = j + 1;
                    str num;
                    while (k < n && std::isdigit(code[k])) {
                        num += code[k++];
                    }
                    if (k < n && code[k] == ']') {
                        expectedCount = std::stoi(num);
                        j = k + 1;
                    }
                }

                while (j < n && isSpace(code[j])) j++;

                                str name;
                while (j < n && isIdentChar(code[j])) {
                    name += code[j++];
                }

                                while (j < n && isSpace(code[j])) j++;
                if (j >= n || code[j] != '<') {
                    flow::raise("error: expected <types>");
                }

                size_t tStart = ++j;
                while (j < n && code[j] != '>') j++;
                str typesStr = code.substr(tStart, j - tStart);
                j++; 
                                std::vector<str> types;

                {
                    std::stringstream ss(typesStr);
                    str item;
                    while (std::getline(ss, item, ',')) types.push_back(trim(item));
                }

                                while (j < n && isSpace(code[j])) j++;
                if (j >= n || code[j] != '{') {
                    flow::raise("error: expected {variants}");
                    std::exit(1);
                }

                size_t vStart = ++j;
                while (j < n && code[j] != '}') j++;
                str varsStr = code.substr(vStart, j - vStart);
                j++; 
                std::vector<str> vars;
                {
                    std::stringstream ss(varsStr);
                    str item;
                    while (std::getline(ss, item, ',')) vars.push_back(trim(item));
                }

                                if (expectedCount != -1 && expectedCount != (int)vars.size()) {
                    flow::raise("variant count mismatch");
                }

                if (types.size() != vars.size()) {
                    flow::raise("types/variants count mismatch");
                    std::exit(1);
                }

                                cppCode += "struct " + name + " {";
                cppCode += "    enum tags { ";

                for (size_t x = 0; x < vars.size(); ++x) {
                    if (x) cppCode += ", ";
                    cppCode += vars[x];
                }
                cppCode += " };\n";

                cppCode += "    tags tag;";
                cppCode += "    union {";

                for (size_t x = 0; x < vars.size(); ++x) {
                    if (types[x] != "void") cppCode += "        " + types[x] + " " + vars[x] + ";";
                }

                cppCode += "    } value;";
                cppCode += "    constexpr operator tags() const noexcept { return tag; }";
                cppCode += "    " + name + "& operator=(tags t) noexcept { tag = t; return *this; }\n";
                cppCode += "    template <typename ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789>";
                cppCode += "    " + name + "& operator=(ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789&& val) {";
                cppCode += "        using U = std::decay_t<ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789>;";
                if (types[0] != "void")
                    cppCode += "        if (tag == tags::" + vars[0] + " && std::is_same_v<U, " + types[0] + ">) { value." + vars[0] + " = std::forward<T>(val); }";

                for (size_t x = 1; x < vars.size(); ++x) {
                    if (types[x] != "void")
                        cppCode += "        else if (tag == tags::" + vars[x] + " && std::is_same_v<U, " + types[x] + ">) { value." + vars[x] + " = std::forward<T>(val); }";
                }

                cppCode += "        else { throw std::runtime_error(\"invalid assignment to tagged union\"); }";
                cppCode += "        return *this;";
                cppCode += "    }";
                cppCode += "};";

                i = j;
                continue;
            }

                        const std::pair<str, str> kws[] = {
                {"any", "flow::any"}, {"anyP", "flow::anyP"}, {"str", "str"}, {"wstr", "flow::wstr"},
                {"loop", "while(true)"}, {"_type", "using"}, {"pub class", "struct"}
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

                        if (i + 3 <= n && code.substr(i, 3) == "Cfn") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 3 < n) ? code[i + 3] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "extern \"C\"";
                    i += 3;
                    continue;
                }
            }

                        if (i + 5 <= n && code.substr(i, 5) == "defer") {
                char prev = (i > 0) ? code[i - 1] : '\0';
                char nextc = (i + 5 < n) ? code[i + 5] : '\0';
                if (isBoundary(prev, nextc)) {
                    cppCode += "auto __DEFER_VAR_" + std::to_string(defers++) + " = ";
                    size_t exprBegin = i + 5;
                    while (exprBegin < n && isSpace(code[exprBegin])) exprBegin++;
                    size_t exprEnd = exprBegin;
                    while (exprEnd < n && code[exprEnd] != ';') exprEnd++;
                    if (exprBegin < exprEnd) {
                        cppCode += "flow::Defer([&](){"+code.substr(exprBegin, exprEnd-exprBegin)+";});";
                        i = exprEnd + 2;
                    } else {
                        cppCode += "flow::Defer()";
                        i += 5;
                    }
                    flowUsed = true;
                    continue;
                }
            }
        }

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

    if (flowUsed && cppCode.find("#include <flow/types>") == str::npos)
        cppCode = "#include <flow/types>\n" + cppCode;

    return cppCode;
}

}
