#pragma once

#include "parser/parser.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include <optional>

namespace nust {

class TypeChecker {
public:
    TypeChecker() = default;
    
    // Main entry point for type checking
    bool check_program(const Program& program);
    
    // Error reporting
    void error(const std::string& message, const Span& span);
    bool has_errors() const { return !errors_.empty(); }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    // Type checking methods for different AST nodes
    bool check_function(const FunctionDecl& func);
    bool check_statement(const Stmt& stmt);
    bool check_expression(const Expr& expr);
    bool check_type(const Type& type);
    
    // Helper methods for type checking
    bool is_assignable(const Type& target, const Type& source);
    bool is_compatible(const Type& lhs, const Type& rhs);
    bool is_mutable_reference(const Type& type) const;
    bool is_reference(const Type& type) const;
    
    // Scope management
    struct VariableInfo {
        std::unique_ptr<Type> type;
        bool is_mut;
    };
    
    std::vector<std::unordered_map<std::string, VariableInfo>> scopes_;
    void enter_scope();
    void exit_scope();
    bool declare_variable(const std::string& name, std::unique_ptr<Type> type, bool is_mut);
    std::optional<VariableInfo> lookup_variable(const std::string& name);
    
    // Error tracking
    std::vector<std::string> errors_;
    const Program* program_ = nullptr;
};

} // namespace nust 