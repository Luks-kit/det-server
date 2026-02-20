#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>

template<class... Ts> 
struct Overload : Ts... {
     using Ts::operator()...; 
};

template<class... Ts> Overload(Ts...) -> Overload<Ts...>;


struct DataValue {
    std::variant<std::string, 
                 std::vector<DataValue>, 
                 std::map<std::string, DataValue>> data;

    // Helpers to create/check types
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isList()   const { return std::holds_alternative<std::vector<DataValue>>(data); }
    bool isDict()   const { return std::holds_alternative<std::map<std::string, DataValue>>(data); }

    std::string toString() const {
        if (isString()) return std::get<std::string>(data);
        return "[Complex Object]"; 
    }
};



class Value {
public:
    // Holds either an int, a string, or a bool
    std::variant<int, std::string, bool> data;

    Value() : data(false) {}
    Value(int v) : data(v) {}
    Value(std::string v) : data(v) {}
    Value(bool v) : data(v) {}

    bool isInt() const { return std::holds_alternative<int>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }

    int asInt() const { return std::get<int>(data); }
    std::string asString() const { 
        if (isString()) return std::get<std::string>(data);
        if (isInt()) return std::to_string(asInt());
        return std::get<bool>(data) ? "true" : "false";
    }

    // Example of a "reduce" helper for math
    Value operator+(const Value& other) const {
        if (this->isInt() && other.isInt()) 
            return Value(this->asInt() + other.asInt());
        // String concatenation fallback
        return Value(this->asString() + other.asString());
    }
    Value operator-(const Value& other) const {
        if (isInt() && other.isInt()) return Value(asInt() - other.asInt());
        throw std::runtime_error("Invalid types for subtraction");
    }

    Value operator*(const Value& other) const {
        if (isInt() && other.isInt()) return Value(asInt() * other.asInt());
        return Value(); 
    }

    Value operator/(const Value& other) const {
        if(isInt() && other.isInt()) return Value(asInt() / other.asInt());
        return Value();
    }

    // Comparisons
    bool operator==(const Value& other) const { return data == other.data; }
    bool operator!=(const Value& other) const { return data != other.data; }
    
    bool operator<(const Value& other) const {
        if (isInt() && other.isInt()) return asInt() < other.asInt();
        if (isString() && other.isString()) return asString() < other.asString();
        return false;
    }

    bool operator>(const Value& other) const { return other < *this; }

    // Truthiness helper for If/For logic
    bool isTruthy() const {
        if (std::holds_alternative<bool>(data)) return std::get<bool>(data);
        if (isInt()) return asInt() != 0;
        if (isString()) return !std::get<std::string>(data).empty();
        return false;
    }
};