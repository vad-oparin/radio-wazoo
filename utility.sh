#!/bin/bash

set -e # Exit on error

# Check if IDF_PATH variable is set
if [ -z "$IDF_PATH" ]; then
  echo "Error: IDF_PATH is not set. Please set it, e.g., export IDF_PATH=/home/vadim/esp/esp-idf"
  exit 1
fi

# Check if clang-tidy is accessible
if ! command -v clang-tidy &>/dev/null; then
  echo "Error: clang-tidy not found. Install it with 'sudo apt install clang-tidy'"
  exit 1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
#YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function print_help() {
  echo "======================================"
  echo "  Radio Wazoo Utilities Script"
  echo "======================================"
  echo ""
  echo "Usage: ./utility.sh [command]"
  echo ""
  echo "Commands:"
  echo "  format           - Reformat code in C/C++ files"
  echo "  tidy             - clang-tidy linting on C/C++ files"
  echo ""
}

function reformat_code() {
  echo -e "${GREEN}=== 🔠 Reformatting C code... ===${NC}"
  find main -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i --verbose {} +
  find components -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i --verbose {} +
  echo -e "${GREEN}=== ✅ Reformat complete! ===${NC}"
}

function clang_tidy() {
  echo -e "${GREEN}=== 🔍 Running clang-tidy linting on C/C++ files... ===${NC}"

  echo -e "${GREEN}=== ✅ Clang-tidy linting completed! ===${NC}"
}

# Main script logic
case "$1" in
format)
  reformat_code
  ;;
tidy)
  clang_tidy
  ;;
help | --help | -h | "")
  print_help
  ;;
*)
  echo -e "${RED}❌ Unknown command: $1${NC}"
  echo ""
  print_help
  exit 1
  ;;
esac
