# RADIO WAZOO

## Setup

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
