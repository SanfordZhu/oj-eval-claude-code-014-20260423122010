#pragma once
#ifndef PYTHON_INTERPRETER_BIGINT_H
#define PYTHON_INTERPRETER_BIGINT_H

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

class BigInt {
private:
    std::vector<int> digits;
    bool negative;

    void removeLeadingZeros() {
        while (digits.size() > 1 && digits.back() == 0) {
            digits.pop_back();
        }
        if (digits.size() == 1 && digits[0] == 0) {
            negative = false;
        }
    }

    BigInt multiplyByDigit(int digit) const {
        if (digit == 0) return BigInt(0);

        BigInt result;
        result.negative = negative;
        result.digits.resize(digits.size() + 1, 0);

        for (size_t i = 0; i < digits.size(); i++) {
            result.digits[i] += digits[i] * digit;
            if (result.digits[i] >= 10) {
                result.digits[i + 1] += result.digits[i] / 10;
                result.digits[i] %= 10;
            }
        }

        result.removeLeadingZeros();
        return result;
    }

public:
    BigInt() : negative(false), digits(1, 0) {}

    BigInt(long long num) : negative(false) {
        if (num < 0) {
            negative = true;
            num = -num;
        }

        if (num == 0) {
            digits.push_back(0);
        } else {
            while (num > 0) {
                digits.push_back(num % 10);
                num /= 10;
            }
        }
    }

    BigInt(const std::string& str) : negative(false) {
        size_t start = 0;
        if (str[0] == '-') {
            negative = true;
            start = 1;
        }

        for (int i = str.length() - 1; i >= static_cast<int>(start); i--) {
            if (str[i] < '0' || str[i] > '9') {
                throw std::invalid_argument("Invalid character in BigInt string");
            }
            digits.push_back(str[i] - '0');
        }

        removeLeadingZeros();
    }

    BigInt operator+(const BigInt& other) const {
        if (negative != other.negative) {
            if (negative) {
                BigInt temp = *this;
                temp.negative = false;
                return other - temp;
            } else {
                BigInt temp = other;
                temp.negative = false;
                return *this - temp;
            }
        }

        BigInt result;
        result.negative = negative;

        size_t maxSize = std::max(digits.size(), other.digits.size());
        result.digits.resize(maxSize + 1, 0);

        for (size_t i = 0; i < maxSize; i++) {
            int sum = result.digits[i];
            if (i < digits.size()) sum += digits[i];
            if (i < other.digits.size()) sum += other.digits[i];

            if (sum >= 10) {
                result.digits[i + 1] += sum / 10;
                sum %= 10;
            }
            result.digits[i] = sum;
        }

        result.removeLeadingZeros();
        return result;
    }

    BigInt operator-(const BigInt& other) const {
        if (negative != other.negative) {
            BigInt temp = other;
            temp.negative = !temp.negative;
            return *this + temp;
        }

        if (negative) {
            BigInt temp1 = *this;
            BigInt temp2 = other;
            temp1.negative = false;
            temp2.negative = false;
            return temp2 - temp1;
        }

        if (*this < other) {
            BigInt result = other - *this;
            result.negative = true;
            return result;
        }

        BigInt result;
        result.negative = false;
        result.digits.resize(digits.size(), 0);

        for (size_t i = 0; i < digits.size(); i++) {
            int diff = digits[i] - result.digits[i];
            if (i < other.digits.size()) diff -= other.digits[i];

            if (diff < 0) {
                diff += 10;
                if (i + 1 < result.digits.size()) result.digits[i + 1]++;
                else throw std::runtime_error("Subtraction underflow");
            }

            result.digits[i] = diff;
        }

        result.removeLeadingZeros();
        return result;
    }

    BigInt operator*(const BigInt& other) const {
        BigInt result;
        result.digits.resize(digits.size() + other.digits.size(), 0);

        for (size_t i = 0; i < digits.size(); i++) {
            for (size_t j = 0; j < other.digits.size(); j++) {
                result.digits[i + j] += digits[i] * other.digits[j];
                if (result.digits[i + j] >= 10) {
                    result.digits[i + j + 1] += result.digits[i + j] / 10;
                    result.digits[i + j] %= 10;
                }
            }
        }

        result.negative = (negative != other.negative);
        result.removeLeadingZeros();
        return result;
    }

    BigInt operator/(const BigInt& other) const {
        if (other.digits.size() == 1 && other.digits[0] == 0) {
            throw std::runtime_error("Division by zero");
        }

        BigInt dividend = *this;
        BigInt divisor = other;
        dividend.negative = false;
        divisor.negative = false;

        BigInt quotient;
        BigInt remainder;

        for (int i = dividend.digits.size() - 1; i >= 0; i--) {
            remainder.digits.insert(remainder.digits.begin(), dividend.digits[i]);
            remainder.removeLeadingZeros();

            int count = 0;
            while (remainder >= divisor) {
                remainder = remainder - divisor;
                count++;
            }

            if (quotient.digits.size() > 0 || count > 0) {
                quotient.digits.insert(quotient.digits.begin(), count);
            }
        }

        if (quotient.digits.empty()) {
            quotient.digits.push_back(0);
        }

        quotient.negative = (negative != other.negative);
        return quotient;
    }

    BigInt operator%(const BigInt& other) const {
        BigInt quotient = *this / other;
        BigInt product = quotient * other;
        return *this - product;
    }

    bool operator<(const BigInt& other) const {
        if (negative != other.negative) {
            return negative;
        }

        if (digits.size() != other.digits.size()) {
            return negative ? (digits.size() > other.digits.size()) : (digits.size() < other.digits.size());
        }

        for (int i = digits.size() - 1; i >= 0; i--) {
            if (digits[i] != other.digits[i]) {
                return negative ? (digits[i] > other.digits[i]) : (digits[i] < other.digits[i]);
            }
        }

        return false;
    }

    bool operator==(const BigInt& other) const {
        return negative == other.negative && digits == other.digits;
    }

    bool operator!=(const BigInt& other) const {
        return !(*this == other);
    }

    bool operator>(const BigInt& other) const {
        return other < *this;
    }

    bool operator<=(const BigInt& other) const {
        return !(other < *this);
    }

    bool operator>=(const BigInt& other) const {
        return !(*this < other);
    }

    BigInt operator-() const {
        BigInt result = *this;
        if (!(digits.size() == 1 && digits[0] == 0)) {
            result.negative = !negative;
        }
        return result;
    }

    std::string toString() const {
        std::string result;
        if (negative) result += "-";
        for (int i = digits.size() - 1; i >= 0; i--) {
            result += std::to_string(digits[i]);
        }
        return result;
    }

    int toInt() const {
        if (digits.size() > 10) {
            throw std::overflow_error("BigInt too large for int");
        }

        long long result = 0;
        for (int i = digits.size() - 1; i >= 0; i--) {
            result = result * 10 + digits[i];
            if (result > 2147483647) {
                throw std::overflow_error("BigInt too large for int");
            }
        }

        return negative ? -static_cast<int>(result) : static_cast<int>(result);
    }

    double toDouble() const {
        double result = 0;
        double power = 1;
        for (size_t i = 0; i < digits.size(); i++) {
            result += digits[i] * power;
            power *= 10;
        }
        return negative ? -result : result;
    }
};

#endif