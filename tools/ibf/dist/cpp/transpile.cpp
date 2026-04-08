#include <flow/types>
#include <flow/errors>


using namespace flow;

#define get(n, text, i) do { \
    if (text[i] == '[') { \
                usize j = i+1; \
                for (;j < text.size() && text[j] != ']';) j++; \
                try { \
                    n = std::stoull(text.substr(i+1,j-i-1)); \
                } catch (...) { \
                    flow::error err; \
                    err.append("invalid number {") \
                        .append(text.substr(i+1,j-i-1)) \
                        .append("}"); \
                    err.raise(1); \
                } \
            i = j+1; \
            } \
    } while(0)

using usize = unsigned long long;
str transpileBF4Humans2BF(str code){
    flow::str bfcode;
    usize i = 0;
    bool inComment = false;
    while (i < code.size()) {
        if (code[i] == '#') {
            inComment = !inComment;
        } else if (inComment) {
                    } else if (code.substr(i, 4) == "incr") {
            i += 4;
            usize n = 1;
            get(n, code, i);
            for (usize m = 0; m < n; m++)
                bfcode += "+";
        } else if (code.substr(i, 4) == "decr") {
            i += 4;
            usize n = 1;
            get(n, code, i);
            for (usize m = 0; m < n; m++)
                bfcode += "-";
        } else if (code.substr(i, 3) == "out") {
            i += 3;
            usize n = 1;
            bfcode += ".";
        } else if (code.substr(i, 3) == "inp") {
            i += 3;
            usize n = 1;
            bfcode += ",";
        } else if (code.substr(i, 4) == "left") {
            i += 4;
            usize n = 1;
            get(n, code, i);
            for (usize m = 0; m < n; m++)
                bfcode += "<";
        } else if (code.substr(i, 5) == "right") {
            i += 5;
            usize n = 1;
            get(n, code, i);
            for (usize m = 0; m < n; m++)
                bfcode += ">";
        } else if (code.substr(i, 5) == "loop(") {
            i += 5;
            usize n = 1;
            bfcode += "[";
        } else if (code[i] == ')') {
            i += 1;
            usize n = 1;
            bfcode += "]";
        } else {
            if (code[i] != '\n' && code[i] != ' ') flow::raise("invalid char");
            else i++; 
        }
    }
    return bfcode;
}