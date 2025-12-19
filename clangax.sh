#!/usr/bin/env bash
set -euo pipefail


PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
BIN_DIR="${HOME}/.local/bin"

echo "== clangAX installer =="
echo "Project root : ${PROJECT_ROOT}"
echo "Build dir    : ${BUILD_DIR}"
echo "Install dir  : ${BIN_DIR}"
echo

# --- Safety checks ---
if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "ERROR: Build directory not found: ${BUILD_DIR}"
  echo "Build first, e.g.: cmake --build cmake-build-debug"
  exit 1
fi

if [[ ! -x "${BUILD_DIR}/clangax" ]]; then
  echo "ERROR: Compiled clangax binary not found/executable: ${BUILD_DIR}/clangax"
  echo "Build first, e.g.: cmake --build cmake-build-debug"
  exit 1
fi

# Tools your compiler expects to run
TOOLS=(irGenerator lexicalAnalyzer parser symbolTable)

missing=0
for t in "${TOOLS[@]}"; do
  if [[ ! -x "${BUILD_DIR}/${t}" ]]; then
    echo "ERROR: Missing executable in build dir: ${BUILD_DIR}/${t}"
    missing=1
  fi
done
if [[ "${missing}" -ne 0 ]]; then
  echo
  echo "One or more tool executables are missing."
  echo "Build first, e.g.: cmake --build cmake-build-debug"
  exit 1
fi

mkdir -p "${BIN_DIR}"

# --- Install wrapper as ~/.local/bin/clangax ---
WRAPPER_PATH="${BIN_DIR}/clangax"

cat > "${WRAPPER_PATH}" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="__BUILD_DIR__"

orig_pwd="$PWD"
args=()
input_done=0

# Rewrite:
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
    if [[ "$out" != /* ]]; then out="$orig_pwd/$out"; fi
    args+=("-o" "$out")
    continue
  fi

  if [[ $input_done -eq 0 && "$a" != -* ]]; then
    # treat as input file
    if [[ "$a" != /* ]]; then a="$orig_pwd/$a"; fi
    input_done=1
  fi

  args+=("$a")
done

cd "$BUILD_DIR"
exec ./clangax "${args[@]}"
EOF

# Inject the real build dir (macOS sed -i needs a backup suffix; '' means none)
sed -i '' "s|__BUILD_DIR__|${BUILD_DIR}|g" "${WRAPPER_PATH}"
chmod +x "${WRAPPER_PATH}"

echo "Installed wrapper: ${WRAPPER_PATH}"

# --- Copy tool executables to ~/.local/bin (optional but handy) ---
# IMPORTANT: we do NOT copy the clangax binary to clangax (would overwrite wrapper).
for t in "${TOOLS[@]}"; do
  cp -f "${BUILD_DIR}/${t}" "${BIN_DIR}/${t}"
  chmod +x "${BIN_DIR}/${t}"
  echo "Installed tool  : ${BIN_DIR}/${t}"
done

echo
echo "Done."

# --- PATH setup (zsh + bash) ---
ensure_path() {
  local profile="$1"
  local line="export PATH=\"${BIN_DIR}:\$PATH\""

  # Create file if it doesn't exist
  touch "$profile"

  if ! grep -Fqs "$BIN_DIR" "$profile"; then
    echo >> "$profile"
    echo "# Added by clangAX installer" >> "$profile"
    echo "$line" >> "$profile"
    echo "Updated PATH in: $profile"
  else
    echo "PATH already set in: $profile"
  fi
}

echo
echo "Configuring PATH..."

ensure_path "$HOME/.zshrc"
ensure_path "$HOME/.bash_profile"

echo
echo "PATH configuration complete."
echo "Restart your shell or run:"
echo "  source ~/.zshrc   # for zsh"
echo "  source ~/.bash_profile  # for bash"

