# Build and installation

## DevKit local inference

Requires an ESP32-S3 DevKit with **32 MB octal Flash and 16 MB PSRAM**. This is not the generic 8 MB XIAO target.

Install PlatformIO, then from the repository root:

```sh
cd espdev_ai/firmware
pio run -c platformio.ini -e local_devkit
```

The platform URL and framework packages are pinned in the configuration. The build generates bootloader, partitions and firmware under `.pio/build/local_devkit/`. Only the environment with supplied source is exposed.

Weights/tokenizer assets are separate and not yet published. Once reviewed assets are available, verify `python3 tools/verify_pack.py PATH_TO_DEVKIT_WEIGHTS` before installation. Required component addresses:

| Component | Flash address |
|---|---|
| built bootloader.bin | 0x0 |
| built partitions.bin | 0x8000 |
| built firmware.bin | 0x10000 |
| weights/weights_q4.bin | 0x410000 |
| weights/scales_f16.bin | 0x1210000 |
| weights/kept_tensors.bin | 0x1310000 |
| tokenizer/tokenfs.bin | 0x1E30000 |

Use esptool for the exact ESP32-S3 and your selected port, with the flash settings from the build. Clear stale OTA selection at `0xE000` (0x2000 bytes) when moving from another firmware. Back up privately before destructive operations. Verify each written component. At boot, require `[TIED30M_PACKAGE] PASS`, `[TIED30M_HEAD_CRC] PASS`, then `[READY]`; do not bypass these checks.

No flashing command is run automatically by these instructions or by the offline check scripts.

## XIAO network keyboard (Gemini / Ollama)

Follow [NETWORK_SETUP.md](NETWORK_SETUP.md) for hardware assembly, Arduino setup, runtime settings, API/LAN configuration and troubleshooting. Provided as compile-checked code; hardware testing is not part of this release scope.

Open `xiao_ai/keyboard_interfaces/xiao_network/xiao_network.ino` in Arduino IDE. Use ESP32 core **2.0.17**, ArduinoJson **7.4.3**, XIAO_ESP32S3, USB CDC enabled. The display library and setup are included. Serial Monitor: **115200**, Newline.

Enter the requested backend, Wi-Fi credentials, API key or Ollama LAN URL, and exact model ID at runtime. See the interface README for the input sequence. Secrets are not hardcoded; keep the input field out of recordings.

This is a separate firmware. A generic Arduino partition map may overwrite the offline model. Preserve private recovery files before changing firmware.

## XIAO local keyboard — built and serial-tested on hardware

Use the pinned PlatformIO project in `xiao_ai/keyboard_interfaces/xiao_local`. It requires a GCC 16.1.0 Xtensa package selected by `POCKETGPT_GCC16_PACKAGE`. Follow [TOOLCHAIN.md](TOOLCHAIN.md) for the official download, checksum and tested PlatformIO setup on Apple Silicon macOS.

```sh
cd xiao_ai/keyboard_interfaces/xiao_local
export POCKETGPT_GCC16_PACKAGE=/path/to/toolchain-package
pio run
```

Build all libraries from this project. Do not reuse link archives from another configuration. The verified full build used platform 54.3.20, Arduino 3.2.0, framework libs 5.4.0+sha.2f7dcd862a, GCC 16.1.0+20260609 and the pinned Seeed TFT commit. The TFT setup include directory must be available when compiling framework libraries too; this is now in the configuration.

The resulting firmware binary is **450,672 bytes**, with **3,984 bytes** before the fixed K10 cache. The previously linked 468,752-byte binary used cached libraries from another build and must not be used. Code, K10 and app-tail addresses remain unchanged; the coredump partition is retained.

The post-build and pre-upload checks reject a firmware larger than **454,656 bytes**. Fitting the 0xA0000 app partition alone is insufficient. Do not bypass the guard.

Package a tested-size firmware together with the reviewed cache components from the repository root:

```sh
python3 tools/package_xiao.py \
  --build-dir xiao_ai/keyboard_interfaces/xiao_local/.pio/build/local_xiao \
  --assets-dir /path/to/reviewed-assets/xiao \
  --out-dir /path/to/new-candidate
```

The packager verifies exact hashes of both caches and the model partition, checks the partition layout, fills unused app space with 0xFF and writes a new checksum manifest. It never opens a serial port or flashes hardware. SD files remain separate in the reviewed assets.

| Component | Address / destination |
|---|---|
| built bootloader.bin | 0x0 |
| built partitions.bin | 0x8000 |
| packaged app_with_tail.bin (includes K10) | 0x10000 |
| model.bin | 0xB0000 |
| reviewed SD files | `/pocketgpt_xiao_v7b50_final_v1/` on microSD |

K10 is embedded at physical 0x7F000 (26,656 bytes); app-tail is at 0x86000 (159,936 bytes). Install the packaged app with its caches for a fresh deployment. Uploading the bare firmware does not provision caches on a blank board.

No NVS image is distributed. Keep any recovery backup private. This candidate was installed after a verified private backup on 2026-10-13. Boot/cache checks passed and three USB Serial inference tests completed. The user confirmed the keyboard is visible; digits, backspace, ABC switching and GO were also confirmed by the user. New evidence is in `xiao_ai/recorded_results/ui_2026-10-13/`.
