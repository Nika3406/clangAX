#ifndef CLANGAX_BYTECODE_H
#define CLANGAX_BYTECODE_H

#include "opcodes.h"
#include <vector>
#include <string>
#include <cstdint>

namespace ClangAX {

// Constant structure
struct Constant {
    ConstantType type;
    union {
        int32_t i;
        float f;
        double d;
    } value;
    std::string strValue;

    Constant() : type(ConstantType::INTEGER) {
        value.i = 0;
    }
};

// Function bytecode structure
struct FunctionBytecode {
    std::string name;
    uint16_t localCount;      // Number of local variables
    uint16_t maxStackDepth;   // Maximum stack depth needed
    std::vector<uint8_t> code; // Bytecode instructions

    FunctionBytecode() : localCount(0), maxStackDepth(0) {}
};

// Complete bytecode module
struct BytecodeModule {
    uint32_t magic;                          // CAXB magic number
    uint16_t version;                        // Format version
    std::vector<Constant> constantPool;      // Constant pool
    std::vector<FunctionBytecode> functions; // All functions
    uint32_t entryPoint;                     // Main function index

    BytecodeModule()
        : magic(CAXB_MAGIC),
          version(CAXB_VERSION),
          entryPoint(0) {}
};

// Bytecode writer helper
class BytecodeWriter {
public:
    void writeByte(uint8_t byte) {
        code.push_back(byte);
    }

    void writeShort(uint16_t val) {
        writeByte((val >> 8) & 0xFF);
        writeByte(val & 0xFF);
    }

    void writeInt(uint32_t val) {
        writeByte((val >> 24) & 0xFF);
        writeByte((val >> 16) & 0xFF);
        writeByte((val >> 8) & 0xFF);
        writeByte(val & 0xFF);
    }

    uint32_t getCurrentOffset() const {
        return static_cast<uint32_t>(code.size());
    }

    void patchInt(uint32_t offset, uint32_t val) {
        if (offset + 3 < code.size()) {
            code[offset] = (val >> 24) & 0xFF;
            code[offset + 1] = (val >> 16) & 0xFF;
            code[offset + 2] = (val >> 8) & 0xFF;
            code[offset + 3] = val & 0xFF;
        }
    }

    const std::vector<uint8_t>& getCode() const {
        return code;
    }

    void clear() {
        code.clear();
    }

private:
    std::vector<uint8_t> code;
};

// Bytecode reader helper
class BytecodeReader {
public:
    BytecodeReader(const std::vector<uint8_t>& data)
        : code(data), pos(0) {}

    uint8_t readByte() {
        return (pos < code.size()) ? code[pos++] : 0;
    }

    uint16_t readShort() {
        uint16_t val = (readByte() << 8) | readByte();
        return val;
    }

    uint32_t readInt() {
        uint32_t val = (readByte() << 24) | (readByte() << 16) |
                       (readByte() << 8) | readByte();
        return val;
    }

    uint32_t getPosition() const {
        return pos;
    }

    void setPosition(uint32_t newPos) {
        pos = newPos;
    }

    bool hasMore() const {
        return pos < code.size();
    }

private:
    const std::vector<uint8_t>& code;
    uint32_t pos;
};

} // namespace ClangAX

#endif // CLANGAX_BYTECODE_H