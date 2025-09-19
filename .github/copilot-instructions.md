# Copilot Instructions for Meshtastic Firmware

## Architecture Overview

**Multi-platform LoRa mesh firmware** supporting ESP32, nRF52, RP2040/RP2350, STM32, and Linux devices.

### Key Components

- **`src/`** - Core firmware logic: mesh networking, modules, graphics, hardware abstraction
- **`arch/`** - Platform-specific implementations (ESP32, nRF52, etc.) with dedicated `.ini` configs
- **`variants/`** - Device-specific configurations (GPIO pins, displays, sensors)
- **`src/mesh/generated/meshtastic/`** - Auto-generated protobuf message definitions
- **`src/modules/`** - Feature modules extending base `MeshModule` class
- **`src/graphics/`** - Display system with Screen framework and UI components

## Critical Development Workflows

### Build System

- **Target-specific builds:** `pio run -e heltec-v3` (replace with your target from `platformio.ini`)
- **Multi-config system:** `platformio.ini` includes `arch/*/*.ini` and `variants/*/*/platformio.ini`
- **Build flags:** Extensive RadioLib exclusions and platform optimizations in `[env]`

### Hardware Detection & Configuration

- **I2C scanning:** `detect/ScanI2C.h` - auto-detects displays, keyboards, sensors
- **Conditional compilation:** Heavy use of `#if HAS_SCREEN`, `#if !MESHTASTIC_EXCLUDE_*`
- **Hardware variants:** Each device in `variants/` has specific GPIO, display, and radio configs

### Module Development Pattern

```cpp
// Extend MeshModule for new features
class YourModule : public ProtobufModule<meshtastic_YourProto>
{
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    virtual bool handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_YourProto *proto) override;
    // Register in src/modules/Modules.cpp
};
```

## UI & Graphics Architecture

### Screen Framework

- **Frame-based UI:** `graphics::Screen` manages multiple display frames (message, node info, settings)
- **Focus management:** Modules call `requestFocus()` to show their UI, `screen->setFrames(FOCUS_PRESERVE)` to release
- **Input handling:** Unified InputBroker supports physical buttons, CardKB, rotary encoders
- **Custom callbacks:** For non-message scenarios (WiFi config), use custom callback pattern like `LaunchFreetextKbPrompt()`

### Display Support

- **Multi-display:** OLED (SSD1306, SH1106), E-Ink, TFT with auto-detection
- **Conditional UI:** `#if HAS_SCREEN` guards all display code
- **Notification system:** `NotificationRenderer` handles banners, text input, overlays

## Protobuf & Messaging

### Generated Code

- **Source:** External protobuf definitions (regenerate with `bin/regen-protos.sh`)
- **Location:** `src/mesh/generated/meshtastic/*.pb.{h,cpp}`
- **Usage:** Each module typically handles one protobuf message type

### Message Flow

```cpp
// Typical module message handling
ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) {
    if (mp.decoded.portnum == meshtastic_PortNum_YOUR_APP) {
        // Process your message type
        return ProcessMessage::STOP; // or CONTINUE
    }
    return ProcessMessage::CONTINUE;
}
```

## Platform-Specific Patterns

### ESP32 Specifics

- **WiFi integration:** `src/mesh/wifi/` and `src/mesh/http/`
- **Bluetooth:** NimBLE stack in `src/nimble/`
- **Power management:** Advanced sleep modes, battery monitoring

### Hardware Integration

- **CardKB detection:** Global `kb_found` variable from InputBroker
- **Sensor modules:** Auto-detected I2C sensors with modular drivers
- **Radio interfaces:** RadioLib-based with chip-specific classes (`SX1262Interface`, etc.)

## Critical Development Notes

### Code Style & Contributions

- **English comments only** - translate any Spanish comments
- **Incremental changes** - insert new code, avoid moving/deleting existing code
- **Pull request oriented** - changes expected as PRs to upstream
- **Hardware conditional** - always guard hardware-specific code with appropriate `#if` directives

### Current Focus Areas

- **WiFi menu system** - ongoing work on WiFi configuration UI
- **Chat history** - screen-based message history with node/channel support
- **CardKB integration** - physical keyboard support for input scenarios

### Common Issues

- **UI focus management** - ensure proper `requestFocus()`/`setFrames()` lifecycle
- **Memory constraints** - especially on nRF52, use `MESHTASTIC_EXCLUDE_*` flags
- **Platform differences** - test across ESP32/nRF52 for compatibility

## Essential Commands

```bash
# Build for specific target
pio run -e heltec-v3

# Flash with auto-detection
bin/device-install.sh  # or .bat on Windows

# Regenerate protobuf files
bin/regen-protos.sh

# Check all targets build
bin/check-all.sh
```

ojo con los espacios trunk
