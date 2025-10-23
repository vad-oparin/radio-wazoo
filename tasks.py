import grp
import os
import pwd
import tempfile
from datetime import datetime

import serial
from invoke import task, Exit, Context
from pathlib import Path
import shutil
import re
import rcssmin
import rjsmin

from colorama import init, Fore  # , Back, Style

import subprocess

# Assets build settings
SRC_DIR = Path("src_www")
DEST_DIR = Path("data/www")

# Firmware build/flash settings
PORT = "/dev/ttyACM0"
LOG_DIR = 'var/build-logs'
MAX_LOG_FILES = 5
BAUD = "460800"
DATA_DIR = "data"
PARTITION_NAME = "storage"
FS_IMAGE = "build/littlefs.bin"

init(autoreset=True)


def log_info(msg: str) -> None:
    """ Print informational message in cyan """
    print(f"{Fore.CYAN}[INFO] {msg}")


def log_warn(msg: str) -> None:
    """ Print warning message in yellow """
    print(f"{Fore.YELLOW}[WARN] {msg}")


def log_error(msg: str) -> None:
    """ Print error message in red """
    print(f"{Fore.RED}[ERROR] {msg}")


def log_step(msg: str) -> None:
    """ Print major step banner in green with separators """
    print(
        f"\n{Fore.GREEN}===================================================\n"
        f"  {msg}\n"
        f"===================================================\n"
    )


def build_logger(output: str) -> None:
    """
    Save build output to timestamped log file and maintain rotation.

    Creates log file in LOG_DIR with format: firmware-build-YYYYMMDD_HHMMSS.log
    Automatically deletes oldest logs when count exceeds MAX_LOG_FILES.
    """
    log_dir = Path(LOG_DIR)
    log_dir.mkdir(exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = log_dir / f"firmware-build-{timestamp}.log"
    with open(log_file, 'w') as handle:
        handle.write(output)
    log_files = sorted(log_dir.glob("firmware-build-*.log"), key=os.path.getmtime, reverse=True)
    if len(log_files) > MAX_LOG_FILES:
        files_to_delete = log_files[MAX_LOG_FILES:]
        for old_file in files_to_delete:
            try:
                os.remove(old_file)
            except OSError as e:
                log_error(f"Can not remove file {old_file.name}: {e}")


"""
Web Assets Build System

This module processes web interface files for embedding into ESP32 firmware.

PIPELINE:
  src_www/              →  data/www/              →  data/              →  ESP32 filesystem
  (source files)           (processed files)         (filesystem root)     (LittleFS partition)

TASKS:
  1. clean      - Remove data/www/ directory
  2. css-min    - Minify CSS files, add .min suffix
  3. js-min     - Minify JavaScript files, add .min suffix
  4. images     - Copy image files unchanged (svg, png, jpg, jpeg, gif, ico, webp)
  5. html       - Copy HTML files, update references (.css → .min.css, .js → .min.js)
  6. build      - Execute all tasks in order (1-5)

WORKFLOW:
  Development:  Edit files in src_www/
  Processing:   Run 'invoke build'
  Filesystem:   data/ directory contains all files to be embedded (www/, configs/, etc.)
  Flashing:     Run 'invoke flash-filesystem' to create and flash LittleFS image

OUTPUT:
  Processed files are written to data/www/. The entire data/ directory is packed
  into a LittleFS image and flashed to the storage partition. Web files are served
  from /www/ path, but data/ can contain other application data as needed.
"""


@task
def clean(ctx: Context) -> None:
    """ Clean build folder """
    log_step("Cleaning build folder")
    if DEST_DIR.exists():
        shutil.rmtree(DEST_DIR)
        log_info(f"Cleaned {DEST_DIR}")
    else:
        log_info(f"{DEST_DIR} does not exist")


@task
def css_min(ctx: Context) -> None:
    """ Minify CSS and add .min suffix """
    log_step("Minifying CSS files")
    css_files = SRC_DIR.glob("**/*.css")

    for css_file in css_files:
        css_content = css_file.read_text(encoding="utf-8")
        minified = rcssmin.cssmin(css_content)

        rel_path = css_file.relative_to(SRC_DIR)
        dest_file = DEST_DIR / rel_path.parent / f"{rel_path.stem}.min.css"
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        dest_file.write_text(minified, encoding="utf-8")
        log_info(f"Minified {css_file} -> {dest_file}")


@task
def js_min(ctx: Context) -> None:
    """ Minify JS and add .min suffix """
    log_step("Minifying JS files")
    js_files = SRC_DIR.glob("**/*.js")

    for js_file in js_files:
        js_content = js_file.read_text(encoding="utf-8")
        minified = rjsmin.jsmin(js_content)

        rel_path = js_file.relative_to(SRC_DIR)
        dest_file = DEST_DIR / rel_path.parent / f"{rel_path.stem}.min.js"
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        dest_file.write_text(minified, encoding="utf-8")
        log_info(f"Minified {js_file} -> {dest_file}")


@task
def images(ctx: Context) -> None:
    """ Copy images """
    log_step("Copying images")
    image_extensions = {".svg", ".png", ".jpg", ".jpeg", ".gif", ".ico", ".webp"}

    for src_file in SRC_DIR.rglob("*"):
        if src_file.is_file() and src_file.suffix.lower() in image_extensions:
            rel_path = src_file.relative_to(SRC_DIR)
            dest_file = DEST_DIR / rel_path
            dest_file.parent.mkdir(parents=True, exist_ok=True)

            shutil.copy2(src_file, dest_file)
            log_info(f"Copied {src_file} -> {dest_file}")


@task
def html(ctx: Context) -> None:
    """ Copy HTML files and update references to .min files """
    log_step("Processing HTML files")
    html_files = SRC_DIR.glob("**/*.html")

    for html_file in html_files:
        html_content = html_file.read_text(encoding="utf-8")

        html_content = re.sub(r'\.css(?=")', '.min.css', html_content)
        html_content = re.sub(r'\.js(?=")', '.min.js', html_content)

        rel_path = html_file.relative_to(SRC_DIR)
        dest_file = DEST_DIR / rel_path
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        dest_file.write_text(html_content, encoding="utf-8")
        log_info(f"Processed {html_file} -> {dest_file}")


@task(pre=[clean, css_min, js_min, images, html])
def build(ctx: Context) -> None:
    """ Run all tasks: clean, minify CSS/JS, copy images and HTML """
    log_step("Build complete!")


"""
Firmware Build System
"""


def run_idf_py(cmd: str) -> None:
    """
    Execute ESP-IDF command with proper environment setup.

    Sources ESP-IDF environment from $IDF_PATH/export.sh then runs idf.py command.
    Logs both successful and failed build output to rotating log files.
    Raises Exit on failure with non-zero exit code.
    """
    try:
        project_path = os.getcwd()
        idf_path = os.getenv('IDF_PATH')
        if not idf_path:
            log_error(f"Variable IDF_PATH is not defined.")
            raise Exit("Unable to continue, exiting.", code=1)
        exp_cmd = ['bash', '-c', f"source {idf_path}/export.sh && idf.py {cmd}"]
        result = subprocess.run(exp_cmd, cwd=project_path, capture_output=True, text=True, check=True)
        build_logger(result.stdout)
    except subprocess.CalledProcessError as e:
        build_logger(f"BUILD FAILED\n\n=== STDOUT ===\n{e.stdout}\n\n=== STDERR ===\n{e.stderr}")
        log_error(f"Subprocess error: {e}")
        raise Exit("Unable to continue, exiting.", code=1)


@task
def check_permissions(ctx: Context) -> None:
    """ Check if running with proper permissions """
    log_step("Checking permissions")
    username = os.getlogin()
    user_info = pwd.getpwnam(username)
    primary_gid = user_info.pw_gid
    gids = os.getgrouplist(username, primary_gid)
    user_groups = set(grp.getgrgid(gid).gr_name for gid in gids)
    groups = {'dialout', 'uucp'}
    if groups.isdisjoint(user_groups):
        log_error(f"User [{username}] not in dialout/uucp group. Run with: sg uucp -c \"<COMMAND>\"")
        raise Exit("Unable to continue, exiting.", code=1)
    else:
        log_info(f"User [{username}] has permission to run dialout/uucp")


@task
def check_device(ctx: Context) -> None:
    """ Check if device is connected """
    log_step("Checking device connection")
    try:
        ser = serial.Serial(PORT, timeout=0.1)
        ser.close()
        log_info(f"Device found at {PORT}")
    except serial.SerialException as e:
        log_error(f"Device not found at {PORT}: {e}")
        raise Exit("Unable to continue, exiting.", code=1)


@task
def build_firmware(ctx: Context) -> None:
    """ Build ESP32 firmware using idf.py """
    log_step("Building project")
    log_info("Running: idf.py build")
    run_idf_py('build')
    log_info("Build successful!")


@task
def flash_firmware(ctx: Context) -> None:
    """ Flash everything (bootloader, partition table, firmware) """
    log_step("Flashing bootloader, partition table, and firmware")
    log_info(f"Running: idf.py -p {PORT} -b {BAUD} flash")
    run_idf_py(f"-p \"{PORT}\" -b \"{BAUD}\" flash")
    log_info("Flash completed successfully")


@task
def flash_filesystem(ctx: Context, force: bool = False) -> None:
    """ Flash filesystem (web files) to storage partition """
    log_step("Creating and flashing filesystem")

    # Check if web assets have been built
    data_www_path = Path(DATA_DIR) / "www"
    if not data_www_path.exists():
        log_error(f"Directory '{data_www_path}' not found")
        log_error("Please run 'invoke build' first to generate web assets")
        raise Exit("Unable to continue, exiting.", code=1)

    data_path = Path(DATA_DIR)
    log_info(f"Data directory: {DATA_DIR}")

    # Parse partition table to get offset and size
    log_info(f"Reading partition information for '{PARTITION_NAME}'...")

    partition_file = Path("partitions.csv")
    if not partition_file.exists():
        log_error("partitions.csv not found")
        raise Exit("Unable to continue, exiting.", code=1)

    partition_line = None
    partition_lines = []
    with open(partition_file, 'r') as f:
        for line in f:
            stripped = line.strip()
            # Skip comments and empty lines
            if stripped and not stripped.startswith('#'):
                partition_lines.append(stripped)
                if stripped.startswith(f"{PARTITION_NAME},"):
                    partition_line = stripped

    if not partition_line:
        log_error(f"Could not find partition '{PARTITION_NAME}' in partitions.csv")
        raise Exit("Unable to continue, exiting.", code=1)

    # Parse CSV line: name, type, subtype, offset, size, flags
    parts = [p.strip() for p in partition_line.split(',')]
    if len(parts) < 5:
        log_error(f"Invalid partition format in partitions.csv: {partition_line}")
        raise Exit("Unable to continue, exiting.", code=1)

    offset = parts[3].strip()
    size_str = parts[4].strip()

    # If offset is empty, calculate it from previous partitions
    if not offset:
        log_info("Partition offset is empty, calculating from previous partitions...")
        current_offset = 0
        for prev_line in partition_lines:
            if prev_line == partition_line:
                break
            prev_parts = [p.strip() for p in prev_line.split(',')]
            if len(prev_parts) >= 5:
                prev_offset_str = prev_parts[3].strip()
                prev_size_str = prev_parts[4].strip()

                if prev_offset_str:
                    if prev_offset_str.startswith('0x'):
                        current_offset = int(prev_offset_str, 16)
                    else:
                        current_offset = int(prev_offset_str)

                if prev_size_str:
                    if prev_size_str.endswith('M'):
                        current_offset += int(prev_size_str[:-1]) * 1024 * 1024
                    elif prev_size_str.endswith('K'):
                        current_offset += int(prev_size_str[:-1]) * 1024
                    elif prev_size_str.startswith('0x'):
                        current_offset += int(prev_size_str, 16)
                    else:
                        current_offset += int(prev_size_str)

        offset = hex(current_offset)
        log_info(f"Calculated partition offset: {offset}")

    if not size_str:
        log_error(f"Partition size is empty in partitions.csv")
        raise Exit("Unable to continue, exiting.", code=1)

    # Convert size to bytes (handles K, M suffixes)
    try:
        if size_str.endswith('M'):
            size = int(size_str[:-1]) * 1024 * 1024
        elif size_str.endswith('K'):
            size = int(size_str[:-1]) * 1024
        elif size_str.startswith('0x'):
            size = int(size_str, 16)
        else:
            size = int(size_str)
    except ValueError as e:
        log_error(f"Invalid size format: {size_str}")
        raise Exit("Unable to continue, exiting.", code=1)

    log_info(f"Partition offset: {offset}")
    log_info(f"Partition size: {size} bytes")

    # Check if mklittlefs is available
    mklittlefs_available = shutil.which('mklittlefs') is not None

    if not mklittlefs_available:
        log_warn("mklittlefs tool not found - skipping filesystem flash")
        log_warn("LittleFS will auto-format on first boot (empty filesystem)")
        log_info("")
        log_info("To flash web files, you have two options:")
        log_info("  1. Install mklittlefs from: https://github.com/earlephilhower/mklittlefs/releases")
        log_info("  2. Upload files to device after boot using a file upload mechanism")
        log_info("")
        return

    # Determine if rebuild is needed
    fs_image_path = Path(FS_IMAGE)
    needs_rebuild = False

    if force:
        log_info("Force filesystem flash requested")
        needs_rebuild = True
    elif not fs_image_path.exists():
        log_info("Filesystem image not found, will create new image")
        needs_rebuild = True
    else:
        # Check if any source file is newer than the filesystem image
        log_info("Checking if source files are newer than filesystem image...")
        fs_image_mtime = fs_image_path.stat().st_mtime

        for file_path in data_path.rglob('*'):
            if file_path.is_file() and file_path.stat().st_mtime > fs_image_mtime:
                log_info(f"File modified: {file_path.name}")
                needs_rebuild = True
                break

        if not needs_rebuild:
            log_info("Filesystem image is up-to-date, skipping rebuild")

    # Rebuild filesystem image if needed
    if needs_rebuild:
        log_info("Creating LittleFS image with mklittlefs...")
        block_size = 4096
        page_size = 256

        # Create temporary directory and copy data/ contents directly
        tmp_fs_dir = Path(tempfile.mkdtemp())

        try:
            # Copy data directory contents to temp root
            for item in data_path.iterdir():
                if item.is_file():
                    shutil.copy2(item, tmp_fs_dir / item.name)
                elif item.is_dir():
                    shutil.copytree(item, tmp_fs_dir / item.name)

            # Ensure build directory exists
            fs_image_path.parent.mkdir(parents=True, exist_ok=True)

            # Create filesystem image
            result = subprocess.run(
                [
                    'mklittlefs',
                    '-c', str(tmp_fs_dir),
                    '-b', str(block_size),
                    '-p', str(page_size),
                    '-s', str(size),
                    str(fs_image_path)
                ],
                capture_output=True,
                text=True,
                check=True
            )
        except subprocess.CalledProcessError as e:
            log_error(f"Failed to create filesystem image: {e}")
            if e.stdout:
                log_error(f"stdout: {e.stdout}")
            if e.stderr:
                log_error(f"stderr: {e.stderr}")
            raise Exit("Unable to continue, exiting.", code=1)
        finally:
            # Cleanup temp directory
            if tmp_fs_dir.exists():
                shutil.rmtree(tmp_fs_dir)

        log_info(f"Filesystem image created: {FS_IMAGE}")
    else:
        log_info("Filesystem image is up-to-date, skipping rebuild")

    # Always flash the image (even if rebuild was skipped)
    if not fs_image_path.exists():
        log_error(f"Filesystem image not found: {FS_IMAGE}")
        raise Exit("Unable to continue, exiting.", code=1)

    log_info(f"Flashing filesystem to offset {offset}...")

    idf_path = os.getenv('IDF_PATH')
    if not idf_path:
        log_error(f"Variable IDF_PATH is not defined.")
        raise Exit("Unable to continue, exiting.", code=1)
    # Flash filesystem image using esptool
    try:
        result = subprocess.run(
            [
                'bash', '-c',
                f'source {idf_path}/export.sh && python -m esptool -p {PORT} -b {BAUD} write_flash {offset} {str(fs_image_path)}'
            ],
            capture_output=True,
            text=True,
            check=True
        )
        log_info("Filesystem flashed successfully")
    except subprocess.CalledProcessError as e:
        log_error(f"Filesystem flash failed: {e}")
        if e.stdout:
            log_error(f"stdout: {e.stdout}")
        if e.stderr:
            log_error(f"stderr: {e.stderr}")
        raise Exit("Unable to continue, exiting.", code=1)


@task(pre=[check_permissions, check_device, build_firmware, flash_firmware, flash_filesystem])
def flash(ctx: Context) -> None:
    """ Complete workflow: validate permissions, check device, build and flash firmware """
    log_step("Build, flash and FS upload complete!")
