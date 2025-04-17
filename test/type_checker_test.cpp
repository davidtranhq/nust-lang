#include "parser/parser.h"
#include "type_checker.h"
#include <gtest/gtest.h>

namespace nust {

TEST(TypeCheckerTest, BasicTypes) {
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
    
    TypeChecker checker;
    ASSERT_TRUE(checker.check_program(*program));
}

TEST(TypeCheckerTest, TypeMismatch) {
    std::string source = R"(
        fn main() {
            let x: i32 = true;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, ArithmeticOperations) {
    std::string source = R"(
        fn main() {
            let x: i32 = 1 + 2 * 3 - 4 / 5;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_TRUE(checker.check_program(*program));
}

TEST(TypeCheckerTest, InvalidArithmetic) {
    std::string source = R"(
        fn main() {
            let x: i32 = true + 42;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, References) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 42;
            let y: &i32 = &x;
            let z: &mut i32 = &mut x;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_TRUE(checker.check_program(*program));
}

TEST(TypeCheckerTest, InvalidReferences) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            let y: &mut i32 = &mut x;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, ControlFlow) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            if x > 0 {
                let y: i32 = x + 1;
            } else {
                let y: i32 = x - 1;
            }
            
            let mut i: i32 = 0;
            while i < 10 {
                i = i + 1;
            }
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_TRUE(checker.check_program(*program));
}

TEST(TypeCheckerTest, InvalidControlFlow) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            if x {
                let y: i32 = x + 1;
            }
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, FunctionReturn) {
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
    
    TypeChecker checker;
    ASSERT_TRUE(checker.check_program(*program));
}

TEST(TypeCheckerTest, InvalidFunctionReturn) {
    std::string source = R"(
        fn add(x: i32, y: i32) -> i32 {
            true
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, ValidBorrows) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            let y: &i32 = &x;
            let mut z: i32 = 10;
            let w: &mut i32 = &mut z;
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_TRUE(checker.check_program(*program));
}

TEST(TypeCheckerTest, InvalidImmutableToMutableBorrow) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
            let a: &mut i32 = &mut x;  // Can't borrow immutable as mutable
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, MultipleMutableBorrows) {
    std::string source = R"(
        fn main() {
            let mut z: i32 = 10;
            let b: &mut i32 = &mut z;
            let c: &mut i32 = &mut z;  // Multiple mutable borrows
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

TEST(TypeCheckerTest, UseWhileBorrowed) {
    std::string source = R"(
        fn main() {
            let mut z: i32 = 10;
            let w: &mut i32 = &mut z;
            z = 20;  // Can't use z while mutably borrowed
        }
    )";
    
    Parser parser(source);
    auto program = parser.parse();
    ASSERT_TRUE(program != nullptr);
    
    TypeChecker checker;
    ASSERT_FALSE(checker.check_program(*program));
    ASSERT_FALSE(checker.errors().empty());
}

} // namespace nust 