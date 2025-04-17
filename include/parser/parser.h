#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>

namespace nust {

// Location information for error reporting
struct Span {
    size_t start;
    size_t end;
    
    Span(size_t start, size_t end) : start(start), end(end) {}
};

// Forward declarations
class ASTNode;
class Program;
class FunctionDecl;
class Stmt;
class Expr;
class Type;

// Precedence levels for binary operators
enum class Precedence {
    None,
    Assignment,  // =
    Or,         // ||
    And,        // &&
    Equality,   // == !=
    Comparison, // < > <= >=
    Term,       // + -
    Factor,     // * /
    Unary,      // ! -
    Call,       // . () []
    Primary
};

// AST Node types
class ASTNode {
public:
    virtual ~ASTNode() = default;
    Span span;
protected:
    ASTNode(Span span) : span(span) {}
};

class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> items;
    Program(Span span, std::vector<std::unique_ptr<ASTNode>> items) 
        : ASTNode(span), items(std::move(items)) {}
};

class FunctionDecl : public ASTNode {
public:
    struct Param {
        bool is_mut;
        std::string name;
        std::unique_ptr<Type> type;
        Span span;
        
        Param(bool is_mut, std::string name, std::unique_ptr<Type> type, Span span)
            : is_mut(is_mut), name(std::move(name)), type(std::move(type)), span(span) {}
    };
    
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Type> return_type;  // Added return type
    std::unique_ptr<Stmt> body;
    
    FunctionDecl(Span span, std::string name, std::vector<Param> params, 
                std::unique_ptr<Type> return_type, std::unique_ptr<Stmt> body)
        : ASTNode(span), name(std::move(name)), params(std::move(params)),
          return_type(std::move(return_type)), body(std::move(body)) {}
};

// Represents a lexical scope for borrow checking
class Scope {
public:
    std::weak_ptr<Scope> parent;
    std::vector<std::string> declarations;  // Variables declared in this scope
    
    explicit Scope(std::weak_ptr<Scope> parent = {}) : parent(parent) {}
};

class Stmt : public ASTNode {
public:
    std::shared_ptr<Scope> scope;  // Each statement has access to its enclosing scope
    virtual ~Stmt() = default;
protected:
    Stmt(Span span, std::shared_ptr<Scope> scope) : ASTNode(span), scope(scope) {}
};

class LetStmt : public Stmt {
public:
    bool is_mut;  // Added mutability flag for let bindings
    std::string name;
    std::unique_ptr<Type> type;
    std::unique_ptr<Expr> init;
    
    LetStmt(Span span, std::shared_ptr<Scope> scope, bool is_mut, std::string name, 
            std::unique_ptr<Type> type, std::unique_ptr<Expr> init)
        : Stmt(span, scope), is_mut(is_mut), name(std::move(name)), 
          type(std::move(type)), init(std::move(init)) {}
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    ExprStmt(Span span, std::shared_ptr<Scope> scope, std::unique_ptr<Expr> expr)
        : Stmt(span, scope), expr(std::move(expr)) {}
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch;
    
    IfStmt(Span span, std::shared_ptr<Scope> scope, std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch)
        : Stmt(span, scope), condition(std::move(condition)),
          then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(Span span, std::shared_ptr<Scope> scope, std::unique_ptr<Expr> condition,
              std::unique_ptr<Stmt> body)
        : Stmt(span, scope), condition(std::move(condition)), body(std::move(body)) {}
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt(Span span, std::shared_ptr<Scope> scope, std::vector<std::unique_ptr<Stmt>> statements)
        : Stmt(span, scope), statements(std::move(statements)) {}
};

class Expr : public ASTNode {
public:
    mutable std::unique_ptr<Type> type;  // Type of the expression, filled in by type checker
    virtual ~Expr() = default;
protected:
    Expr(Span span) : ASTNode(span) {}
};

class IntLiteral : public Expr {
public:
    int value;
    IntLiteral(Span span, int value) : Expr(span), value(value) {}
};

class BoolLiteral : public Expr {
public:
    bool value;
    BoolLiteral(Span span, bool value) : Expr(span), value(value) {}
};

class StringLiteral : public Expr {
public:
    std::string value;
    StringLiteral(Span span, std::string value) : Expr(span), value(std::move(value)) {}
};

class Identifier : public Expr {
public:
    std::string name;
    mutable bool is_mut_binding;  // Whether this identifier refers to a mutable binding
    Identifier(Span span, std::string name)
        : Expr(span), name(std::move(name)), is_mut_binding(false) {}
};

class BinaryExpr : public Expr {
public:
    enum class Op {
        Add, Sub, Mul, Div,
        Eq, Ne, Lt, Gt, Le, Ge,
        And, Or,  // Logical AND and OR
        Assignment  // =
    };
    Op op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    
    BinaryExpr(Span span, Op op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : Expr(span), op(op), left(std::move(left)), right(std::move(right)) {}
};

class UnaryExpr : public Expr {
public:
    enum class Op { Neg, Not };
    Op op;
    std::unique_ptr<Expr> expr;
    
    UnaryExpr(Span span, Op op, std::unique_ptr<Expr> expr)
        : Expr(span), op(op), expr(std::move(expr)) {}
};

class BorrowExpr : public Expr {
public:
    bool is_mut;
    std::unique_ptr<Expr> expr;
    
    BorrowExpr(Span span, bool is_mut, std::unique_ptr<Expr> expr)
        : Expr(span), is_mut(is_mut), expr(std::move(expr)) {}
};

class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
    
    CallExpr(Span span, std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : Expr(span), callee(std::move(callee)), args(std::move(args)) {}
};

class Type {
public:
    enum class Kind {
        I32, Bool, Str,
        Ref, MutRef
    };
    Kind kind;
    std::unique_ptr<Type> base_type; // For Ref and MutRef
    Span span;
    
    Type(Kind kind, Span span) : kind(kind), span(span) {}
    Type(Kind kind, std::unique_ptr<Type> base_type, Span span)
        : kind(kind), base_type(std::move(base_type)), span(span) {}
    
    // Clone method for deep copying
    std::unique_ptr<Type> clone() const {
        if (base_type) {
            return std::make_unique<Type>(kind, base_type->clone(), span);
        } else {
            return std::make_unique<Type>(kind, span);
        }
    }
};

class Parser {
public:
    Parser(std::string source);
    std::unique_ptr<Program> parse();

private:
    // Helper functions
    bool match(const std::string& expected);
    bool peek(const std::string& expected);
    void expect(const std::string& expected);
    std::string consume_identifier();
    int consume_integer();
    std::string consume_string();
    void skip_whitespace();
    bool at_end();
    void error(const std::string& message);
    void synchronize();
    Span make_span(size_t start) const { return Span(start, pos); }
    
    // Scope management
    std::shared_ptr<Scope> current_scope;
    std::shared_ptr<Scope> enter_scope();
    void exit_scope();
    
    // Parsing functions
    std::unique_ptr<FunctionDecl> parse_function();
    std::vector<FunctionDecl::Param> parse_params();
    std::unique_ptr<Type> parse_type();
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<LetStmt> parse_let();
    std::unique_ptr<IfStmt> parse_if();
    std::unique_ptr<WhileStmt> parse_while();
    std::unique_ptr<BlockStmt> parse_block();
    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_equality();
    std::unique_ptr<Expr> parse_comparison();
    std::unique_ptr<Expr> parse_term();
    std::unique_ptr<Expr> parse_factor();
    std::unique_ptr<Expr> parse_unary();
    std::unique_ptr<Expr> parse_call();
    std::unique_ptr<Expr> parse_primary();
    std::unique_ptr<Expr> parse_or();
    std::unique_ptr<Expr> parse_and();
    std::unique_ptr<Expr> parse_assignment();
    
    std::string source;
    size_t pos = 0;
};

} // namespace nust 