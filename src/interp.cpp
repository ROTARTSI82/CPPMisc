
#include <cstdio>
#include <string_view>
#include <vector>
#include <iostream>
#include <cstring>

#include <cstdint>

//#define PUSH(v) ret_stack.emplace_back(v)
//#define POP() ret_stack.back(); ret_stack.pop_back()

#define PUSH(v) (*(sp++) = (v))
#define POP() (*(--sp))
#define PEEK() (*(sp-1))


std::string_view round(std::string_view view, int col) {
    return view.substr(std::max(col - 4, 0), 8);
}

void run_brainfuck(const std::string_view &view) {
    uint8_t arr[30000] = {0};
    std::memset(arr, 0, 30000);

    const char *ret_stack[4096];
    const char **sp = ret_stack;
    uint8_t *ptr = arr;

    const auto back = view.data() + view.size();
    for (const char *pc = view.data(); pc < back; pc++) {
        switch (*pc) {
            case '>':
                ++ptr;
                break;
            case '<':
                --ptr;
                break;
            case '+':
                ++(*ptr);
                break;
            case '-':
                --(*ptr);
                break;
            case '.':
                putchar(*ptr);
                break;
            case ',':
                *ptr = static_cast<uint8_t>(getchar());
                break;
            case '[': {
                if (*ptr == 0) {
                    // we don't want to execute this loop, so we skip
                    // ahead until the end

                    // TODO: This procedure is relatively inefficient
                    int hanging = 1;
                    while (hanging) {
                        char c = *++pc;
                        if (c == ']')
                            hanging--;
                        else if (c == '[')
                            hanging++;
                    }
                } else
                    PUSH(pc);
                break;
            }
            case ']': {
                if (*ptr != 0)
                    pc = PEEK();
                else
                    POP();
                break;
            }
            default:
                // ignore
                break;
        }
    }
}

#include <sstream>
#include <fstream>

int main() {
    std::ifstream t("bf.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();

    auto str = buffer.str();
    std::cout << str << '\n';
    run_brainfuck(std::string_view{str});
}