#pragma once

#include <string>
#include <vector>

namespace ClangAX {

// Best-effort runtime detection of GPU compute backends.
// Returns a list containing some of: "cuda", "metal", "opencl".
std::vector<std::string> detectAvailableGpuBackends();

}
