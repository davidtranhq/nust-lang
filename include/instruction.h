#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace nust {

// Opcodes for the virtual machine
enum class Opcode : uint8_t {
    // Stack operations
    PUSH_I32,   // Push 32-bit integer constant
    PUSH_BOOL,  // Push boolean constant
    PUSH_STR,   // Push string constant
    POP,        // Pop top value from stack
    DUP,
    SWAP,
    
    // Variable operations
    LOAD,       // Load local variable onto stack
    STORE,      // Store top of stack into local variable
    LOAD_REF,   // Load reference to local variable
    STORE_REF,  // Store reference to local variable
    
    // Arithmetic operations
    ADD_I32,    // Add two integers
    SUB_I32,    // Subtract two integers
    MUL_I32,    // Multiply two integers
    DIV_I32,    // Divide two integers
    NEG_I32,    // Negate integer
    
    // Comparison operations
    EQ_I32,     // Integer equality
    NE_I32,     // Integer inequality
    LT_I32,     // Integer less than
    GT_I32,     // Integer greater than
    LE_I32,     // Integer less than or equal
    GE_I32,     // Integer greater than or equal
    
    // Logical operations
    AND,        // Logical AND
    OR,         // Logical OR
    NOT,        // Logical NOT
    
    // Control flow
    JMP,        // Unconditional jump
    JMP_IF,     // Jump if top of stack is true
    JMP_IF_NOT, // Jump if top of stack is false
    CALL,       // Call function
    RET,        // Return from function (no value)
    RET_VAL,    // Return from function with value
    
    // Reference operations
    BORROW,     // Create immutable reference
    BORROW_MUT, // Create mutable reference
    DEREF,      // Dereference reference
    DEREF_MUT   // Dereference mutable reference
};

// Convert opcode to string representation
inline std::string opcode_to_string(Opcode opcode) {
    switch (opcode) {
        // Stack operations
        case Opcode::PUSH_I32:  return "PUSH_I32";
        case Opcode::PUSH_BOOL: return "PUSH_BOOL";
        case Opcode::PUSH_STR:  return "PUSH_STR";
        case Opcode::POP:       return "POP";
        case Opcode::DUP:       return "DUP";
        case Opcode::SWAP:      return "SWAP";
        
        // Variable operations
        case Opcode::LOAD:      return "LOAD";
        case Opcode::STORE:     return "STORE";
        case Opcode::LOAD_REF:  return "LOAD_REF";
        case Opcode::STORE_REF: return "STORE_REF";
        
        // Arithmetic operations
        case Opcode::ADD_I32:   return "ADD_I32";
        case Opcode::SUB_I32:   return "SUB_I32";
        case Opcode::MUL_I32:   return "MUL_I32";
        case Opcode::DIV_I32:   return "DIV_I32";
        case Opcode::NEG_I32:   return "NEG_I32";
        
        // Comparison operations
        case Opcode::EQ_I32:    return "EQ_I32";
        case Opcode::NE_I32:    return "NE_I32";
        case Opcode::LT_I32:    return "LT_I32";
        case Opcode::GT_I32:    return "GT_I32";
        case Opcode::LE_I32:    return "LE_I32";
        case Opcode::GE_I32:    return "GE_I32";
        
        // Logical operations
        case Opcode::AND:       return "AND";
        case Opcode::OR:        return "OR";
        case Opcode::NOT:       return "NOT";
        
        // Control flow
        case Opcode::JMP:       return "JMP";
        case Opcode::JMP_IF:    return "JMP_IF";
        case Opcode::JMP_IF_NOT: return "JMP_IF_NOT";
        case Opcode::CALL:      return "CALL";
        case Opcode::RET:       return "RET";
        case Opcode::RET_VAL:   return "RET_VAL";
        
        // Reference operations
        case Opcode::BORROW:    return "BORROW";
        case Opcode::BORROW_MUT: return "BORROW_MUT";
        case Opcode::DEREF:     return "DEREF";
        case Opcode::DEREF_MUT: return "DEREF_MUT";
        
        default:
            return "UNKNOWN_OPCODE";
    }
}

// Instruction structure
struct Instruction {
    Opcode opcode;
    size_t operand;  // Optional operand (e.g., constant index, local variable index, jump offset)
    
    Instruction(Opcode opcode) : opcode(opcode), operand(0) {}
    Instruction(Opcode opcode, size_t operand) : opcode(opcode), operand(operand) {}
    
    // Helper to determine if this instruction type has an operand
    bool has_operand() const {
        switch (opcode) {
            case Opcode::PUSH_I32:
            case Opcode::PUSH_BOOL:
            case Opcode::PUSH_STR:
            case Opcode::LOAD:
            case Opcode::STORE:
            case Opcode::LOAD_REF:
            case Opcode::JMP:
            case Opcode::JMP_IF:
            case Opcode::JMP_IF_NOT:
            case Opcode::CALL:
                return true;
            default:
                return false;
        }
    }
};

} // namespace nust 