#include <iostream>
#include <fstream>
#include <sstream>
#include "parser/parser.h"
#include "type_checker.h"
#include "compiler.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>\n";
        return 1;
    }
    
    // Read source file
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    try {
        // Parse source code
        nust::Parser parser(source);
        auto program = parser.parse();
        
        // Type check
        nust::TypeChecker type_checker;
        if (!type_checker.check_program(*program)) {
            std::cerr << "Type checking failed\n";
            return 1;
        }
        
        // Compile to bytecode
        nust::Compiler compiler;
        auto instructions = compiler.compile(*program);

        // get the filename without the extension
        std::string filename = argv[1];
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            filename = filename.substr(0, dot_pos);
        }

        // Output instructions as assembly to *.ns file
        std::ofstream output_asm_file(filename + std::string(".ns"));
        if (!output_asm_file.is_open()) {
            std::cerr << "Failed to open output file: " << filename + std::string(".s") << "\n";
            return 1;
        }
        for (const auto& instr : instructions) {
            output_asm_file << nust::opcode_to_string(instr.opcode);
            if (instr.has_operand()) {
                output_asm_file << " " << instr.operand;
            }
            output_asm_file << "\n";
        }
        

        // Output bytecode to *.no file
        std::ofstream output_bytecode_file(filename + std::string(".no"));
        if (!output_bytecode_file.is_open()) {
            std::cerr << "Failed to open output file: " << filename + std::string(".no") << "\n";
            return 1;
        }
        
        for (const auto& instr : instructions) {
            output_bytecode_file << static_cast<uint8_t>(instr.opcode);
            if (instr.has_operand()) {
                // Encode operand as little-endian
                for (size_t i = 0; i < sizeof(size_t); ++i) {
                    output_bytecode_file << static_cast<uint8_t>((instr.operand >> (i * 8)) & 0xFF);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
} 