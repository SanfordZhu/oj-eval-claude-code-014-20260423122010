#pragma once
#ifndef PYTHON_INTERPRETER_VALUE_H
#define PYTHON_INTERPRETER_VALUE_H

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "BigInt.h"

enum class ValueType {
    NONE,
    BOOL,
    INT,
    FLOAT,
    STR,
    TUPLE
};

class Value {
private:
    std::variant<
        std::monostate,
        bool,
        BigInt,
        double,
        std::string,
        std::vector<Value>
    > data;

    ValueType type;

public:
    Value() : type(ValueType::NONE) {}

    Value(bool b) : data(b), type(ValueType::BOOL) {}

    Value(int i) : data(BigInt(i)), type(ValueType::INT) {}

    Value(long long ll) : data(BigInt(ll)), type(ValueType::INT) {}

    Value(const BigInt& bi) : data(bi), type(ValueType::INT) {}

    Value(double d) : data(d), type(ValueType::FLOAT) {}

    Value(const std::string& s) : data(s), type(ValueType::STR) {}

    Value(const char* s) : data(std::string(s)), type(ValueType::STR) {}

    Value(const std::vector<Value>& v) : data(v), type(ValueType::TUPLE) {}

    ValueType getType() const { return type; }

    bool isNone() const { return type == ValueType::NONE; }
    bool isBool() const { return type == ValueType::BOOL; }
    bool isInt() const { return type == ValueType::INT; }
    bool isFloat() const { return type == ValueType::FLOAT; }
    bool isStr() const { return type == ValueType::STR; }
    bool isTuple() const { return type == ValueType::TUPLE; }

    bool asBool() const {
        switch (type) {
            case ValueType::NONE:
                return false;
            case ValueType::BOOL:
                return std::get<bool>(data);
            case ValueType::INT:
                return !(std::get<BigInt>(data) == BigInt(0));
            case ValueType::FLOAT:
                return std::get<double>(data) != 0.0;
            case ValueType::STR:
                return !std::get<std::string>(data).empty();
            case ValueType::TUPLE:
                return !std::get<std::vector<Value>>(data).empty();
        }
        return false;
    }

    BigInt asInt() const {
        switch (type) {
            case ValueType::BOOL:
                return BigInt(std::get<bool>(data) ? 1 : 0);
            case ValueType::INT:
                return std::get<BigInt>(data);
            case ValueType::FLOAT:
                return BigInt(static_cast<long long>(std::get<double>(data)));
            case ValueType::STR:
                try {
                    return BigInt(std::get<std::string>(data));
                } catch (...) {
                    return BigInt(0);
                }
            default:
                return BigInt(0);
        }
    }

    double asFloat() const {
        switch (type) {
            case ValueType::BOOL:
                return std::get<bool>(data) ? 1.0 : 0.0;
            case ValueType::INT:
                return std::get<BigInt>(data).toDouble();
            case ValueType::FLOAT:
                return std::get<double>(data);
            case ValueType::STR:
                try {
                    return std::stod(std::get<std::string>(data));
                } catch (...) {
                    return 0.0;
                }
            default:
                return 0.0;
        }
    }

    std::string asString() const {
        switch (type) {
            case ValueType::NONE:
                return "None";
            case ValueType::BOOL:
                return std::get<bool>(data) ? "True" : "False";
            case ValueType::INT:
                return std::get<BigInt>(data).toString();
            case ValueType::FLOAT: {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << std::get<double>(data);
                std::string result = oss.str();
                size_t dot_pos = result.find('.');
                if (dot_pos != std::string::npos) {
                    size_t last_non_zero = result.find_last_not_of('0');
                    if (last_non_zero != std::string::npos && last_non_zero > dot_pos) {
                        result = result.substr(0, last_non_zero + 1);
                    }
                    if (result.back() == '.') {
                        result += "000000";
                    } else if (result.find('.') != std::string::npos) {
                        size_t decimal_places = result.length() - result.find('.') - 1;
                        if (decimal_places < 6) {
                            result.append(6 - decimal_places, '0');
                        }
                    }
                }
                return result;
            }
            case ValueType::STR:
                return std::get<std::string>(data);
            case ValueType::TUPLE: {
                const auto& tuple = std::get<std::vector<Value>>(data);
                std::string result = "(";
                for (size_t i = 0; i < tuple.size(); i++) {
                    if (i > 0) result += ", ";
                    result += tuple[i].asString();
                }
                if (tuple.size() == 1) result += ",";
                result += ")";
                return result;
            }
        }
        return "";
    }

    std::vector<Value> asTuple() const {
        if (type == ValueType::TUPLE) {
            return std::get<std::vector<Value>>(data);
        }
        return {*this};
    }

    Value operator+(const Value& other) const {
        if (isStr() && other.isStr()) {
            return Value(asString() + other.asString());
        }
        if (isStr() && other.isInt()) {
            std::string result;
            int times = other.asInt().toInt();
            for (int i = 0; i < times; i++) {
                result += asString();
            }
            return Value(result);
        }
        if (isInt() && other.isStr()) {
            return other + *this;
        }
        if (isInt() && other.isInt()) {
            return Value(asInt() + other.asInt());
        }
        if (isFloat() || other.isFloat()) {
            return Value(asFloat() + other.asFloat());
        }
        if (isInt() && other.isFloat()) {
            return Value(asInt().toDouble() + other.asFloat());
        }
        if (isFloat() && other.isInt()) {
            return Value(asFloat() + other.asInt().toDouble());
        }
        return Value();
    }

    Value operator-(const Value& other) const {
        if (isInt() && other.isInt()) {
            return Value(asInt() - other.asInt());
        }
        if (isFloat() || other.isFloat()) {
            return Value(asFloat() - other.asFloat());
        }
        if (isInt() && other.isFloat()) {
            return Value(asInt().toDouble() - other.asFloat());
        }
        if (isFloat() && other.isInt()) {
            return Value(asFloat() - other.asInt().toDouble());
        }
        return Value();
    }

    Value operator*(const Value& other) const {
        if (isStr() && other.isInt()) {
            std::string result;
            int times = other.asInt().toInt();
            for (int i = 0; i < times; i++) {
                result += asString();
            }
            return Value(result);
        }
        if (isInt() && other.isStr()) {
            return other * *this;
        }
        if (isInt() && other.isInt()) {
            return Value(asInt() * other.asInt());
        }
        if (isFloat() || other.isFloat()) {
            return Value(asFloat() * other.asFloat());
        }
        if (isInt() && other.isFloat()) {
            return Value(asInt().toDouble() * other.asFloat());
        }
        if (isFloat() && other.isInt()) {
            return Value(asFloat() * other.asInt().toDouble());
        }
        return Value();
    }

    Value operator/(const Value& other) const {
        if (other.asFloat() == 0.0) {
            throw std::runtime_error("Division by zero");
        }
        return Value(asFloat() / other.asFloat());
    }

    Value floorDiv(const Value& other) const {
        if (isInt() && other.isInt()) {
            BigInt dividend = asInt();
            BigInt divisor = other.asInt();

            if (divisor == BigInt(0)) {
                throw std::runtime_error("Division by zero");
            }

            BigInt quotient = dividend / divisor;

            if ((dividend < BigInt(0)) != (divisor < BigInt(0)) && (dividend % divisor != BigInt(0))) {
                quotient = quotient - BigInt(1);
            }

            return Value(quotient);
        }
        return Value(BigInt(static_cast<long long>(std::floor(asFloat() / other.asFloat()))));
    }

    Value operator%(const Value& other) const {
        if (isInt() && other.isInt()) {
            BigInt a = asInt();
            BigInt b = other.asInt();

            if (b == BigInt(0)) {
                throw std::runtime_error("Modulo by zero");
            }

            return Value(a - (a / b) * b);
        }
        return Value(std::fmod(asFloat(), other.asFloat()));
    }

    bool operator==(const Value& other) const {
        if (type != other.type) {
            if ((isInt() || isFloat()) && (other.isInt() || other.isFloat())) {
                return asFloat() == other.asFloat();
            }
            if (isInt() && other.isBool()) {
                return asInt() == BigInt(other.asBool() ? 1 : 0);
            }
            if (isBool() && other.isInt()) {
                return BigInt(asBool() ? 1 : 0) == other.asInt();
            }
            return false;
        }

        switch (type) {
            case ValueType::NONE:
                return true;
            case ValueType::BOOL:
                return std::get<bool>(data) == std::get<bool>(other.data);
            case ValueType::INT:
                return std::get<BigInt>(data) == std::get<BigInt>(other.data);
            case ValueType::FLOAT:
                return std::get<double>(data) == std::get<double>(other.data);
            case ValueType::STR:
                return std::get<std::string>(data) == std::get<std::string>(other.data);
            case ValueType::TUPLE:
                return std::get<std::vector<Value>>(data) == std::get<std::vector<Value>>(other.data);
        }
        return false;
    }

    bool operator<(const Value& other) const {
        if (type != other.type) {
            if ((isInt() || isFloat()) && (other.isInt() || other.isFloat())) {
                return asFloat() < other.asFloat();
            }
            return false;
        }

        switch (type) {
            case ValueType::INT:
                return std::get<BigInt>(data) < std::get<BigInt>(other.data);
            case ValueType::FLOAT:
                return std::get<double>(data) < std::get<double>(other.data);
            case ValueType::STR:
                return std::get<std::string>(data) < std::get<std::string>(other.data);
            default:
                return false;
        }
    }

    bool operator>(const Value& other) const {
        return other < *this;
    }

    bool operator<=(const Value& other) const {
        return !(other < *this);
    }

    bool operator>=(const Value& other) const {
        return !(*this < other);
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    Value operator-() const {
        if (isInt()) {
            return Value(-asInt());
        }
        if (isFloat()) {
            return Value(-asFloat());
        }
        return Value();
    }

    Value operator!() const {
        return Value(!asBool());
    }

    Value operator&&(const Value& other) const {
        return Value(asBool() && other.asBool());
    }

    Value operator||(const Value& other) const {
        return Value(asBool() || other.asBool());
    }
};

#endif