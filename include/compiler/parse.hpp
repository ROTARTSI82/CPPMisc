#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler/operator.hpp"
#include "compiler/value.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <charconv>
#include <iomanip>
#include <unordered_map>

#include "template_ternary.hpp"

#include "compiler/kaleidoscope_jit.hpp"


#define ADD_PRE_PASSES(fpm)                                \
    fpm.add(llvm::createStraightLineStrengthReducePass()); \
    fpm.add(llvm::createLoopRerollPass());                 \
    fpm.add(llvm::createLoopUnrollPass(3));                \
    fpm.add(llvm::createLoopUnrollAndJamPass(3));          \
    fpm.add(llvm::createLoopRotatePass());                 \
                                                           \
    fpm.add(llvm::createLoopSimplifyCFGPass());            \
    fpm.add(llvm::createLoopSimplifyPass());               \
    fpm.add(llvm::createLICMPass());                       \
    fpm.add(llvm::createLoopSinkPass());                   \
    fpm.add(llvm::createLoopPredicationPass());            \
    fpm.add(llvm::createLoopUnswitchPass());               \
    fpm.add(llvm::createLoopInstSimplifyPass());           \
                                                           \
    fpm.add(llvm::createLoopVersioningLICMPass());         \
    fpm.add(llvm::createLoopStrengthReducePass());         \
    fpm.add(llvm::createLoopIdiomPass());                  \
    fpm.add(llvm::createLoopDeletionPass());               \
                                                           \
    fpm.add(llvm::createCFGSimplificationPass());          \
    fpm.add(llvm::createPromoteMemoryToRegisterPass());    \
    fpm.add(llvm::createSeparateConstOffsetFromGEPPass()); \
    fpm.add(llvm::createSROAPass());                       \
                                                           \
    fpm.add(llvm::createInstructionCombiningPass());       \
    fpm.add(llvm::createReassociatePass());                \
    fpm.add(llvm::createGVNPass());                        \
                                                           \
    fpm.add(llvm::createMergedLoadStoreMotionPass());


#define ADD_POST_PASSES(fpm)                            \
    fpm.add(llvm::createConstantHoistingPass());        \
    fpm.add(llvm::createConstantPropagationPass());     \
    fpm.add(llvm::createLowerConstantIntrinsicsPass()); \
    fpm.add(llvm::createLoopSimplifyCFGPass());         \
    fpm.add(llvm::createCFGSimplificationPass());       \
                                                        \
    fpm.add(llvm::createDeadStoreEliminationPass());    \
    fpm.add(llvm::createDeadCodeEliminationPass());     \
    fpm.add(llvm::createDeadInstEliminationPass());

class Parser;


void eat_function(Parser *parser) {
    //    std::cout << "eat func";
}

template<typename T>
void emit_typename(Parser *parser) {
    //    std::cout << "emit typename";
}

void emit_addition(Parser *parser);

using float64_t = double;
using float32_t = float;

static_assert(sizeof(float64_t) == 8);
static_assert(sizeof(float32_t) == 4);

inline std::unordered_map<std::string_view, std::function<void(Parser *)>> &get_keywords() {
    static std::unordered_map<std::string_view, std::function<void(Parser *)>> KEYWORDS = {
            {"func", eat_function},

            {"i64", emit_typename<int64_t>},
            {"i32", emit_typename<int32_t>},
            {"i16", emit_typename<int16_t>},
            {"i8", emit_typename<int8_t>},

            {"u64", emit_typename<uint64_t>},
            {"u32", emit_typename<uint32_t>},
            {"u16", emit_typename<uint16_t>},
            {"u8", emit_typename<uint8_t>},

            {"f64", emit_typename<float64_t>},
            {"f32", emit_typename<float32_t>},

            {"bool", emit_typename<bool>}};
    return KEYWORDS;
}

inline std::array<std::unordered_map<std::string_view, std::function<void(Parser *)>>, 2> &get_operators() {
    static std::array<std::unordered_map<std::string_view, std::function<void(Parser *)>>, 2>
            OPERATORS =
                    {{{
                              {"+", emit_addition},

                      },
                      {}}};

    return OPERATORS;
}


class Parser {
public:
    std::string_view input;
    std::size_t ind = 0;
    std::size_t line_no = 1;// this is synced with `ind`

    //    OperandStack operands;
    std::vector<Value> operands;
    //    std::vector<std::unique_ptr<Operator>> operators;


    std::vector<std::size_t> lines;// input[lines[X]] is the beginning of line X+2.

    llvm::LLVMContext *ctx;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;

    llvm::Function *main = nullptr;

    llvm::legacy::FunctionPassManager fpm;
    llvm::PassManagerBuilder pm_builder{};
    //    llvm::PassManager gpm;

    explicit Parser(const std::string_view &inp, llvm::LLVMContext *ctx) : input(inp), ctx(ctx), module(std::make_unique<llvm::Module>("Module", *ctx)), builder(*ctx), fpm(module.get()) {

        pm_builder.OptLevel = 3;
        pm_builder.SizeLevel = 2;

        ADD_PRE_PASSES(fpm)

        pm_builder.populateFunctionPassManager(fpm);

        ADD_POST_PASSES(fpm)

        fpm.doInitialization();

        // TODO: What the fuck are pointer address spaces? i'm just using getUnqual rn
        llvm::FunctionType *entry_point_signature = llvm::FunctionType::get(llvm::Type::getInt32Ty(*ctx), std::vector<llvm::Type *>{llvm::Type::getInt32Ty(*ctx), llvm::PointerType::getUnqual(llvm::Type::getInt8PtrTy(*ctx))}, false);
        main = llvm::Function::Create(entry_point_signature, llvm::Function::ExternalLinkage, "main", module.get());

        llvm::BasicBlock *block = llvm::BasicBlock::Create(*ctx, "entry", main);
        builder.SetInsertPoint(block);
    }

    ~Parser() = default;

    //    std::unique_ptr<Operator> get_operator() {
    //        return nullptr;
    //    }

    std::string_view get_line(std::size_t line) {
        if (line - 2 >= lines.size())
            return input;

        std::size_t begin = line <= 1 ? 0 : lines[line - 2];

        if (line - 1 >= lines.size())
            return input.substr(begin);
        return input.substr(begin, lines[line - 1]);
    }

    // binary search to find the line this index is on
    [[nodiscard]] std::size_t lookup_line_no(std::size_t index) const {
        if (lines.empty()) return 1;

        std::size_t lo = 0;
        std::size_t hi = lines.size() - 1;

        while (lo <= hi) {
            std::size_t mid = (lo + hi) / 2;
            std::size_t val = lines[mid];

            if (val > index) {
                hi = mid - 1;
            } else if (val < index) {
                lo = mid + 1;
            } else {
                hi = mid;
                break;
            }
        }

        return hi + 2;
    }

    inline void emit_error(const std::string &msg) {
        //        auto line_no = lookup_line_no(ind);
        std::cerr << "input[" << ind << "] (line " << line_no << "): " << msg << '\n';

        constexpr std::size_t LINE_LENGTH_PRINT_THRESHOLD = 256;

        std::string_view line = get_line(line_no);
        if (line.size() >= LINE_LENGTH_PRINT_THRESHOLD)
            std::cerr << '\t' << line.substr(line.size() - LINE_LENGTH_PRINT_THRESHOLD) << "...\n";
        else
            std::cerr << '\t' << line << '\n';

        throw std::runtime_error{msg};
    }


    // returns true if the callback was called to consume a newline
    inline bool consume_newline(const std::function<void()> &callback) {
        char cur = input[ind];
        if (cur == '\n' || cur == '\r') {
            ind++;

            // handle \n\r and \r\n as single newlines
            if (ind < input.size() && input[ind] != cur && (input[ind] == '\n' || input[ind] == '\r'))
                ind++;

            callback();
            return true;
        }

        return false;
    }

    // loads the correct indices into `lines`
    // this method touches ind, so it will need to be reset after calling
    void scan_lines() {
        while (ind < input.size()) {
            if (!consume_newline([&]() {
                    lines.emplace_back(ind);
                }))
                ind++;
        }
    }

    void handle_symbol(const std::string_view &sym) {
        if (get_keywords().count(sym) != 0)
            get_keywords()[sym](this);
    }

    template<std::size_t byte_size>
    inline void push_float(float64_t value) {
        operands.emplace_back(Value{llvm::ConstantFP::get(*ctx, byte_size == 8 ? llvm::APFloat(value) : llvm::APFloat(static_cast<float32_t>(value))), get_type<FloatingPointType, byte_size>()});
    }

    // TODO: APInt and APFloat are pretty inelegant
    template<std::size_t byte_size, bool is_signed>
    inline void push_int(uint64_t value) {
        // this code is so fucking cursed, and i love it
        operands.emplace_back(Value{llvm::ConstantInt::get(*ctx, llvm::APInt(8 * byte_size, value, is_signed)), get_type<typename Ternary<is_signed, SignedIntegerType, UnsignedIntegerType>::value, byte_size>()});
    }

    void handle_numeric_literal() {
        uint64_t whole_part = 0;

        ind--;// ind already points to the start of the numeric literal
        while (++ind < input.size() && std::isdigit(input[ind])) {
            uint64_t old = whole_part;
            whole_part *= 10;
            whole_part += input[ind] - '0';
            if (whole_part <= old)
                emit_error("Numeric literal too large: Overflow detected in u64");
        }

        // float literal
        if (ind < input.size() && input[ind] == '.') {
            auto val = static_cast<float64_t>(whole_part);

            uint64_t pow = 10;
            while (++ind < input.size() && std::isdigit(input[ind])) {
                val += (input[ind] - '0') / static_cast<float64_t>(pow);
                pow *= 10.0;
            }

            // I would use
            // std::string_view res = input.substr(begin, ind - begin);
            // auto err = std::from_chars(res.data(), res.data() + res.size(), val);
            // but sadly it's not implemented :(

            std::cout << "got float: " << std::setprecision(100) << val << '\n';

            if (ind >= input.size()) return;
            std::string_view type_lit = input.substr(ind, 3);// no risk of index out of bounds

            std::size_t orig_size = operands.size();
            if (type_lit == "f64")
                push_float<8>(val);
            else if (type_lit == "f32")
                push_float<4>(val);

            if (operands.size() != orig_size)
                ind += 3;
            else
                push_float<8>(val);// default to f64 for no ending

            return;
        }

        std::cout << "got int: " << whole_part << '\n';

        if (ind >= input.size()) return;
        std::string_view type_lit = input.substr(ind, 3);
        // turn this into a map probably

        // used to detect if any cases succeeded. TODO: This is a terrible hack
        std::size_t operands_size = operands.size();
        if (type_lit == "i64")
            push_int<8, true>(whole_part);
        else if (type_lit == "i32")
            push_int<4, true>(whole_part);
        else if (type_lit == "i16")
            push_int<2, true>(whole_part);
        else if (type_lit == "i8")
            push_int<1, true>(whole_part);
        else if (type_lit == "u64")
            push_int<8, false>(whole_part);
        else if (type_lit == "u32")
            push_int<4, false>(whole_part);
        else if (type_lit == "u16")
            push_int<2, false>(whole_part);
        else if (type_lit == "u8")
            push_int<1, false>(whole_part);
        else if (type_lit == "f64")
            push_float<8>(static_cast<float64_t>(whole_part));
        else if (type_lit == "f32")
            push_float<4>(static_cast<float64_t>(whole_part));

        if (operands.size() != operands_size)
            ind += 3;
        else
            push_int<4, true>(whole_part);// default i32 for no ending
    }

    llvm::Value *shunting_yard() {
        while (ind < input.size()) {
            char cur = input[ind];

            if (consume_newline([&]() { line_no++; }))
                continue;

            if (std::isspace(cur)) {
                ind++;
                continue;
            } else if (std::isdigit(cur)) {
                // No support for 0x, 0b, or 0o
                handle_numeric_literal();
                continue;
            } else if (std::isalpha(cur)) {
                // symbol name
                std::size_t begin = ind;
                while (++ind < input.size() && std::isalnum(input[ind]))
                    ;

                std::string_view sym_name = input.substr(begin, ind - begin);
                handle_symbol(sym_name);
            } else {
                for (int i = 1; i >= 0; i--) {
                    std::string_view view = input.substr(ind, i + 1);
                    if (get_operators()[i].count(view) > 0) {
                        // ind now points to the last character of our operator,
                        // and we rely on the later ind++; to increment it to the correct one
                        ind += i;

                        get_operators()[i][view](this);
                        break;
                    }
                }

                ind++;
                continue;
            }

            //            if (cur == 'x')
            //                emit_error("test error");

            ind++;
        }

        while (operands.size() >= 2) {
            Value back = operands.back();
            Value other = *(operands.end() - 2);
            operands.pop_back();
            operands.pop_back();

            operands.emplace_back(Value{builder.CreateAdd(back.llvm, other.llvm), get_type<FloatingPointType, 8>()});
        }

        if (!operands.empty())
            builder.CreateRet(operands.back().llvm);

        //        builder->CreateCast

        llvm::verifyFunction(*main);
        llvm::verifyModule(*module);

        module->print(llvm::outs(), nullptr);

        return nullptr;
    }

    void brainfuck();
};

void emit_addition(Parser *parser) {
    std::cout << "plus operator\n";
}