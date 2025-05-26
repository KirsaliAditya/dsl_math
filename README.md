# 🧮 DSL for Mathematical Computation

This project implements a **Domain-Specific Language (DSL)** for evaluating mathematical expressions through a hybrid approach: **LLVM IR JIT compilation** for arithmetic and **C++ AST evaluation** for functions and equations. Users can interactively compute values, define variables, solve equations, and inspect generated IR.

---

## ✨ Features

- ✅ Arithmetic operations: `+`, `-`, `*`, `/`, `^`
- ✅ Built-in math functions: `sin`, `cos`, `log`, `sqrt`
- ✅ Variable assignment: `var x = 5;`
- ✅ Equation solving:
  - Symbolic solving for single-variable linear equations
  - Numerical solving for non-linear equations using Newton-Raphson and bisection
- ✅ Automatic differentiation support
- ✅ REPL (interactive shell)
- ✅ LLVM IR generation and JIT execution
- ✅ IR Dump
- ✅ File-based script execution

---

## 🔧 Build Instructions

### Prerequisites

- LLVM (version 13+ recommended)
- Flex
- Bison
- Clang++ or G++

### Build

```bash
make
