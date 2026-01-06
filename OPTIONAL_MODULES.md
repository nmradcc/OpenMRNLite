# Optional Modules

OpenMRNLite supports optional modules that can be enabled or disabled at build time to reduce code size and dependencies for projects that don't need certain features.

## Available Optional Modules

### BLE (Bluetooth Low Energy) Module

Located in `src/ble/`, this module provides Bluetooth Low Energy support for OpenMRN.

**CMake Option:** `OPENMRN_ENABLE_BLE`
- Default: `OFF`
- Feature Flag: `OPENMRN_FEATURE_BLE`

**Components:**
- Advertisement support
- Connection management
- Service definitions
- BLE-specific definitions and utilities

### DCC (Digital Command Control) Module

Located in `src/dcc/`, this module provides Digital Command Control protocol support for model railroads.

**CMake Option:** `OPENMRN_ENABLE_DCC`
- Default: `OFF`
- Feature Flag: `OPENMRN_FEATURE_DCC`

**Components:**
- DCC packet generation and processing
- Locomotive control
- Track interface
- RailCom support
- Programming track backend
- DCC update loops

## Usage

### Enabling Optional Modules

By default, both modules are **disabled**. To enable one or both modules during CMake configuration:

```bash
# Enable BLE support
cmake -DOPENMRN_ENABLE_BLE=ON ..

# Enable DCC support
cmake -DOPENMRN_ENABLE_DCC=ON ..

# Enable both modules
cmake -DOPENMRN_ENABLE_BLE=ON -DOPENMRN_ENABLE_DCC=ON ..
```

### Using Feature Flags in Code

The build system defines feature flags that can be used in your code:

```cpp
#include "openmrn_features.h"

#if OPENMRN_FEATURE_DCC
    // DCC-specific code
    #include "dcc/Loco.hxx"
    // ... use DCC features
#endif

#if OPENMRN_FEATURE_BLE
    // BLE-specific code
    #include "ble/Service.hxx"
    // ... use BLE features
#endif
```

### Module Dependencies

**BLE Module:**
- Self-contained within `src/ble/`
- No external dependencies on other optional modules

**DCC Module:**
- Used by some OpenLCB traction features in `src/openlcb/`:
  - `TractionCvSpace.hxx`
  - `TractionProxy.cpp`
  - `TrainInterface.hxx`
  - `TractionDefs.hxx`
  - `DccAccyConsumer.hxx`
  
**Note:** If you disable the DCC module and use OpenLCB traction features, you may need to wrap DCC-dependent code with `#if OPENMRN_FEATURE_DCC` guards or avoid using those specific traction features.

## Benefits

Disabling unused modules provides:
- **Reduced binary size** - Smaller firmware footprint
- **Faster compilation** - Fewer files to compile
- **Clearer dependencies** - Only include what you need
- **Resource savings** - Less RAM and flash usage on embedded systems

## Default Configuration

By default, both modules are **disabled** to provide a minimal footprint. Projects that need BLE or DCC support must explicitly enable them via CMake options.

## CMake Presets

You can create CMake presets in `CMakePresets.json` to easily switch between different configurations:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "minimal",
      "description": "Minimal build without BLE and DCC",
      "cacheVariables": {
        "OPENMRN_ENABLE_BLE": "OFF",
        "OPENMRN_ENABLE_DCC": "OFF"
      }
    },
    {
      "name": "dcc-only",
      "description": "Build with DCC support only",
      "cacheVariables": {
        "OPENMRN_ENABLE_BLE": "OFF",
        "OPENMRN_ENABLE_DCC": "ON"
      }
    }
  ]
}
```

Then use: `cmake --preset minimal ..`
