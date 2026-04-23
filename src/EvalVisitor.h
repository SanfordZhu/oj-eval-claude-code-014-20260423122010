#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include "Value.h"
#include "Environment.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <cmath>

class ReturnException : public std::exception {
private:
    Value value;
public:
    ReturnException(const Value& val) : value(val) {}
    Value getValue() const { return value; }
};

class BreakException : public std::exception {};
class ContinueException : public std::exception {};

class EvalVisitor : public Python3ParserBaseVisitor {
private:
    std::shared_ptr<Environment> globalEnvironment;
    std::shared_ptr<Environment> currentEnvironment;
    bool inLoop;
    bool inFunction;

    Value evaluateExpression(Python3Parser::TestContext* ctx);
    Value evaluateArithExpr(Python3Parser::Arith_exprContext* ctx);
    Value evaluateTerm(Python3Parser::TermContext* ctx);
    Value evaluateFactor(Python3Parser::FactorContext* ctx);
    Value evaluateAtomExpr(Python3Parser::Atom_exprContext* ctx);
    Value evaluateAtom(Python3Parser::AtomContext* ctx);
    Value evaluateComparison(Python3Parser::ComparisonContext* ctx);
    Value evaluateOrTest(Python3Parser::Or_testContext* ctx);
    Value evaluateAndTest(Python3Parser::And_testContext* ctx);
    Value evaluateNotTest(Python3Parser::Not_testContext* ctx);

    std::string evaluateFormatString(Python3Parser::Format_stringContext* ctx);
    std::vector<Value> evaluateTestList(Python3Parser::TestlistContext* ctx);
    std::vector<Value> evaluateArgList(Python3Parser::ArglistContext* ctx);

    void executeSuite(Python3Parser::SuiteContext* ctx);
    void executeStmt(Python3Parser::StmtContext* ctx);
    void executeSimpleStmt(Python3Parser::Simple_stmtContext* ctx);
    void executeExprStmt(Python3Parser::Expr_stmtContext* ctx);
    void executeFlowStmt(Python3Parser::Flow_stmtContext* ctx);
    void executeReturnStmt(Python3Parser::Return_stmtContext* ctx);
    void executeBreakStmt(Python3Parser::Break_stmtContext* ctx);
    void executeContinueStmt(Python3Parser::Continue_stmtContext* ctx);
    void executeIfStmt(Python3Parser::If_stmtContext* ctx);
    void executeWhileStmt(Python3Parser::While_stmtContext* ctx);

    Value callBuiltInFunction(const std::string& name, const std::vector<Value>& args);
    Value callUserFunction(std::shared_ptr<Function> func, const std::vector<Value>& args);

public:
    EvalVisitor();

    virtual std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    virtual std::any visitFuncdef(Python3Parser::FuncdefContext *ctx) override;
    virtual std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    virtual std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    virtual std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    virtual std::any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override;
    virtual std::any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override;
    virtual std::any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override;
    virtual std::any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override;
    virtual std::any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override;
    virtual std::any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override;
    virtual std::any visitSuite(Python3Parser::SuiteContext *ctx) override;
    virtual std::any visitTest(Python3Parser::TestContext *ctx) override;
    virtual std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    virtual std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    virtual std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    virtual std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    virtual std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    virtual std::any visitTerm(Python3Parser::TermContext *ctx) override;
    virtual std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    virtual std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    virtual std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    virtual std::any visitFormat_string(Python3Parser::Format_stringContext *ctx) override;
    virtual std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;
    virtual std::any visitArglist(Python3Parser::ArglistContext *ctx) override;
    virtual std::any visitArgument(Python3Parser::ArgumentContext *ctx) override;
};

#endif