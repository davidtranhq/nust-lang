#ifndef NUST_VALUE_H
#define NUST_VALUE_H

#include <variant>
#include <string>
#include <memory>

namespace nust {

class Value {
public:
    // Supported value types
    using IntType = int32_t;
    using BoolType = bool;
    using StringType = std::string;
    using RefType = std::shared_ptr<Value>;

    // Variant to hold any of our supported types
    using ValueType = std::variant<IntType, BoolType, StringType, RefType>;

    // Default constructor - initializes to integer 0
    Value() : data_(IntType(0)) {}

    // Constructors for each type
    Value(IntType value) : data_(value) {}
    Value(BoolType value) : data_(value) {}
    Value(StringType value) : data_(value) {}
    Value(RefType value) : data_(value) {}

    // Type checking
    bool is_int() const { return std::holds_alternative<IntType>(data_); }
    bool is_bool() const { return std::holds_alternative<BoolType>(data_); }
    bool is_string() const { return std::holds_alternative<StringType>(data_); }
    bool is_ref() const { return std::holds_alternative<RefType>(data_); }

    // Value getters with type checking
    IntType as_int() const { return std::get<IntType>(data_); }
    BoolType as_bool() const { return std::get<BoolType>(data_); }
    StringType as_string() const { return std::get<StringType>(data_); }
    RefType as_ref() const { return std::get<RefType>(data_); }

private:
    ValueType data_;
};

} // namespace nust

#endif // NUST_VALUE_H 