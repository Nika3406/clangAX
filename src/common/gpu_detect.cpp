#include "gpu_detect.h"

#include <vector>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace ClangAX {

namespace {

static bool tryLoadAny(const std::vector<const char*>& libs) {
#if defined(_WIN32)
    for (auto* name : libs) {
        HMODULE h = LoadLibraryA(name);
        if (h) {
            FreeLibrary(h);
            return true;
        }
    }
    return false;
#else
    for (auto* name : libs) {
        void* h = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
        if (h) {
            dlclose(h);
            return true;
        }
    }
    return false;
#endif
}

}

std::vector<std::string> detectAvailableGpuBackends() {
    std::vector<std::string> out;

    // CUDA
    if (tryLoadAny({
            "nvcuda.dll",            // Windows
            "libcuda.so", "libcuda.so.1", // Linux
            "libcuda.dylib"          // macOS (rare)
        })) {
        out.push_back("cuda");
    }

    // OpenCL
    if (tryLoadAny({
            "OpenCL.dll", "opencl.dll",
            "libOpenCL.so", "libOpenCL.so.1",
            "/System/Library/Frameworks/OpenCL.framework/OpenCL" // macOS
        })) {
        out.push_back("opencl");
    }

    // Metal (macOS)
    if (tryLoadAny({
            "/System/Library/Frameworks/Metal.framework/Metal",
            "/System/Library/Frameworks/MetalPerformanceShaders.framework/MetalPerformanceShaders"
        })) {
        out.push_back("metal");
    }

    return out;
}

}
