#ifndef JovianVM_h
#define JovianVM_h

#include <iostream>
#include <map>
#include <regex>
#include <string>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include  "parser/JovianParser.h"

using syntax::JovianParser;


class JovianVM {
public:
    JovianVM(): parser(std::make_unique<JovianParser>()) {
        initializeModule();
        setupExternalFunction();
    }

    void execute(const std::string& source) {
        auto ast = parser->parse(source);

        compile(ast);

        module->print(llvm::outs(), nullptr);

        std::cout << "\n";

        saveModuleToFile("./out.ll");

    }

private:

    void compile(const Value &exp) {

        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false));

        auto result = gen(exp);

        auto i32Result = builder->CreateIntCast(result, builder->getInt32Ty(), false);

        builder->CreateRet(i32Result);
    }

    llvm::Value *gen(const Value &exp) {
        auto x = exp;
        // switch (exp.type) {

        //     case ExpType::NUMBER: 
        //         return builder->getInt32(exp.number);
        //         break;
        
        //     case ExpType::STRING: 
        //         auto re = std::regex("\\\\n");
        //         auto str = std::regex_replace(exp.string, re, "\n");
        //         return builder->CreateGlobalStringPtr(str);
        //         break;

        //     case ExpType::SYMBOL:
        //         if (exp.string == "true" || exp.string == "false") {
        //             return builder->getInt1(exp.string == "true" ? true : false);
        //         }
        //         break;

        //     case ExpType::LIST:
        //         auto tag = exp.list[0];

        //         if(tag.type == ExpType::SYMBOL) {
        //             auto op = tag.string;

        //             if(op == "printf") {
        //                 auto printFn = module->getFunction("printf");

        //                 std::vector<llvm::Value*> args{};

        //                 for (auto i = 1; i < exp.list.size(); i++) {
        //                     args.push_back(gen(exp.list[i], env));
        //                 }
        //                 return builder->CreateCall(printFn, args);
        //             }
        //         }


        //         break;


        //     default:
        //         break;

        // }

        // return builder->getInt32(exp.number);
    }

    void saveModuleToFile(const std::string& fileName) {
        std::error_code errorCode;

        llvm::raw_fd_ostream outLL(fileName, errorCode);

        module->print(outLL, nullptr);
    }


    void initializeModule() {
        ctx = std::make_unique<llvm::LLVMContext>();

        module = std::make_unique<llvm::Module>("JovianVM", *ctx);

        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);

        varsBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    llvm::Function *createFunction(const std::string &fnName, llvm::FunctionType *fnType) {
        auto fn = module->getFunction(fnName);

        if (fn == nullptr) {
            fn = createFunctionProto(fnName, fnType);
        }

        createFunctionBlock(fn);

        return fn;
    }

    void createFunctionBlock(llvm::Function *fn) {
        auto entry = createBB("entry", fn);
        builder->SetInsertPoint(entry);
    }

    llvm::BasicBlock *createBB(std::string name, llvm::Function *fn = nullptr) {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }

    llvm::Function* createFunctionProto(const std::string& fnName, llvm::FunctionType* fnType) {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);

        verifyFunction(*fn);

        // env->define(fnName, fn);

        return fn;
    }

    void setupExternalFunction() {
        auto bytePtrTy = builder->getInt8Ty()->getPointerTo();

        module->getOrInsertFunction("printf", llvm::FunctionType::get(builder->getInt32Ty(), bytePtrTy, true));

        module->getOrInsertFunction("malloc", llvm::FunctionType::get(bytePtrTy, builder->getInt64Ty(), false));
    }

    /**
     * Parser.
     */
    std::unique_ptr<JovianParser> parser;

    //std::shared_ptr<Environment> GlobalEnv;

    llvm::StructType *cls = nullptr;

    //std::map<std::string, ClassInfo> classMap_;

    llvm::Function *fn;

    std::unique_ptr<llvm::LLVMContext> ctx;

    std::unique_ptr<llvm::Module> module;

    std::unique_ptr<llvm::IRBuilder<>> varsBuilder;

    std::unique_ptr<llvm::IRBuilder<>> builder;
    
};

#endif