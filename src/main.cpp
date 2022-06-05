//
// Created by grant on 5/26/22.
//

#include <iostream>
#include <fstream>
#include <sstream>

#include "compiler/brainfuck.cpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"

int main(int argc, char **argv) {
    std::ifstream t("bf.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();

    std::string str = buffer.str();
    std::cout << str << '\n';

    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    // object file generation
    auto target_triple = llvm::sys::getDefaultTargetTriple();

//    LLVMInitializeAllTargetInfos();
//    LLVMInitializeAllTargets();
//    LLVMInitializeAllTargetMCs();
//    LLVMInitializeAllAsmParsers();
//    LLVMInitializeAllAsmPrinters();

    std::string err;
    auto targ = llvm::TargetRegistry::lookupTarget(target_triple, err);
    if (!targ) {
        llvm::errs() << err;
        throw std::runtime_error{"asdf"};
    }

    auto cpu = "generic";
    auto feats = "";

    llvm::TargetOptions opt;
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto targ_machine = targ->createTargetMachine(target_triple, cpu, feats, opt, rm);




    auto jit = llvm::cantFail(llvm::orc::KaleidoscopeJIT::Create());

    Parser parse{str, &jit->getContext()};
//    parse.module->setDataLayout(jit->getDataLayout());
    parse.module->setDataLayout(targ_machine->createDataLayout());
    parse.module->setTargetTriple(target_triple);

    parse.scan_lines();
    parse.ind = 0;
//    parse.shunting_yard();
    parse.brainfuck();


    auto filename = "output.o";
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message();
        throw std::runtime_error{"asdf"};
    }

    llvm::legacy::PassManager pass;
    auto filetype = llvm::CGFT_ObjectFile;

    if (targ_machine->addPassesToEmitFile(pass, dest, nullptr, filetype)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        throw std::runtime_error{"asdf"};
    }

    pass.run(*parse.module);
    dest.flush();




    cantFail(jit->addModule(std::move(parse.module)));

    int value = reinterpret_cast<int(*)(int, char **)>(llvm::cantFail(jit->lookup("main")).getAddress())(0, nullptr);
    std::cout << "==== VALUE = " << value << ", float = " << *reinterpret_cast<float *>(&value) << '\n';
    jit.reset();
}
