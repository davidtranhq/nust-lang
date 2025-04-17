#include "compiler.h"
#include "parser/parser.h"
#include <stdexcept>
#include <iostream>

namespace nust {

Compiler::Compiler() : next_local_index(0) {}

std::vector<Instruction> Compiler::compile(const Program& program) {
    // Reset state
    instructions.clear();
    string_constants.clear();
    local_vars.clear();
    next_local_index = 0;
    function_table = FunctionTable();
    
    // First pass: add all functions to the function table
    for (const auto& item : program.items) {
        if (auto func = dynamic_cast<const FunctionDecl*>(item.get())) {
            function_table.add_function(*func, 0); // Placeholder entry point
        }
    }
    
    // Second pass: compile functions
    for (const auto& item : program.items) {
        if (auto func = dynamic_cast<const FunctionDecl*>(item.get())) {
            size_t entry_point = instructions.size();
            compile_function(func);
            
            // Update function entry point in the table
            const_cast<FunctionInfo&>(function_table.get_function(
                function_table.get_function_index(func->name)
            )).entry_point = entry_point;
        }
    }
    
    return instructions;
}

void Compiler::compile_function(const FunctionDecl* func) {
    // Reset local variables for new function
    local_vars.clear();
    next_local_index = 0;
    
    // Add parameters to local variables
    for (const auto& param : func->params) {
        local_vars[param.name] = next_local_index++;
    }
    
    // Compile function body
    compile_statement(func->body.get());
    
    // If function has no explicit return, add one
    if (instructions.empty() || instructions.back().opcode != Opcode::RET_VAL) {
        emit(Instruction{Opcode::RET});
    }
    
    // Update number of locals in function table
    const_cast<FunctionInfo&>(function_table.get_function(
        function_table.get_function_index(func->name)
    )).num_locals = next_local_index;
}

void Compiler::compile_statement(const Stmt* stmt) {
    if (auto let = dynamic_cast<const LetStmt*>(stmt)) {
        compile_let(let);
    } else if (auto if_stmt = dynamic_cast<const IfStmt*>(stmt)) {
        compile_if(if_stmt);
    } else if (auto while_stmt = dynamic_cast<const WhileStmt*>(stmt)) {
        compile_while(while_stmt);
    } else if (auto block = dynamic_cast<const BlockStmt*>(stmt)) {
        compile_block(block);
    } else if (auto expr = dynamic_cast<const ExprStmt*>(stmt)) {
        compile_expression(expr->expr.get());
        // Pop the result if it's not used
        emit(Instruction{Opcode::POP});
    }
}

void Compiler::compile_let(const LetStmt* stmt) {
    // Compile initializer expression
    compile_expression(stmt->init.get());
    
    // Add variable to local variables if not already present
    if (local_vars.find(stmt->name) == local_vars.end()) {
        local_vars[stmt->name] = next_local_index++;
    }
    
    // Store in local variable
    size_t index = get_local_index(stmt->name);
    emit(Instruction{Opcode::STORE, index});
}

void Compiler::compile_expression(const Expr* expr) {
    if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
        
        // Handle assignment
        if (binary->op == BinaryExpr::Op::Assignment) {
            // Compile the right-hand side first
            compile_expression(binary->right.get());
            
            // Get the target variable
            auto* target = dynamic_cast<const Identifier*>(binary->left.get());
            if (!target) {
                throw std::runtime_error("Assignment target must be an identifier");
            }
            
            // Store in the target variable
            size_t index = get_local_index(target->name);
            emit(Instruction{Opcode::STORE, index});
            
            // Load the value back for use in expressions
            emit(Instruction{Opcode::LOAD, index});
            return;
        }
        
        compile_expression(binary->left.get());
        compile_expression(binary->right.get());
        
        switch (binary->op) {
            case BinaryExpr::Op::Add:
                emit(Instruction{Opcode::ADD_I32});
                break;
            case BinaryExpr::Op::Sub:
                emit(Instruction{Opcode::SUB_I32});
                break;
            case BinaryExpr::Op::Mul:
                emit(Instruction{Opcode::MUL_I32});
                break;
            case BinaryExpr::Op::Div:
                emit(Instruction{Opcode::DIV_I32});
                break;
            case BinaryExpr::Op::Eq:
                emit(Instruction{Opcode::EQ_I32});
                break;
            case BinaryExpr::Op::Ne:
                emit(Instruction{Opcode::NE_I32});
                break;
            case BinaryExpr::Op::Lt:
                emit(Instruction{Opcode::LT_I32});
                break;
            case BinaryExpr::Op::Gt:
                emit(Instruction{Opcode::GT_I32});
                break;
            case BinaryExpr::Op::Le:
                emit(Instruction{Opcode::LE_I32});
                break;
            case BinaryExpr::Op::Ge:
                emit(Instruction{Opcode::GE_I32});
                break;
            case BinaryExpr::Op::And:
                emit(Instruction{Opcode::AND});
                break;
            case BinaryExpr::Op::Or:
                emit(Instruction{Opcode::OR});
                break;
            default:
                throw std::runtime_error("Unknown binary operator");
        }
    } else if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        compile_expression(unary->expr.get());
        switch (unary->op) {
            case UnaryExpr::Op::Neg:
                emit(Instruction{Opcode::NEG_I32});
                break;
            case UnaryExpr::Op::Not:
                emit(Instruction{Opcode::NOT});
                break;
        }
    } else if (auto int_lit = dynamic_cast<const IntLiteral*>(expr)) {
        emit(Instruction{Opcode::PUSH_I32, static_cast<size_t>(int_lit->value)});
    } else if (auto bool_lit = dynamic_cast<const BoolLiteral*>(expr)) {
        emit(Instruction{Opcode::PUSH_BOOL, static_cast<size_t>(bool_lit->value)});
    } else if (auto str_lit = dynamic_cast<const StringLiteral*>(expr)) {
        size_t index = string_constants.size();
        string_constants.push_back(str_lit->value);
        emit(Instruction{Opcode::PUSH_STR, index});
    } else if (auto ident = dynamic_cast<const Identifier*>(expr)) {
        compile_identifier(ident);
    } else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        compile_call(call);
    } else if (auto borrow = dynamic_cast<const BorrowExpr*>(expr)) {
        compile_borrow(borrow);
    }
}

void Compiler::compile_identifier(const Identifier* ident) {
    size_t index = get_local_index(ident->name);
    emit(Instruction{Opcode::LOAD, index});
}

void Compiler::compile_call(const CallExpr* expr) {
    // Compile arguments in reverse order
    for (auto it = expr->args.rbegin(); it != expr->args.rend(); ++it) {
        compile_expression((*it).get());
    }
    
    // Get function index from the function table
    auto* callee = dynamic_cast<const Identifier*>(expr->callee.get());
    if (!callee) {
        throw std::runtime_error("Function callee must be an identifier");
    }
    size_t func_index = function_table.get_function_index(callee->name);
    
    // Call function
    emit(Instruction{Opcode::CALL, func_index});
}

void Compiler::compile_borrow(const BorrowExpr* expr) {
    compile_expression(expr->expr.get());
    
    if (expr->is_mut) {
        emit(Instruction{Opcode::BORROW_MUT});
    } else {
        emit(Instruction{Opcode::BORROW});
    }
}

void Compiler::compile_if(const IfStmt* if_stmt) {
    // Compile condition
    compile_expression(if_stmt->condition.get());
    
    // Emit conditional jump
    size_t else_jump = emit_instruction(Opcode::JMP_IF_NOT, 0);
    
    // Compile then branch
    compile_statement(if_stmt->then_branch.get());
    
    // If there's an else branch, emit jump to skip it
    size_t end_jump = 0;
    if (if_stmt->else_branch) {
        end_jump = emit_instruction(Opcode::JMP, 0);
    }
    
    // Update else jump offset
    instructions[else_jump].operand = instructions.size();
    
    // Compile else branch if present
    if (if_stmt->else_branch) {
        compile_statement(if_stmt->else_branch.get());
        // Update end jump offset
        instructions[end_jump].operand = instructions.size();
    }
}

void Compiler::compile_while(const WhileStmt* while_stmt) {
    // Save loop start position
    size_t loop_start = instructions.size();
    
    // Compile condition
    compile_expression(while_stmt->condition.get());
    
    // Emit conditional jump
    size_t exit_jump = emit_instruction(Opcode::JMP_IF_NOT, 0);
    
    
    // Compile body
    compile_statement(while_stmt->body.get());
    
    // Emit jump back to condition
    emit_instruction(Opcode::JMP, loop_start);
    
    // Update exit jump offset
    instructions[exit_jump].operand = instructions.size();
}

void Compiler::compile_block(const BlockStmt* block) {
    for (const auto& stmt : block->statements) {
        compile_statement(stmt.get());
    }
}

void Compiler::emit(Instruction instr) {
    instructions.push_back(instr);
}

size_t Compiler::emit_instruction(Opcode opcode, size_t operand) {
    size_t index = instructions.size();
    instructions.push_back(Instruction{opcode, operand});
    return index;
}

size_t Compiler::add_constant(const std::string& str) {
    string_constants.push_back(str);
    return string_constants.size() - 1;
}

size_t Compiler::get_local_index(const std::string& name) {
    auto it = local_vars.find(name);
    if (it == local_vars.end()) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    return it->second;
}

} // namespace nust 