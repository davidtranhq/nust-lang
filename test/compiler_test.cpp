#include "parser/parser.h"
#include "type_checker.h"
#include "compiler.h"
#include <gtest/gtest.h>
#include <sstream>
#include <iostream>

namespace nust {

class CompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup code
    }
    
    std::vector<Instruction> compile_source(const std::string& source) {
        Parser parser(source);
        auto program = parser.parse();
        EXPECT_TRUE(program != nullptr);
        
        TypeChecker type_checker;
        EXPECT_TRUE(type_checker.check_program(*program));
        
        Compiler compiler;
        auto instructions = compiler.compile(*program);
        
        return instructions;
    }
    
    void expect_instruction(const std::vector<Instruction>& instructions, 
                          size_t index, 
                          Opcode expected_opcode,
                          size_t expected_operand = 0) {
        ASSERT_LT(index, instructions.size());
        EXPECT_EQ(instructions[index].opcode, expected_opcode);
        if (instructions[index].has_operand()) {
            EXPECT_EQ(instructions[index].operand, expected_operand);
        }
    }
    
private:
    std::string opcode_to_string(Opcode op) {
        switch (op) {
            case Opcode::PUSH_I32: return "PUSH_I32";
            case Opcode::PUSH_BOOL: return "PUSH_BOOL";
            case Opcode::PUSH_STR: return "PUSH_STR";
            case Opcode::POP: return "POP";
            case Opcode::DUP: return "DUP";
            case Opcode::SWAP: return "SWAP";
            case Opcode::LOAD: return "LOAD";
            case Opcode::STORE: return "STORE";
            case Opcode::LOAD_REF: return "LOAD_REF";
            case Opcode::STORE_REF: return "STORE_REF";
            case Opcode::ADD_I32: return "ADD_I32";
            case Opcode::SUB_I32: return "SUB_I32";
            case Opcode::MUL_I32: return "MUL_I32";
            case Opcode::DIV_I32: return "DIV_I32";
            case Opcode::NEG_I32: return "NEG_I32";
            case Opcode::EQ_I32: return "EQ_I32";
            case Opcode::NE_I32: return "NE_I32";
            case Opcode::LT_I32: return "LT_I32";
            case Opcode::GT_I32: return "GT_I32";
            case Opcode::LE_I32: return "LE_I32";
            case Opcode::GE_I32: return "GE_I32";
            case Opcode::AND: return "AND";
            case Opcode::OR: return "OR";
            case Opcode::NOT: return "NOT";
            case Opcode::JMP: return "JMP";
            case Opcode::JMP_IF: return "JMP_IF";
            case Opcode::JMP_IF_NOT: return "JMP_IF_NOT";
            case Opcode::CALL: return "CALL";
            case Opcode::RET: return "RET";
            case Opcode::RET_VAL: return "RET_VAL";
            case Opcode::BORROW: return "BORROW";
            case Opcode::BORROW_MUT: return "BORROW_MUT";
            case Opcode::DEREF: return "DEREF";
            case Opcode::DEREF_MUT: return "DEREF_MUT";
            default: return "UNKNOWN";
        }
    }
};

TEST_F(CompilerTest, BasicFunction) {
    std::string source = R"(
        fn main() {
            let x: i32 = 42;
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode:
    // PUSH_I32 42
    // STORE 0
    // RET
    
    ASSERT_GE(instructions.size(), 3);
    expect_instruction(instructions, 0, Opcode::PUSH_I32, 42);
    expect_instruction(instructions, 1, Opcode::STORE, 0);
    expect_instruction(instructions, 2, Opcode::RET);
}

TEST_F(CompilerTest, ArithmeticExpressions) {
    std::string source = R"(
        fn main() {
            let x: i32 = 1 + 2 * 3;
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode:
    // PUSH_I32 1
    // PUSH_I32 2
    // PUSH_I32 3
    // MUL_I32
    // ADD_I32
    // STORE 0
    // RET
    
    ASSERT_GE(instructions.size(), 7);
    expect_instruction(instructions, 0, Opcode::PUSH_I32, 1);
    expect_instruction(instructions, 1, Opcode::PUSH_I32, 2);
    expect_instruction(instructions, 2, Opcode::PUSH_I32, 3);
    expect_instruction(instructions, 3, Opcode::MUL_I32);
    expect_instruction(instructions, 4, Opcode::ADD_I32);
    expect_instruction(instructions, 5, Opcode::STORE, 0);
    expect_instruction(instructions, 6, Opcode::RET);
}

TEST_F(CompilerTest, ControlFlow) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 42;
            if (x > 0) {
                x = x + 1;
            }
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode:
    // PUSH_I32 42
    // STORE 0
    // LOAD 0
    // PUSH_I32 0
    // GT_I32
    // JMP_IF_NOT <else_jump>
    // LOAD 0
    // PUSH_I32 1
    // ADD_I32
    // STORE 0
    // JMP <end>
    // <else_jump>:
    // <end>:
    // RET
    
    ASSERT_GE(instructions.size(), 12);
    expect_instruction(instructions, 0, Opcode::PUSH_I32, 42);
    expect_instruction(instructions, 1, Opcode::STORE, 0);
    expect_instruction(instructions, 2, Opcode::LOAD, 0);
    expect_instruction(instructions, 3, Opcode::PUSH_I32, 0);
    expect_instruction(instructions, 4, Opcode::GT_I32);
    expect_instruction(instructions, 5, Opcode::JMP_IF_NOT, 12);
    expect_instruction(instructions, 6, Opcode::LOAD, 0);
    expect_instruction(instructions, 7, Opcode::PUSH_I32, 1);
    expect_instruction(instructions, 8, Opcode::ADD_I32);
    expect_instruction(instructions, 9, Opcode::STORE, 0);
    expect_instruction(instructions, 10, Opcode::LOAD, 0);
    expect_instruction(instructions, 11, Opcode::POP);
    expect_instruction(instructions, 12, Opcode::RET);
}

TEST_F(CompilerTest, FunctionCalls) {
    std::string source = R"(
        fn add(x: i32, y: i32) -> i32 {
            x + y
        }
        
        fn main() {
            let result: i32 = add(1, 2);
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode for add:
    // LOAD 0
    // LOAD 1
    // ADD_I32
    // POP
    // RET
    
    // Expected bytecode for main:
    // PUSH_I32 2
    // PUSH_I32 1
    // CALL 0
    // STORE 0
    // RET
    
    ASSERT_GE(instructions.size(), 10);
    
    // Check add function
    expect_instruction(instructions, 0, Opcode::LOAD, 0);
    expect_instruction(instructions, 1, Opcode::LOAD, 1);
    expect_instruction(instructions, 2, Opcode::ADD_I32);
    expect_instruction(instructions, 3, Opcode::POP);
    expect_instruction(instructions, 4, Opcode::RET);
    
    // Check main function
    expect_instruction(instructions, 5, Opcode::PUSH_I32, 2);
    expect_instruction(instructions, 6, Opcode::PUSH_I32, 1);
    expect_instruction(instructions, 7, Opcode::CALL, 0);
    expect_instruction(instructions, 8, Opcode::STORE, 0);
    expect_instruction(instructions, 9, Opcode::RET);
}

TEST_F(CompilerTest, References) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 42;
            let y: &i32 = &x;
            let z: &mut i32 = &mut x;
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode:
    // PUSH_I32 42
    // STORE 0
    // LOAD 0
    // BORROW
    // STORE 1
    // LOAD 0
    // BORROW_MUT
    // STORE 2
    // RET
    
    ASSERT_GE(instructions.size(), 9);
    expect_instruction(instructions, 0, Opcode::PUSH_I32, 42);
    expect_instruction(instructions, 1, Opcode::STORE, 0);
    expect_instruction(instructions, 2, Opcode::LOAD, 0);
    expect_instruction(instructions, 3, Opcode::BORROW);
    expect_instruction(instructions, 4, Opcode::STORE, 1);
    expect_instruction(instructions, 5, Opcode::LOAD, 0);
    expect_instruction(instructions, 6, Opcode::BORROW_MUT);
    expect_instruction(instructions, 7, Opcode::STORE, 2);
    expect_instruction(instructions, 8, Opcode::RET);
}

TEST_F(CompilerTest, WhileLoop) {
    std::string source = R"(
        fn main() {
            let mut x: i32 = 10;
            while (x > 0) {
                x = x - 1;
            }
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode:
    // PUSH_I32 10
    // STORE 0
    // LOAD 0
    // PUSH_I32 0
    // GT_I32
    // JMP_IF_NOT <end>
    // LOAD 0
    // PUSH_I32 1
    // SUB_I32
    // STORE 0
    // JMP <start>
    // <end>:
    // RET
    
    ASSERT_GE(instructions.size(), 12);
    expect_instruction(instructions, 0, Opcode::PUSH_I32, 10);
    expect_instruction(instructions, 1, Opcode::STORE, 0);
    expect_instruction(instructions, 2, Opcode::LOAD, 0);
    expect_instruction(instructions, 3, Opcode::PUSH_I32, 0);
    expect_instruction(instructions, 4, Opcode::GT_I32);
    expect_instruction(instructions, 5, Opcode::JMP_IF_NOT, 13);
    expect_instruction(instructions, 6, Opcode::LOAD, 0);
    expect_instruction(instructions, 7, Opcode::PUSH_I32, 1);
    expect_instruction(instructions, 8, Opcode::SUB_I32);
    expect_instruction(instructions, 9, Opcode::STORE, 0);
    expect_instruction(instructions, 10, Opcode::LOAD, 0);
    expect_instruction(instructions, 11, Opcode::POP);
    expect_instruction(instructions, 12, Opcode::JMP, 2);
    expect_instruction(instructions, 13, Opcode::RET);
}

TEST_F(CompilerTest, StringLiterals) {
    std::string source = R"(
        fn main() {
            let s: str = "hello";
        }
    )";
    
    auto instructions = compile_source(source);
    
    // Expected bytecode:
    // PUSH_STR 0
    // STORE 0
    // RET
    
    ASSERT_GE(instructions.size(), 3);
    expect_instruction(instructions, 0, Opcode::PUSH_STR, 0);
    expect_instruction(instructions, 1, Opcode::STORE, 0);
    expect_instruction(instructions, 2, Opcode::RET);
}

} // namespace nust 