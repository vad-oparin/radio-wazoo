# RADIO WAZOO

## Setup

### Partition Table Setup

The project includes partition table templates for different flash sizes. Choose one based on your ESP32 module:

**Create your partition table:**
```bash
cp partitions_4MB.csv partitions.csv  # For 4MB flash (most common)
```

**Available templates:**
- `partitions_2MB.csv` - 1.28MB app, 704KB storage
- `partitions_4MB.csv` - 1.5MB app, 2.4MB storage
- `partitions_8MB.csv` - 2MB app, 5.9MB storage
- `partitions_16MB.csv` - 3MB app, 13MB storage

Customize `partitions.csv` for your project needs (e.g., adjust storage size for web assets). The file is gitignored as it's project-specific.

### Target Configuration

The project uses ESP-IDF's automatic config merging for multi-target support:

**Set your target chip:**
```bash
idf.py set-target esp32s2  # or esp32, esp32s3, esp32c3, etc.
```

**Configuration files:**
- `sdkconfig.defaults` - Universal settings (all chips)
- `sdkconfig.defaults.esp32s2` - ESP32-S2 specific (USB CDC console)

ESP-IDF automatically merges the appropriate chip-specific config. To add support for other chips, create `sdkconfig.defaults.<chip>` files.

### Building Web Assets

The web interface files need to be minified and processed before being embedded into the ESP32 firmware.

1. **Create a Python virtual environment:**
   ```bash
   python -m venv .venv
   ```
   This creates an isolated Python environment to avoid conflicts with system packages.

2. **Activate the virtual environment:**
   ```bash
   source .venv/bin/activate
   ```
   On Windows, use `.venv\Scripts\activate` instead.

3. **Install build dependencies:**
   ```bash
   pip install -r requirements.txt
   ```
   This installs Invoke (task runner), rcssmin (CSS minifier), and rjsmin (JavaScript minifier).

4. **Build the web assets:**
   ```bash
   invoke build
   ```
   This processes files from `src_www/` and outputs minified versions to `data/www/`, which will be embedded into the ESP32 firmware.

   To see all available tasks, run `invoke --list`.
