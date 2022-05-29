//
// Created by grant on 5/26/22.
//

#include <iostream>
#include <fstream>
#include <sstream>

#include "compiler/parse.hpp"

int main(int argc, char **argv) {
    std::ifstream t("inp.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();

    std::string str = buffer.str();
    std::cout << str << '\n';

    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    auto jit = llvm::cantFail(llvm::orc::KaleidoscopeJIT::Create());

    Parser parse{str, &jit->getContext()};
    parse.module->setDataLayout(jit->getDataLayout());

    parse.scan_lines();
    parse.ind = 0;
    parse.shunting_yard();
    cantFail(jit->addModule(std::move(parse.module)));

    std::cout << "==== VALUE = " << reinterpret_cast<int(*)(int, char **)>(llvm::cantFail(jit->lookup("main")).getAddress())(0, nullptr) << '\n';
    jit.reset();
}
