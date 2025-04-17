#include "type_checker.h"
#include <sstream>
#include <unordered_map>
#include <iostream>

namespace nust {

bool TypeChecker::check_program(const Program& program) {
    program_ = &program;
    for (const auto& item : program.items) {
        if (auto func = dynamic_cast<const FunctionDecl*>(item.get())) {
            if (!check_function(*func)) {
                return false;
            }
        }
    }
    return !has_errors();
}

bool TypeChecker::check_function(const FunctionDecl& func) {
    enter_scope();
    
    // Add parameters to scope
    for (const auto& param : func.params) {
        auto param_type = std::make_unique<Type>(param.type->kind, param.type->span);
        if (param.type->base_type) {
            param_type->base_type = std::make_unique<Type>(param.type->base_type->kind, param.type->base_type->span);
        }
        if (!declare_variable(param.name, std::move(param_type), param.is_mut)) {
            error("Duplicate parameter name: " + param.name, param.span);
            return false;
        }
    }
    
    // Check function body
    bool success = check_statement(*func.body);
    
    // Check return type
    if (auto block = dynamic_cast<const BlockStmt*>(func.body.get())) {
        if (!block->statements.empty()) {
            if (auto expr_stmt = dynamic_cast<const ExprStmt*>(block->statements.back().get())) {
                if (!check_expression(*expr_stmt->expr)) {
                    success = false;
                } else if (!is_assignable(*func.return_type, *expr_stmt->expr->type)) {
                    error("Function return type mismatch", expr_stmt->span);
                    success = false;
                }
            }
        }
    }
    
    exit_scope();
    return success;
}

bool TypeChecker::check_statement(const Stmt& stmt) {
    if (auto let = dynamic_cast<const LetStmt*>(&stmt)) {
        // Check initializer expression
        if (!check_expression(*let->init)) {
            return false;
        }
        
        // Check type compatibility
        if (!is_assignable(*let->type, *let->init->type)) {
            error("Type mismatch in let binding", let->span);
            return false;
        }
        
        // Declare variable
        auto var_type = std::make_unique<Type>(let->type->kind, let->type->span);
        if (let->type->base_type) {
            var_type->base_type = std::make_unique<Type>(let->type->base_type->kind, let->type->base_type->span);
        }
        if (!declare_variable(let->name, std::move(var_type), let->is_mut)) {
            error("Duplicate variable name: " + let->name, let->span);
            return false;
        }
    }
    else if (auto expr = dynamic_cast<const ExprStmt*>(&stmt)) {
        return check_expression(*expr->expr);
    }
    else if (auto if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        if (!check_expression(*if_stmt->condition)) {
            return false;
        }
        
        if (!if_stmt->condition->type || if_stmt->condition->type->kind != Type::Kind::Bool) {
            error("If condition must be boolean", if_stmt->condition->span);
            return false;
        }
        
        enter_scope();
        bool then_success = check_statement(*if_stmt->then_branch);
        exit_scope();
        
        if (if_stmt->else_branch) {
            enter_scope();
            bool else_success = check_statement(*if_stmt->else_branch);
            exit_scope();
            return then_success && else_success;
        }
        
        return then_success;
    }
    else if (auto while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        if (!check_expression(*while_stmt->condition)) {
            return false;
        }
        
        if (!while_stmt->condition->type || while_stmt->condition->type->kind != Type::Kind::Bool) {
            error("While condition must be boolean", while_stmt->condition->span);
            return false;
        }
        
        enter_scope();
        bool success = check_statement(*while_stmt->body);
        exit_scope();
        return success;
    }
    else if (auto block = dynamic_cast<const BlockStmt*>(&stmt)) {
        enter_scope();
        for (const auto& stmt : block->statements) {
            if (!check_statement(*stmt)) {
                exit_scope();
                return false;
            }
        }
        exit_scope();
        return true;
    }
    
    return true;
}

bool TypeChecker::check_expression(const Expr& expr) {
    if (auto int_lit = dynamic_cast<const IntLiteral*>(&expr)) {
        expr.type = std::make_unique<Type>(Type::Kind::I32, expr.span);
        return true;
    }
    else if (auto bool_lit = dynamic_cast<const BoolLiteral*>(&expr)) {
        expr.type = std::make_unique<Type>(Type::Kind::Bool, expr.span);
        return true;
    }
    else if (auto str_lit = dynamic_cast<const StringLiteral*>(&expr)) {
        expr.type = std::make_unique<Type>(Type::Kind::Str, expr.span);
        return true;
    }
    else if (auto ident = dynamic_cast<const Identifier*>(&expr)) {
        // First try to find a function with this name
        for (const auto& item : program_->items) {
            if (auto func = dynamic_cast<const FunctionDecl*>(item.get())) {
                if (func->name == ident->name) {
                    // Found a function, but it can only be used in a call expression
                    return true;
                }
            }
        }
        
        // If not a function, look for a variable
        auto var_info = lookup_variable(ident->name);
        if (!var_info) {
            error("Undefined variable: " + ident->name, ident->span);
            return false;
        }
        expr.type = std::make_unique<Type>(var_info->type->kind, expr.span);
        if (var_info->type->base_type) {
            expr.type->base_type = std::make_unique<Type>(var_info->type->base_type->kind, var_info->type->base_type->span);
        }
        ident->is_mut_binding = var_info->is_mut;
        return true;
    }
    else if (auto binary = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (binary->op == BinaryExpr::Op::Assignment) {
            // Check if left side is an identifier
            if (auto ident = dynamic_cast<const Identifier*>(binary->left.get())) {
                // Look up the variable
                auto var_info = lookup_variable(ident->name);
                if (!var_info) {
                    error("Undefined variable: " + ident->name, expr.span);
                    return false;
                }

                // Check if the variable is mutably borrowed
                if (var_info->type && var_info->type->kind == Type::Kind::MutRef) {
                    error("Cannot use variable while mutably borrowed: " + ident->name, expr.span);
                    return false;
                }

                // Check if the variable is mutable
                if (!var_info->is_mut) {
                    error("Cannot assign to immutable variable: " + ident->name, expr.span);
                    return false;
                }

                // Check right side
                if (!check_expression(*binary->right)) {
                    return false;
                }

                // Check type compatibility
                if (!is_assignable(*var_info->type, *binary->right->type)) {
                    error("Type mismatch in assignment", expr.span);
                    return false;
                }

                expr.type = binary->right->type->clone();
                return true;
            }
            error("Left side of assignment must be an identifier", expr.span);
            return false;
        }
        if (!check_expression(*binary->left) || !check_expression(*binary->right)) {
            return false;
        }
        
        // Make sure both operands have types (they might be function identifiers)
        if (!binary->left->type || !binary->right->type) {
            error("Invalid operands in binary expression", expr.span);
            return false;
        }
        
        switch (binary->op) {
            case BinaryExpr::Op::Add:
            case BinaryExpr::Op::Sub:
            case BinaryExpr::Op::Mul:
            case BinaryExpr::Op::Div:
                if (binary->left->type->kind != Type::Kind::I32 ||
                    binary->right->type->kind != Type::Kind::I32) {
                    error("Arithmetic operations require integer operands", expr.span);
                    return false;
                }
                expr.type = std::make_unique<Type>(Type::Kind::I32, expr.span);
                break;
                
            case BinaryExpr::Op::Eq:
            case BinaryExpr::Op::Ne:
            case BinaryExpr::Op::Lt:
            case BinaryExpr::Op::Gt:
            case BinaryExpr::Op::Le:
            case BinaryExpr::Op::Ge:
                if (!is_compatible(*binary->left->type, *binary->right->type)) {
                    error("Incompatible types in comparison", expr.span);
                    return false;
                }
                expr.type = std::make_unique<Type>(Type::Kind::Bool, expr.span);
                break;
                
            case BinaryExpr::Op::And:
            case BinaryExpr::Op::Or:
                if (binary->left->type->kind != Type::Kind::Bool ||
                    binary->right->type->kind != Type::Kind::Bool) {
                    error("Logical operations require boolean operands", expr.span);
                    return false;
                }
                expr.type = std::make_unique<Type>(Type::Kind::Bool, expr.span);
                break;
        }
        return true;
    }
    else if (auto unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (!check_expression(*unary->expr)) {
            return false;
        }
        
        // Make sure the operand has a type (it might be a function identifier)
        if (!unary->expr->type) {
            error("Invalid operand in unary expression", expr.span);
            return false;
        }
        
        switch (unary->op) {
            case UnaryExpr::Op::Neg:
                if (unary->expr->type->kind != Type::Kind::I32) {
                    error("Negation requires integer operand", expr.span);
                    return false;
                }
                expr.type = std::make_unique<Type>(Type::Kind::I32, expr.span);
                break;
                
            case UnaryExpr::Op::Not:
                if (unary->expr->type->kind != Type::Kind::Bool) {
                    error("Logical not requires boolean operand", expr.span);
                    return false;
                }
                expr.type = std::make_unique<Type>(Type::Kind::Bool, expr.span);
                break;
        }
        return true;
    }
    else if (auto borrow = dynamic_cast<const BorrowExpr*>(&expr)) {
        if (!check_expression(*borrow->expr)) {
            return false;
        }
        
        // Make sure the operand has a type (it might be a function identifier)
        if (!borrow->expr->type) {
            error("Invalid operand in borrow expression", expr.span);
            return false;
        }
        
        // Check if we're borrowing a mutable variable
        if (borrow->is_mut) {
            if (auto ident = dynamic_cast<const Identifier*>(borrow->expr.get())) {
                if (!ident->is_mut_binding) {
                    error("Cannot borrow immutable variable as mutable", expr.span);
                    return false;
                }
                
                // Check if the variable is already mutably borrowed
                auto var_info = lookup_variable(ident->name);
                if (var_info && var_info->type && var_info->type->kind == Type::Kind::MutRef) {
                    error("Variable already mutably borrowed: " + ident->name, expr.span);
                    return false;
                }
                
                // Mark the variable as mutably borrowed
                if (var_info) {
                    // Update the variable's type in all scopes where it exists
                    for (auto& scope : scopes_) {
                        auto it = scope.find(ident->name);
                        if (it != scope.end()) {
                            // Create a new type with the same base type but as a MutRef
                            auto base_type = std::make_unique<Type>(it->second.type->kind, it->second.type->span);
                            if (it->second.type->base_type) {
                                base_type->base_type = std::make_unique<Type>(
                                    it->second.type->base_type->kind,
                                    it->second.type->base_type->span
                                );
                            }
                            it->second.type = std::make_unique<Type>(Type::Kind::MutRef, std::move(base_type), expr.span);
                        }
                    }
                }
            }
        }
        
        auto base_type = std::make_unique<Type>(borrow->expr->type->kind, borrow->expr->type->span);
        if (borrow->expr->type->base_type) {
            base_type->base_type = std::make_unique<Type>(borrow->expr->type->base_type->kind, borrow->expr->type->base_type->span);
        }
        
        expr.type = std::make_unique<Type>(
            borrow->is_mut ? Type::Kind::MutRef : Type::Kind::Ref,
            std::move(base_type),
            expr.span
        );
        return true;
    }
    else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        if (!check_expression(*call->callee)) {
            return false;
        }
        
        // Check if the callee is a function identifier
        auto callee_ident = dynamic_cast<const Identifier*>(call->callee.get());
        if (!callee_ident) {
            error("Function call requires a function name", expr.span);
            return false;
        }
        
        // Find the function declaration
        const FunctionDecl* func_decl = nullptr;
        for (const auto& item : program_->items) {
            if (auto func = dynamic_cast<const FunctionDecl*>(item.get())) {
                if (func->name == callee_ident->name) {
                    func_decl = func;
                    break;
                }
            }
        }
        
        if (!func_decl) {
            error("Undefined function: " + callee_ident->name, expr.span);
            return false;
        }
        
        // Check argument count
        if (call->args.size() != func_decl->params.size()) {
            error("Wrong number of arguments for function " + callee_ident->name, expr.span);
            return false;
        }
        
        // Check argument types
        for (size_t i = 0; i < call->args.size(); ++i) {
            if (!check_expression(*call->args[i])) {
                return false;
            }
            
            // Make sure the argument has a type (it might be a function identifier)
            if (!call->args[i]->type) {
                error("Invalid argument in function call", expr.span);
                return false;
            }
            
            if (!is_assignable(*func_decl->params[i].type, *call->args[i]->type)) {
                error("Type mismatch in argument " + std::to_string(i + 1) + " of function " + callee_ident->name, call->args[i]->span);
                return false;
            }
        }
        
        // Set the return type
        expr.type = std::make_unique<Type>(func_decl->return_type->kind, expr.span);
        if (func_decl->return_type->base_type) {
            expr.type->base_type = std::make_unique<Type>(func_decl->return_type->base_type->kind, func_decl->return_type->base_type->span);
        }
        return true;
    }
    
    return true;
}

bool TypeChecker::is_assignable(const Type& target, const Type& source) {
    if (target.kind == source.kind) {
        if (target.kind == Type::Kind::Ref || target.kind == Type::Kind::MutRef) {
            return is_assignable(*target.base_type, *source.base_type);
        }
        return true;
    }
    
    // Allow implicit conversion from &mut T to &T
    if (target.kind == Type::Kind::Ref && source.kind == Type::Kind::MutRef) {
        return is_assignable(*target.base_type, *source.base_type);
    }
    
    return false;
}

bool TypeChecker::is_compatible(const Type& lhs, const Type& rhs) {
    if (lhs.kind == rhs.kind) {
        if (lhs.kind == Type::Kind::Ref || lhs.kind == Type::Kind::MutRef) {
            return is_compatible(*lhs.base_type, *rhs.base_type);
        }
        return true;
    }
    
    // Allow comparison between &T and &mut T
    if ((lhs.kind == Type::Kind::Ref && rhs.kind == Type::Kind::MutRef) ||
        (lhs.kind == Type::Kind::MutRef && rhs.kind == Type::Kind::Ref)) {
        return is_compatible(*lhs.base_type, *rhs.base_type);
    }
    
    return false;
}

bool TypeChecker::is_mutable_reference(const Type& type) const {
    return type.kind == Type::Kind::MutRef;
}

bool TypeChecker::is_reference(const Type& type) const {
    return type.kind == Type::Kind::Ref || type.kind == Type::Kind::MutRef;
}

void TypeChecker::enter_scope() {
    scopes_.emplace_back();
}

void TypeChecker::exit_scope() {
    scopes_.pop_back();
}

bool TypeChecker::declare_variable(const std::string& name, std::unique_ptr<Type> type, bool is_mut) {
    if (scopes_.empty()) {
        scopes_.emplace_back();
    }
    
    auto& current_scope = scopes_.back();
    if (current_scope.find(name) != current_scope.end()) {
        return false;
    }
    
    current_scope[name] = {std::move(type), is_mut};
    return true;
}

std::optional<TypeChecker::VariableInfo> TypeChecker::lookup_variable(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            auto type = std::make_unique<Type>(found->second.type->kind, found->second.type->span);
            if (found->second.type->base_type) {
                type->base_type = std::make_unique<Type>(found->second.type->base_type->kind, found->second.type->base_type->span);
            }
            return VariableInfo{std::move(type), found->second.is_mut};
        }
    }
    return std::nullopt;
}

void TypeChecker::error(const std::string& message, const Span& span) {
    std::stringstream ss;
    ss << "Type error at " << span.start << ":" << span.end << ": " << message;
    errors_.push_back(ss.str());
    std::cerr << "Error: " << ss.str() << std::endl;
}

} // namespace nust 