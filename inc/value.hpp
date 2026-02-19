#include <string>
#include <vector>
#include <map>
#include <variant>

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