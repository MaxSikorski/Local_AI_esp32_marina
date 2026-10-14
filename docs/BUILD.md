# Build reference

For a first installation, follow [Run PocketGPT on ESP32-S3](ASSETS.md). It covers tools, downloads, building, flashing and the first prompt in order.

## DevKit local inference

- Hardware: ESP32-S3 DevKit with **32 MB octal Flash / 16 MB PSRAM**.
- Project: `espdev_ai/firmware`; PlatformIO environment: `local_devkit`.
- Main source: `espdev_ai/firmware/src/main.cpp`.
- The platform, framework libraries and Flash layout are pinned in `platformio.ini`.
- Use the matching DevKit release package. `tools/verify_pack.py` checks its hashes, size and shared embedding/head layout.

## XIAO local keyboard — built and serial-tested on hardware

- Hardware: **XIAO ESP32-S3, 8 MB Flash / 8 MB PSRAM**, Seeed Round Display and microSD.
- Project: `xiao_ai/keyboard_interfaces/xiao_local`; PlatformIO environment: `local_xiao`.
- Main source: `xiao_ai/keyboard_interfaces/xiao_local/src/main.cpp`.
- Compiler: official **GCC 16.1.0**, selected by `POCKETGPT_GCC16_PACKAGE`. See [compiler setup](TOOLCHAIN.md).
- The project pins the platform and Seeed TFT library. Build all libraries with this configuration.
- Firmware must be at most **454,656 bytes**. The build guard protects the fixed K10 cache; do not bypass it.
- `tools/package_xiao.py` checks the partition layout and exact model/cache hashes, then creates `app_with_tail.bin` and `model.bin` for installation. It does not flash hardware.
- Copy the six SD files into `/pocketgpt_xiao_v7b50_final_v1/` on the card.

Uploading only `firmware.bin` does not install the required model caches. Use the complete [XIAO installation steps](ASSETS.md#xiao-with-touchscreen-keyboard).

## XIAO network keyboard (Gemini / Ollama)

This is a separate firmware in `xiao_ai/keyboard_interfaces/xiao_network/xiao_network.ino`. It uses Arduino IDE, ESP32 core **2.0.17** and ArduinoJson **7.4.3**. Follow [NETWORK_SETUP.md](NETWORK_SETUP.md) for installation and runtime settings.

Switching firmware can overwrite the offline model layout. Return to PocketGPT using its complete installation guide.

## Source files

| File pattern | Purpose |
|---|---|
| `main.cpp` | Application entry point and inference |
| `model_storage.inc` | Read model matrices from Flash, PSRAM and SD |
| `output_projection.inc` | Output projection and repair rows |
| `model_*layout.h`, `model_offsets.h`, `model_storage_checksums.h` | Model locations and validation constants |
| `q4_*.inc`, `batched_*.inc`, `two_token_*.inc`, `prompt_prefill.inc` | Quantized matrix operations and prompt processing |
| `partitions_*.csv`, `check_firmware_size.py` | Flash layout and XIAO size guard |

`.inc` files are included by the application; they are not standalone programs. Board-specific layouts and model packages are not interchangeable. See [validation results](RELEASE_STATUS.md).
