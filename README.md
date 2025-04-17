# Nust

# Dependencies

The only dependency for this project is for the test suite, which is [GoogleTest](https://github.com/google/googletest). 

# Build

To build the project, run `make` in the root directory and run the executable with a `.ns` file as an argument.

Some source files have been provided in `examples/`. Run `./nust examples/[foo|bar|quux].nust` to generate the corresponding assembly `.ns` or compiled bytecode `.no` file.

# Test

Run `make test` to run the test suite.
