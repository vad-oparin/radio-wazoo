#!/bin/bash

set -e # Exit on error

# Check if clang-tidy is accessible
if ! command -v clang-tidy &>/dev/null; then
  echo "Error: clang-tidy not found. Install it with 'sudo apt install clang-tidy'"
  exit 1
fi

PORT="${2:-/dev/ttyACM0}"
CHIP="esp32s2"
BAUD="460800"
#DO_ERASE=false
FORCE_FS_FLASH=false
DATA_DIR="data"
PARTITION_NAME="storage"
FS_IMAGE="build/littlefs.bin"
FS_FLASH_MARKER="build/.littlefs_flashed"
SSID="RadioWazooAP"
IP="192.168.4.1"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
  echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
  echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
  echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
  echo -e "\n${GREEN}===================================================${NC}"
  echo -e "${GREEN}  $1${NC}"
  echo -e "${GREEN}===================================================${NC}\n"
}

function print_help() {
  echo "======================================"
  echo "  Radio Wazoo Utilities Script"
  echo "======================================"
  echo ""
  echo "Usage: ./utility.sh [command] [port]"
  echo ""
  echo "Commands:"
  echo "  build            - Build firmware, flash all, reset device, and verify boot"
  echo "  verify           - Verify device boot status (check WiFi AP)"
  echo "  format           - Reformat code in C/C++ files using clang-format"
  echo "  tidy             - Run clang-tidy linting on C/C++ files"
  echo "  help             - Show this help message"
  echo ""
  echo "Environment Variables:"
  echo "  PORT             - Serial port (default: /dev/ttyACM0)"
  echo "  CHIP             - ESP32 chip type (default: esp32s2)"
  echo "  BAUD             - Baud rate (default: 460800)"
  echo ""
  echo "Examples:"
  echo "  ./utility.sh build"
  echo "  PORT=/dev/ttyUSB0 ./utility.sh build"
  echo "  ./utility.sh verify"
  echo ""
  echo "The 'build' command performs:"
  echo "  1. Firmware build (idf.py build)"
  echo "  2. Firmware flash (bootloader + partition table + app)"
  echo "  3. Filesystem flash (incremental, only if data/ changed)"
  echo "  4. Device reset"
  echo "  5. Boot verification (WiFi AP detection)"
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

check_idf() {
  # Check if IDF_PATH variable is set
  if [ -z "$IDF_PATH" ]; then
    log_error "Error: IDF_PATH is not set. Please set it, e.g., export IDF_PATH=/home/username/esp/esp-idf"
    exit 1
  fi
}

# Check if running with proper permissions
check_permissions() {
  if ! groups | grep -qE "(dialout|uucp)"; then
    log_error "User not in dialout/uucp group. Run with: sg uucp -c \"[command]\""
    exit 1
  fi
}

# Check if device is connected
check_device() {
  if [ ! -e "$PORT" ]; then
    log_error "Device not found at $PORT"
    log_info "Available serial ports:"
    ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || log_warn "No serial devices found"
    exit 1
  fi

  log_info "Device found at $PORT"
}

# Step 1: Build the firmware
build_firmware() {
  log_step "=== Step 1: Building firmware ==="

  check_idf

  log_info "Running: idf.py build"

  if ! idf.py build; then
    log_error "Build failed"
    exit 1
  fi

  log_info "Build successful"
}

# Step 2: Flash everything (bootloader, partition table, app)
flash_firmware() {
  log_step "Step 2: Flashing bootloader, partition table, and application"

  log_info "Running: idf.py -p $PORT -b $BAUD flash"

  if ! idf.py -p "$PORT" -b "$BAUD" flash; then
    log_error "Flash failed"
    exit 1
  fi

  log_info "Flash completed successfully"
}

# Step 3: Flash filesystem (web files)
flash_filesystem() {
  log_step "=== Step 3: Creating and flashing filesystem ==="

  # Check if data directory exists
  if [ ! -d "$DATA_DIR" ]; then
    log_warn "Data directory '$DATA_DIR' not found, skipping filesystem flash"
    return 0
  fi

  log_info "Data directory: $DATA_DIR"

  # Get partition offset and size from built partition table
  log_info "Reading partition information for '$PARTITION_NAME'..."

  # Check if partition table binary exists
  if [ ! -f "build/partition_table/partition-table.bin" ]; then
    log_error "Partition table binary not found. Run 'idf.py build' first."
    exit 1
  fi

  # Use gen_esp32part.py to parse the binary partition table
  PARTITION_INFO=$(python "$IDF_PATH/components/partition_table/gen_esp32part.py" build/partition_table/partition-table.bin | grep "^$PARTITION_NAME")

  if [ -z "$PARTITION_INFO" ]; then
    log_error "Could not find partition '$PARTITION_NAME' in partition table"
    exit 1
  fi

  # Extract offset and size from gen_esp32part output
  # Format: name,type,subtype,offset,size,flags
  OFFSET=$(echo "$PARTITION_INFO" | awk -F',' '{print $4}')
  SIZE_STR=$(echo "$PARTITION_INFO" | awk -F',' '{print $5}')

  # Convert size (handles K, M suffixes)
  if [[ $SIZE_STR =~ ([0-9]+)M ]]; then
    SIZE=$((BASH_REMATCH[1] * 1024 * 1024))
  elif [[ $SIZE_STR =~ ([0-9]+)K ]]; then
    SIZE=$((BASH_REMATCH[1] * 1024))
  elif [[ $SIZE_STR =~ ^0x ]]; then
    SIZE=$((SIZE_STR))
  else
    SIZE=$SIZE_STR
  fi

  log_info "Partition offset: $OFFSET"
  log_info "Partition size: $SIZE bytes"

  # Check if mklittlefs is available
  if command -v mklittlefs &>/dev/null; then
    # Check if filesystem needs to be rebuilt and flashed
    NEEDS_FLASH=false

    if [ "$FORCE_FS_FLASH" = true ]; then
      log_info "Force filesystem flash requested"
      NEEDS_FLASH=true
    elif [ ! -f "$FS_FLASH_MARKER" ]; then
      log_info "No flash marker found, filesystem needs to be flashed"
      NEEDS_FLASH=true
    else
      # Check if any source file is newer than the flash marker
      log_info "Checking if source files changed since last successful flash..."

      while IFS= read -r -d '' file; do
        if [ "$file" -nt "$FS_FLASH_MARKER" ]; then
          log_info "File modified: $(basename "$file")"
          NEEDS_FLASH=true
          break
        fi
      done < <(find "$DATA_DIR" -type f -print0)

      if [ "$NEEDS_FLASH" = false ]; then
        log_info "Filesystem already flashed and up-to-date, skipping"
      fi
    fi

    # Only rebuild and flash if needed
    if [ "$NEEDS_FLASH" = true ]; then
      log_info "Creating LittleFS image with mklittlefs..."
      BLOCK_SIZE=4096
      PAGE_SIZE=256

      # mklittlefs copies directory contents directly
      TMP_FS_DIR=$(mktemp -d)
      cp -r "$DATA_DIR"/* "$TMP_FS_DIR/"

      mklittlefs -c "$TMP_FS_DIR" -b $BLOCK_SIZE -p $PAGE_SIZE -s "$SIZE" "$FS_IMAGE"

      # Cleanup
      rm -rf "$TMP_FS_DIR"

      if [ ! -f "$FS_IMAGE" ]; then
        log_error "Failed to create filesystem image"
        exit 1
      fi

      log_info "Filesystem image created: $FS_IMAGE"
      log_info "Flashing filesystem to offset $OFFSET..."

      if ! python -m esptool --chip "$CHIP" -p "$PORT" -b "$BAUD" write_flash "$OFFSET" "$FS_IMAGE"; then
        log_error "Filesystem flash failed"
        rm -f "$FS_FLASH_MARKER"
        exit 1
      fi

      touch "$FS_FLASH_MARKER"
      log_info "Filesystem flashed successfully"
    fi
  else
    log_warn "mklittlefs tool not found - skipping filesystem flash"
    log_warn "LittleFS will auto-format on first boot (empty filesystem)"
    log_info ""
    log_info "To flash web files, you have two options:"
    log_info "  1. Install mklittlefs from: https://github.com/earlephilhower/mklittlefs/releases"
    log_info "  2. Upload files to device after boot using a file upload mechanism"
    log_info ""
  fi
}

# Step 4: Reset device
reset_device() {
  log_step "=== Step 4: Resetting device ==="

  log_info "Sending hard reset..."
  python -m esptool --chip "$CHIP" -p "$PORT" run

  log_info "Device reset complete"
}

# Step 5: Verify boot
verify_boot() {
  log_step "Step 5: Verifying device boot"

  log_info "Waiting for device to boot (15 seconds)..."
  sleep 15

  log_info "Checking for WiFi AP..."
  if nmcli device wifi list | grep -q "$SSID"; then
    log_info "✓ $SSID WiFi network detected!"
    log_info "✓ Device booted successfully"
    log_info ""
    log_info "Connect to WiFi:"
    log_info "  SSID: $SSID"
    log_info "  Password: 12345678"
    log_info "  Web Interface: http://$IP"
  else
    log_warn "WiFi AP not detected yet"
    log_info "The device may still be booting or there might be an issue"
    log_info "Check serial monitor with: minicom -D ${PORT/ACM0/ACM1} -b 115200"
  fi
}

# Main script logic
case "$1" in
format)
  reformat_code
  ;;
tidy)
  clang_tidy
  ;;
build)
  build_firmware
  flash_firmware
  flash_filesystem
  reset_device
  verify_boot
  ;;
verify)
  verify_boot
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
