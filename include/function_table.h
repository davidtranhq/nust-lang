#pragma once

#include "parser/parser.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace nust {

struct FunctionInfo {
    size_t entry_point;      // Instruction pointer where function starts
    size_t num_params;       // Number of parameters
    size_t num_locals;       // Number of local variables
    std::unique_ptr<Type> return_type;  // Function's return type
    std::vector<std::unique_ptr<Type>> param_types;  // Types of parameters
    std::string name;        // Function name for debugging
};

class FunctionTable {
public:
    FunctionTable();
    
    // Add a function to the table
    size_t add_function(const FunctionDecl& func, size_t entry_point);
    
    // Get function info by index
    const FunctionInfo& get_function(size_t index) const;
    
    // Get function index by name
    size_t get_function_index(const std::string& name) const;
    
    // Get total number of functions
    size_t size() const;
    
private:
    std::vector<FunctionInfo> functions;
    std::unordered_map<std::string, size_t> name_to_index;
};

} // namespace nust 