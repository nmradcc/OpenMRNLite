# OpenMRNLite + ESP-IDF

This directory contains a minimal ESP-IDF project that builds OpenMRNLite as a local component. It is intended as a starting point you can copy into your own ESP-IDF application or use directly for experiments.

## Prerequisites
- ESP-IDF v5 or newer installed and `IDF_PATH` set
- An ESP32 target toolchain installed (for example `esp32`, `esp32s3`, etc.)

## Building
1. Select your target (replace `esp32s3` with your chip):
   ```
   idf.py -C examples/esp-idf set-target esp32s3
   ```
2. Configure (optional):
   ```
   idf.py -C examples/esp-idf menuconfig
   ```
3. Build and flash:
   ```
   idf.py -C examples/esp-idf build
   idf.py -C examples/esp-idf flash monitor
   ```

The example application just logs a boot message but links the full OpenMRNLite library so you can start wiring in real functionality.

## Using OpenMRNLite in another ESP-IDF project
- Add this repository path to `EXTRA_COMPONENT_DIRS` (or copy `examples/esp-idf/components/openmrnlite` into your project's `components/` folder).
- Link your component against `openmrnlite` using `PRIV_REQUIRES openmrnlite` or `REQUIRES openmrnlite`.
- Include headers from `OpenMRNLite/src` as needed (for example `#include "OpenMRNLite.h"`).
