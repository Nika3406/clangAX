#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
BIN_DIR="${HOME}/.local/bin"

echo "Project root : ${PROJECT_ROOT}"
echo "Build dir    : ${BUILD_DIR}"
echo "Install dir  : ${BIN_DIR}"
echo

# --- Safety checks ---
if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "ERROR: Build directory not found: ${BUILD_DIR}"
  exit 1
fi

# Executables we need (NO LLVM TOOLS!)
REQUIRED_BINS=(clangax caxvm)

missing=0
for tool in "${REQUIRED_BINS[@]}"; do
  if [[ ! -x "${BUILD_DIR}/${tool}" ]]; then
    echo "ERROR: Missing executable: ${BUILD_DIR}/${tool}"
    missing=1
  fi
done

if [[ "${missing}" -ne 0 ]]; then
  echo
  echo "One or more executables are missing."
  exit 1
fi

echo "All binaries found"
echo

# Create bin directory if it doesn't exist
mkdir -p "${BIN_DIR}"

# --- Install clangax wrapper ---
CLANGAX_WRAPPER="${BIN_DIR}/clangax"

cat > "${CLANGAX_WRAPPER}" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="__BUILD_DIR__"

orig_pwd="$PWD"
args=()
input_done=0

# Rewrite paths to absolute:
# - first non-flag argument = input file -> absolute if relative
# - value after -o -> absolute if relative
while [[ $# -gt 0 ]]; do
  a="$1"
  shift

  if [[ "$a" == "-o" ]]; then
    if [[ $# -eq 0 ]]; then
      echo "ERROR: -o requires an argument" >&2
      exit 1
    fi
    out="$1"
    shift
    # Make output path absolute
    if [[ "$out" != /* ]]; then out="$orig_pwd/$out"; fi
    args+=("-o" "$out")
    continue
  fi

  # First non-flag argument is the input file
  if [[ $input_done -eq 0 && "$a" != -* ]]; then
    # Make input path absolute
    if [[ "$a" != /* ]]; then a="$orig_pwd/$a"; fi
    input_done=1
  fi

  args+=("$a")
done

# Execute the real compiler from build directory
exec "$BUILD_DIR/clangax" "${args[@]}"
EOF

# Inject the real build directory path
if [[ "$OSTYPE" == "darwin"* ]]; then
  # macOS sed requires backup suffix
  sed -i '' "s|__BUILD_DIR__|${BUILD_DIR}|g" "${CLANGAX_WRAPPER}"
else
  # Linux sed
  sed -i "s|__BUILD_DIR__|${BUILD_DIR}|g" "${CLANGAX_WRAPPER}"
fi

chmod +x "${CLANGAX_WRAPPER}"
echo "Installed: ${CLANGAX_WRAPPER}"

# --- Install caxvm wrapper ---
CAXVM_WRAPPER="${BIN_DIR}/caxvm"

cat > "${CAXVM_WRAPPER}" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="__BUILD_DIR__"

orig_pwd="$PWD"
args=()

# Make all .caxb file paths absolute
for arg in "$@"; do
  if [[ "$arg" == *.caxb && "$arg" != /* ]]; then
    args+=("$orig_pwd/$arg")
  else
    args+=("$arg")
  fi
done

exec "$BUILD_DIR/caxvm" "${args[@]}"
EOF

# Inject the real build directory path
if [[ "$OSTYPE" == "darwin"* ]]; then
  sed -i '' "s|__BUILD_DIR__|${BUILD_DIR}|g" "${CAXVM_WRAPPER}"
else
  sed -i "s|__BUILD_DIR__|${BUILD_DIR}|g" "${CAXVM_WRAPPER}"
fi

chmod +x "${CAXVM_WRAPPER}"
echo "Installed: ${CAXVM_WRAPPER}"

# --- Optional: Install disassembler if built ---
if [[ -x "${BUILD_DIR}/caxdis" ]]; then
  CAXDIS_WRAPPER="${BIN_DIR}/caxdis"

  cat > "${CAXDIS_WRAPPER}" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="__BUILD_DIR__"

orig_pwd="$PWD"
args=()

for arg in "$@"; do
  if [[ "$arg" == *.caxb && "$arg" != /* ]]; then
    args+=("$orig_pwd/$arg")
  else
    args+=("$arg")
  fi
done

exec "$BUILD_DIR/caxdis" "${args[@]}"
EOF

  if [[ "$OSTYPE" == "darwin"* ]]; then
    sed -i '' "s|__BUILD_DIR__|${BUILD_DIR}|g" "${CAXDIS_WRAPPER}"
  else
    sed -i "s|__BUILD_DIR__|${BUILD_DIR}|g" "${CAXDIS_WRAPPER}"
  fi

  chmod +x "${CAXDIS_WRAPPER}"
  echo "Installed: ${CAXDIS_WRAPPER}"
fi

echo

# --- PATH setup (zsh + bash) ---
ensure_path() {
  local profile="$1"
  local line="export PATH=\"${BIN_DIR}:\$PATH\""

  # Create file if it doesn't exist
  touch "$profile"

  if ! grep -Fqs "$BIN_DIR" "$profile"; then
    echo >> "$profile"
    echo "# Added by ClangAX installer" >> "$profile"
    echo "$line" >> "$profile"
    echo "Updated PATH in: $profile"
  else
    echo "PATH already configured in: $profile"
  fi
}

echo "Configuring shell PATH..."
echo

ensure_path "$HOME/.zshrc"
ensure_path "$HOME/.bash_profile"

echo "Installation Complete!"
echo "Installed tools:"
echo "  * clangax - ClangAX bytecode compiler"
echo "  * caxvm   - ClangAX virtual machine"
if [[ -x "${BUILD_DIR}/caxdis" ]]; then
  echo "  * caxdis  - Bytecode disassembler"
fi
