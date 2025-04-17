# Nust Virtual Machine Bytecode

This document describes the bytecode format for the Nust virtual machine. The bytecode is designed to be a simple stack-based instruction set that can efficiently execute programs written in the Nust language.

## Value Types

The VM supports the following value types:
- `i32`: 32-bit signed integer
- `bool`: Boolean value (true/false)
- `str`: String reference
- `ref`: Reference to a value
- `mut_ref`: Mutable reference to a value
- `fn`: Function reference

## Stack Frame Layout

Each function call creates a new stack frame with the following layout:
```
+----------------+
| Return Address |
+----------------+
| Frame Pointer  |
+----------------+
| Local Var 1    |
| Local Var 2    |
| ...            |
+----------------+
| Argument 1     |
| Argument 2     |
| ...            |
+----------------+
```

## Instructions

### Stack Operations

- `PUSH_I32 <value>`: Push a 32-bit integer onto the stack
- `PUSH_BOOL <value>`: Push a boolean value onto the stack
- `PUSH_STR <index>`: Push a string constant onto the stack
- `POP`: Remove the top value from the stack
- `DUP`: Duplicate the top value on the stack
- `SWAP`: Swap the top two values on the stack

### Variable Operations

- `LOAD <index>`: Load a local variable onto the stack
- `STORE <index>`: Store the top value into a local variable
- `LOAD_REF <index>`: Load a reference to a local variable
- `STORE_REF`: Store a value through a reference

### Arithmetic Operations

- `ADD_I32`: Pop two integers, add them, push result
- `SUB_I32`: Pop two integers, subtract them, push result
- `MUL_I32`: Pop two integers, multiply them, push result
- `DIV_I32`: Pop two integers, divide them, push result
- `NEG_I32`: Pop an integer, negate it, push result

### Comparison Operations

- `EQ_I32`: Pop two integers, push true if equal
- `NE_I32`: Pop two integers, push true if not equal
- `LT_I32`: Pop two integers, push true if first < second
- `GT_I32`: Pop two integers, push true if first > second
- `LE_I32`: Pop two integers, push true if first <= second
- `GE_I32`: Pop two integers, push true if first >= second

### Logical Operations

- `AND`: Pop two booleans, push their logical AND
- `OR`: Pop two booleans, push their logical OR
- `NOT`: Pop a boolean, push its logical NOT

### Control Flow

- `JMP <offset>`: Unconditional jump
- `JMP_IF <offset>`: Pop a boolean, jump if true
- `JMP_IF_NOT <offset>`: Pop a boolean, jump if false
- `CALL <index>`: Call a function
- `RET`: Return from a function
- `RET_VAL`: Return a value from a function

### Reference Operations

- `BORROW`: Create an immutable reference to a value
- `BORROW_MUT`: Create a mutable reference to a value
- `DEREF`: Dereference a reference
- `DEREF_MUT`: Dereference a mutable reference

## Function Calls

Function calls in the VM are handled through a combination of stack operations and control flow instructions. Here's how they work:

### Function Call Process

1. **Argument Preparation**:
   - Arguments are pushed onto the stack in reverse order (right-to-left)
   - This ensures they appear in the correct order in the callee's frame

2. **Call Instruction**:
   - `CALL <index>` instruction is executed
   - The index points to the function in the function table
   - Current instruction pointer is saved as return address
   - New stack frame is created

3. **Stack Frame Creation**:
   ```
   +----------------+
   | Return Address |  <- Saved instruction pointer
   +----------------+
   | Frame Pointer  |  <- Points to previous frame
   +----------------+
   | Local Var 1    |  <- Space for local variables
   | Local Var 2    |
   | ...            |
   +----------------+
   | Argument 1     |  <- Function arguments
   | Argument 2     |
   | ...            |
   +----------------+
   ```

4. **Function Execution**:
   - Function body executes in new frame
   - Local variables are accessed via `LOAD`/`STORE` with frame-relative indices
   - Arguments are accessed as if they were local variables

5. **Return Process**:
   - `RET` or `RET_VAL` instruction is executed
   - Return value (if any) is left on stack
   - Stack frame is popped
   - Control returns to caller

### Example Function Call

Consider this code:
```rust
fn add(x: i32, y: i32) -> i32 {
    x + y
}

fn main() {
    let result = add(1, 2);
}
```

Compiles to:
```
; Function add
LOAD 0    ; Load x
LOAD 1    ; Load y
ADD_I32   ; Add them
RET_VAL   ; Return the result

; Function main
PUSH_I32 2    ; Push second argument
PUSH_I32 1    ; Push first argument
CALL 0        ; Call add (index 0 in function table)
STORE 0       ; Store result in local variable
```

### Function Table

The VM maintains a function table that contains:
1. Function entry point (instruction pointer)
2. Number of parameters
3. Number of local variables
4. Return type information

### Special Cases

1. **Recursive Calls**:
   - Each call creates a new stack frame
   - Return addresses are properly maintained
   - Local variables are independent per call

2. **Function References**:
   - Functions can be passed as values
   - `PUSH_FN <index>` pushes a function reference
   - `CALL_INDIRECT` calls through a function reference

3. **Closures** (Future Extension):
   - Will require capturing environment
   - Special closure type in value system
   - Additional instructions for closure creation/call

### Error Handling

The VM checks for:
1. Correct number of arguments
2. Argument type compatibility
3. Stack overflow during call
4. Invalid function indices
5. Return type mismatches

## Example Bytecode

Here's an example of how a simple function would be compiled to bytecode:

```rust
fn add(x: i32, y: i32) -> i32 {
    x + y
}
```

Compiles to:
```
LOAD 0    ; Load x
LOAD 1    ; Load y
ADD_I32   ; Add them
RET_VAL   ; Return the result
```

## Implementation Notes

1. The VM uses a stack-based architecture for simplicity and ease of implementation.
2. Local variables are accessed by index within the current stack frame.
3. Function calls create new stack frames with space for local variables.
4. References are implemented as pointers to values on the stack.
5. String constants are stored in a separate constant pool.
6. The instruction set is designed to be simple but complete enough to support all language features.

## Future Extensions

Potential future extensions to the bytecode:
1. Support for floating-point numbers
2. Support for arrays and other collection types
3. Support for closures and anonymous functions
4. Support for exception handling
5. Support for concurrency primitives

### Function Table Implementation

The function table is a critical component of the VM that maintains metadata about all functions in the program. Here's its detailed implementation:

#### Structure

```cpp
struct FunctionInfo {
    size_t entry_point;      // Instruction pointer where function starts
    size_t num_params;       // Number of parameters
    size_t num_locals;       // Number of local variables
    Type return_type;        // Function's return type
    std::vector<Type> param_types;  // Types of parameters
    std::string name;        // Function name for debugging
};

class FunctionTable {
    std::vector<FunctionInfo> functions;
    std::unordered_map<std::string, size_t> name_to_index;
    
public:
    // Add a new function to the table
    size_t add_function(FunctionInfo info) {
        size_t index = functions.size();
        functions.push_back(std::move(info));
        name_to_index[functions.back().name] = index;
        return index;
    }
    
    // Lookup function by index
    const FunctionInfo& get_function(size_t index) const {
        return functions[index];
    }
    
    // Lookup function by name
    std::optional<size_t> find_function(const std::string& name) const {
        auto it = name_to_index.find(name);
        if (it != name_to_index.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};
```

#### Usage in Bytecode

The function table is used in several ways:

1. **Function Definition**:
```
; Function definition header
DEF_FN 0          ; Define function at index 0
PARAM_COUNT 2     ; Number of parameters
LOCAL_COUNT 1     ; Number of local variables
PARAM_TYPE i32    ; First parameter type
PARAM_TYPE i32    ; Second parameter type
RETURN_TYPE i32   ; Return type
```

2. **Function Call**:
```
PUSH_I32 42       ; Push arguments
PUSH_I32 24
CALL 0           ; Call function at index 0
```

3. **Function Reference**:
```
PUSH_FN 0        ; Push reference to function at index 0
```

#### Memory Layout

The function table is stored in the VM's memory as follows:

```
+------------------+
| Function Count   |  <- Number of functions
+------------------+
| Function 0 Info  |  <- First function's metadata
| - entry_point    |
| - num_params     |
| - num_locals     |
| - return_type    |
| - param_types[]  |
+------------------+
| Function 1 Info  |  <- Second function's metadata
| ...              |
+------------------+
```

#### Runtime Checks

The VM performs these checks using the function table:

1. **Call Validation**:
```cpp
void validate_call(size_t fn_index, const std::vector<Value>& args) {
    const auto& fn_info = function_table.get_function(fn_index);
    
    // Check argument count
    if (args.size() != fn_info.num_params) {
        throw RuntimeError("Wrong number of arguments");
    }
    
    // Check argument types
    for (size_t i = 0; i < args.size(); ++i) {
        if (!is_assignable(fn_info.param_types[i], args[i].type)) {
            throw RuntimeError("Type mismatch in argument " + std::to_string(i));
        }
    }
}
```

2. **Return Validation**:
```cpp
void validate_return(const FunctionInfo& fn_info, const Value& ret_val) {
    if (!is_assignable(fn_info.return_type, ret_val.type)) {
        throw RuntimeError("Return type mismatch");
    }
}
```

#### Optimization Opportunities

1. **Function Inlining**:
   - Small functions can be inlined at compile time
   - Function table helps identify candidates based on size

2. **Type Specialization**:
   - Create specialized versions of functions for common types
   - Function table tracks specialized versions

3. **Call Site Optimization**:
   - Cache frequently called function indices
   - Use direct jumps for monomorphic calls

#### Example Usage

Here's how the function table is used in a complete example:

```rust
fn add(x: i32, y: i32) -> i32 {
    x + y
}

fn main() {
    let result = add(1, 2);
}
```

Compiles to:
```
; Function table entries
DEF_FN 0          ; add
PARAM_COUNT 2
LOCAL_COUNT 0
PARAM_TYPE i32
PARAM_TYPE i32
RETURN_TYPE i32

DEF_FN 1          ; main
PARAM_COUNT 0
LOCAL_COUNT 1
RETURN_TYPE ()

; Function add
LOAD 0
LOAD 1
ADD_I32
RET_VAL

; Function main
PUSH_I32 2
PUSH_I32 1
CALL 0
STORE 0
``` 