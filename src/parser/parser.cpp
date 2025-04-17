#include "parser/parser.h"
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace nust {

Parser::Parser(std::string source) 
    : source(std::move(source)), current_scope(std::make_shared<Scope>()) {}

std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<ASTNode>> items;
    size_t start = pos;
    
    skip_whitespace();
    while (!at_end()) {
        items.push_back(parse_function());
        skip_whitespace();
    }

    return std::make_unique<Program>(make_span(start), std::move(items));
}

std::unique_ptr<FunctionDecl> Parser::parse_function() {
    size_t start = pos;
    expect("fn");
    skip_whitespace();
    
    std::string name = consume_identifier();
    skip_whitespace();
    
    expect("(");
    auto params = parse_params();
    expect(")");
    skip_whitespace();
    
    // Parse return type if present
    std::unique_ptr<Type> return_type;
    if (match("->")) {
        skip_whitespace();
        return_type = parse_type();
        skip_whitespace();
    } else {
        // Default return type is unit/void
        return_type = std::make_unique<Type>(Type::Kind::I32, make_span(pos));
    }
    
    // Create new scope for function body
    auto function_scope = enter_scope();
    
    // Add parameters to scope
    for (const auto& param : params) {
        function_scope->declarations.push_back(param.name);
    }
    
    auto body = parse_block();
    
    exit_scope();
    
    return std::make_unique<FunctionDecl>(
        make_span(start),
        std::move(name),
        std::move(params),
        std::move(return_type),
        std::move(body)
    );
}

std::vector<FunctionDecl::Param> Parser::parse_params() {
    std::vector<FunctionDecl::Param> params;
    
    skip_whitespace();
    if (peek(")")) return params;
    
    do {
        size_t param_start = pos;
        skip_whitespace();
        bool is_mut = match("mut");
        if (is_mut) skip_whitespace();
        
        std::string name = consume_identifier();
        skip_whitespace();
        
        expect(":");
        skip_whitespace();
        
        auto type = parse_type();
        
        params.push_back(FunctionDecl::Param{
            is_mut,
            std::move(name),
            std::move(type),
            make_span(param_start)
        });
        
        skip_whitespace();
    } while (match(","));
    
    return params;
}

std::unique_ptr<Type> Parser::parse_type() {
    size_t start = pos;
    
    if (match("&")) {
        bool is_mut = match("mut");
        if (is_mut) skip_whitespace();
        auto inner = parse_type();
        return std::make_unique<Type>(
            is_mut ? Type::Kind::MutRef : Type::Kind::Ref,
            std::move(inner),
            make_span(start)
        );
    }
    
    if (match("i32")) return std::make_unique<Type>(Type::Kind::I32, make_span(start));
    if (match("bool")) return std::make_unique<Type>(Type::Kind::Bool, make_span(start));
    if (match("str")) return std::make_unique<Type>(Type::Kind::Str, make_span(start));
    
    error("Expected type");
    return nullptr;
}

std::unique_ptr<Stmt> Parser::parse_statement() {
    skip_whitespace();
    
    if (match("let")) return parse_let();
    if (match("if")) return parse_if();
    if (match("while")) return parse_while();
    if (peek("{")) return parse_block();
    
    // Expression statement
    size_t start = pos;
    auto expr = parse_expr();
    skip_whitespace();
    
    // If we're at the end of a block or at EOF, allow missing semicolon
    if (!peek("}") && !at_end()) {
        expect(";");
    }
    
    return std::make_unique<ExprStmt>(make_span(start), current_scope, std::move(expr));
}

std::unique_ptr<LetStmt> Parser::parse_let() {
    size_t start = pos;
    skip_whitespace();
    bool is_mut = match("mut");
    if (is_mut) skip_whitespace();
    
    std::string name = consume_identifier();
    skip_whitespace();
    
    expect(":");
    skip_whitespace();
    
    auto type = parse_type();
    skip_whitespace();
    
    expect("=");
    skip_whitespace();
    
    auto init = parse_expr();
    skip_whitespace();
    expect(";");
    
    // Add variable to current scope
    current_scope->declarations.push_back(name);
    
    return std::make_unique<LetStmt>(
        make_span(start),
        current_scope,
        is_mut,
        std::move(name),
        std::move(type),
        std::move(init)
    );
}

std::unique_ptr<IfStmt> Parser::parse_if() {
    size_t start = pos;
    skip_whitespace();
    
    auto condition = parse_expr();
    skip_whitespace();
    
    auto then_scope = enter_scope();
    auto then_branch = parse_block();
    exit_scope();
    
    skip_whitespace();
    
    std::unique_ptr<Stmt> else_branch;
    if (match("else")) {
        skip_whitespace();
        auto else_scope = enter_scope();
        if (match("if")) {
            else_branch = parse_if();
        } else {
            else_branch = parse_block();
        }
        exit_scope();
    }
    
    return std::make_unique<IfStmt>(
        make_span(start),
        current_scope,
        std::move(condition),
        std::move(then_branch),
        std::move(else_branch)
    );
}

std::unique_ptr<WhileStmt> Parser::parse_while() {
    size_t start = pos;
    skip_whitespace();
    
    auto condition = parse_expr();
    skip_whitespace();
    
    auto body_scope = enter_scope();
    auto body = parse_block();
    exit_scope();
    
    return std::make_unique<WhileStmt>(
        make_span(start),
        current_scope,
        std::move(condition),
        std::move(body)
    );
}

std::unique_ptr<BlockStmt> Parser::parse_block() {
    size_t start = pos;
    expect("{");
    
    auto block_scope = enter_scope();
    std::vector<std::unique_ptr<Stmt>> statements;
    
    skip_whitespace();
    while (!at_end() && !peek("}")) {
        statements.push_back(parse_statement());
        skip_whitespace();
    }
    
    expect("}");
    
    auto block = std::make_unique<BlockStmt>(
        make_span(start),
        block_scope,
        std::move(statements)
    );
    
    exit_scope();
    return block;
}

std::unique_ptr<Expr> Parser::parse_expr() {
    return parse_assignment();
}

std::unique_ptr<Expr> Parser::parse_assignment() {
    auto lhs = parse_or();
    
    if (match("=")) {
        skip_whitespace();
        // Validate that left side is an identifier
        if (dynamic_cast<Identifier*>(lhs.get()) == nullptr) {
            throw std::runtime_error("Invalid assignment target");
        }
        auto rhs = parse_assignment();  // Right-associative
        return std::make_unique<BinaryExpr>(
            make_span(lhs->span.start),
            BinaryExpr::Op::Assignment,
            std::move(lhs),
            std::move(rhs)
        );
    }
    
    return lhs;
}

std::unique_ptr<Expr> Parser::parse_or() {
    auto expr = parse_and();
    
    while (match("||")) {
        skip_whitespace();
        auto right = parse_and();
        expr = std::make_unique<BinaryExpr>(
            make_span(expr->span.start),
            BinaryExpr::Op::Or,
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_and() {
    auto expr = parse_equality();
    
    while (match("&&")) {
        skip_whitespace();
        auto right = parse_equality();
        expr = std::make_unique<BinaryExpr>(
            make_span(expr->span.start),
            BinaryExpr::Op::And,
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_equality() {
    auto expr = parse_comparison();
    
    while (true) {
        if (match("==")) {
            skip_whitespace();
            auto right = parse_comparison();
            expr = std::make_unique<BinaryExpr>(
                make_span(expr->span.start),
                BinaryExpr::Op::Eq,
                std::move(expr),
                std::move(right)
            );
        } else if (match("!=")) {
            skip_whitespace();
            auto right = parse_comparison();
            expr = std::make_unique<BinaryExpr>(
                make_span(expr->span.start),
                BinaryExpr::Op::Ne,
                std::move(expr),
                std::move(right)
            );
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_comparison() {
    size_t start = pos;
    auto expr = parse_term();
    
    while (true) {
        skip_whitespace();
        BinaryExpr::Op op;
        if (match("<")) op = BinaryExpr::Op::Lt;
        else if (match("<=")) op = BinaryExpr::Op::Le;
        else if (match(">")) op = BinaryExpr::Op::Gt;
        else if (match(">=")) op = BinaryExpr::Op::Ge;
        else break;
        
        skip_whitespace();
        auto right = parse_term();
        expr = std::make_unique<BinaryExpr>(
            make_span(start),
            op,
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_term() {
    size_t start = pos;
    auto expr = parse_factor();
    
    while (true) {
        skip_whitespace();
        BinaryExpr::Op op;
        if (match("+")) op = BinaryExpr::Op::Add;
        else if (match("-")) op = BinaryExpr::Op::Sub;
        else break;
        
        skip_whitespace();
        auto right = parse_factor();
        expr = std::make_unique<BinaryExpr>(
            make_span(start),
            op,
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_factor() {
    size_t start = pos;
    auto expr = parse_unary();
    
    while (true) {
        skip_whitespace();
        BinaryExpr::Op op;
        if (match("*")) op = BinaryExpr::Op::Mul;
        else if (match("/")) op = BinaryExpr::Op::Div;
        else break;
        
        skip_whitespace();
        auto right = parse_unary();
        expr = std::make_unique<BinaryExpr>(
            make_span(start),
            op,
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_unary() {
    size_t start = pos;
    skip_whitespace();
    
    if (match("-")) {
        auto operand = parse_unary();
        return std::make_unique<UnaryExpr>(
            make_span(start),
            UnaryExpr::Op::Neg,
            std::move(operand)
        );
    }
    
    if (match("!")) {
        auto operand = parse_unary();
        return std::make_unique<UnaryExpr>(
            make_span(start),
            UnaryExpr::Op::Not,
            std::move(operand)
        );
    }
    
    if (match("&")) {
        bool is_mut = match("mut");
        if (is_mut) skip_whitespace();
        auto expr = parse_unary();
        return std::make_unique<BorrowExpr>(
            make_span(start),
            is_mut,
            std::move(expr)
        );
    }
    
    return parse_call();
}

std::unique_ptr<Expr> Parser::parse_call() {
    size_t start = pos;
    auto expr = parse_primary();
    
    while (true) {
        skip_whitespace();
        if (match("(")) {
            std::vector<std::unique_ptr<Expr>> args;
            
            skip_whitespace();
            if (!peek(")")) {
                do {
                    args.push_back(parse_expr());
                    skip_whitespace();
                } while (match(","));
            }
            
            expect(")");
            expr = std::make_unique<CallExpr>(
                make_span(start),
                std::move(expr),
                std::move(args)
            );
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parse_primary() {
    size_t start = pos;
    skip_whitespace();
    
    if (std::isdigit(source[pos])) {
        return std::make_unique<IntLiteral>(make_span(start), consume_integer());
    }
    
    if (match("true")) {
        return std::make_unique<BoolLiteral>(make_span(start), true);
    }
    
    if (match("false")) {
        return std::make_unique<BoolLiteral>(make_span(start), false);
    }
    
    if (source[pos] == '"') {
        return std::make_unique<StringLiteral>(make_span(start), consume_string());
    }
    
    if (std::isalpha(source[pos]) || source[pos] == '_') {
        auto ident = std::make_unique<Identifier>(make_span(start), consume_identifier());
        // Check if identifier is mutable in current scope
        // This will be used by the type checker
        return ident;
    }
    
    if (match("(")) {
        auto expr = parse_expr();
        expect(")");
        return expr;
    }
    
    error("Expected expression");
    return nullptr;
}

bool Parser::match(const std::string& expected) {
    if (source.compare(pos, expected.length(), expected) == 0) {
        pos += expected.length();
        return true;
    }
    return false;
}

bool Parser::peek(const std::string& expected) {
    return source.compare(pos, expected.length(), expected) == 0;
}

void Parser::expect(const std::string& expected) {
    if (!match(expected)) {
        std::stringstream ss;
        ss << "Expected '" << expected << "'";
        error(ss.str());
    }
}

std::string Parser::consume_identifier() {
    size_t start = pos;
    
    if (!std::isalpha(source[pos]) && source[pos] != '_') {
        error("Expected identifier");
    }
    
    while (pos < source.length() && 
           (std::isalnum(source[pos]) || source[pos] == '_')) {
        pos++;
    }
    
    return source.substr(start, pos - start);
}

int Parser::consume_integer() {
    size_t start = pos;
    
    while (pos < source.length() && std::isdigit(source[pos])) {
        pos++;
    }
    
    return std::stoi(source.substr(start, pos - start));
}

std::string Parser::consume_string() {
    if (source[pos] != '"') {
        error("Expected string");
    }
    pos++; // Skip opening quote
    
    size_t start = pos;
    while (pos < source.length() && source[pos] != '"') {
        if (source[pos] == '\\') {
            pos++; // Skip escape character
            if (pos >= source.length()) {
                error("Unterminated string");
            }
        }
        pos++;
    }
    
    if (pos >= source.length()) {
        error("Unterminated string");
    }
    
    std::string value = source.substr(start, pos - start);
    pos++; // Skip closing quote
    return value;
}

void Parser::skip_whitespace() {
    while (pos < source.length()) {
        // Skip whitespace characters
        if (std::isspace(source[pos]) || source[pos] == '\n' || source[pos] == '\r') {
            pos++;
            continue;
        }
        
        // Handle comments
        if (pos + 1 < source.length() && source[pos] == '/' && source[pos + 1] == '/') {
            // Skip until we find a newline or reach the end
            pos += 2;  // Skip the "//"
            while (pos < source.length() && source[pos] != '\n' && source[pos] != '\r') {
                pos++;
            }
            continue;
        }
        
        break;
    }
}

bool Parser::at_end() {
    return pos >= source.length();
}

void Parser::error(const std::string& message) {
    std::stringstream ss;
    ss << "Parse error at position " << pos << ": " << message;
    throw std::runtime_error(ss.str());
}

std::shared_ptr<Scope> Parser::enter_scope() {
    auto new_scope = std::make_shared<Scope>(current_scope);
    current_scope = new_scope;
    return new_scope;
}

void Parser::exit_scope() {
    if (auto parent = current_scope->parent.lock()) {
        current_scope = parent;
    }
}

} // namespace nust