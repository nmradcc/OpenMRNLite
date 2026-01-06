# Migration Guide for Optional Modules

## Overview

Starting with this version, OpenMRNLite makes the BLE and DCC modules optional and **disabled by default**. This is a **breaking change** - existing projects that use BLE or DCC features will need to explicitly enable these modules.

## What Changed

### Before
All modules (BLE and DCC) were always compiled into the library.

### After
- BLE and DCC modules are **optional** and controlled by CMake options
- By default, both modules are **enabled** (backward compatible)
- New CMake options: `OPENMRN_ENABLE_BLE` and `OPENMRN_ENABLE_DCC`
- New feature flags: `OPENMRN_FEATURE_BLE` and `OPENMRN_FEATURE_DCC`

## Impact on Existing Projects

### Action Required (Breaking Change)
If your project uses BLE or DCC features, you **must** enable them explicitly:
- Both BLE and DCC modules are now **disabled by default**
- Projects using these features will fail to compile without enabling them
- Modifications to your CMakeLists.txt are required if you use these modules

### Enabling Required Modules
If your project uses BLE or DCC features:

1. **Identify which modules you use:**
   - Do you use any BLE features? (files in `src/ble/`)
   - Do you use any DCC features? (files in `src/dcc/`, traction features)

2. **Update your CMake configuration:**
   ```cmake
   # In your project's CMakeLists.txt, before including OpenMRNLite:
   set(OPENMRN_ENABLE_BLE ON)  # If you use BLE
   set(OPENMRN_ENABLE_DCC ON)  # If you use DCC
   ```

   Or via command line:
   ```bash
   cmake -DOPENMRN_ENABLE_BLE=ON -DOPENMRN_ENABLE_DCC=ON ..
   ```

3. **Add feature guards in your code (optional):**
   If you have conditional code that depends on these modules:
   ```cpp
   #include "openmrn_features.h"
   
   #if OPENMRN_FEATURE_DCC
       // DCC-dependent code
   #endif
   ```

## Code That May Be Affected

### If You Disable DCC Module

The following OpenLCB components have DCC dependencies and may require modifications:

- `openlcb/TractionCvSpace.hxx` - CV programming support
- `openlcb/TractionProxy.cpp` - Proxy for DCC locomotives
- `openlcb/TrainInterface.hxx` - Train control interface
- `openlcb/TractionDefs.hxx` - Traction definitions
- `openlcb/DccAccyConsumer.hxx` - DCC accessory decoder consumer

**Solution:** Either keep DCC enabled, or avoid using these specific OpenLCB traction features.

### If You Disable BLE Module

The BLE module is self-contained and has no dependencies in other parts of the library.

**Solution:** Simply don't include BLE headers in your application code.

## Testing Your Migration

1. **With modules disabled (default):**
   ```bash
   cmake ..
   cmake --build .
   # Check for compilation errors
   # If errors occur, you need to enable required modules
   ```

2. **With modules enabled:**
   ```bash
   cmake -DOPENMRN_ENABLE_BLE=ON -DOPENMRN_ENABLE_DCC=ON ..
   cmake --build .
   # Verify your application works with the modules you need
   ```

## Examples

### Example 1: DCC-Only Project
```cmake
# CMakeLists.txt
# BLE is OFF by default, no need to set it
set(OPENMRN_ENABLE_DCC ON)   # Need DCC for track control

add_subdirectory(OpenMRNLite)
target_link_libraries(my_project OpenMRNLite)
```

### Example 2: Minimal OpenLCB Project
```cmake
# CMakeLists.txt
# Both modules are OFF by default - no configuration needed

add_subdirectory(OpenMRNLite)
target_link_libraries(my_project OpenMRNLite)
```

### Example 3: Full-Featured Project with Both Modules
```cmake
# CMakeLists.txt
set(OPENMRN_ENABLE_BLE ON)   # Need BLE support
set(OPENMRN_ENABLE_DCC ON)   # Need DCC support

add_subdirectory(OpenMRNLite)
target_link_libraries(my_project OpenMRNLite)
```

### Example 4: Using CMake Presets
```bash
# Copy the example presets file
cp CMakePresets.example.json CMakePresets.json

# Build with default preset (all modules enabled)
cmake --preset default
cmake --build --preset default
```

## Benefits of This Change

- **Smaller binaries by default:** Minimal footprint for projects that don't need BLE/DCC
- **Faster builds:** Fewer files to compile by default
- **Explicit dependencies:** Projects clearly declare what features they use
- **Better for embedded:** Optimized flash and RAM usage out of the box

## Support

If you encounter issues during migration:

1. Check the [OPTIONAL_MODULES.md](OPTIONAL_MODULES.md) documentation
2. Review the [BUILD.md](BUILD.md) for build instructions
3. Verify your CMake version is 3.12 or newer
4. Check that feature flags are properly defined in your build

## Upgrading from Previous Versions

If you're upgrading from a version where these modules were enabled by default:

```cmake
# Enable modules that were previously included by default
set(OPENMRN_ENABLE_BLE ON)
set(OPENMRN_ENABLE_DCC ON)
```

This restores the behavior of previous versions where both modules were always included.
