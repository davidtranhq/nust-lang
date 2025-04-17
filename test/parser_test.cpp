#include "parser/parser.h"
#include <gtest/gtest.h>
#include <sstream>

namespace nust {

TEST(ParserTest, BasicFunction) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            let y: bool = true;
            let z: str = "hello";
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, ArithmeticExpressions) {
    std::string source = R"(
        fn main() {
            let x: i32 = 1 + 2 * 3;
            let y: i32 = (1 + 2) * 3;
            let z: i32 = -x + y;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, ControlFlow) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            if (x > 0) {
                let y: i32 = x * 2;
            } else {
                let y: i32 = 0;
            }
            
            while (x > 0) {
                x = x - 1;
            }
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, References) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            let y: &i32 = &x;
            let z: &mut i32 = &mut x;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, FunctionCalls) {
    std::string source = R"(
        fn add(x: i32, y: i32) -> i32 {
            x + y
        }
        
        fn main() {
            let result: i32 = add(1, 2);
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, FunctionReturnTypes) {
    std::string source = R"(
        fn add(x: i32, y: i32) -> i32 {
            x + y
        }
        
        fn get_ref(x: &i32) -> &i32 {
            x
        }
        
        fn get_mut_ref(x: &mut i32) -> &mut i32 {
            x
        }
        
        fn main() -> i32 {
            42
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, Mutability) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 42;
            let y: i32 = 10;
            
            x = x + y;  // Should be allowed
            // y = y + x;  // Should be disallowed by type checker
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, NestedScopes) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            {
                let y: i32 = x;  // Can access outer scope
                let x: i32 = 10; // Shadows outer x
                {
                    let z: i32 = x + y;  // Uses inner x
                }
            }
            // y is out of scope here
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, BorrowChecking) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 42;
            let y: &i32 = &x;      // Immutable borrow
            let z: &mut i32 = &mut x;  // Mutable borrow
            
            // x = 10;  // Should be disallowed by type checker (mutable borrow active)
            // let w: &i32 = &x;  // Should be disallowed (mutable borrow active)
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, ComplexExpressions) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            let y: &mut i32 = &mut x;
            let z: &&i32 = &&x;
            let w: &mut &mut i32 = &mut &mut x;
            
            let a: bool = !(x == 42);
            let b: i32 = -(-x);
            let c: i32 = (1 + 2) * (3 - 4) / 5;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
}

TEST(ParserTest, ParseTreeStructure) {
    std::string source = R"(
        fn add(x: i32, y: i32) -> i32 {
            x + y
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    // Check program has one function
    ASSERT_EQ(program->items.size(), 1);
    auto* func = dynamic_cast<FunctionDecl*>(program->items[0].get());
    ASSERT_TRUE(func != nullptr);
    
    // Check function declaration
    ASSERT_EQ(func->name, "add");
    ASSERT_EQ(func->params.size(), 2);
    
    // Check parameters
    ASSERT_EQ(func->params[0].name, "x");
    ASSERT_EQ(func->params[0].type->kind, Type::Kind::I32);
    ASSERT_FALSE(func->params[0].is_mut);
    
    ASSERT_EQ(func->params[1].name, "y");
    ASSERT_EQ(func->params[1].type->kind, Type::Kind::I32);
    ASSERT_FALSE(func->params[1].is_mut);
    
    // Check return type
    ASSERT_EQ(func->return_type->kind, Type::Kind::I32);
    
    // Check function body
    auto* body = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_TRUE(body != nullptr);
    ASSERT_EQ(body->statements.size(), 1);
    
    // Check the expression statement (x + y)
    auto* expr_stmt = dynamic_cast<ExprStmt*>(body->statements[0].get());
    ASSERT_TRUE(expr_stmt != nullptr);
    
    auto* binary = dynamic_cast<BinaryExpr*>(expr_stmt->expr.get());
    ASSERT_TRUE(binary != nullptr);
    ASSERT_EQ(binary->op, BinaryExpr::Op::Add);
    
    auto* left = dynamic_cast<Identifier*>(binary->left.get());
    ASSERT_TRUE(left != nullptr);
    ASSERT_EQ(left->name, "x");
    
    auto* right = dynamic_cast<Identifier*>(binary->right.get());
    ASSERT_TRUE(right != nullptr);
    ASSERT_EQ(right->name, "y");
}

TEST(ParserTest, ComplexParseTree) {
    std::string source = R"(
        fn test() {
            let mut x: i32 = 42;
            let y: &mut i32 = &mut x;
            if (x > 0) {
                y = x + 10;
            }
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    // Check program has one function
    ASSERT_EQ(program->items.size(), 1);
    auto* func = dynamic_cast<FunctionDecl*>(program->items[0].get());
    ASSERT_TRUE(func != nullptr);
    
    // Check function declaration
    ASSERT_EQ(func->name, "test");
    ASSERT_EQ(func->params.size(), 0);
    
    // Check function body
    auto* body = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_TRUE(body != nullptr);
    ASSERT_EQ(body->statements.size(), 3);  // let x, let y, if statement
    
    // Check first let statement (let mut x: i32 = 42)
    auto* let_x = dynamic_cast<LetStmt*>(body->statements[0].get());
    ASSERT_TRUE(let_x != nullptr);
    ASSERT_EQ(let_x->name, "x");
    ASSERT_TRUE(let_x->is_mut);
    ASSERT_EQ(let_x->type->kind, Type::Kind::I32);
    
    auto* x_init = dynamic_cast<IntLiteral*>(let_x->init.get());
    ASSERT_TRUE(x_init != nullptr);
    ASSERT_EQ(x_init->value, 42);
    
    // Check second let statement (let y: &mut i32 = &mut x)
    auto* let_y = dynamic_cast<LetStmt*>(body->statements[1].get());
    ASSERT_TRUE(let_y != nullptr);
    ASSERT_EQ(let_y->name, "y");
    ASSERT_FALSE(let_y->is_mut);
    ASSERT_EQ(let_y->type->kind, Type::Kind::MutRef);
    ASSERT_EQ(let_y->type->base_type->kind, Type::Kind::I32);
    
    auto* y_init = dynamic_cast<BorrowExpr*>(let_y->init.get());
    ASSERT_TRUE(y_init != nullptr);
    ASSERT_TRUE(y_init->is_mut);
    
    auto* borrowed_x = dynamic_cast<Identifier*>(y_init->expr.get());
    ASSERT_TRUE(borrowed_x != nullptr);
    ASSERT_EQ(borrowed_x->name, "x");
    
    // Check if statement
    auto* if_stmt = dynamic_cast<IfStmt*>(body->statements[2].get());
    ASSERT_TRUE(if_stmt != nullptr);
    
    // Check if condition (x > 0)
    auto* condition = dynamic_cast<BinaryExpr*>(if_stmt->condition.get());
    ASSERT_TRUE(condition != nullptr);
    ASSERT_EQ(condition->op, BinaryExpr::Op::Gt);
    
    auto* cond_left = dynamic_cast<Identifier*>(condition->left.get());
    ASSERT_TRUE(cond_left != nullptr);
    ASSERT_EQ(cond_left->name, "x");
    
    auto* cond_right = dynamic_cast<IntLiteral*>(condition->right.get());
    ASSERT_TRUE(cond_right != nullptr);
    ASSERT_EQ(cond_right->value, 0);
}

TEST(ParserTest, ExpressionPrecedence) {
    std::string source = R"(
        fn test() {
            let x: i32 = 1 + 2 * 3;
            let y: bool = !true && false || true;
            let z: i32 = (1 + 2) * (3 + 4);
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    auto* func = dynamic_cast<FunctionDecl*>(program->items[0].get());
    ASSERT_TRUE(func != nullptr);
    
    auto* body = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_TRUE(body != nullptr);
    ASSERT_EQ(body->statements.size(), 3);
    
    // Check first expression: 1 + 2 * 3
    auto* let_x = dynamic_cast<LetStmt*>(body->statements[0].get());
    ASSERT_TRUE(let_x != nullptr);
    
    auto* expr1 = dynamic_cast<BinaryExpr*>(let_x->init.get());
    ASSERT_TRUE(expr1 != nullptr);
    ASSERT_EQ(expr1->op, BinaryExpr::Op::Add);
    
    auto* left1 = dynamic_cast<IntLiteral*>(expr1->left.get());
    ASSERT_TRUE(left1 != nullptr);
    ASSERT_EQ(left1->value, 1);
    
    auto* right1 = dynamic_cast<BinaryExpr*>(expr1->right.get());
    ASSERT_TRUE(right1 != nullptr);
    ASSERT_EQ(right1->op, BinaryExpr::Op::Mul);
    
    // Check second expression: !true && false || true
    auto* let_y = dynamic_cast<LetStmt*>(body->statements[1].get());
    ASSERT_TRUE(let_y != nullptr);
    
    auto* expr2 = dynamic_cast<BinaryExpr*>(let_y->init.get());
    ASSERT_TRUE(expr2 != nullptr);
    ASSERT_EQ(expr2->op, BinaryExpr::Op::Or);
    
    // Check left side of OR: !true && false
    auto* left2 = dynamic_cast<BinaryExpr*>(expr2->left.get());
    ASSERT_TRUE(left2 != nullptr);
    ASSERT_EQ(left2->op, BinaryExpr::Op::And);
    
    // Check !true
    auto* not_expr = dynamic_cast<UnaryExpr*>(left2->left.get());
    ASSERT_TRUE(not_expr != nullptr);
    ASSERT_EQ(not_expr->op, UnaryExpr::Op::Not);
    
    auto* true_lit = dynamic_cast<BoolLiteral*>(not_expr->expr.get());
    ASSERT_TRUE(true_lit != nullptr);
    ASSERT_TRUE(true_lit->value);
    
    // Check false
    auto* false_lit = dynamic_cast<BoolLiteral*>(left2->right.get());
    ASSERT_TRUE(false_lit != nullptr);
    ASSERT_FALSE(false_lit->value);
    
    // Check right side of OR: true
    auto* right2 = dynamic_cast<BoolLiteral*>(expr2->right.get());
    ASSERT_TRUE(right2 != nullptr);
    ASSERT_TRUE(right2->value);
    
    // Check third expression: (1 + 2) * (3 + 4)
    auto* let_z = dynamic_cast<LetStmt*>(body->statements[2].get());
    ASSERT_TRUE(let_z != nullptr);
    
    auto* expr3 = dynamic_cast<BinaryExpr*>(let_z->init.get());
    ASSERT_TRUE(expr3 != nullptr);
    ASSERT_EQ(expr3->op, BinaryExpr::Op::Mul);
    
    auto* left3 = dynamic_cast<BinaryExpr*>(expr3->left.get());
    ASSERT_TRUE(left3 != nullptr);
    ASSERT_EQ(left3->op, BinaryExpr::Op::Add);
    
    auto* right3 = dynamic_cast<BinaryExpr*>(expr3->right.get());
    ASSERT_TRUE(right3 != nullptr);
    ASSERT_EQ(right3->op, BinaryExpr::Op::Add);
}

TEST(ParserTest, AssignmentExpressions) {
    // Valid assignments
    std::string valid_source = R"(
        fn main() {
            let mut x: i32 = 42;
            x = 10;
            (x) = 20;  // Parenthesized identifier is allowed
            let mut y: bool = true;
            y = false;
        }
    )";
    
    Parser parser(valid_source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    // Check the parse tree structure
    auto* func = dynamic_cast<FunctionDecl*>(program->items[0].get());
    ASSERT_TRUE(func != nullptr);
    
    auto* body = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_TRUE(body != nullptr);
    ASSERT_EQ(body->statements.size(), 5);
    
    // Check the third statement ((x) = 20)
    auto* expr_stmt = dynamic_cast<ExprStmt*>(body->statements[2].get());
    ASSERT_TRUE(expr_stmt != nullptr);
    
    auto* assign = dynamic_cast<BinaryExpr*>(expr_stmt->expr.get());
    ASSERT_TRUE(assign != nullptr);
    ASSERT_EQ(assign->op, BinaryExpr::Op::Assignment);
    
    // Check left side (x)
    auto* x_ident = dynamic_cast<Identifier*>(assign->left.get());
    ASSERT_TRUE(x_ident != nullptr);
    ASSERT_EQ(x_ident->name, "x");
    
    // Check right side (20)
    auto* twenty_lit = dynamic_cast<IntLiteral*>(assign->right.get());
    ASSERT_TRUE(twenty_lit != nullptr);
    ASSERT_EQ(twenty_lit->value, 20);
    
    // Invalid assignment - should throw because it's not an identifier
    std::string invalid_source = R"(
        fn main() {
            let mut x: i32 = 42;
            x + 1 = 10;  // Cannot assign to binary expression
        }
    )";
    
    Parser parser2(invalid_source);
    EXPECT_THROW(parser2.parse(), std::runtime_error);
}

TEST(ParserTest, AssignmentPrecedence) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 42;
            let mut y: i32 = 10;
            x = y = 5;  // Right-associative: x = (y = 5)
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    // Check the parse tree structure
    auto* func = dynamic_cast<FunctionDecl*>(program->items[0].get());
    ASSERT_TRUE(func != nullptr);
    
    auto* body = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_TRUE(body != nullptr);
    ASSERT_EQ(body->statements.size(), 3);
    
    // Check the last statement (x = y = 5)
    auto* expr_stmt = dynamic_cast<ExprStmt*>(body->statements[2].get());
    ASSERT_TRUE(expr_stmt != nullptr);
    
    auto* outer_assign = dynamic_cast<BinaryExpr*>(expr_stmt->expr.get());
    ASSERT_TRUE(outer_assign != nullptr);
    ASSERT_EQ(outer_assign->op, BinaryExpr::Op::Assignment);
    
    // Check left side (x)
    auto* x_ident = dynamic_cast<Identifier*>(outer_assign->left.get());
    ASSERT_TRUE(x_ident != nullptr);
    ASSERT_EQ(x_ident->name, "x");
    
    // Check right side (y = 5)
    auto* inner_assign = dynamic_cast<BinaryExpr*>(outer_assign->right.get());
    ASSERT_TRUE(inner_assign != nullptr);
    ASSERT_EQ(inner_assign->op, BinaryExpr::Op::Assignment);
    
    // Check left side of inner assignment (y)
    auto* y_ident = dynamic_cast<Identifier*>(inner_assign->left.get());
    ASSERT_TRUE(y_ident != nullptr);
    ASSERT_EQ(y_ident->name, "y");
    
    // Check right side of inner assignment (5)
    auto* five_lit = dynamic_cast<IntLiteral*>(inner_assign->right.get());
    ASSERT_TRUE(five_lit != nullptr);
    ASSERT_EQ(five_lit->value, 5);
}

TEST(ParserTest, AssignmentWithLogicalOperators) {
    std::string source = R"(
        fn main() {
            let mut x: bool = true;
            let mut y: bool = false;
            x = y || true;  // Should be parsed as x = (y || true)
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    // Check the parse tree structure
    auto* func = dynamic_cast<FunctionDecl*>(program->items[0].get());
    ASSERT_TRUE(func != nullptr);
    
    auto* body = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_TRUE(body != nullptr);
    ASSERT_EQ(body->statements.size(), 3);
    
    // Check the last statement (x = y || true)
    auto* expr_stmt = dynamic_cast<ExprStmt*>(body->statements[2].get());
    ASSERT_TRUE(expr_stmt != nullptr);
    
    auto* assign = dynamic_cast<BinaryExpr*>(expr_stmt->expr.get());
    ASSERT_TRUE(assign != nullptr);
    ASSERT_EQ(assign->op, BinaryExpr::Op::Assignment);
    
    // Check left side (x)
    auto* x_ident = dynamic_cast<Identifier*>(assign->left.get());
    ASSERT_TRUE(x_ident != nullptr);
    ASSERT_EQ(x_ident->name, "x");
    
    // Check right side (y || true)
    auto* or_expr = dynamic_cast<BinaryExpr*>(assign->right.get());
    ASSERT_TRUE(or_expr != nullptr);
    ASSERT_EQ(or_expr->op, BinaryExpr::Op::Or);
    
    // Check left side of OR (y)
    auto* y_ident = dynamic_cast<Identifier*>(or_expr->left.get());
    ASSERT_TRUE(y_ident != nullptr);
    ASSERT_EQ(y_ident->name, "y");
    
    // Check right side of OR (true)
    auto* true_lit = dynamic_cast<BoolLiteral*>(or_expr->right.get());
    ASSERT_TRUE(true_lit != nullptr);
    ASSERT_TRUE(true_lit->value);
}

} // namespace nust 