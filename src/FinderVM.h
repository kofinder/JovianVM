
#ifndef FinderVM_h
#define FinderVM_h

#include <iostream>
#include <map>
#include <regex>
#include <string>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include "./Environment.h"
#include "./Logger.h"
#include "./parser/JovianParser.h"

using syntax::JovianParser;

using Env = std::shared_ptr<Environment>;

/**
 * Class info. Contains struct type and field names.
 */
struct ClassInfo
{
    llvm::StructType *cls;
    llvm::StructType *parent;
    std::map<std::string, llvm::Type *> fieldsMap;
    std::map<std::string, llvm::Function *> methodsMap;
};

/**
 * Index of the vTable in the class fields.
 */
static const size_t VTABLE_INDEX = 0;

/**
 * Each class has set of reserved fields at the
 * beginning of its layout. Currently it's only
 * the vTable used to resolve methods.
 */
static const size_t RESERVED_FIELDS_COUNT = 1;

// Generic binary operator:
#define GEN_BINARY_OP(Op, varName)             \
    do                                         \
    {                                          \
        auto op1 = gen(exp.list[1], env);      \
        auto op2 = gen(exp.list[2], env);      \
        return builder->Op(op1, op2, varName); \
    } while (false)

class FinderVM
{
public:
    FinderVM() : parser(std::make_unique<JovianParser>())
    {
        moduleInit();
        setupExternalFunction();
        setupGlobalEnvironment();
        setupTargetTriple();
    }

    /**
     * Executes a program.
     */
    void exec(const std::string &program)
    {
        // 1. Parse the program
        auto ast = parser->parse("(begin" + program + ")");

        // 2. Compile to LLVM IR:
        compile(ast);

        // Print generated code.
        module->print(llvm::outs(), nullptr);

        std::cout << "\n";

        // 3. Save module IR to file:
        saveModuleToFile("./out.ll");
    }

private:
    void compile(const Exp &ast)
    {
        // create main function
        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false), GlobalEnv);

        createGlobalVar("VERSION", builder->getInt32(42));

        // compile main body
        gen(ast, GlobalEnv);

        builder->CreateRet(builder->getInt32(0));
    }

    /**
     * Main compile loop
     */
    llvm::Value *gen(const Exp &exp, Env env)
    {

        switch (exp.type)
        {
        case ExpType::NUMBER:
        {
            return builder->getInt32(exp.number);
        }

        case ExpType::STRING:
        {
            auto re = std::regex("\\\\n");
            auto str = std::regex_replace(exp.string, re, "\n");
            return builder->CreateGlobalStringPtr(str);
        }

        case ExpType::SYMBOL:
        {
            if (exp.string == "true" || exp.string == "false")
            {
                return builder->getInt1(exp.string == "true" ? true : false);
            }
            else
            {

                auto varName = exp.string;
                auto value = env->lookup(varName);

                if (auto localVar = llvm::dyn_cast<llvm::AllocaInst>(value))
                {
                    return builder->CreateLoad(localVar->getAllocatedType(), localVar, varName.c_str());
                }

                else if (auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value))
                {
                    return builder->CreateLoad(globalVar->getInitializer()->getType(), globalVar, varName.c_str());
                }

                else
                {
                    return value;
                }

                // return module->getNamedGlobal(exp.string)->getInitializer();
            }

            // return builder->getInt32(0);
        }

        case ExpType::LIST:
        {
            auto tag = exp.list[0];
            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                // --------------------------------------------
                // Binary math operations:

                if (op == "+")
                {
                    GEN_BINARY_OP(CreateAdd, "tmpadd");
                }

                else if (op == "-")
                {
                    GEN_BINARY_OP(CreateSub, "tmpsub");
                }

                else if (op == "*")
                {
                    GEN_BINARY_OP(CreateMul, "tmpmul");
                }

                else if (op == "/")
                {
                    GEN_BINARY_OP(CreateSDiv, "tmpdiv");
                }

                // --------------------------------------------
                // Compare operations: (> 5 10)

                // UGT - unsigned, greater than
                else if (op == ">")
                {
                    GEN_BINARY_OP(CreateICmpUGT, "tmpcmp");
                }

                // ULT - unsigned, less than
                else if (op == "<")
                {
                    GEN_BINARY_OP(CreateICmpULT, "tmpcmp");
                }

                // EQ - equal
                else if (op == "==")
                {
                    GEN_BINARY_OP(CreateICmpEQ, "tmpcmp");
                }

                // NE - not equal
                else if (op == "!=")
                {
                    GEN_BINARY_OP(CreateICmpNE, "tmpcmp");
                }

                // UGE - greater or equal
                else if (op == ">=")
                {
                    GEN_BINARY_OP(CreateICmpUGE, "tmpcmp");
                }

                // ULE - less or equal
                else if (op == "<=")
                {
                    GEN_BINARY_OP(CreateICmpULE, "tmpcmp");
                }

                // --------------------------------------------
                // Branch instruction:

                /**
                 * (if <cond> <then> <else>)
                 */
                else if (op == "if")
                {
                    // Compile <cond>:
                    auto cond = gen(exp.list[1], env);

                    auto thenBlock = createBB("then", fn);
                    auto elseBlock = createBB("else");
                    auto ifEndBlock = createBB("ifend");

                    builder->CreateCondBr(cond, thenBlock, elseBlock);

                    builder->SetInsertPoint(thenBlock);
                    auto thenRes = gen(exp.list[2], env);
                    builder->CreateBr(ifEndBlock);

                    thenBlock = builder->GetInsertBlock();
                    fn->getBasicBlockList().push_back(elseBlock);

                    builder->SetInsertPoint(elseBlock);
                    auto elseRes = gen(exp.list[3], env);
                    builder->CreateBr(ifEndBlock);

                    elseBlock = builder->GetInsertBlock();
                    fn->getBasicBlockList().push_back(ifEndBlock);
                    builder->SetInsertPoint(ifEndBlock);

                    auto phi = builder->CreatePHI(thenRes->getType(), 2, "tmpif");
                    phi->addIncoming(thenRes, thenBlock);
                    phi->addIncoming(elseRes, elseBlock);

                    return phi;
                }

                // --------------------------------------------
                // While loop:

                /**
                 * (while <cond> <body>)
                 *
                 */
                else if (op == "while")
                {
                    auto condBlock = createBB("cond", fn);
                    builder->CreateBr(condBlock);

                    auto bodyBlock = createBB("body");
                    auto loopEndBlock = createBB("loopend");

                    builder->SetInsertPoint(condBlock);
                    auto cond = gen(exp.list[1], env);

                    builder->CreateCondBr(cond, bodyBlock, loopEndBlock);

                    fn->getBasicBlockList().push_back(bodyBlock);
                    builder->SetInsertPoint(bodyBlock);
                    gen(exp.list[2], env);
                    builder->CreateBr(condBlock);

                    fn->getBasicBlockList().push_back(loopEndBlock);
                    builder->SetInsertPoint(loopEndBlock);

                    return builder->getInt32(0);
                }

                // --------------------------------------------
                // Function declaration: (def <name> <params> <body>)
                //

                else if (op == "def")
                {
                    return compileFunction(exp, /* name */ exp.list[1].string, env);
                }

                if (op == "var")
                {
                    if (cls != nullptr)
                    {
                        return builder->getInt32(0);
                    }

                    auto varNameDecl = exp.list[1];
                    auto varName = extractVarName(varNameDecl);

                    if (isNew(exp.list[2]))
                    {
                        auto instance = createInstance(exp.list[2], env, varName);
                        return env->define(varName, instance);
                    }

                    auto init = gen(exp.list[2], env);

                    auto varTy = extractVarType(varNameDecl);

                    auto varBinding = allocVar(varName, varTy, env);

                    return builder->CreateStore(init, varBinding);
                }

                else if (op == "set")
                {
                    auto value = gen(exp.list[2], env);

                    if (isProp(exp.list[1]))
                    {
                        auto instance = gen(exp.list[1].list[1], env);
                        auto fieldName = exp.list[1].list[2].string;
                        auto ptrName = std::string("p") + fieldName;

                        auto cls = (llvm::StructType *)(instance->getType()->getContainedType(0));

                        auto fieldIdx = getFieldIndex(cls, fieldName);

                        auto address = builder->CreateStructGEP(cls, instance, fieldIdx, ptrName);

                        builder->CreateStore(value, address);

                        return value;
                    }

                    else
                    {
                        auto varName = exp.list[1].string;

                        auto varBinding = env->lookup(varName);

                        builder->CreateStore(value, varBinding);

                        return value;
                    }
                }

                else if (op == "begin")
                {
                    auto blockEnv = std::make_shared<Environment>(
                        std::map<std::string, llvm::Value *>{}, env);

                    llvm::Value *blockRes;
                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        blockRes = gen(exp.list[i], blockEnv);
                    }
                    return blockRes;
                }

                else if (op == "printf")
                {
                    auto printFn = module->getFunction("printf");

                    std::vector<llvm::Value*> args{};

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        args.push_back(gen(exp.list[i], env));
                    }

                    return builder->CreateCall(printFn, args);
                }

                else if (op == "class")
                {
                    auto name = exp.list[1].string;

                    auto parent = exp.list[2].string == "null"
                                      ? nullptr
                                      : getClassByName(exp.list[2].string);

                    cls = llvm::StructType::create(*ctx, name);

                    if (parent != nullptr)
                    {
                        inheritClass(cls, parent);
                    }
                    else
                    {
                        classMap_[name] = {cls, parent, {}, {}};
                    }

                    buildClassInfo(cls, exp, env);

                    gen(exp.list[3], env);

                    cls = nullptr;

                    return builder->getInt32(0);
                }

                else if (op == "new")
                {
                    return createInstance(exp, env, "");
                }

                else if (op == "prop")
                {
                    auto instance = gen(exp.list[1], env);
                    auto fieldName = exp.list[2].string;
                    auto ptrName = std::string("p") + fieldName;

                    auto cls = (llvm::StructType*)(instance->getType()->getContainedType(0));
                    auto fieldIdx = getFieldIndex(cls, fieldName);

                    auto address = builder->CreateStructGEP(cls, instance, fieldIdx, ptrName);

                    return builder->CreateLoad(cls->getElementType(fieldIdx), address, fieldName);
                }

                else if(op == "method") {
                    auto methodName = exp.list[2].string;

                    llvm::StructType* cls;
                    llvm::Value* vTable;
                    llvm::StructType* vTableTy;

                    if(isSuper(exp.list[1])) {
                        auto className = exp.list[1].list[1].string;
                        cls = classMap_[className].parent;
                        auto parentName = std::string{cls->getName().data()};

                        vTable = module->getNamedGlobal(parentName + "_vTable");
                        vTableTy = llvm::StructType::getTypeByName(*ctx, parentName + "_vTable");
                    } else {
                        auto instance = gen(exp.list[1], env);
                        cls = (llvm::StructType*) (instance->getType()->getContainedType(0));
                        auto vTableAddr = builder->CreateStructGEP(cls, instance, VTABLE_INDEX);

                        vTable = builder->CreateLoad(cls->getElementType(VTABLE_INDEX), vTableAddr, "vt");

                        vTableTy = (llvm::StructType*)(vTable->getType()->getContainedType(0));
                    }

                    auto methodIdx = getMethodIndex(cls, methodName);

                    auto methodTy= (llvm::FunctionType*) vTableTy->getElementType(methodIdx);

                    auto methodAddr = builder->CreateStructGEP(vTableTy, vTable, methodIdx);

                    return builder->CreateLoad(methodTy, methodAddr);
                }

                else
                {

                    auto callable = gen(exp.list[0], env);
                    auto callableTy = callable->getType()->getContainedType(0);
                    
                    std::vector<llvm::Value*> args{};
                    auto argIdx = 0;

                    if(callableTy->isStructTy()) {
                        auto cls = (llvm::StructType*) callableTy;
                        std::string className{cls->getName().data()};

                        args.push_back(callable);
                        argIdx++;

                        callable = module->getFunction(className + "___call__");
                    }


                    auto fn = (llvm::Function*) callable;
                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        auto argValue = gen(exp.list[i], env);
                        auto paramTy = fn->getArg(argIdx) ->getType();
                        auto bitCastArgVal = builder->CreateBitCast(argValue, paramTy);
                        args.push_back(bitCastArgVal);
                    }

                    return builder->CreateCall(fn, args);
                }
            }

            else {
                auto loadedMethod = (llvm::LoadInst*) gen(exp.list[0], env);
                auto fnTy = (llvm::FunctionType*) (loadedMethod->getPointerOperand()->getType()->getContainedType(0)->getContainedType(0));
            
                std::vector<llvm::Value*> args{};

                for(auto i = 1; i < exp.list.size(); i++) {
                    auto argValue = gen(exp.list[i], env);

                    auto paramTy = fnTy->getParamType(i -1);
                    if(argValue->getType() != paramTy) {
                        auto bitCastArgVal = builder->CreateBitCast(argValue, paramTy);
                        args.push_back(bitCastArgVal);
                    } else {
                        args.push_back(argValue);
                    }
                }

                return builder->CreateCall(fnTy, loadedMethod, args);
            }
        }

        break;
        }

        return builder->getInt32(exp.number);
    }

    /**
     * Returns field index.
     */
    size_t getFieldIndex(llvm::StructType *cls, const std::string &fieldName)
    {
        auto fields = &classMap_[cls->getName().data()].fieldsMap;
        auto it = fields->find(fieldName);
        return std::distance(fields->begin(), it) + RESERVED_FIELDS_COUNT;
    }

    /**
     * Returns method index.
     */
    size_t getMethodIndex(llvm::StructType *cls, const std::string &methodName)
    {
        auto methods = &classMap_[cls->getName().data()].methodsMap;
        auto it = methods->find(methodName);
        return std::distance(methods->begin(), it);
    }

    /**
     * Creates an instance of a class.
     */
    llvm::Value *createInstance(const Exp &exp, Env env, const std::string &name)
    {
        auto className = exp.list[1].string;
        auto cls = getClassByName(className);

        if (cls == nullptr)
        {
            DIE << "[EVALLVM]: Unknow class" << cls;
        }

        // auto instance = name.empty() ? builder->CreateAlloca(cls)
        //                              : builder->CreateAlloca(cls, 0, name);

        auto instance = mallocInstance(cls, name);

        auto ctor = module->getFunction(className + "_constructor");

        std::vector<llvm::Value *> args{instance};

        for (auto i = 2; i < exp.list.size(); i++)
        {
            args.push_back(gen(exp.list[i], env));
        }

        builder->CreateCall(ctor, args);

        return instance;
    }

    /**
     * Allocates an object of a given class on the heap.
     */
    llvm::Value *mallocInstance(llvm::StructType *cls, const std::string &name)
    {
        auto typeSize = builder->getInt64(getTypeSize(cls));

        auto mallocPtr = builder->CreateCall(module->getFunction("malloc"), typeSize, name);

        auto instance = builder->CreatePointerCast(mallocPtr, cls->getPointerTo());

        std::string className(cls->getName().data());
        auto vTableName = className + "_vTable";
        auto vTableAddr = builder->CreateStructGEP(cls, instance, VTABLE_INDEX);
        auto vTable = module->getNamedGlobal(vTableName);
        builder->CreateStore(vTable, vTableAddr);

        return instance;

        // return builder->CreatePointerCast(mallocPtr, cls->getPointerTo());
    }

    /**
     * Returns size of a type in bytes.
     */
    size_t getTypeSize(llvm::Type *type_)
    {
        return module->getDataLayout().getTypeAllocSize(type_);
    }

    /**
     * Inherits parent class fields.
     */
    void inheritClass(llvm::StructType *cls, llvm::StructType *parent)
    {
        auto parentClassInfo = &classMap_[parent->getName().data()];

        classMap_[cls->getName().data()] = {
            cls,
            parent,
            parentClassInfo->fieldsMap,
            parentClassInfo->methodsMap
        };
    }

    /**
     * Extracts fields and methods from a class expression.
     */
    void buildClassInfo(llvm::StructType *cls, const Exp &clsExp, Env env)
    {
        auto className = clsExp.list[1].string;
        auto classInfo = &classMap_[className];

        auto body = clsExp.list[3];

        for (auto i = 1; i < body.list.size(); i++)
        {
            auto exp = body.list[i];

            if (isVar(exp))
            {
                auto varNameDecl = exp.list[1];
                auto fieldName = extractVarName(varNameDecl);
                auto fieldTy = extractVarType(varNameDecl);

                classInfo->fieldsMap[fieldName] = fieldTy;
            }
            else if (isDef(exp))
            {
                auto methodName = exp.list[1].string;
                auto fnName = className + "_" + methodName;

                classInfo->methodsMap[methodName] = createFunctionProto(fnName, extractFunctionType(exp), env);
            }
        }

        buildClassBody(cls);
    }

    /**
     * Builds class body from class info.
     */
    void buildClassBody(llvm::StructType* cls)
    {
        std::string className{cls->getName().data()};

        auto classInfo = &classMap_[className];

        auto vTableName = className + "_vTable";
        auto vTabley = llvm::StructType::create(*ctx, vTableName);

        auto clsFields = std::vector<llvm::Type*>{vTabley->getPointerTo()};

        for (const auto &fieldInfo : classInfo->fieldsMap)
        {
            clsFields.push_back(fieldInfo.second);
        }

        cls->setBody(clsFields, false);

        buildVTable(cls);
    }

    /**
     * Creates a vtable per class.
     *
     * vTable stores method references to support
     * inheritance and methods overloading.
     */
    void buildVTable(llvm::StructType *cls)
    {
        std::string classsName(cls->getName().data());
        auto vTableName = classsName + "_vTable";

        auto vTableTy = llvm::StructType::getTypeByName(*ctx, vTableName);

        std::vector<llvm::Constant*> vtableMethods;
        std::vector<llvm::Type*> vtableMethodTys;

        for(auto& methodInfo : classMap_[classsName].methodsMap) {
            auto method = methodInfo.second;
            vtableMethods.push_back(method);
            vtableMethodTys.push_back(method->getType());
        }


        vTableTy->setBody(vtableMethodTys);
        auto vTableValue = llvm::ConstantStruct::get(vTableTy, vtableMethods);
        createGlobalVar(vTableName, vTableValue);

    }

    /**
     * Tagged lists.
     */
    bool isTaggedList(const Exp &exp, const std::string &tag)
    {
        return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL &&
               exp.list[0].string == tag;
    }

    /**
     * (var ...)
     */
    bool isVar(const Exp &exp) { return isTaggedList(exp, "var"); }

    /**
     * (def ...)
     */
    bool isDef(const Exp &exp) { return isTaggedList(exp, "def"); }

    /**
     * (new ...)
     */
    bool isNew(const Exp &exp) { return isTaggedList(exp, "new"); }

    /**
     * (prop ...)
     */
    bool isProp(const Exp &exp) { return isTaggedList(exp, "prop"); }

    /**
     * (super ...)
     */
    bool isSuper(const Exp &exp) { return isTaggedList(exp, "super"); }

    llvm::StructType *getClassByName(const std::string &name)
    {
        return llvm::StructType::getTypeByName(*ctx, name);
    }

    std::string extractVarName(const Exp &exp)
    {
        return exp.type == ExpType::LIST ? exp.list[0].string : exp.string;
    }

    llvm::Type *extractVarType(const Exp &exp)
    {
        return exp.type == ExpType::LIST ? getTypeFromString(exp.list[1].string) : builder->getInt32Ty();
    }

    llvm::Type *getTypeFromString(const std::string &type_)
    {
        if (type_ == "number")
        {
            return builder->getInt32Ty();
        }

        if (type_ == "string")
        {
            return builder->getInt8Ty()->getPointerTo();
        }

        return classMap_[type_].cls->getPointerTo();
    }

    bool hasReturnType(const Exp &fnExp)
    {
        return fnExp.list[3].type == ExpType::SYMBOL &&
               fnExp.list[3].string == "->";
    }

    llvm::FunctionType *extractFunctionType(const Exp &fnExp)
    {
        auto params = fnExp.list[2];

        auto returnType = hasReturnType(fnExp)
                              ? getTypeFromString(fnExp.list[4].string)
                              : builder->getInt32Ty();

        std::vector<llvm::Type *> paramTypes;

        for (auto &param : params.list)
        {
            auto paramName = extractVarName(param);
            auto paramType = extractVarType(param);

            // paramTypes.push_back(paramType);

            paramTypes.push_back(
                paramName == "self" ? (llvm::Type*)cls->getPointerTo() : paramType);
        }

        return llvm::FunctionType::get(returnType, paramTypes, false);
    }

    llvm::Value *compileFunction(const Exp &fnExp, std::string fnName, Env env)
    {
        auto params = fnExp.list[2];
        auto body = hasReturnType(fnExp) ? fnExp.list[5] : fnExp.list[3];

        auto prevFn = fn;
        auto prevBlock = builder->GetInsertBlock();

        auto origName = fnName;

        if (cls != nullptr)
        {
            fnName = std::string(cls->getName().data()) + "_" + fnName;
        }

        auto newFn = createFunction(fnName, extractFunctionType(fnExp), env);
        fn = newFn;

        auto idx = 0;

        auto fnEnv = std::make_shared<Environment>(
            std::map<std::string, llvm::Value *>{}, env);

        for (auto &arg : fn->args())
        {
            auto param = params.list[idx++];
            auto argName = extractVarName(param);

            arg.setName(argName);
            auto argBinding = allocVar(argName, arg.getType(), fnEnv);
            builder->CreateStore(&arg, argBinding);
        }

        builder->CreateRet(gen(body, fnEnv));

        builder->SetInsertPoint(prevBlock);
        fn = prevFn;

        return newFn;
    }

    llvm::Value *allocVar(std::string &name, llvm::Type *type_, Env env)
    {
        varsBuilder->SetInsertPoint(&fn->getEntryBlock());

        auto varAlloc = varsBuilder->CreateAlloca(type_, 0, name.c_str());
        env->define(name, varAlloc);

        return varAlloc;
    }

    /**
     * Creates a gloabl variable
     */
    llvm::GlobalVariable *createGlobalVar(const std::string &name, llvm::Constant *init)
    {
        module->getOrInsertGlobal(name, init->getType());
        auto variable = module->getNamedGlobal(name);
        variable->setAlignment(llvm::MaybeAlign(4));
        variable->setConstant(false);
        variable->setInitializer(init);
        return variable;
    }

    /**
     * Define external function(from libc++)
     */
    void setupExternalFunction()
    {
        // 18* to substitue for char*, void*, etc
        auto bytePtrTy = builder->getInt8Ty()->getPointerTo();

        module->getOrInsertFunction("printf", llvm::FunctionType::get(
                                                  /*return type*/ builder->getInt32Ty(),
                                                  /*format arg*/ bytePtrTy,
                                                  /* varargs*/ true));

        module->getOrInsertFunction(
            "malloc", llvm::FunctionType::get(bytePtrTy, builder->getInt64Ty(), false));
    }

    /**
     * Create function
     */
    llvm::Function *createFunction(const std::string &fnName, llvm::FunctionType *fnType, Env env)
    {
        // function prototype might already be defined:
        auto fn = module->getFunction(fnName);

        // if not, allocate th function:
        if (fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType, env);
        }

        createFunctionBlock(fn);
        return fn;
    }

    /**
     * Create function
     */
    void createFunctionBlock(llvm::Function *fn)
    {
        auto entry = createBB("entry", fn);
        builder->SetInsertPoint(entry);
    }

    /**
     * Create a basic block if the fn is passed, this block is
     * automically appended to the parent function. Otherwise,
     * the block should later be append manually via
     * fn->getBasicBlockList().push_back(block)
     */
    llvm::BasicBlock *createBB(std::string name, llvm::Function *fn = nullptr)
    {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }

    llvm::Function *createFunctionProto(const std::string &fnName, llvm::FunctionType *fnType, Env env)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);

        verifyFunction(*fn);

        env->define(fnName, fn);

        return fn;
    }

    /**
     * Initialize the module.
     */
    void moduleInit()
    {
        // Open a new context and module.
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("EvaLLVM", *ctx);

        // Create a new builder for the module.
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);

        varsBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    void setupGlobalEnvironment()
    {

        std::map<std::string, llvm::Value *> globalObject{
            {"VERSION", builder->getInt32(42)}};

        std::map<std::string, llvm::Value *> globalRec{};

        for (auto &entry : globalObject)
        {
            globalRec[entry.first] = createGlobalVar(entry.first, (llvm::Constant *)entry.second);
        }

        GlobalEnv = std::make_shared<Environment>(globalRec, nullptr);
    }

    void setupTargetTriple()
    {
        module->setTargetTriple("x86_64-pc-linux-gnu");
    }

    /**
     * Saves IR to file.
     */
    void saveModuleToFile(const std::string &filename)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(filename, errorCode);
        module->print(outLL, nullptr);
    }

    /**
     * Parser.
     */
    std::unique_ptr<JovianParser> parser;

    /**
     * Global Environment (symbol table).
     */
    std::shared_ptr<Environment> GlobalEnv;

    /**
     * Currently compiling class.
     */
    llvm::StructType *cls = nullptr;

    /**
     * Class info.
     */
    std::map<std::string, ClassInfo> classMap_;

    /**
     * Currently compiling function.
     */
    llvm::Function *fn;

    /**
     * Global LLVM context.
     * It owns and manages the core "global" data of LLVM's core
     * infrastructure, including the type and constant unique tables.
     */
    std::unique_ptr<llvm::LLVMContext> ctx;

    /**
     * A Module instance is used to store all the information related to an
     * LLVM module. Modules are the top level container of all other LLVM
     * Intermediate Representation (IR) objects. Each module directly contains a
     * list of globals variables, a list of functions, a list of libraries (or
     * other modules) this module depends on, a symbol table, and various data
     * about the target's characteristics.
     *
     * A module maintains a GlobalList object that is used to hold all
     * constant references to global variables in the module.  When a global
     * variable is destroyed, it should have no entries in the GlobalList.
     * The main container class for the LLVM Intermediate Representation.
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * Extra builder for variables declaration.
     * This builder always prepends to the beginning of the
     * function entry block.
     */
    std::unique_ptr<llvm::IRBuilder<>> varsBuilder;

    /**
     * IR Builder.
     *
     * This provides a uniform API for creating instructions and inserting
     * them into a basic block: either at the end of a BasicBlock, or at a
     * specific iterator location in a block.
     */
    std::unique_ptr<llvm::IRBuilder<>> builder;
};

#endif