#include <flow/types>
#include <flow/io>

#include <flow/fs>

#include <flow/collections>


using namespace flow;

extern str transpileBF4Humans2BF(str code);

int main(int argc, char** argv){
    if (argc == 1) {
        flow::error err;
        err.append("Use: ").append(argv[0]).append(" file.bf");
        err.raise(1);
    }
    flow::File file = flow::openFile(argv[1]);
    
    flow::str code = file.read();
    if (argc == 3 && flow::str(argv[2]) == "--compile-syntax") {
        flow::str newcode = transpileBF4Humans2BF(code);
        flow::println(newcode);
        std::exit(0);
    }

    flow::vector<unsigned char> stack = {0,0,0,0,0,0,0,0,0,0};
    size_t index = 0;
    size_t ptr = 0;
    flow::vector<size_t> brackets_index = {};

    while (index < code.size()) {
        char now = code[index];
        switch(now) {
            case '.':
                flow::print((char)stack[ptr]);
                break;
            case ',': {
                flow::str inp = flow::input("");
                stack[ptr] = inp[0];
                break;
            }
            case '<':
                if (ptr > 0)ptr--;
                break;
            case '>':
                ptr++;
                break;
            case '+':
                stack[ptr]++;
                break;
            case '-':
                stack[ptr]--;
                break;
            case '[': {
                if (stack[ptr] == 0) {
                    size_t brackets = 1;
                    size_t index2 = index+1;
                    while (brackets && index2 < code.size()) {
                        char nowc = code[index2];
                        switch (nowc) {
                            case '[':
                                brackets++;
                                break;
                            case ']':
                                brackets--;
                        }
                        index2++;
                    }
                    if (! (brackets == 0)) {
                        flow::raise("brackets mismatch");
                    }
                    index = index2-1;
                    break;
                }
                brackets_index.push_back(index);
                break;
            }
            case ']':
                if (stack[ptr] != 0) {
                    index = brackets_index.back();
                } else {
                    brackets_index.pop_back();
                }
                break;
        }
        if (ptr >= stack.size()) {
            for(size_t _ = stack.size(); _ <= ptr; _++) stack.push_back(0);
        }
        index++;
    }
}