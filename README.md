# Iris
Compiler for "Iris"

## 1. Memory Safety Without Garbage Collection
- **Borrowed and Scoped References**: Automatic lifetime inference prevents dangling pointers.
- **Stack-only Allocations**: Temporary objects are automatically deallocated safely.
- **Compile-time Detection**: Catch use-after-free, buffer overflows, and double frees at compile time.

## 2. Built-in Data Security
- **Encrypted Types**: Variables can encrypt and decrypt automatically in memory.
- **Secure Zeroization**: Memory holding sensitive data is wiped after use.
- **Immutable-by-Default Variables**: Prevent accidental data leaks; mutability must be explicitly declared.

## 3. Safe Concurrency
- **Data-Race-Free Primitives**: Native constructs ensure safe concurrent access.
- **Ownership for Threads**: Only threads that “own” data can modify it.
- **Transactional Memory / Versioned References**: Provides safe multi-threaded operations.

## 4. Compile-time Security Checks
- **Input Validation Types**: Strings or numbers can carry constraints (e.g., `EmailString`, `PositiveInt`) enforced at compile time.
- **Taint Tracking**: Untrusted input is tracked and cannot reach sensitive operations unless sanitized.

## 5. Built-in Cryptography
- **Constant-Time Operations**: Prevent timing attacks in cryptography.
- **Secure Random Number Generation**: Enforced at the type level.
- **Memory-Hard Key Derivation**: Optional protection for sensitive operations.

## 6. Sandboxing & Capability-Based Security
- **Capabilities for Operations**: Control file, network, or hardware access at the language level.
- **Compile-time Verification**: Only authorized functions/modules can perform sensitive operations.
- **Fine-Grained Privileges**: Security enforced per function or object.

## 7. Deterministic and Verifiable Execution
- **Formally Verifiable Binaries**: Optional proofs of memory safety or functional correctness.
- **Lightweight Formal Verification**: Detect undefined behavior at compile time.

## 8. Obfuscation and Anti-Tampering
- **Binary Obfuscation**: Protect code and data from reverse engineering.
- **Runtime Integrity Checks**: Detect modifications to binaries during execution.

## 9. Unique Type System Ideas
- **Capability Types**: Functions and variables carry permissions enforced by the compiler.
- **Sealed/Ephemeral Objects**: Prevent accidental persistence or serialization unless explicitly allowed.

## 10. Security-Focused Tooling
- **Built-in Static Analysis**: Detect SQL injection, buffer overflows, format string issues, etc.
- **Secure Module Import**: Only cryptographically verified modules can be linked.
- **Audit Trails**: Optional metadata tracks which functions access sensitive data.

---

## Example Concept Syntax

```mysecurelang
secret key: Encrypted[256];
file f: Writable[capability=filesystem_write];

f.write(key); // compile error: secret cannot leave memory without explicit decryption

Treat security as a first-class citizen: types, threads, and modules carry security annotations enforced by the compiler.


---

If you want, I can also make a **clean, fully structured `.md` roadmap** showing which of these features could be implemented **early vs advanced**, like a full project plan. This could double as a pitch document for your language.  

Do you want me to make that next?
