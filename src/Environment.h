#pragma once
#ifndef PYTHON_INTERPRETER_ENVIRONMENT_H
#define PYTHON_INTERPRETER_ENVIRONMENT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Value.h"

class Function {
public:
    std::vector<std::string> parameters;
    std::vector<std::string> defaults;
    std::vector<Value> defaultValues;
    std::string name;
    void* node;

    Function(const std::string& name, void* node) : name(name), node(node) {}
};

class Environment {
private:
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, std::shared_ptr<Function>> functions;
    std::shared_ptr<Environment> parent;
    bool isFunctionScope;

public:
    Environment(bool isFunctionScope = false) : isFunctionScope(isFunctionScope) {}

    Environment(std::shared_ptr<Environment> parent, bool isFunctionScope = true)
        : parent(parent), isFunctionScope(isFunctionScope) {}

    void setVariable(const std::string& name, const Value& value) {
        variables[name] = value;
    }

    Value getVariable(const std::string& name) {
        if (variables.find(name) != variables.end()) {
            return variables[name];
        }

        if (parent) {
            return parent->getVariable(name);
        }

        return Value();
    }

    bool hasVariable(const std::string& name) {
        if (variables.find(name) != variables.end()) {
            return true;
        }

        if (parent) {
            return parent->hasVariable(name);
        }

        return false;
    }

    void setFunction(const std::string& name, std::shared_ptr<Function> func) {
        functions[name] = func;
    }

    std::shared_ptr<Function> getFunction(const std::string& name) {
        if (functions.find(name) != functions.end()) {
            return functions[name];
        }

        if (parent) {
            return parent->getFunction(name);
        }

        return nullptr;
    }

    bool hasFunction(const std::string& name) {
        if (functions.find(name) != functions.end()) {
            return true;
        }

        if (parent) {
            return parent->hasFunction(name);
        }

        return false;
    }

    std::shared_ptr<Environment> getParent() {
        return parent;
    }

    bool isFunctionEnvironment() const {
        return isFunctionScope;
    }

    void clearLocalVariables() {
        variables.clear();
    }

    void copyGlobalsFrom(std::shared_ptr<Environment> source) {
        if (!source) return;

        for (const auto& pair : source->variables) {
            if (!hasVariable(pair.first)) {
                setVariable(pair.first, pair.second);
            }
        }

        for (const auto& pair : source->functions) {
            if (!hasFunction(pair.first)) {
                setFunction(pair.first, pair.second);
            }
        }
    }
};

#endif