#pragma once

#include "parser/parser.h"
#include "instruction.h"
#include "function_table.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace nust {

class Compiler {
public:
    Compiler();
    
    // Compile a program AST to bytecode
    std::vector<Instruction> compile(const Program& program);
    
    // Get the function table after compilation
    const FunctionTable& get_function_table() const { return function_table; }
    
private:
    // Function compilation
    void compile_function(const FunctionDecl* func);
    void compile_params(const std::vector<FunctionDecl::Param>& params);
    void compile_statement(const Stmt* stmt);
    void compile_expression(const Expr* expr);
    
    // Control flow
    void compile_if(const IfStmt* stmt);
    void compile_while(const WhileStmt* stmt);
    void compile_block(const BlockStmt* block);
    
    // Variable management
    void compile_let(const LetStmt* stmt);
    void compile_identifier(const Identifier* ident);
    
    // Expression compilation
    void compile_binary(const BinaryExpr* expr);
    void compile_unary(const UnaryExpr* expr);
    void compile_call(const CallExpr* expr);
    void compile_borrow(const BorrowExpr* expr);
    
    // Helper functions
    void emit(Instruction instr);
    size_t emit_instruction(Opcode opcode, size_t operand = 0);
    size_t add_constant(const std::string& str);
    size_t get_local_index(const std::string& name);
    
    // State
    std::vector<Instruction> instructions;
    std::vector<std::string> string_constants;
    std::unordered_map<std::string, size_t> local_vars;
    size_t next_local_index;
    FunctionTable function_table;
};

} // namespace nust 