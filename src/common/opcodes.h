#ifndef CLANGAX_OPCODES_H
#define CLANGAX_OPCODES_H

#include <cstdint>

namespace ClangAX {

    // Bytecode opcodes - must match between compiler and VM
    enum Opcode : uint8_t {
        // Constants & Literals (0x00-0x0F)
        NOP = 0x00,
        ICONST_0 = 0x01,
        ICONST_1 = 0x02,
        ICONST_M1 = 0x03,
        FCONST_0 = 0x04,
        FCONST_1 = 0x05,
        LDC = 0x06,

        // Load/Store (0x10-0x1F)
        ILOAD = 0x10,
        FLOAD = 0x11,
        ALOAD = 0x12,
        ISTORE = 0x13,
        FSTORE = 0x14,
        ASTORE = 0x15,

        // Arithmetic (0x20-0x2F)
        IADD = 0x20,
        ISUB = 0x21,
        IMUL = 0x22,
        IDIV = 0x23,
        IMOD = 0x24,
        INEG = 0x25,
        FADD = 0x26,
        FSUB = 0x27,
        FMUL = 0x28,
        FDIV = 0x29,

        // Comparison (0x30-0x3F)
        ICMP_EQ = 0x30,
        ICMP_NE = 0x31,
        ICMP_LT = 0x32,
        ICMP_GT = 0x33,
        ICMP_LE = 0x34,
        ICMP_GE = 0x35,

        // Control Flow (0x40-0x4F)
        GOTO = 0x40,
        IFEQ = 0x41,
        IFNE = 0x42,
        IFLT = 0x43,
        IFGT = 0x44,
        CALL = 0x45,
        RET = 0x46,
        IRET = 0x47,
        FRET = 0x48,

        // Array Operations (0x50-0x5F)
        NEWARRAY = 0x50,
        ARRAYLENGTH = 0x51,
        IALOAD = 0x52,
        IASTORE = 0x53,
        FALOAD = 0x54,
        FASTORE = 0x55,

        // I/O Operations (0x60-0x6F)
        PRINT_I = 0x60,
        PRINT_F = 0x61,
        PRINT_S = 0x62,
        PRINT_NL = 0x63,

        // GPU Hints (0x70-0x7F)
        GPU_HINT = 0x70,
        GPU_SYNC = 0x71,

        // ------------------------------
        // CAX v0.1+ opcodes (0x80-0xBF)
        // Dynamic, tagged values.
        // ------------------------------

        // Stack / locals
        POP = 0x80,
        LOAD = 0x81,   // u16 slot
        STORE = 0x82,  // u16 slot

        // Generic arithmetic
        ADD = 0x83,
        SUB = 0x84,
        MUL = 0x85,
        DIV = 0x86,
        BOOLIFY = 0x87,
        NEG = 0x88,

        // I/O
        PRINT = 0x90,        // pops 1 value, prints it
        WRITE_LOCAL = 0x91,  // u16 slot, reads stdin and stores converted value
        // Function arguments
        ARG_GET = 0x92,     // u16 index -> pushes argument value (or null)

        // Indexing (arrays + slices)
        INDEX_GET = 0xA0,    // base, index -> value
        INDEX_SET = 0xA1,    // base, index, value -> (no push)

        // Privileged exec dispatcher
        EXEC = 0xB0,         // u32 opNameConstIdx, u16 posArgc, u16 namedArgc

        // Manual memory management (0xC0-0xC4)
        ALLOC      = 0xC0,   // value -> ptr          (allocates a heap cell, stores value in it)
        FREE       = 0xC1,   // ptr  -> (void)        (marks the heap cell dead, increments generation)
        DEREF      = 0xC2,   // ptr  -> value         (reads the heap cell; aborts if dead/stale)
        DEREF_STORE= 0xC3,   // ptr, value -> (void)  (overwrites an existing heap cell)
        ADDR_OF    = 0xC4,   // u16 slot -> ptr       (returns a pointer to a local variable's heap copy)
    };

    // Constant pool types
    enum class ConstantType : uint8_t {
        INTEGER = 0x01,
        FLOAT = 0x02,
        STRING = 0x03,
        DOUBLE = 0x04,
    };

    // Bytecode file format constants
    constexpr uint32_t CAXB_MAGIC = 0x43415842;  // "CAXB"
    constexpr uint16_t CAXB_VERSION = 0x0002;     // v2: manual memory management opcodes

} // namespace ClangAX

#endif // CLANGAX_OPCODES_H