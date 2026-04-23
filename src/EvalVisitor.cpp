#include "EvalVisitor.h"
#include "Python3Lexer.h"
#include "Python3Parser.h"
#include <antlr4-runtime.h>
#include <regex>

EvalVisitor::EvalVisitor()
    : globalEnvironment(std::make_shared<Environment>(false))
    , currentEnvironment(globalEnvironment)
    , inLoop(false)
    , inFunction(false) {
}

std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    if (!ctx) return {};
    try {
        auto stmts = ctx->stmt();
        for (size_t i = 0; i < stmts.size(); i++) {
            if (stmts[i]) {
                try {
                    visit(stmts[i]);
                } catch (const std::exception& e) {
                    // Skip statements that fail (like comments)
                    continue;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in visitFile_input: " << e.what() << std::endl;
        throw;
    }
    return {};
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    std::string funcName = ctx->NAME()->getText();
    auto func = std::make_shared<Function>(funcName, ctx);

    if (ctx->parameters() && ctx->parameters()->typedargslist()) {
        auto typedargs = ctx->parameters()->typedargslist();
        if (typedargs->tfpdef().size() > 0) {
            for (auto tfpdef : typedargs->tfpdef()) {
                func->parameters.push_back(tfpdef->NAME()->getText());
            }
        }
    }

    currentEnvironment->setFunction(funcName, func);
    return {};
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    try {
        executeStmt(ctx);
    } catch (const std::exception& e) {
        std::cerr << "Error in visitStmt: " << e.what() << std::endl;
        throw;
    }
    return {};
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    executeSimpleStmt(ctx);
    return {};
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    executeExprStmt(ctx);
    return {};
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    executeFlowStmt(ctx);
    return {};
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    executeBreakStmt(ctx);
    return {};
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    executeContinueStmt(ctx);
    return {};
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    executeReturnStmt(ctx);
    return {};
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    executeIfStmt(ctx);
    return {};
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    executeWhileStmt(ctx);
    return {};
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    executeSuite(ctx);
    return {};
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    return std::make_any<Value>(evaluateExpression(ctx));
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    return std::make_any<Value>(evaluateOrTest(ctx));
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    return std::make_any<Value>(evaluateAndTest(ctx));
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    return std::make_any<Value>(evaluateNotTest(ctx));
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    return std::make_any<Value>(evaluateComparison(ctx));
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    return std::make_any<Value>(evaluateArithExpr(ctx));
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    return std::make_any<Value>(evaluateTerm(ctx));
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    return std::make_any<Value>(evaluateFactor(ctx));
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    return std::make_any<Value>(evaluateAtomExpr(ctx));
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (!ctx) {
        return std::make_any<Value>(Value());
    }
    return std::make_any<Value>(evaluateAtom(ctx));
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    return std::make_any<Value>(evaluateFormatString(ctx));
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    return std::make_any<std::vector<Value>>(evaluateTestList(ctx));
}

std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    return std::make_any<std::vector<Value>>(evaluateArgList(ctx));
}

std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    if (ctx->test().size() == 1) {
        return std::make_any<Value>(evaluateExpression(ctx->test(0)));
    } else if (ctx->test().size() == 2) {
        return std::make_any<Value>(evaluateExpression(ctx->test(1)));
    }
    return std::make_any<Value>(Value());
}

Value EvalVisitor::evaluateExpression(Python3Parser::TestContext* ctx) {
    if (!ctx || !ctx->or_test()) return Value();
    return evaluateOrTest(ctx->or_test());
}

Value EvalVisitor::evaluateOrTest(Python3Parser::Or_testContext* ctx) {
    if (ctx->and_test().size() == 1) {
        return evaluateAndTest(ctx->and_test(0));
    }

    Value result = evaluateAndTest(ctx->and_test(0));
    for (size_t i = 1; i < ctx->and_test().size(); i++) {
        if (result.asBool()) {
            return Value(true);
        }
        Value next = evaluateAndTest(ctx->and_test(i));
        result = result || next;
    }
    return result;
}

Value EvalVisitor::evaluateAndTest(Python3Parser::And_testContext* ctx) {
    if (ctx->not_test().size() == 1) {
        return evaluateNotTest(ctx->not_test(0));
    }

    Value result = evaluateNotTest(ctx->not_test(0));
    for (size_t i = 1; i < ctx->not_test().size(); i++) {
        if (!result.asBool()) {
            return Value(false);
        }
        Value next = evaluateNotTest(ctx->not_test(i));
        result = result && next;
    }
    return result;
}

Value EvalVisitor::evaluateNotTest(Python3Parser::Not_testContext* ctx) {
    if (ctx->NOT()) {
        return !evaluateNotTest(ctx->not_test());
    }
    return evaluateComparison(ctx->comparison());
}

Value EvalVisitor::evaluateComparison(Python3Parser::ComparisonContext* ctx) {
    Value left = evaluateArithExpr(ctx->arith_expr(0));

    if (ctx->comp_op().empty()) {
        return left;
    }

    for (size_t i = 0; i < ctx->comp_op().size(); i++) {
        Value right = evaluateArithExpr(ctx->arith_expr(i + 1));
        std::string op = ctx->comp_op(i)->getText();

        bool result;
        if (op == "<") {
            result = left < right;
        } else if (op == ">") {
            result = left > right;
        } else if (op == "<=") {
            result = left <= right;
        } else if (op == ">=") {
            result = left >= right;
        } else if (op == "==") {
            result = left == right;
        } else if (op == "!=") {
            result = left != right;
        } else {
            result = false;
        }

        if (!result) {
            return Value(false);
        }

        left = right;
    }

    return Value(true);
}

Value EvalVisitor::evaluateArithExpr(Python3Parser::Arith_exprContext* ctx) {
    if (ctx->term().size() == 1) {
        return evaluateTerm(ctx->term(0));
    }

    Value result = evaluateTerm(ctx->term(0));
    for (size_t i = 1; i < ctx->term().size(); i++) {
        Value next = evaluateTerm(ctx->term(i));
        std::string op = ctx->addorsub_op(i - 1)->getText();

        if (op == "+") {
            result = result + next;
        } else if (op == "-") {
            result = result - next;
        }
    }
    return result;
}

Value EvalVisitor::evaluateTerm(Python3Parser::TermContext* ctx) {
    if (ctx->factor().size() == 1) {
        return evaluateFactor(ctx->factor(0));
    }

    Value result = evaluateFactor(ctx->factor(0));
    for (size_t i = 1; i < ctx->factor().size(); i++) {
        Value next = evaluateFactor(ctx->factor(i));
        std::string op = ctx->muldivmod_op(i - 1)->getText();

        if (op == "*") {
            result = result * next;
        } else if (op == "/") {
            result = result / next;
        } else if (op == "//") {
            result = result.floorDiv(next);
        } else if (op == "%") {
            result = result % next;
        }
    }
    return result;
}

Value EvalVisitor::evaluateFactor(Python3Parser::FactorContext* ctx) {
    if (ctx->factor()) {
        if (ctx->MINUS()) {
            Value value = evaluateFactor(ctx->factor());
            return -value;
        } else if (ctx->ADD()) {
            return evaluateFactor(ctx->factor());
        }
    }
    return evaluateAtomExpr(ctx->atom_expr());
}

Value EvalVisitor::evaluateAtomExpr(Python3Parser::Atom_exprContext* ctx) {
    if (!ctx || !ctx->atom()) return Value();

    Value atom = evaluateAtom(ctx->atom());

    auto trailer = ctx->trailer();
    if (trailer && trailer->OPEN_PAREN()) {
        std::vector<Value> args;
        if (trailer->arglist()) {
            try {
                args = evaluateArgList(trailer->arglist());
            } catch (const std::exception& e) {
                std::cerr << "Error evaluating args: " << e.what() << std::endl;
                throw;
            }
        }

        if (atom.isStr()) {
            std::string funcName = atom.asString();
            try {
                if (funcName == "print") {
                    atom = callBuiltInFunction("print", args);
                } else if (funcName == "int") {
                    atom = callBuiltInFunction("int", args);
                } else if (funcName == "float") {
                    atom = callBuiltInFunction("float", args);
                } else if (funcName == "str") {
                    atom = callBuiltInFunction("str", args);
                } else if (funcName == "bool") {
                    atom = callBuiltInFunction("bool", args);
                } else {
                    auto func = currentEnvironment->getFunction(funcName);
                    if (func) {
                        atom = callUserFunction(func, args);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error calling function " << funcName << ": " << e.what() << std::endl;
                throw;
            }
        }
    }

    return atom;
}

Value EvalVisitor::evaluateAtom(Python3Parser::AtomContext* ctx) {
    if (ctx->NONE()) {
        return Value();
    }
    if (ctx->TRUE()) {
        return Value(true);
    }
    if (ctx->FALSE()) {
        return Value(false);
    }
    if (ctx->NUMBER()) {
        std::string numStr = ctx->NUMBER()->getText();
        if (numStr.find('.') != std::string::npos) {
            return Value(std::stod(numStr));
        } else {
            return Value(BigInt(numStr));
        }
    }
    if (ctx->STRING().size() > 0) {
        std::string str = ctx->STRING(0)->getText();
        str = str.substr(1, str.length() - 2);

        size_t pos = 0;
        while ((pos = str.find("\\n", pos)) != std::string::npos) {
            str.replace(pos, 2, "\n");
            pos += 1;
        }
        pos = 0;
        while ((pos = str.find("\\t", pos)) != std::string::npos) {
            str.replace(pos, 2, "\t");
            pos += 1;
        }
        pos = 0;
        while ((pos = str.find("\\\"", pos)) != std::string::npos) {
            str.replace(pos, 2, "\"");
            pos += 1;
        }
        pos = 0;
        while ((pos = str.find("\\'", pos)) != std::string::npos) {
            str.replace(pos, 2, "'");
            pos += 1;
        }
        pos = 0;
        while ((pos = str.find("\\\\", pos)) != std::string::npos) {
            str.replace(pos, 2, "\\");
            pos += 1;
        }

        return Value(str);
    }
    if (ctx->NAME()) {
        std::string name = ctx->NAME()->getText();
        Value var = currentEnvironment->getVariable(name);
        if (!var.isNone()) {
            return var;
        }
        // Return the name as a string value for function calls
        return Value(name);
    }
    if (ctx->test()) {
        return evaluateExpression(ctx->test());
    }
    if (ctx->format_string()) {
        std::string result = evaluateFormatString(ctx->format_string());
        return Value(result);
    }
    return Value();
}

std::string EvalVisitor::evaluateFormatString(Python3Parser::Format_stringContext* ctx) {
    std::string result;
    for (auto child : ctx->children) {
        if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            std::string text = terminal->getText();
            if (text == "f\"" || text == "\"") {
                continue;
            } else if (text == "{{") {
                result += "{";
            } else if (text == "}}") {
                result += "}";
            } else {
                result += text;
            }
        } else if (auto testlist = dynamic_cast<Python3Parser::TestlistContext*>(child)) {
            auto tests = testlist->test();
            for (auto test : tests) {
                Value val = evaluateExpression(test);
                result += val.asString();
            }
        }
    }
    return result;
}

std::vector<Value> EvalVisitor::evaluateTestList(Python3Parser::TestlistContext* ctx) {
    std::vector<Value> result;
    for (auto test : ctx->test()) {
        result.push_back(evaluateExpression(test));
    }
    return result;
}

std::vector<Value> EvalVisitor::evaluateArgList(Python3Parser::ArglistContext* ctx) {
    std::vector<Value> result;
    for (auto arg : ctx->argument()) {
        result.push_back(std::any_cast<Value>(visit(arg)));
    }
    return result;
}

void EvalVisitor::executeSuite(Python3Parser::SuiteContext* ctx) {
    for (auto stmt : ctx->stmt()) {
        executeStmt(stmt);
    }
}

void EvalVisitor::executeStmt(Python3Parser::StmtContext* ctx) {
    if (ctx->simple_stmt()) {
        executeSimpleStmt(ctx->simple_stmt());
    } else if (ctx->compound_stmt()) {
        visit(ctx->compound_stmt());
    }
}

void EvalVisitor::executeSimpleStmt(Python3Parser::Simple_stmtContext* ctx) {
    auto small_stmt = ctx->small_stmt();
    if (small_stmt->expr_stmt()) {
        executeExprStmt(small_stmt->expr_stmt());
    } else if (small_stmt->flow_stmt()) {
        executeFlowStmt(small_stmt->flow_stmt());
    }
}

void EvalVisitor::executeExprStmt(Python3Parser::Expr_stmtContext* ctx) {
    if (!ctx) return;

    try {
        if (ctx->augassign()) {
            auto testlists = ctx->testlist();
            if (testlists.size() >= 1) {
                auto test0 = testlists[0]->test(0);
                auto atom_expr = test0->or_test()->and_test(0)->not_test(0)->comparison()->arith_expr(0)->term(0)->factor(0)->atom_expr();
                auto atom = atom_expr->atom();
                if (atom->NAME()) {
                    std::string varName = atom->NAME()->getText();
                    Value current = currentEnvironment->getVariable(varName);
                    Value expr = evaluateExpression(ctx->testlist(1)->test(0));
                    std::string op = ctx->augassign()->getText();

                    Value result;
                    if (op == "+=") {
                        result = current + expr;
                    } else if (op == "-=") {
                        result = current - expr;
                    } else if (op == "*=") {
                        result = current * expr;
                    } else if (op == "/=") {
                        result = current / expr;
                    } else if (op == "//=") {
                        result = current.floorDiv(expr);
                    } else if (op == "%=") {
                        result = current % expr;
                    }

                    currentEnvironment->setVariable(varName, result);
                }
            }
        } else if (ctx->testlist().size() >= 2) {
            std::vector<Value> targets = evaluateTestList(ctx->testlist(0));
            std::vector<Value> values = evaluateTestList(ctx->testlist(1));

            for (size_t i = 0; i < targets.size() && i < values.size(); i++) {
                if (targets[i].isStr()) {
                    currentEnvironment->setVariable(targets[i].asString(), values[i]);
                }
            }
        } else if (ctx->testlist().size() == 1) {
            evaluateExpression(ctx->testlist(0)->test(0));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in executeExprStmt: " << e.what() << std::endl;
        throw;
    }
}

void EvalVisitor::executeFlowStmt(Python3Parser::Flow_stmtContext* ctx) {
    if (ctx->break_stmt()) {
        executeBreakStmt(ctx->break_stmt());
    } else if (ctx->continue_stmt()) {
        executeContinueStmt(ctx->continue_stmt());
    } else if (ctx->return_stmt()) {
        executeReturnStmt(ctx->return_stmt());
    }
}

void EvalVisitor::executeBreakStmt(Python3Parser::Break_stmtContext* ctx) {
    if (!inLoop) {
        throw std::runtime_error("break outside loop");
    }
    throw BreakException();
}

void EvalVisitor::executeContinueStmt(Python3Parser::Continue_stmtContext* ctx) {
    if (!inLoop) {
        throw std::runtime_error("continue outside loop");
    }
    throw ContinueException();
}

void EvalVisitor::executeReturnStmt(Python3Parser::Return_stmtContext* ctx) {
    if (!inFunction) {
        throw std::runtime_error("return outside function");
    }

    Value returnValue;
    if (ctx->testlist()) {
        auto values = evaluateTestList(ctx->testlist());
        if (values.size() == 1) {
            returnValue = values[0];
        } else {
            returnValue = Value(values);
        }
    }

    throw ReturnException(returnValue);
}

void EvalVisitor::executeIfStmt(Python3Parser::If_stmtContext* ctx) {
    bool executed = false;

    if (evaluateExpression(ctx->test(0)).asBool()) {
        executeSuite(ctx->suite(0));
        executed = true;
    } else {
        for (size_t i = 1; i < ctx->ELIF().size(); i++) {
            if (evaluateExpression(ctx->test(i)).asBool()) {
                executeSuite(ctx->suite(i));
                executed = true;
                break;
            }
        }
    }

    if (!executed && ctx->ELSE()) {
        executeSuite(ctx->suite(ctx->suite().size() - 1));
    }
}

void EvalVisitor::executeWhileStmt(Python3Parser::While_stmtContext* ctx) {
    bool wasInLoop = inLoop;
    inLoop = true;

    try {
        while (evaluateExpression(ctx->test()).asBool()) {
            try {
                executeSuite(ctx->suite());
            } catch (ContinueException&) {
                continue;
            }
        }
    } catch (BreakException&) {
    }

    inLoop = wasInLoop;
}

Value EvalVisitor::callBuiltInFunction(const std::string& name, const std::vector<Value>& args) {
    if (name == "print") {
        try {
            bool first = true;
            for (const auto& arg : args) {
                if (!first) std::cout << " ";
                std::cout << arg.asString();
                first = false;
            }
            std::cout << std::endl;
            std::cout.flush();
        } catch (const std::exception& e) {
            std::cerr << "Error in print: " << e.what() << std::endl;
            throw;
        }
        return Value();
    } else if (name == "int") {
        if (args.size() != 1) {
            throw std::runtime_error("int() takes exactly one argument");
        }
        return Value(args[0].asInt());
    } else if (name == "float") {
        if (args.size() != 1) {
            throw std::runtime_error("float() takes exactly one argument");
        }
        return Value(args[0].asFloat());
    } else if (name == "str") {
        if (args.size() != 1) {
            throw std::runtime_error("str() takes exactly one argument");
        }
        return Value(args[0].asString());
    } else if (name == "bool") {
        if (args.size() != 1) {
            throw std::runtime_error("bool() takes exactly one argument");
        }
        return Value(args[0].asBool());
    }

    return Value();
}

Value EvalVisitor::callUserFunction(std::shared_ptr<Function> func, const std::vector<Value>& args) {
    auto funcEnv = std::make_shared<Environment>(currentEnvironment, true);
    funcEnv->copyGlobalsFrom(globalEnvironment);

    size_t paramCount = func->parameters.size();
    size_t argCount = args.size();

    if (argCount > paramCount) {
        throw std::runtime_error("Too many arguments");
    }

    for (size_t i = 0; i < argCount; i++) {
        funcEnv->setVariable(func->parameters[i], args[i]);
    }

    auto oldEnv = currentEnvironment;
    bool oldInFunction = inFunction;
    currentEnvironment = funcEnv;
    inFunction = true;

    Value returnValue;
    try {
        auto ctx = static_cast<Python3Parser::FuncdefContext*>(func->node);
        executeSuite(ctx->suite());
    } catch (ReturnException& e) {
        returnValue = e.getValue();
    }

    currentEnvironment = oldEnv;
    inFunction = oldInFunction;

    return returnValue;
}