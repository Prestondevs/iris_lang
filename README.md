# Iris Language Compiler

Iris is a custom programming language and compiler that translates `.ir` source files into native x86-64 assembly and produces executable ELF binaries.

## Overview

This project implements a full compilation pipeline:

- `.ir` source code → parsing → abstract syntax tree (AST)  
- AST → x86-64 assembly generation  
- Assembly → object file using GNU assembler (`as`)  
- Object file → executable via system linker  

The language includes a runtime system for handling basic data types (integers, floats, strings) and supports expressions, variables, and function calls.

## Features

- Custom `.ir` language syntax  
- AST-based compilation  
- Direct x86-64 assembly code generation  
- Tagged value system for runtime type handling  
- Function support with parameter passing  
- Integration with GNU toolchain (`as`, `ld` / `gcc`)  
- Produces native ELF executables  

## Build

Compile the compiler:

```bash
g++ -std=c++17 -o iris main.cpp