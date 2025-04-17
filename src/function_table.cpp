#include "function_table.h"
#include <stdexcept>

namespace nust {

FunctionTable::FunctionTable() {}

size_t FunctionTable::add_function(const FunctionDecl& func, size_t entry_point) {
    FunctionInfo info;
    info.entry_point = entry_point;
    info.num_params = func.params.size();
    info.num_locals = 0; // Will be updated during compilation
    info.return_type = func.return_type->clone();
    info.name = func.name;
    
    // Copy parameter types
    for (const auto& param : func.params) {
        info.param_types.push_back(param.type->clone());
    }
    
    size_t index = functions.size();
    functions.push_back(std::move(info));
    name_to_index[func.name] = index;
    return index;
}

const FunctionInfo& FunctionTable::get_function(size_t index) const {
    if (index >= functions.size()) {
        throw std::runtime_error("Invalid function index");
    }
    return functions[index];
}

size_t FunctionTable::get_function_index(const std::string& name) const {
    auto it = name_to_index.find(name);
    if (it == name_to_index.end()) {
        throw std::runtime_error("Function not found: " + name);
    }
    return it->second;
}

size_t FunctionTable::size() const {
    return functions.size();
}

} // namespace nust 