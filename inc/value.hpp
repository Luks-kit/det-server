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

    std::variant<int, 
                std::string, 
                bool,
                std::vector<Value>,
                std::map<std::string, Value>,
                std::monostate> data;

    Value() : data(false) {}
    Value(int v) : data(v) {}
    Value(std::string v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(std::vector<Value> v): data(v) {}
    Value(std::map<std::string, Value> v): data (v) {}

    bool isInt() const { return std::holds_alternative<int>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }


    int asInt() const { return std::get<int>(data); }
    int toInt() const {
        if (auto* i = std::get_if<int>(&data)) return *i;
        if (auto* s = std::get_if<std::string>(&data)) {
            try { return std::stoi(*s); } catch (...) { return 0; }
        }
        return 0;
    }

    std::string asString() const {
        if (auto* s = std::get_if<std::string>(&data)) return *s;
        if (auto* i = std::get_if<int>(&data)) return std::to_string(*i);
        if (auto* b = std::get_if<bool>(&data)) return *b ? "true" : "false";
        if (isList()) return "[List]";
        if (isObject()) return "[Object]";
        return "";
    }

    bool isList() const { return std::holds_alternative<std::vector<Value>>(data);}
    const std::vector<Value>& asList() const { return std::get<std::vector<Value>>(data);}

    bool isObject() const { return std::holds_alternative<std::map<std::string, Value>>(data);}
    const std::map<std::string, Value>& asObject() const { return std::get<std::map<std::string, Value>>(data);}

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
        if (isList()) return !asList().empty();
        if (isObject()) return !asObject().empty();
        if (std::holds_alternative<std::monostate>(data)) return false;
        return false;
    }
};