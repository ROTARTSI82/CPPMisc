#include "compiler/parse.hpp"

struct RetInfo {
    llvm::BasicBlock *cont, *header;
};

void Parser::brainfuck() {
    constexpr static auto BUF_SIZE = 30000;

    // TODO: Maybe i should generate the phis myself instead of relying on mem2reg
    llvm::Value *ptr_ind = builder.CreateAlloca(llvm::Type::getInt64Ty(*ctx));
    builder.CreateStore(builder.getInt64(0), ptr_ind);

    // what should the linkage types be??
    llvm::FunctionType *putchar_signature = llvm::FunctionType::get(builder.getInt32Ty(), std::initializer_list<llvm::Type *>{builder.getInt32Ty()}, false);
    llvm::Function *f_putchar = llvm::Function::Create(putchar_signature, llvm::Function::LinkOnceAnyLinkage, "putchar", *module);

    llvm::FunctionType *getchar_signautre = llvm::FunctionType::get(builder.getInt32Ty(), std::initializer_list<llvm::Type *>{}, false);
    llvm::Function *f_getchar = llvm::Function::Create(getchar_signautre, llvm::Function::LinkOnceAnyLinkage, "getchar", *module);

    // declare void @llvm.memset.p0i8.i64(i8* <dest>, i8 <val>, i64 <len>, i1 <isvolatile>)
    llvm::FunctionType *llvm_memset_signature = llvm::FunctionType::get(builder.getVoidTy(), std::initializer_list<llvm::Type *>{builder.getInt8PtrTy(), builder.getInt8Ty(), builder.getInt64Ty(), builder.getInt1Ty()}, false);
    llvm::Function *f_llvm_memset = llvm::Function::Create(llvm_memset_signature, llvm::Function::LinkOnceAnyLinkage, "llvm.memset.p0i8.i64", *module);

    // pointer initially points to first element of alloca
    llvm::Value *arr = builder.CreateAlloca(llvm::Type::getInt8Ty(*ctx), builder.getInt64(BUF_SIZE));
    builder.CreateCall(f_llvm_memset, std::initializer_list<llvm::Value *>{arr, builder.getInt8(0), builder.getInt64(BUF_SIZE), builder.getInt1(false)});


    std::vector<RetInfo> loop_ret_addrs;

    int loop_counter = 0;

    while (ind < input.size()) {
        char cur = input[ind];

        if (consume_newline([&]() { line_no++; }))
            continue;

        if (std::isspace(cur)) {
            ind++;
            continue;
        }

        llvm::Value *gep;

        switch (cur) {
            case '[': {
                auto count = std::to_string(loop_counter++);

                llvm::BasicBlock *header = llvm::BasicBlock::Create(*ctx, "loop_head" + count,  main);
                llvm::BasicBlock *body = llvm::BasicBlock::Create(*ctx, "loop_body" + count, main);
                llvm::BasicBlock *cont = llvm::BasicBlock::Create(*ctx, "loop_continue" + count, main);

                // current block
                builder.CreateBr(header);

                builder.SetInsertPoint(header);
                gep = builder.CreateGEP(arr, builder.CreateLoad(ptr_ind));
                builder.CreateCondBr(builder.CreateICmpNE(builder.CreateLoad(gep), builder.getInt8(0)), body, cont);

                builder.SetInsertPoint(body);

                loop_ret_addrs.emplace_back(RetInfo{cont, header});
                break;
            }
            case ']': {
                if (loop_ret_addrs.empty())
                    emit_error("Unmatched ']': No loop to close");
                auto ret = loop_ret_addrs.back();
                builder.CreateBr(ret.header);
                builder.SetInsertPoint(ret.cont);
                loop_ret_addrs.pop_back();
                break;
            }
            case '<':
                builder.CreateStore(builder.CreateSub(builder.CreateLoad(ptr_ind), builder.getInt64(1)), ptr_ind);
                break;
            case '>':
                builder.CreateStore(builder.CreateAdd(builder.CreateLoad(ptr_ind), builder.getInt64(1)), ptr_ind);
                break;
            case '+':
                gep = builder.CreateGEP(arr, builder.CreateLoad(ptr_ind));
                builder.CreateStore(builder.CreateAdd(builder.CreateLoad(gep), builder.getInt8(1)), gep);
                break;
            case '-':
                gep = builder.CreateGEP(arr, builder.CreateLoad(ptr_ind));
                builder.CreateStore(builder.CreateSub(builder.CreateLoad(gep), builder.getInt8(1)), gep);
                break;
            case '.':
                gep = builder.CreateGEP(arr, builder.CreateLoad(ptr_ind));
                builder.CreateCall(f_putchar, std::initializer_list<llvm::Value *>{builder.CreateLoad(gep)}); // calls in with i8 to func expecting i32
                break;
            case ',':
                gep = builder.CreateGEP(arr, builder.CreateLoad(ptr_ind));
                builder.CreateStore(builder.CreateCall(f_getchar, std::initializer_list<llvm::Value *>{}), gep);  // relies on auto trunc to convert i32 to i8
                break;
            default:
                emit_error("Unexpected character: " + std::string{cur});
                break;
        }

        ind++;
    }

    if (!loop_ret_addrs.empty())
        emit_error("Unmatched '[': Loop not closed!");


    builder.CreateRet(builder.getInt32(0));

    //        builder.CreateCast

    llvm::verifyFunction(*main);
    fpm.run(*main);

    llvm::verifyModule(*module);


    module->print(llvm::outs(), nullptr);
}