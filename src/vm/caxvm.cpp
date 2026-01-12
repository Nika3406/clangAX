#include "../common/opcodes.h"
#include "../common/bytecode.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


#if defined(_WIN32)
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

using namespace ClangAX;

namespace {

enum class ValueTag : uint8_t {
    Null,
    Int,
    Float,
    Bool,
    String,
    Array,
    Handle,
    Slice,
};

enum class HandleKind : uint8_t {
    None = 0,
    GpuDevice,
    GpuContext,
    GpuBuffer,
    FileMap,
};

struct Value {
    ValueTag tag{ValueTag::Null};
    union {
        int32_t i;
        float f;
        uint8_t b;
        uint32_t id;
        uint32_t u;
    } as{};
    HandleKind handleKind{HandleKind::None};

    static Value null() { return Value{}; }
    static Value fromInt(int32_t v) { Value x; x.tag = ValueTag::Int; x.as.i = v; return x; }
    static Value fromFloat(float v) { Value x; x.tag = ValueTag::Float; x.as.f = v; return x; }
    static Value fromBool(bool v) { Value x; x.tag = ValueTag::Bool; x.as.b = static_cast<uint8_t>(v ? 1 : 0); return x; }
    static Value fromString(uint32_t sid) { Value x; x.tag = ValueTag::String; x.as.id = sid; return x; }
    static Value fromArray(uint32_t aid) { Value x; x.tag = ValueTag::Array; x.as.id = aid; return x; }
    static Value fromHandle(HandleKind k, uint32_t hid) { Value x; x.tag = ValueTag::Handle; x.handleKind = k; x.as.id = hid; return x; }
    static Value fromSlice(uint32_t sid) { Value x; x.tag = ValueTag::Slice; x.as.id = sid; return x; }
};

struct Array {
    std::vector<Value> elems;
};

struct Slice {
    HandleKind backing{HandleKind::None};
    uint32_t backingId{0};
    uint32_t baseOffset{0};
    uint32_t length{0};
    uint32_t stride{1};
};

struct FileMap {
    std::string path;
    std::vector<uint8_t> bytes;
};

struct GpuDevice {
    std::string backend; // "cuda" / "metal" / "opencl" / "none"
};

struct GpuContext { uint32_t deviceId{0}; };

struct GpuBuffer { uint32_t ctxId{0}; std::vector<uint8_t> bytes; };

// -------- dynamic library probing (cross-platform-ish) --------
static bool tryLoadAny(const std::vector<std::string>& names) {
    for (const auto& n : names) {
#if defined(_WIN32)
        HMODULE h = LoadLibraryA(n.c_str());
        if (h) { FreeLibrary(h); return true; }
#else
        void* h = dlopen(n.c_str(), RTLD_LAZY);
        if (h) { dlclose(h); return true; }
#endif
    }
    return false;
}

static bool hasCUDA() {
#if defined(_WIN32)
    return tryLoadAny({"nvcuda.dll", "cudart64_110.dll", "cudart64_120.dll"});
#elif defined(__APPLE__)
    // CUDA on macOS is effectively unavailable on modern systems.
    return false;
#else
    return tryLoadAny({"libcuda.so", "libcuda.so.1", "libcudart.so", "libcudart.so.11.0", "libcudart.so.12"});
#endif
}

static bool hasOpenCL() {
#if defined(_WIN32)
    return tryLoadAny({"OpenCL.dll"});
#elif defined(__APPLE__)
    // OpenCL framework exists, but is deprecated. Still detectable.
    return tryLoadAny({"/System/Library/Frameworks/OpenCL.framework/OpenCL"});
#else
    return tryLoadAny({"libOpenCL.so", "libOpenCL.so.1"});
#endif
}

static bool hasMetal() {
#if defined(__APPLE__)
    return tryLoadAny({"/System/Library/Frameworks/Metal.framework/Metal"});
#else
    return false;
#endif
}

static std::string autoSelectGpuBackend() {
    if (hasCUDA()) return "cuda";
    if (hasMetal()) return "metal";
    if (hasOpenCL()) return "opencl";
    return "none";
}

static std::string trim(const std::string& s) {
    size_t a = 0; while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) a++;
    size_t b = s.size(); while (b > a && std::isspace(static_cast<unsigned char>(s[b-1]))) b--;
    return s.substr(a, b - a);
}

} // namespace

class ClangaxVM {
public:
    bool loadBytecode(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "VM error: Failed to open bytecode file: " << filename << "\n";
            return false;
        }

        uint32_t magic = 0;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != CAXB_MAGIC) {
            std::cerr << "VM error: Invalid bytecode file (bad magic number)\n";
            return false;
        }

        uint16_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));

        // Constant pool
        uint32_t poolSize = 0;
        file.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));
        constantPool.clear();
        constantPool.reserve(poolSize);

        for (uint32_t i = 0; i < poolSize; i++) {
            Constant c;
            uint8_t type = 0;
            file.read(reinterpret_cast<char*>(&type), sizeof(type));
            c.type = static_cast<ConstantType>(type);

            switch (c.type) {
                case ConstantType::INTEGER:
                    file.read(reinterpret_cast<char*>(&c.value.i), sizeof(int32_t));
                    break;
                case ConstantType::FLOAT:
                    file.read(reinterpret_cast<char*>(&c.value.f), sizeof(float));
                    break;
                case ConstantType::STRING: {
                    uint32_t len = 0;
                    file.read(reinterpret_cast<char*>(&len), sizeof(len));
                    c.strValue.resize(len);
                    file.read(&c.strValue[0], len);
                    break;
                }
                case ConstantType::DOUBLE:
                    file.read(reinterpret_cast<char*>(&c.value.d), sizeof(double));
                    break;
            }
            constantPool.push_back(c);
        }

        // Build string heap mapping so string values are uniform at runtime
        constStringToHeap.assign(constantPool.size(), std::numeric_limits<uint32_t>::max());
        stringHeap.clear();
        for (size_t i = 0; i < constantPool.size(); i++) {
            if (constantPool[i].type == ConstantType::STRING) {
                constStringToHeap[i] = static_cast<uint32_t>(stringHeap.size());
                stringHeap.push_back(constantPool[i].strValue);
            }
        }

        // Functions
        uint32_t funcCount = 0;
        file.read(reinterpret_cast<char*>(&funcCount), sizeof(funcCount));
        functions.clear();
        functions.reserve(funcCount);

        for (uint32_t i = 0; i < funcCount; i++) {
            Function func;
            uint32_t nameIdx = 0;
            file.read(reinterpret_cast<char*>(&nameIdx), sizeof(nameIdx));
            if (nameIdx < constantPool.size() && constantPool[nameIdx].type == ConstantType::STRING) {
                func.name = constantPool[nameIdx].strValue;
            }
            file.read(reinterpret_cast<char*>(&func.localCount), sizeof(func.localCount));
            file.read(reinterpret_cast<char*>(&func.stackSize), sizeof(func.stackSize));

            uint32_t codeLen = 0;
            file.read(reinterpret_cast<char*>(&codeLen), sizeof(codeLen));
            func.code.resize(codeLen);
            file.read(reinterpret_cast<char*>(func.code.data()), codeLen);
            functions.push_back(std::move(func));
        }

        file.read(reinterpret_cast<char*>(&entryPoint), sizeof(entryPoint));
        return true;
    }

    void printStats() {
        std::cout << "\n=== VM Statistics ===\n";
        std::cout << "Constant Pool Size: " << constantPool.size() << "\n";
        std::cout << "Functions: " << functions.size() << "\n";
        if (entryPoint < functions.size()) {
            std::cout << "Entry Point: " << entryPoint << " (" << functions[entryPoint].name << ")\n";
        }
    }

    void execute() {
        if (entryPoint >= functions.size()) {
            std::cerr << "VM error: Invalid entry point\n";
            return;
        }
        runFunction(entryPoint);
    }

private:
    struct Function {
        std::string name;
        uint16_t localCount{0};
        uint16_t stackSize{0};
        std::vector<uint8_t> code;
    };

    struct Frame {
        uint32_t funcIdx{0};
        uint32_t returnPC{0};
        std::vector<Value> locals;
        std::vector<Value> stack;
        std::vector<Value> args; // NEW: positional args
    };

    std::vector<Constant> constantPool;
    std::vector<Function> functions;
    uint32_t entryPoint{0};

    // Heaps
    std::vector<std::string> stringHeap;
    std::vector<uint32_t> constStringToHeap;
    std::vector<std::unique_ptr<Array>> arrays;
    std::vector<std::unique_ptr<Slice>> slices;
    std::vector<std::unique_ptr<FileMap>> fileMaps;
    std::vector<std::unique_ptr<GpuDevice>> gpuDevices;
    std::vector<std::unique_ptr<GpuContext>> gpuContexts;
    std::vector<std::unique_ptr<GpuBuffer>> gpuBuffers;

    // ------- helpers -------
    static uint16_t readU16(const std::vector<uint8_t>& code, uint32_t& pc) {
        uint16_t v = static_cast<uint16_t>((code[pc] << 8) | code[pc + 1]);
        pc += 2;
        return v;
    }
    static uint32_t readU32(const std::vector<uint8_t>& code, uint32_t& pc) {
        uint32_t v = (code[pc] << 24) | (code[pc + 1] << 16) | (code[pc + 2] << 8) | code[pc + 3];
        pc += 4;
        return v;
    }

    std::string stringify(const Value& v) {
        switch (v.tag) {
            case ValueTag::Null: return "null";
            case ValueTag::Int: return std::to_string(v.as.i);
            case ValueTag::Float: {
                std::ostringstream os; os << v.as.f; return os.str();
            }
            case ValueTag::Bool: return v.as.b ? "true" : "false";
            case ValueTag::String:
                if (v.as.id < stringHeap.size()) return stringHeap[v.as.id];
                return "";
            case ValueTag::Array:
                return "<array len=" + std::to_string(arrays.at(v.as.id)->elems.size()) + ">";
            case ValueTag::Handle: {
                std::string k = "handle";
                switch (v.handleKind) {
                    case HandleKind::GpuDevice: k = "gpu.device"; break;
                    case HandleKind::GpuContext: k = "gpu.ctx"; break;
                    case HandleKind::GpuBuffer: k = "gpu.buf"; break;
                    case HandleKind::FileMap: k = "fs.map"; break;
                    default: break;
                }
                return "<" + k + "#" + std::to_string(v.as.id) + ">";
            }
            case ValueTag::Slice: {
                const auto& s = *slices.at(v.as.id);
                return "<slice len=" + std::to_string(s.length) + ">";
            }
        }
        return "<unknown>";
    }

    static bool isTruthy(const Value& v) {
        switch (v.tag) {
            case ValueTag::Null: return false;
            case ValueTag::Bool: return v.as.b != 0;
            case ValueTag::Int: return v.as.i != 0;
            case ValueTag::Float: return v.as.f != 0.0f;
            case ValueTag::String: return true;
            case ValueTag::Array: return true;
            case ValueTag::Handle: return true;
            case ValueTag::Slice: return true;
        }
        return false;
    }

    Value convertInput(const std::string& raw, const Value& current) {
        std::string s = trim(raw);
        if (current.tag == ValueTag::Int) {
            try { return Value::fromInt(std::stoi(s)); } catch (...) { return Value::fromInt(0); }
        }
        if (current.tag == ValueTag::Float) {
            try { return Value::fromFloat(std::stof(s)); } catch (...) { return Value::fromFloat(0.0f); }
        }
        if (current.tag == ValueTag::Bool) {
            if (s == "true" || s == "1") return Value::fromBool(true);
            if (s == "false" || s == "0") return Value::fromBool(false);
            return Value::fromBool(false);
        }
        // default: string
        uint32_t sid = static_cast<uint32_t>(stringHeap.size());
        stringHeap.push_back(s);
        return Value::fromString(sid);
    }

    Value doAdd(const Value& a, const Value& b) {
        if (a.tag == ValueTag::String || b.tag == ValueTag::String) {
            std::string res = stringify(a) + stringify(b);
            uint32_t sid = static_cast<uint32_t>(stringHeap.size());
            stringHeap.push_back(res);
            return Value::fromString(sid);
        }
        if (a.tag == ValueTag::Float || b.tag == ValueTag::Float) {
            float af = (a.tag == ValueTag::Float) ? a.as.f : static_cast<float>(a.as.i);
            float bf = (b.tag == ValueTag::Float) ? b.as.f : static_cast<float>(b.as.i);
            return Value::fromFloat(af + bf);
        }
        return Value::fromInt(a.as.i + b.as.i);
    }

    Value doSub(const Value& a, const Value& b) {
        if (a.tag == ValueTag::Float || b.tag == ValueTag::Float) {
            float af = (a.tag == ValueTag::Float) ? a.as.f : static_cast<float>(a.as.i);
            float bf = (b.tag == ValueTag::Float) ? b.as.f : static_cast<float>(b.as.i);
            return Value::fromFloat(af - bf);
        }
        return Value::fromInt(a.as.i - b.as.i);
    }

    Value doMul(const Value& a, const Value& b) {
        if (a.tag == ValueTag::Float || b.tag == ValueTag::Float) {
            float af = (a.tag == ValueTag::Float) ? a.as.f : static_cast<float>(a.as.i);
            float bf = (b.tag == ValueTag::Float) ? b.as.f : static_cast<float>(b.as.i);
            return Value::fromFloat(af * bf);
        }
        return Value::fromInt(a.as.i * b.as.i);
    }

    Value doDiv(const Value& a, const Value& b) {
        float bf = (b.tag == ValueTag::Float) ? b.as.f : static_cast<float>(b.as.i);
        if (bf == 0.0f) return Value::null();
        float af = (a.tag == ValueTag::Float) ? a.as.f : static_cast<float>(a.as.i);
        return Value::fromFloat(af / bf);
    }

    // ------- exec dispatcher -------
    Value dispatchExec(const std::string& op,
                       const std::vector<Value>& pos,
                       const std::map<std::string, Value>& named) {

        // =============================================================================
        // _MATH LIBRARY IMPLEMENTATIONS
        // =============================================================================

        // Basic operations
        if (op == "math.sqrt") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::sqrt(x));
        }

        if (op == "math.pow") {
            if (pos.size() < 2) return Value::null();
            float base = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            float exp = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
            return Value::fromFloat(std::pow(base, exp));
        }

        if (op == "math.abs") {
            if (pos.empty()) return Value::null();
            if (pos[0].tag == ValueTag::Float) {
                return Value::fromFloat(std::fabs(pos[0].as.f));
            }
            return Value::fromInt(std::abs(pos[0].as.i));
        }

        // Trigonometric
        if (op == "math.sin") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::sin(x));
        }

        if (op == "math.cos") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::cos(x));
        }

        if (op == "math.tan") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::tan(x));
        }

        if (op == "math.asin") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::asin(x));
        }

        if (op == "math.acos") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::acos(x));
        }

        if (op == "math.atan") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::atan(x));
        }

        if (op == "math.atan2") {
            if (pos.size() < 2) return Value::null();
            float y = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            float x = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
            return Value::fromFloat(std::atan2(y, x));
        }

        // Hyperbolic
        if (op == "math.sinh") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::sinh(x));
        }

        if (op == "math.cosh") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::cosh(x));
        }

        if (op == "math.tanh") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::tanh(x));
        }

        // Exponential and logarithmic
        if (op == "math.exp") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::exp(x));
        }

        if (op == "math.log") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::log(x));
        }

        if (op == "math.log10") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::log10(x));
        }

        if (op == "math.log2") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::log2(x));
        }

        // Rounding
        if (op == "math.floor") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::floor(x));
        }

        if (op == "math.ceil") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::ceil(x));
        }

        if (op == "math.round") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::round(x));
        }

        if (op == "math.trunc") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::trunc(x));
        }

        if (op == "math.fmod") {
            if (pos.size() < 2) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            float y = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
            return Value::fromFloat(std::fmod(x, y));
        }

        // Min/Max
        if (op == "math.min") {
            if (pos.size() < 2) return Value::null();
            if (pos[0].tag == ValueTag::Float || pos[1].tag == ValueTag::Float) {
                float a = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
                float b = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
                return Value::fromFloat(std::min(a, b));
            }
            return Value::fromInt(std::min(pos[0].as.i, pos[1].as.i));
        }

        if (op == "math.max") {
            if (pos.size() < 2) return Value::null();
            if (pos[0].tag == ValueTag::Float || pos[1].tag == ValueTag::Float) {
                float a = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
                float b = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
                return Value::fromFloat(std::max(a, b));
            }
            return Value::fromInt(std::max(pos[0].as.i, pos[1].as.i));
        }

        // Special functions
        if (op == "math.hypot") {
            if (pos.size() < 2) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            float y = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
            return Value::fromFloat(std::hypot(x, y));
        }

        if (op == "math.cbrt") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::cbrt(x));
        }

        // Algebra
        if (op == "math.quad") {
            if (pos.size() < 3) return Value::null();
            float a = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            float b = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);
            float c = (pos[2].tag == ValueTag::Float) ? pos[2].as.f : static_cast<float>(pos[2].as.i);

            float discriminant = b * b - 4.0f * a * c;
            auto arr = std::make_unique<Array>();

            if (discriminant < 0) {
                arr->elems.resize(0);
            } else if (discriminant == 0) {
                arr->elems.push_back(Value::fromFloat(-b / (2.0f * a)));
            } else {
                float sqrtDisc = std::sqrt(discriminant);
                arr->elems.push_back(Value::fromFloat((-b + sqrtDisc) / (2.0f * a)));
                arr->elems.push_back(Value::fromFloat((-b - sqrtDisc) / (2.0f * a)));
            }

            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(arr));
            return Value::fromArray(id);
        }

        if (op == "math.gcd") {
            if (pos.size() < 2) return Value::null();
            int32_t a = (pos[0].tag == ValueTag::Int) ? pos[0].as.i : static_cast<int32_t>(pos[0].as.f);
            int32_t b = (pos[1].tag == ValueTag::Int) ? pos[1].as.i : static_cast<int32_t>(pos[1].as.f);
            return Value::fromInt(std::gcd(a, b));
        }

        if (op == "math.lcm") {
            if (pos.size() < 2) return Value::null();
            int32_t a = (pos[0].tag == ValueTag::Int) ? pos[0].as.i : static_cast<int32_t>(pos[0].as.f);
            int32_t b = (pos[1].tag == ValueTag::Int) ? pos[1].as.i : static_cast<int32_t>(pos[1].as.f);
            return Value::fromInt(std::lcm(a, b));
        }

        if (op == "math.factorial") {
            if (pos.empty()) return Value::null();
            int32_t n = (pos[0].tag == ValueTag::Int) ? pos[0].as.i : static_cast<int32_t>(pos[0].as.f);
            if (n < 0) return Value::null();
            int32_t result = 1;
            for (int32_t i = 2; i <= n; i++) {
                result *= i;
            }
            return Value::fromInt(result);
        }

        // Statistics
        if (op == "math.mean") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();

            float sum = 0.0f;
            for (const auto& v : arr) {
                sum += (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
            }
            return Value::fromFloat(sum / arr.size());
        }

        if (op == "math.variance") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();

            float mean = 0.0f;
            for (const auto& v : arr) {
                mean += (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
            }
            mean /= arr.size();

            float variance = 0.0f;
            for (const auto& v : arr) {
                float x = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                variance += (x - mean) * (x - mean);
            }
            return Value::fromFloat(variance / arr.size());
        }

        if (op == "math.stddev") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();

            float mean = 0.0f;
            for (const auto& v : arr) {
                mean += (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
            }
            mean /= arr.size();

            float variance = 0.0f;
            for (const auto& v : arr) {
                float x = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                variance += (x - mean) * (x - mean);
            }
            return Value::fromFloat(std::sqrt(variance / arr.size()));
        }

        // =============================================================================
        // _DATASTR LIBRARY IMPLEMENTATIONS
        // =============================================================================

        // Stack (using arrays internally)
        if (op == "datastr.stack_new") {
            auto arr = std::make_unique<Array>();
            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(arr));
            return Value::fromArray(id);
        }

        if (op == "datastr.stack_push") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array) return Value::null();
            arrays.at(pos[0].as.id)->elems.push_back(pos[1]);
            return Value::null();
        }

        if (op == "datastr.stack_pop") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();
            Value v = arr.back();
            arr.pop_back();
            return v;
        }

        if (op == "datastr.stack_peek") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();
            return arr.back();
        }

        if (op == "datastr.stack_empty") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::fromBool(true);
            return Value::fromBool(arrays.at(pos[0].as.id)->elems.empty());
        }

        // Queue (using arrays)
        if (op == "datastr.queue_new") {
            auto arr = std::make_unique<Array>();
            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(arr));
            return Value::fromArray(id);
        }

        if (op == "datastr.queue_enqueue") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array) return Value::null();
            arrays.at(pos[0].as.id)->elems.push_back(pos[1]);
            return Value::null();
        }

        if (op == "datastr.queue_dequeue") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();
            Value v = arr.front();
            arr.erase(arr.begin());
            return v;
        }

        if (op == "datastr.queue_front") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;
            if (arr.empty()) return Value::null();
            return arr.front();
        }

        if (op == "datastr.queue_empty") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::fromBool(true);
            return Value::fromBool(arrays.at(pos[0].as.id)->elems.empty());
        }

        // Heap (min-heap using arrays)
        if (op == "datastr.heap_new") {
            auto arr = std::make_unique<Array>();
            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(arr));
            return Value::fromArray(id);
        }

        if (op == "datastr.heap_insert") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array) return Value::null();
            auto& heap = arrays.at(pos[0].as.id)->elems;
            heap.push_back(pos[1]);

            // Bubble up
            size_t idx = heap.size() - 1;
            while (idx > 0) {
                size_t parent = (idx - 1) / 2;
                float childVal = (heap[idx].tag == ValueTag::Float) ? heap[idx].as.f : static_cast<float>(heap[idx].as.i);
                float parentVal = (heap[parent].tag == ValueTag::Float) ? heap[parent].as.f : static_cast<float>(heap[parent].as.i);

                if (childVal >= parentVal) break;
                std::swap(heap[idx], heap[parent]);
                idx = parent;
            }
            return Value::null();
        }

        if (op == "datastr.heap_extract") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& heap = arrays.at(pos[0].as.id)->elems;
            if (heap.empty()) return Value::null();

            Value min = heap[0];
            heap[0] = heap.back();
            heap.pop_back();

            // Bubble down
            size_t idx = 0;
            while (true) {
                size_t left = 2 * idx + 1;
                size_t right = 2 * idx + 2;
                size_t smallest = idx;

                if (left < heap.size()) {
                    float leftVal = (heap[left].tag == ValueTag::Float) ? heap[left].as.f : static_cast<float>(heap[left].as.i);
                    float smallestVal = (heap[smallest].tag == ValueTag::Float) ? heap[smallest].as.f : static_cast<float>(heap[smallest].as.i);
                    if (leftVal < smallestVal) smallest = left;
                }

                if (right < heap.size()) {
                    float rightVal = (heap[right].tag == ValueTag::Float) ? heap[right].as.f : static_cast<float>(heap[right].as.i);
                    float smallestVal = (heap[smallest].tag == ValueTag::Float) ? heap[smallest].as.f : static_cast<float>(heap[smallest].as.i);
                    if (rightVal < smallestVal) smallest = right;
                }

                if (smallest == idx) break;
                std::swap(heap[idx], heap[smallest]);
                idx = smallest;
            }

            return min;
        }

        // =============================================================================
        // _ALG LIBRARY IMPLEMENTATIONS
        // =============================================================================

        // Sorting algorithms
        if (op == "alg.quicksort") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;

            std::sort(arr.begin(), arr.end(), [](const Value& a, const Value& b) {
                float av = (a.tag == ValueTag::Float) ? a.as.f : static_cast<float>(a.as.i);
                float bv = (b.tag == ValueTag::Float) ? b.as.f : static_cast<float>(b.as.i);
                return av < bv;
            });
            return Value::null();
        }

        if (op == "alg.mergesort") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;

            std::stable_sort(arr.begin(), arr.end(), [](const Value& a, const Value& b) {
                float av = (a.tag == ValueTag::Float) ? a.as.f : static_cast<float>(a.as.i);
                float bv = (b.tag == ValueTag::Float) ? b.as.f : static_cast<float>(b.as.i);
                return av < bv;
            });
            return Value::null();
        }

        // Search algorithms
        if (op == "alg.binary_search") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array) return Value::fromInt(-1);
            auto& arr = arrays.at(pos[0].as.id)->elems;
            float target = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);

            int32_t left = 0, right = static_cast<int32_t>(arr.size()) - 1;
            while (left <= right) {
                int32_t mid = left + (right - left) / 2;
                float midVal = (arr[mid].tag == ValueTag::Float) ? arr[mid].as.f : static_cast<float>(arr[mid].as.i);

                if (midVal == target) return Value::fromInt(mid);
                if (midVal < target) left = mid + 1;
                else right = mid - 1;
            }
            return Value::fromInt(-1);
        }

        if (op == "alg.linear_search") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array) return Value::fromInt(-1);
            auto& arr = arrays.at(pos[0].as.id)->elems;
            float target = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);

            for (size_t i = 0; i < arr.size(); i++) {
                float val = (arr[i].tag == ValueTag::Float) ? arr[i].as.f : static_cast<float>(arr[i].as.i);
                if (val == target) return Value::fromInt(static_cast<int32_t>(i));
            }
            return Value::fromInt(-1);
        }

        // Shuffle
        if (op == "alg.shuffle") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& arr = arrays.at(pos[0].as.id)->elems;

            for (size_t i = arr.size() - 1; i > 0; i--) {
                size_t j = rand() % (i + 1);
                std::swap(arr[i], arr[j]);
            }
            return Value::null();
        }

        // =============================================================================
        // _ML LIBRARY IMPLEMENTATIONS
        // =============================================================================

        // Linear Regression
        if (op == "ml.linear_fit") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array || pos[1].tag != ValueTag::Array) {
                return Value::null();
            }

            auto& x = arrays.at(pos[0].as.id)->elems;
            auto& y = arrays.at(pos[1].as.id)->elems;

            if (x.size() != y.size() || x.empty()) return Value::null();

            // Calculate slope and intercept: y = mx + b
            float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
            size_t n = x.size();

            for (size_t i = 0; i < n; i++) {
                float xi = (x[i].tag == ValueTag::Float) ? x[i].as.f : static_cast<float>(x[i].as.i);
                float yi = (y[i].tag == ValueTag::Float) ? y[i].as.f : static_cast<float>(y[i].as.i);

                sumX += xi;
                sumY += yi;
                sumXY += xi * yi;
                sumX2 += xi * xi;
            }

            float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
            float intercept = (sumY - slope * sumX) / n;

            // Return model as array [slope, intercept]
            auto model = std::make_unique<Array>();
            model->elems.push_back(Value::fromFloat(slope));
            model->elems.push_back(Value::fromFloat(intercept));

            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(model));
            return Value::fromArray(id);
        }

        if (op == "ml.linear_predict") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array) return Value::null();

            auto& model = arrays.at(pos[0].as.id)->elems;
            if (model.size() < 2) return Value::null();

            float slope = (model[0].tag == ValueTag::Float) ? model[0].as.f : static_cast<float>(model[0].as.i);
            float intercept = (model[1].tag == ValueTag::Float) ? model[1].as.f : static_cast<float>(model[1].as.i);
            float x = (pos[1].tag == ValueTag::Float) ? pos[1].as.f : static_cast<float>(pos[1].as.i);

            return Value::fromFloat(slope * x + intercept);
        }

        // K-Nearest Neighbors
        if (op == "ml.knn_fit") {
            if (pos.size() < 3 || pos[0].tag != ValueTag::Array || pos[1].tag != ValueTag::Array) {
                return Value::null();
            }

            // Store training data and k in a model array
            auto model = std::make_unique<Array>();
            model->elems.push_back(pos[0]);  // x training data
            model->elems.push_back(pos[1]);  // y training labels
            model->elems.push_back(pos[2]);  // k value

            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(model));
            return Value::fromArray(id);
        }

        if (op == "ml.knn_predict") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array || pos[1].tag != ValueTag::Array) {
                return Value::null();
            }

            auto& model = arrays.at(pos[0].as.id)->elems;
            if (model.size() < 3) return Value::null();

            auto& xTrain = arrays.at(model[0].as.id)->elems;
            auto& yTrain = arrays.at(model[1].as.id)->elems;
            int32_t k = (model[2].tag == ValueTag::Int) ? model[2].as.i : 3;

            auto& testPoint = arrays.at(pos[1].as.id)->elems;

            // Calculate distances
            std::vector<std::pair<float, float>> distances;

            for (size_t i = 0; i < xTrain.size(); i++) {
                if (xTrain[i].tag != ValueTag::Array) continue;
                auto& trainPoint = arrays.at(xTrain[i].as.id)->elems;

                float dist = 0.0f;
                for (size_t j = 0; j < std::min(trainPoint.size(), testPoint.size()); j++) {
                    float ti = (testPoint[j].tag == ValueTag::Float) ? testPoint[j].as.f : static_cast<float>(testPoint[j].as.i);
                    float pi = (trainPoint[j].tag == ValueTag::Float) ? trainPoint[j].as.f : static_cast<float>(trainPoint[j].as.i);
                    dist += (ti - pi) * (ti - pi);
                }
                dist = std::sqrt(dist);

                float label = (yTrain[i].tag == ValueTag::Float) ? yTrain[i].as.f : static_cast<float>(yTrain[i].as.i);
                distances.push_back({dist, label});
            }

            // Sort by distance and get k nearest
            std::sort(distances.begin(), distances.end());

            // Majority vote for classification
            std::unordered_map<int32_t, int32_t> votes;
            for (int32_t i = 0; i < k && i < static_cast<int32_t>(distances.size()); i++) {
                int32_t label = static_cast<int32_t>(distances[i].second);
                votes[label]++;
            }

            int32_t maxVotes = 0;
            int32_t prediction = 0;
            for (const auto& [label, count] : votes) {
                if (count > maxVotes) {
                    maxVotes = count;
                    prediction = label;
                }
            }

            return Value::fromInt(prediction);
        }

        // K-Means Clustering
        if (op == "ml.kmeans_fit") {
            if (pos.size() < 3 || pos[0].tag != ValueTag::Array) return Value::null();

            auto& data = arrays.at(pos[0].as.id)->elems;
            int32_t k = (pos[1].tag == ValueTag::Int) ? pos[1].as.i : 3;
            int32_t maxIter = (pos[2].tag == ValueTag::Int) ? pos[2].as.i : 100;

            if (data.empty() || k <= 0) return Value::null();

            // Initialize centroids randomly
            std::vector<std::vector<float>> centroids(k);
            for (int32_t i = 0; i < k && i < static_cast<int32_t>(data.size()); i++) {
                if (data[i].tag != ValueTag::Array) continue;
                auto& point = arrays.at(data[i].as.id)->elems;
                for (const auto& v : point) {
                    float val = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                    centroids[i].push_back(val);
                }
            }

            // Run k-means iterations
            for (int32_t iter = 0; iter < maxIter; iter++) {
                std::vector<std::vector<size_t>> clusters(k);

                // Assign points to nearest centroid
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].tag != ValueTag::Array) continue;
                    auto& point = arrays.at(data[i].as.id)->elems;

                    float minDist = std::numeric_limits<float>::max();
                    int32_t bestCluster = 0;

                    for (int32_t c = 0; c < k; c++) {
                        float dist = 0.0f;
                        for (size_t d = 0; d < std::min(point.size(), centroids[c].size()); d++) {
                            float pv = (point[d].tag == ValueTag::Float) ? point[d].as.f : static_cast<float>(point[d].as.i);
                            dist += (pv - centroids[c][d]) * (pv - centroids[c][d]);
                        }
                        dist = std::sqrt(dist);

                        if (dist < minDist) {
                            minDist = dist;
                            bestCluster = c;
                        }
                    }

                    clusters[bestCluster].push_back(i);
                }

                // Update centroids
                for (int32_t c = 0; c < k; c++) {
                    if (clusters[c].empty()) continue;

                    size_t dim = centroids[c].size();
                    std::vector<float> newCentroid(dim, 0.0f);

                    for (size_t idx : clusters[c]) {
                        if (data[idx].tag != ValueTag::Array) continue;
                        auto& point = arrays.at(data[idx].as.id)->elems;

                        for (size_t d = 0; d < std::min(dim, point.size()); d++) {
                            float pv = (point[d].tag == ValueTag::Float) ? point[d].as.f : static_cast<float>(point[d].as.i);
                            newCentroid[d] += pv;
                        }
                    }

                    for (size_t d = 0; d < dim; d++) {
                        newCentroid[d] /= clusters[c].size();
                    }

                    centroids[c] = newCentroid;
                }
            }

            // Store centroids in model
            auto model = std::make_unique<Array>();
            for (const auto& centroid : centroids) {
                auto carr = std::make_unique<Array>();
                for (float v : centroid) {
                    carr->elems.push_back(Value::fromFloat(v));
                }
                uint32_t cid = static_cast<uint32_t>(arrays.size());
                arrays.push_back(std::move(carr));
                model->elems.push_back(Value::fromArray(cid));
            }

            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(model));
            return Value::fromArray(id);
        }

        // Activation functions
        if (op == "ml.sigmoid") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(1.0f / (1.0f + std::exp(-x)));
        }

        if (op == "ml.relu") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::max(0.0f, x));
        }

        if (op == "ml.tanh_act") {
            if (pos.empty()) return Value::null();
            float x = (pos[0].tag == ValueTag::Float) ? pos[0].as.f : static_cast<float>(pos[0].as.i);
            return Value::fromFloat(std::tanh(x));
        }

        // Loss functions
        if (op == "ml.mse") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array || pos[1].tag != ValueTag::Array) {
                return Value::null();
            }

            auto& yTrue = arrays.at(pos[0].as.id)->elems;
            auto& yPred = arrays.at(pos[1].as.id)->elems;

            if (yTrue.size() != yPred.size()) return Value::null();

            float mse = 0.0f;
            for (size_t i = 0; i < yTrue.size(); i++) {
                float t = (yTrue[i].tag == ValueTag::Float) ? yTrue[i].as.f : static_cast<float>(yTrue[i].as.i);
                float p = (yPred[i].tag == ValueTag::Float) ? yPred[i].as.f : static_cast<float>(yPred[i].as.i);
                mse += (t - p) * (t - p);
            }

            return Value::fromFloat(mse / yTrue.size());
        }

        // Metrics
        if (op == "ml.r2_score") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array || pos[1].tag != ValueTag::Array) {
                return Value::null();
            }

            auto& yTrue = arrays.at(pos[0].as.id)->elems;
            auto& yPred = arrays.at(pos[1].as.id)->elems;

            if (yTrue.size() != yPred.size()) return Value::null();

            float meanTrue = 0.0f;
            for (const auto& v : yTrue) {
                meanTrue += (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
            }
            meanTrue /= yTrue.size();

            float ssRes = 0.0f, ssTot = 0.0f;
            for (size_t i = 0; i < yTrue.size(); i++) {
                float t = (yTrue[i].tag == ValueTag::Float) ? yTrue[i].as.f : static_cast<float>(yTrue[i].as.i);
                float p = (yPred[i].tag == ValueTag::Float) ? yPred[i].as.f : static_cast<float>(yPred[i].as.i);
                ssRes += (t - p) * (t - p);
                ssTot += (t - meanTrue) * (t - meanTrue);
            }

            return Value::fromFloat(1.0f - (ssRes / ssTot));
        }

        if (op == "ml.accuracy") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::Array || pos[1].tag != ValueTag::Array) {
                return Value::null();
            }

            auto& yTrue = arrays.at(pos[0].as.id)->elems;
            auto& yPred = arrays.at(pos[1].as.id)->elems;

            if (yTrue.size() != yPred.size()) return Value::null();

            int32_t correct = 0;
            for (size_t i = 0; i < yTrue.size(); i++) {
                int32_t t = (yTrue[i].tag == ValueTag::Int) ? yTrue[i].as.i : static_cast<int32_t>(yTrue[i].as.f);
                int32_t p = (yPred[i].tag == ValueTag::Int) ? yPred[i].as.i : static_cast<int32_t>(yPred[i].as.f);
                if (t == p) correct++;
            }

            return Value::fromFloat(static_cast<float>(correct) / yTrue.size());
        }

        // Data preprocessing
        if (op == "ml.normalize") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& data = arrays.at(pos[0].as.id)->elems;

            if (data.empty()) return pos[0];

            float minVal = std::numeric_limits<float>::max();
            float maxVal = std::numeric_limits<float>::lowest();

            for (const auto& v : data) {
                float val = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                minVal = std::min(minVal, val);
                maxVal = std::max(maxVal, val);
            }

            float range = maxVal - minVal;
            if (range == 0) return pos[0];

            auto normalized = std::make_unique<Array>();
            for (const auto& v : data) {
                float val = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                normalized->elems.push_back(Value::fromFloat((val - minVal) / range));
            }

            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(normalized));
            return Value::fromArray(id);
        }

        if (op == "ml.standardize") {
            if (pos.empty() || pos[0].tag != ValueTag::Array) return Value::null();
            auto& data = arrays.at(pos[0].as.id)->elems;

            if (data.empty()) return pos[0];

            float mean = 0.0f;
            for (const auto& v : data) {
                mean += (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
            }
            mean /= data.size();

            float variance = 0.0f;
            for (const auto& v : data) {
                float val = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                variance += (val - mean) * (val - mean);
            }
            float stddev = std::sqrt(variance / data.size());

            if (stddev == 0) return pos[0];

            auto standardized = std::make_unique<Array>();
            for (const auto& v : data) {
                float val = (v.tag == ValueTag::Float) ? v.as.f : static_cast<float>(v.as.i);
                standardized->elems.push_back(Value::fromFloat((val - mean) / stddev));
            }

            uint32_t id = static_cast<uint32_t>(arrays.size());
            arrays.push_back(std::move(standardized));
            return Value::fromArray(id);
        }

        // io.*
        if (op == "io.write") {
            if (!pos.empty()) {
                std::cout << stringify(pos[0]);
            }
            return Value::null();
        }
        if (op == "io.read") {
            std::string line;
            std::getline(std::cin, line);
            uint32_t sid = static_cast<uint32_t>(stringHeap.size());
            stringHeap.push_back(line);
            return Value::fromString(sid);
        }
        if (op == "io.flush") {
            std::cout << std::flush;
            return Value::null();
        }

        // fs.*
        if (op == "fs.read") {
            if (pos.size() < 1 || pos[0].tag != ValueTag::String) return Value::null();
            const std::string& path = stringHeap[pos[0].as.id];
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open()) return Value::null();
            std::ostringstream ss;
            ss << f.rdbuf();
            uint32_t sid = static_cast<uint32_t>(stringHeap.size());
            stringHeap.push_back(ss.str());
            return Value::fromString(sid);
        }
        if (op == "fs.write") {
            if (pos.size() < 2 || pos[0].tag != ValueTag::String) return Value::null();
            const std::string& path = stringHeap[pos[0].as.id];
            std::ofstream f(path, std::ios::binary);
            if (!f.is_open()) return Value::null();
            std::string data = stringify(pos[1]);
            f.write(data.data(), static_cast<std::streamsize>(data.size()));
            return Value::null();
        }
        if (op == "fs.mmap") {
            if (pos.size() < 1 || pos[0].tag != ValueTag::String) return Value::null();
            const std::string& path = stringHeap[pos[0].as.id];
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open()) return Value::null();
            auto map = std::make_unique<FileMap>();
            map->path = path;
            map->bytes.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
            uint32_t mid = static_cast<uint32_t>(fileMaps.size());
            fileMaps.push_back(std::move(map));

            auto s = std::make_unique<Slice>();
            s->backing = HandleKind::FileMap;
            s->backingId = mid;
            s->baseOffset = 0;
            s->length = static_cast<uint32_t>(fileMaps[mid]->bytes.size());
            s->stride = 1;
            uint32_t sid = static_cast<uint32_t>(slices.size());
            slices.push_back(std::move(s));
            return Value::fromSlice(sid);
        }

        // gpu.* (stubbed resources, but runtime picks a backend string)
        if (op == "gpu.select") {
            std::string backend = "auto";
            auto it = named.find("backend");
            if (it != named.end() && it->second.tag == ValueTag::String) {
                backend = stringHeap[it->second.as.id];
            }
            if (backend == "auto") backend = autoSelectGpuBackend();
            auto dev = std::make_unique<GpuDevice>();
            dev->backend = backend;
            uint32_t id = static_cast<uint32_t>(gpuDevices.size());
            gpuDevices.push_back(std::move(dev));
            return Value::fromHandle(HandleKind::GpuDevice, id);
        }
        if (op == "gpu.context") {
            if (pos.size() < 1 || pos[0].tag != ValueTag::Handle || pos[0].handleKind != HandleKind::GpuDevice) {
                return Value::null();
            }
            auto ctx = std::make_unique<GpuContext>();
            ctx->deviceId = pos[0].as.id;
            uint32_t id = static_cast<uint32_t>(gpuContexts.size());
            gpuContexts.push_back(std::move(ctx));
            return Value::fromHandle(HandleKind::GpuContext, id);
        }
        if (op == "gpu.buffer") {
            if (pos.size() < 1 || pos[0].tag != ValueTag::Handle || pos[0].handleKind != HandleKind::GpuContext) {
                return Value::null();
            }
            uint32_t bytes = 0;
            auto it = named.find("bytes");
            if (it != named.end() && it->second.tag == ValueTag::Int) bytes = static_cast<uint32_t>(it->second.as.i);
            if (bytes == 0 && pos.size() >= 2 && pos[1].tag == ValueTag::Int) bytes = static_cast<uint32_t>(pos[1].as.i);
            auto buf = std::make_unique<GpuBuffer>();
            buf->ctxId = pos[0].as.id;
            buf->bytes.resize(bytes);
            uint32_t id = static_cast<uint32_t>(gpuBuffers.size());
            gpuBuffers.push_back(std::move(buf));
            return Value::fromHandle(HandleKind::GpuBuffer, id);
        }
        if (op == "gpu.map") {
            if (pos.size() < 2) return Value::null();
            if (pos[1].tag != ValueTag::Handle || pos[1].handleKind != HandleKind::GpuBuffer) return Value::null();

            uint32_t bid = pos[1].as.id;
            auto s = std::make_unique<Slice>();
            s->backing = HandleKind::GpuBuffer;
            s->backingId = bid;
            s->baseOffset = 0;
            s->length = static_cast<uint32_t>(gpuBuffers[bid]->bytes.size());
            s->stride = 1;
            uint32_t sid = static_cast<uint32_t>(slices.size());
            slices.push_back(std::move(s));
            return Value::fromSlice(sid);
        }

        return Value::null();
    }

    void runFunction(uint32_t funcIdx) {
        Frame frame;
        frame.funcIdx = funcIdx;
        frame.returnPC = 0;
        frame.locals.assign(functions[funcIdx].localCount, Value::null());
        frame.stack.reserve(functions[funcIdx].stackSize + 16);

        std::vector<Frame> callStack;
        uint32_t pc = 0;

        auto push = [&](Value v) { frame.stack.push_back(v); };
        auto pop = [&]() -> Value {
            if (frame.stack.empty()) {
                std::cerr << "Stack underflow\n";
                return Value::null();
            }
            Value v = frame.stack.back();
            frame.stack.pop_back();
            return v;
        };

        while (true) {
            auto& code = functions[frame.funcIdx].code;
            if (pc >= code.size()) return;

            uint8_t op = code[pc++];
            switch (op) {
                // -------- legacy opcodes (subset) --------
                case ARG_GET: {
                    uint16_t idx = readU16(code, pc);
                    if (idx < frame.args.size()) push(frame.args[idx]);
                    else push(Value::null());
                    break;
                }
                case NOP: break;
                case ICONST_0: push(Value::fromInt(0)); break;
                case ICONST_1: push(Value::fromInt(1)); break;
                case ICONST_M1: push(Value::fromInt(-1)); break;
                case FCONST_0: push(Value::fromFloat(0.0f)); break;
                case FCONST_1: push(Value::fromFloat(1.0f)); break;
                case LDC: {
                    uint32_t idx = readU32(code, pc);
                    if (idx >= constantPool.size()) { push(Value::null()); break; }
                    const auto& c = constantPool[idx];
                    switch (c.type) {
                        case ConstantType::INTEGER: push(Value::fromInt(c.value.i)); break;
                        case ConstantType::FLOAT: push(Value::fromFloat(c.value.f)); break;
                        case ConstantType::STRING: {
                            uint32_t sid = constStringToHeap[idx];
                            push(Value::fromString(sid));
                            break;
                        }
                        case ConstantType::DOUBLE: push(Value::fromFloat(static_cast<float>(c.value.d))); break;
                    }
                    break;
                }
                // -------- comparisons (legacy opcodes, but operate on dynamic Values) --------
                case ICMP_EQ:
                case ICMP_NE:
                case ICMP_LT:
                case ICMP_GT:
                case ICMP_LE:
                case ICMP_GE: {
                    Value b = pop();
                    Value a = pop();

                    auto asNumber = [](const Value& v, bool& isFloatOut) -> double {
                        if (v.tag == ValueTag::Float) { isFloatOut = true; return static_cast<double>(v.as.f); }
                        if (v.tag == ValueTag::Int)   { return static_cast<double>(v.as.i); }
                        if (v.tag == ValueTag::Bool)  { return v.as.b ? 1.0 : 0.0; }
                        return 0.0;
                    };

                    bool result = false;

                    // Numeric comparison when both are numeric-ish.
                    const bool aNum = (a.tag == ValueTag::Int || a.tag == ValueTag::Float || a.tag == ValueTag::Bool);
                    const bool bNum = (b.tag == ValueTag::Int || b.tag == ValueTag::Float || b.tag == ValueTag::Bool);
                    if (aNum && bNum) {
                        bool af = false, bf = false;
                        double av = asNumber(a, af);
                        double bv = asNumber(b, bf);
                        switch (op) {
                            case ICMP_EQ: result = (av == bv); break;
                            case ICMP_NE: result = (av != bv); break;
                            case ICMP_LT: result = (av <  bv); break;
                            case ICMP_GT: result = (av >  bv); break;
                            case ICMP_LE: result = (av <= bv); break;
                            case ICMP_GE: result = (av >= bv); break;
                            default: break;
                        }
                        push(Value::fromBool(result));
                        break;
                    }

                    // String comparisons (lexicographic for ordering, exact for eq/ne).
                    if (a.tag == ValueTag::String && b.tag == ValueTag::String) {
                        const std::string& as = stringHeap[a.as.id];
                        const std::string& bs = stringHeap[b.as.id];
                        switch (op) {
                            case ICMP_EQ: result = (as == bs); break;
                            case ICMP_NE: result = (as != bs); break;
                            case ICMP_LT: result = (as <  bs); break;
                            case ICMP_GT: result = (as >  bs); break;
                            case ICMP_LE: result = (as <= bs); break;
                            case ICMP_GE: result = (as >= bs); break;
                            default: break;
                        }
                        push(Value::fromBool(result));
                        break;
                    }

                    // Fallback: only support eq/ne across different tags.
                    if (op == ICMP_EQ) result = (a.tag == b.tag && a.as.u == b.as.u);
                    else if (op == ICMP_NE) result = !(a.tag == b.tag && a.as.u == b.as.u);
                    else result = false;

                    push(Value::fromBool(result));
                    break;
                }

                case GOTO: {
                    uint32_t target = readU32(code, pc);
                    pc = target;
                    break;
                }
                case IFEQ: {
                    Value v = pop();
                    uint32_t target = readU32(code, pc);
                    if (!isTruthy(v)) pc = target;
                    break;
                }
                case CALL: {
                    uint32_t callee = readU32(code, pc);
                    uint16_t argc = readU16(code, pc);

                    // Pop args from caller stack (reverse), then reverse to keep order
                    std::vector<Value> args;
                    args.reserve(argc);
                    for (uint16_t i = 0; i < argc; i++) {
                        args.push_back(pop());
                    }
                    std::reverse(args.begin(), args.end());

                    // Save current frame
                    Frame saved = frame;
                    saved.returnPC = pc;
                    callStack.push_back(std::move(saved));

                    // New frame
                    frame = Frame{};
                    frame.funcIdx = callee;
                    frame.args = std::move(args);
                    frame.locals.assign(functions[callee].localCount, Value::null());
                    frame.stack.reserve(functions[callee].stackSize + 16);

                    pc = 0;
                    break;
                }
                case RET: {
                    // return value = top of callee stack (or null if none)
                    Value ret = Value::null();
                    if (!frame.stack.empty()) ret = pop();

                    if (callStack.empty()) return;

                    uint32_t returnTo = callStack.back().returnPC;
                    Frame restored = std::move(callStack.back());
                    callStack.pop_back();

                    frame = std::move(restored);
                    push(ret);          // push return value for caller
                    pc = returnTo;
                    break;
                }

                case NEWARRAY: {
                    uint32_t n = readU32(code, pc);
                    auto arr = std::make_unique<Array>();
                    arr->elems.assign(n, Value::null());
                    uint32_t id = static_cast<uint32_t>(arrays.size());
                    arrays.push_back(std::move(arr));
                    push(Value::fromArray(id));
                    break;
                }
                case ARRAYLENGTH: {
                    Value base = pop();
                    if (base.tag == ValueTag::Array && base.as.id < arrays.size()) {
                        push(Value::fromInt(static_cast<int32_t>(arrays[base.as.id]->elems.size())));
                    } else if (base.tag == ValueTag::Slice && base.as.id < slices.size()) {
                        push(Value::fromInt(static_cast<int32_t>(slices[base.as.id]->length)));
                    } else {
                        push(Value::fromInt(0));
                    }
                    break;
                }

                case PRINT_I: std::cout << pop().as.i; break;
                case PRINT_F: std::cout << pop().as.f; break;
                case PRINT_S: {
                    uint32_t idx = readU32(code, pc);
                    if (idx < constantPool.size() && constantPool[idx].type == ConstantType::STRING) {
                        std::cout << constantPool[idx].strValue;
                    }
                    break;
                }
                case PRINT_NL: std::cout << std::endl; break;

                // -------- CAX v0.1+ dynamic opcodes --------
                case POP: {
                    (void)pop();
                    break;
                }
                case LOAD: {
                    uint16_t slot = readU16(code, pc);
                    if (slot >= frame.locals.size()) push(Value::null());
                    else push(frame.locals[slot]);
                    break;
                }
                case STORE: {
                    uint16_t slot = readU16(code, pc);
                    Value v = pop();
                    if (slot >= frame.locals.size()) frame.locals.resize(slot + 1, Value::null());
                    frame.locals[slot] = v;
                    break;
                }
                case BOOLIFY: {
                    Value v = pop();
                    bool b = isTruthy(v);
                    push(Value::fromBool(b));
                    break;
                }
                case NEG: {
                    Value v = pop();
                    if (v.tag == ValueTag::Float) push(Value::fromFloat(-v.as.f));
                    else if (v.tag == ValueTag::Int) push(Value::fromInt(-v.as.i));
                    else push(Value::null());
                    break;
                }
                case ADD: {
                    Value b = pop();
                    Value a = pop();
                    push(doAdd(a, b));
                    break;
                }
                case SUB: {
                    Value b = pop();
                    Value a = pop();
                    push(doSub(a, b));
                    break;
                }
                case MUL: {
                    Value b = pop();
                    Value a = pop();
                    push(doMul(a, b));
                    break;
                }
                case DIV: {
                    Value b = pop();
                    Value a = pop();
                    push(doDiv(a, b));
                    break;
                }
                case PRINT: {
                    Value v = pop();
                    std::cout << stringify(v);
                    break;
                }
                case WRITE_LOCAL: {
                    uint16_t slot = readU16(code, pc);
                    if (slot >= frame.locals.size()) frame.locals.resize(slot + 1, Value::null());
                    std::string line;
                    std::getline(std::cin, line);
                    frame.locals[slot] = convertInput(line, frame.locals[slot]);
                    break;
                }
                case INDEX_GET: {
                    Value idxV = pop();
                    Value base = pop();
                    int32_t idx = (idxV.tag == ValueTag::Int) ? idxV.as.i : 0;
                    if (idx < 0) { push(Value::null()); break; }
                    if (base.tag == ValueTag::Array && base.as.id < arrays.size()) {
                        auto& a = arrays[base.as.id]->elems;
                        if (static_cast<size_t>(idx) >= a.size()) { push(Value::null()); break; }
                        push(a[static_cast<size_t>(idx)]);
                        break;
                    }
                    if (base.tag == ValueTag::Slice && base.as.id < slices.size()) {
                        auto& s = *slices[base.as.id];
                        if (static_cast<uint32_t>(idx) >= s.length) { push(Value::null()); break; }
                        uint32_t offset = s.baseOffset + static_cast<uint32_t>(idx) * s.stride;
                        if (s.backing == HandleKind::FileMap) {
                            auto& bytes = fileMaps.at(s.backingId)->bytes;
                            if (offset >= bytes.size()) { push(Value::null()); break; }
                            push(Value::fromInt(static_cast<int32_t>(bytes[offset])));
                            break;
                        }
                        if (s.backing == HandleKind::GpuBuffer) {
                            auto& bytes = gpuBuffers.at(s.backingId)->bytes;
                            if (offset >= bytes.size()) { push(Value::null()); break; }
                            push(Value::fromInt(static_cast<int32_t>(bytes[offset])));
                            break;
                        }
                    }
                    push(Value::null());
                    break;
                }
                case INDEX_SET: {
                    Value val = pop();
                    Value idxV = pop();
                    Value base = pop();
                    int32_t idx = (idxV.tag == ValueTag::Int) ? idxV.as.i : 0;
                    if (idx < 0) break;
                    if (base.tag == ValueTag::Array && base.as.id < arrays.size()) {
                        auto& a = arrays[base.as.id]->elems;
                        if (static_cast<size_t>(idx) < a.size()) a[static_cast<size_t>(idx)] = val;
                        break;
                    }
                    if (base.tag == ValueTag::Slice && base.as.id < slices.size()) {
                        auto& s = *slices[base.as.id];
                        if (static_cast<uint32_t>(idx) >= s.length) break;
                        uint32_t offset = s.baseOffset + static_cast<uint32_t>(idx) * s.stride;
                        int32_t byte = 0;
                        if (val.tag == ValueTag::Int) byte = val.as.i;
                        if (val.tag == ValueTag::Bool) byte = val.as.b ? 1 : 0;
                        if (s.backing == HandleKind::FileMap) {
                            auto& bytes = fileMaps.at(s.backingId)->bytes;
                            if (offset < bytes.size()) bytes[offset] = static_cast<uint8_t>(byte & 0xFF);
                        }
                        if (s.backing == HandleKind::GpuBuffer) {
                            auto& bytes = gpuBuffers.at(s.backingId)->bytes;
                            if (offset < bytes.size()) bytes[offset] = static_cast<uint8_t>(byte & 0xFF);
                        }
                    }
                    break;
                }
                case EXEC: {
                    uint32_t opNameIdx = readU32(code, pc);
                    uint16_t posArgc = readU16(code, pc);
                    uint16_t namedArgc = readU16(code, pc);

                    std::map<std::string, Value> named;
                    for (uint16_t i = 0; i < namedArgc; i++) {
                        Value val = pop();
                        Value nameV = pop();
                        std::string key = (nameV.tag == ValueTag::String) ? stringHeap[nameV.as.id] : "";
                        named[key] = val;
                    }

                    std::vector<Value> pos;
                    pos.reserve(posArgc);
                    for (uint16_t i = 0; i < posArgc; i++) {
                        pos.push_back(pop());
                    }
                    std::reverse(pos.begin(), pos.end());

                    std::string opName;
                    if (opNameIdx < constantPool.size() && constantPool[opNameIdx].type == ConstantType::STRING) {
                        opName = constantPool[opNameIdx].strValue;
                    }

                    Value r = dispatchExec(opName, pos, named);
                    push(r);
                    break;
                }

                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << (int)op << std::dec << "\n";
                    return;
            }
        }
    }
};

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
            std::cerr << "VM error: Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (bytecodePath.empty()) {
        std::cerr << "VM error: No bytecode file specified\n";
        return 1;
    }

    ClangaxVM vm;
    if (!vm.loadBytecode(bytecodePath)) {
        return 1;
    }

    if (verbose) {
        std::cout << "ClangAX Virtual Machine\n";
        std::cout << "=====================\n";
        vm.printStats();
        std::cout << "\n";
    }

    std::cout << "=== Execution ===\n";
    vm.execute();
    std::cout << "\n=== Execution Complete ===\n";
    return 0;
}

