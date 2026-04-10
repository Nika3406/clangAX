#include "../common/opcodes.h"
#include "../common/bytecode.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

using namespace ClangAX;

const char* opcodeToString(uint8_t opcode) {
    switch(opcode) {
        // Legacy opcodes
        case NOP: return "NOP";
        case ICONST_0: return "ICONST_0";
        case ICONST_1: return "ICONST_1";
        case ICONST_M1: return "ICONST_M1";
        case FCONST_0: return "FCONST_0";
        case FCONST_1: return "FCONST_1";
        case LDC: return "LDC";
        case ILOAD: return "ILOAD";
        case FLOAD: return "FLOAD";
        case ALOAD: return "ALOAD";
        case ISTORE: return "ISTORE";
        case FSTORE: return "FSTORE";
        case ASTORE: return "ASTORE";
        case IADD: return "IADD";
        case ISUB: return "ISUB";
        case IMUL: return "IMUL";
        case IDIV: return "IDIV";
        case IMOD: return "IMOD";
        case INEG: return "INEG";
        case FADD: return "FADD";
        case FSUB: return "FSUB";
        case FMUL: return "FMUL";
        case FDIV: return "FDIV";
        case ICMP_EQ: return "ICMP_EQ";
        case ICMP_NE: return "ICMP_NE";
        case ICMP_LT: return "ICMP_LT";
        case ICMP_GT: return "ICMP_GT";
        case ICMP_LE: return "ICMP_LE";
        case ICMP_GE: return "ICMP_GE";
        case GOTO: return "GOTO";
        case IFEQ: return "IFEQ";
        case IFNE: return "IFNE";
        case IFLT: return "IFLT";
        case IFGT: return "IFGT";
        case CALL: return "CALL";
        case RET: return "RET";
        case IRET: return "IRET";
        case FRET: return "FRET";
        case NEWARRAY: return "NEWARRAY";
        case ARRAYLENGTH: return "ARRAYLENGTH";
        case IALOAD: return "IALOAD";
        case IASTORE: return "IASTORE";
        case FALOAD: return "FALOAD";
        case FASTORE: return "FASTORE";
        case PRINT_I: return "PRINT_I";
        case PRINT_F: return "PRINT_F";
        case PRINT_S: return "PRINT_S";
        case PRINT_NL: return "PRINT_NL";
        case GPU_HINT: return "GPU_HINT";
        case GPU_SYNC: return "GPU_SYNC";

        // v0.1+ opcodes
        case POP: return "POP";
        case LOAD: return "LOAD";
        case STORE: return "STORE";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case BOOLIFY: return "BOOLIFY";
        case NEG: return "NEG";
        case PRINT: return "PRINT";
        case WRITE_LOCAL: return "WRITE_LOCAL";
        case INDEX_GET: return "INDEX_GET";
        case INDEX_SET: return "INDEX_SET";
        case EXEC: return "EXEC";

        // v2: manual memory management opcodes
        case ALLOC:       return "ALLOC";
        case FREE:        return "FREE";
        case DEREF:       return "DEREF";
        case DEREF_STORE: return "DEREF_STORE";
        case ADDR_OF:     return "ADDR_OF";

        default: return "UNKNOWN";
    }
}

void disassemble(const std::string& filename, bool verbose) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open: " << filename << "\n";
        return;
    }

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != CAXB_MAGIC) {
        std::cerr << "ERROR: Invalid bytecode (bad magic)\n";
        return;
    }
    if (verbose) {
        std::cout << "Magic: 0x" << std::hex << magic << std::dec << "\n";
    }

    uint16_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (verbose) {
        std::cout << "Version: " << version << "\n";
    }

    uint32_t poolSize;
    file.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));
    std::cout << "Constant Pool (" << poolSize << " entries)\n";

    std::vector<Constant> constants;
    for (uint32_t i = 0; i < poolSize; i++) {
        Constant c;
        uint8_t type;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        c.type = static_cast<ConstantType>(type);

        std::cout << "#" << i << ": ";

        switch (c.type) {
            case ConstantType::INTEGER:
                file.read(reinterpret_cast<char*>(&c.value.i), sizeof(int32_t));
                std::cout << "INT " << c.value.i << std::endl;
                break;
            case ConstantType::FLOAT:
                file.read(reinterpret_cast<char*>(&c.value.f), sizeof(float));
                std::cout << "FLOAT " << c.value.f << std::endl;
                break;
            case ConstantType::STRING: {
                uint32_t len;
                file.read(reinterpret_cast<char*>(&len), sizeof(len));
                c.strValue.resize(len);
                file.read(&c.strValue[0], len);
                std::cout << "STRING \"" << c.strValue << "\"" << std::endl;
                break;
            }
            case ConstantType::DOUBLE:
                file.read(reinterpret_cast<char*>(&c.value.d), sizeof(double));
                std::cout << "DOUBLE " << c.value.d << std::endl;
                break;
        }
        constants.push_back(c);
    }

    // Read functions
    uint32_t funcCount;
    file.read(reinterpret_cast<char*>(&funcCount), sizeof(funcCount));
    std::cout << "\n=== Functions (" << funcCount << ") ===" << std::endl;

    for (uint32_t fidx = 0; fidx < funcCount; fidx++) {
        uint32_t nameIdx;
        file.read(reinterpret_cast<char*>(&nameIdx), sizeof(nameIdx));

        std::string funcName = "unknown";
        if (nameIdx < constants.size() && constants[nameIdx].type == ConstantType::STRING) {
            funcName = constants[nameIdx].strValue;
        }

        uint16_t localCount, stackSize;
        file.read(reinterpret_cast<char*>(&localCount), sizeof(localCount));
        file.read(reinterpret_cast<char*>(&stackSize), sizeof(stackSize));

        uint32_t codeLen;
        file.read(reinterpret_cast<char*>(&codeLen), sizeof(codeLen));

        std::vector<uint8_t> code(codeLen);
        file.read(reinterpret_cast<char*>(code.data()), codeLen);

        std::cout << "\nFunction #" << fidx << ": " << funcName << std::endl;
        std::cout << "  Locals: " << localCount << ", Stack: " << stackSize << std::endl;
        std::cout << "  Code (" << codeLen << " bytes):" << std::endl;

        // Disassemble bytecode
        uint32_t pc = 0;
        while (pc < code.size()) {
            std::cout << "    " << std::setw(4) << pc << ": ";
            uint8_t op = code[pc++];
            std::cout << opcodeToString(op);

            switch(op) {
                case LDC:
                case PRINT_S: {
                    if (pc + 4 <= code.size()) {
                        uint32_t idx = (code[pc] << 24) | (code[pc+1] << 16) |
                                      (code[pc+2] << 8) | code[pc+3];
                        pc += 4;
                        std::cout << " #" << idx;
                        if (idx < constants.size()) {
                            if (constants[idx].type == ConstantType::INTEGER)
                                std::cout << " (" << constants[idx].value.i << ")";
                            else if (constants[idx].type == ConstantType::STRING)
                                std::cout << " (\"" << constants[idx].strValue << "\")";
                            else if (constants[idx].type == ConstantType::FLOAT)
                                std::cout << " (" << constants[idx].value.f << ")";
                        }
                    }
                    break;
                }
                case LOAD:
                case STORE:
                case WRITE_LOCAL: {
                    if (pc + 2 <= code.size()) {
                        uint16_t slot = (code[pc] << 8) | code[pc+1];
                        pc += 2;
                        std::cout << " slot#" << slot;
                    }
                    break;
                }
                // ARG_GET carries a u16 argument index — must be decoded to keep pc aligned.
                case ARG_GET: {
                    if (pc + 2 <= code.size()) {
                        uint16_t idx = (code[pc] << 8) | code[pc+1];
                        pc += 2;
                        std::cout << " arg#" << idx;
                    }
                    break;
                }
                case ILOAD:
                case FLOAD:
                case ALOAD:
                case ISTORE:
                case FSTORE:
                case ASTORE: {
                    if (pc + 2 <= code.size()) {
                        uint16_t local = (code[pc] << 8) | code[pc+1];
                        pc += 2;
                        std::cout << " local#" << local;
                    }
                    break;
                }
                // ADDR_OF carries a u16 local slot number.
                case ADDR_OF: {
                    if (pc + 2 <= code.size()) {
                        uint16_t slot = (code[pc] << 8) | code[pc+1];
                        pc += 2;
                        std::cout << " slot#" << slot;
                    }
                    break;
                }
                // All branch opcodes take a 4-byte absolute target address.
                // IFNE, IFLE, IFGE were previously missing — their 4 operand bytes
                // would have been misread as opcodes, corrupting all output after them.
                case GOTO:
                case IFEQ:
                case IFNE:
                case IFLT:
                case IFGT:
                case ICMP_LE:   // 0x34 — reused as IFLE in jump encoding
                case ICMP_GE: { // 0x35 — reused as IFGE in jump encoding
                    if (pc + 4 <= code.size()) {
                        uint32_t offset = (code[pc] << 24) | (code[pc+1] << 16) |
                                         (code[pc+2] << 8) | code[pc+3];
                        pc += 4;
                        std::cout << " -> " << offset;
                    }
                    break;
                }
                case CALL: {
                    if (pc + 6 <= code.size()) {
                        uint32_t funcIdx = (code[pc] << 24) | (code[pc+1] << 16) |
                                          (code[pc+2] << 8) | code[pc+3];
                        uint16_t argCount = (code[pc+4] << 8) | code[pc+5];
                        pc += 6;
                        std::cout << " func#" << funcIdx << " args=" << argCount;
                    }
                    break;
                }
                case NEWARRAY: {
                    if (pc + 4 <= code.size()) {
                        uint32_t size = (code[pc] << 24) | (code[pc+1] << 16) |
                                       (code[pc+2] << 8) | code[pc+3];
                        pc += 4;
                        std::cout << " size=" << size;
                    }
                    break;
                }
                case EXEC: {
                    if (pc + 8 <= code.size()) {
                        uint32_t opIdx = (code[pc] << 24) | (code[pc+1] << 16) |
                                        (code[pc+2] << 8) | code[pc+3];
                        uint16_t posArgc   = (code[pc+4] << 8) | code[pc+5];
                        uint16_t namedArgc = (code[pc+6] << 8) | code[pc+7];
                        pc += 8;
                        std::cout << " op=#" << opIdx;
                        if (opIdx < constants.size() && constants[opIdx].type == ConstantType::STRING) {
                            std::cout << " (\"" << constants[opIdx].strValue << "\")";
                        }
                        std::cout << " pos=" << posArgc << " named=" << namedArgc;
                    }
                    break;
                }
            }
            std::cout << std::endl;
        }
    }

    // Read entry point
    uint32_t entryPoint;
    file.read(reinterpret_cast<char*>(&entryPoint), sizeof(entryPoint));
    std::cout << "\n=== Entry Point: Function #" << entryPoint << " ===" << std::endl;

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <bytecode.caxb> [--verbose]\n";
        return 1;
    }

    bool verbose = false;
    std::string bytecodePath;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (!arg.empty() && arg[0] != '-') {
            bytecodePath = arg;
        } else {
            std::cerr << "ERROR: Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (bytecodePath.empty()) {
        std::cerr << "ERROR: No bytecode file specified\n";
        return 1;
    }

    disassemble(bytecodePath, verbose);
    return 0;
}