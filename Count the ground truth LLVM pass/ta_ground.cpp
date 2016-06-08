#define FOREACH_FUNC_INS(F, I, B) do { \
    for (Function::const_iterator __FI = F->begin(), __FE = F->end(); __FI != __FE; ++__FI) { \
        for (BasicBlock::const_iterator __BI = __FI->begin(), BE = __FI->end(); __BI != BE; ++__BI) { \
            Instruction *I = (Instruction*) ((unsigned long) &(*__BI)); \
            B \
        } \
    } \
} while(0)

#define FOREACH_FUNC_CS(F, CS, B) do { \
    FOREACH_FUNC_INS(F, I, \
        CallSite CS = PassUtil::getCallSiteFromInstruction(I); \
        if (!CS.getInstruction()) \
            continue; \
        B \
    ); \
} while(0)

inline bool is_indirect_call(CallSite &CS) {
    Function *target = CS.getCalledFunction();
    if (target == NULL) return true;
    return false;
}

inline void InfoPass::addVoidFunctions()
{
    std::string name("info-non-voids");
    if (!VoidOpt)
        return;
    std::vector<std::string> values;
    Module::FunctionListType &functionList = M->getFunctionList();
    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        Function *F = it;
        if (!F->getFunctionType()->getReturnType()->isVoidTy() &&
            !F->getFunctionType()->getReturnType()->isFloatingPointTy()) {
            values.push_back(getFunctionName(F));
        }
    }
    OutputEntry entry(name, values);
    voidOutput.addEntry(entry);
}

inline void InfoPass::addNonVoidICalls()
{
    std::string name("info-non-void-icalls");
    if (!NonVoidICallsOpt)
        return;

    std::vector<std::string> values;

    Module::FunctionListType &functionList = M->getFunctionList();
    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        Function *F = it;

        int i = 0;
        FOREACH_FUNC_CS(F, CS,
            if (is_indirect_call(CS) && !isa<ConstantExpr>(CS.getCalledValue())) {
                if (CS.getType()->isVoidTy()) continue;
                std::stringstream sstm;
                sstm << getFunctionName(F) << "." << i;
                values.push_back(sstm.str());
                i++;
            }
        );
    }
    OutputEntry entry(name, values);
    nonVoidICallOutput.addEntry(entry);

}

inline void InfoPass::addArguments()
{
    if (!ArgsOpt)
        return;

    std::vector<std::string> keys;
    std::vector<std::string> values;

    Module::FunctionListType &functionList = M->getFunctionList();
    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        Function *F = it;


        if (ICallArgsOpt) {
            /* Get the argcount for each indirect callsite in this function */
            int i = 0;
            FOREACH_FUNC_CS(F, CS,
                if (is_indirect_call(CS) && !isa<ConstantExpr>(CS.getCalledValue())) {
                    std::stringstream sstm;
                    sstm << getFunctionName(F) << "." << i;
                    keys.push_back(sstm.str());

                    std::stringstream arg_size;
                    arg_size << CS.arg_size();
                    values.push_back(arg_size.str());
                    i++;
                }
            );

        } else {
            if (VarArgsOpt && !F->isVarArg()) continue;
            if (NoVarArgsOpt && F->isVarArg()) continue;
            keys.push_back(getFunctionName(F));

            unsigned float_params = 0;
            if (NoFloatsOpt) {
                FunctionType *FT = F->getFunctionType();
                for (unsigned i = 0; i < FT->getNumParams(); i++)
                    if (FT->getParamType(i)->isFloatingPointTy())
                        float_params++;
            }

            /* convert F->arg_size() to a string */
            std::stringstream arg_size;
            arg_size << F->arg_size() - float_params;
            values.push_back(arg_size.str());
        }
    }
    OutputEntry entry(keys, values);
    argsOutput.addEntry(entry);
}

