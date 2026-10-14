# Run PocketGPT on ESP32-S3

The model runs on the board without Wi-Fi or an API key. You will download the trained model, build the firmware and install both. Follow **Prepare your computer**, then choose **DevKit** or **XIAO** below.

## Prepare your computer

You need Git, Python 3, a USB data cable and the matching hardware listed below. These commands use macOS/Linux shell syntax; builds were checked on Apple Silicon macOS.

### 1. Download the source and install tools

Run in a folder where you want to keep the project:

```sh
git clone https://github.com/mmm1712/Local_AI_esp32.git
cd Local_AI_esp32
export POCKETGPT_REPO="$PWD"
python3 -m venv .venv
source .venv/bin/activate
python -m pip install platformio==6.1.19 esptool==4.11.0
```

Keep this terminal open for the remaining steps. If you already downloaded the source as a ZIP, open its extracted folder and start at `export POCKETGPT_REPO="$PWD"`.

### 2. Download the model for your board

| Hardware | Model package |
|---|---|
| ESP32-S3 DevKit, **32 MB octal Flash / 16 MB PSRAM** | [DevKit ZIP](https://github.com/mmm1712/Local_AI_esp32/releases/download/v0.1.0-model-assets/pocketgpt-devkit-assets-v0.1.0.zip) |
| XIAO ESP32-S3, **8 MB Flash / 8 MB PSRAM**, Seeed Round Display and microSD | [XIAO ZIP](https://github.com/mmm1712/Local_AI_esp32/releases/download/v0.1.0-model-assets/pocketgpt-xiao-assets-v0.1.0.zip) |

Download [SHA256SUMS.txt](https://github.com/mmm1712/Local_AI_esp32/releases/download/v0.1.0-model-assets/SHA256SUMS.txt) into the same folder as your ZIP. In that folder, check the download:

```sh
shasum -a 256 --ignore-missing -c SHA256SUMS.txt
```

Continue only when **your downloaded ZIP is listed with `OK`**. You only need one board's ZIP. Extract it outside the source folder and keep its included license and attribution files. On Windows, use `Get-FileHash filename.zip -Algorithm SHA256` and compare with the matching line in `SHA256SUMS.txt`.

### 3. Find your USB port

Connect the board, then run:

```sh
python -m serial.tools.list_ports
export POCKETGPT_PORT=/dev/cu.usbmodemYOURPORT
```

Replace the example with your board's port, such as `/dev/ttyACM0` on Linux. Close Serial Monitor before flashing. Installation replaces the current application; keep any recovery backup private. Use only the addresses for your selected board.

## DevKit

Requires **32 MB octal Flash / 16 MB PSRAM**. Complete the shared preparation above first.

### 1. Build and check the model

Set `DEVKIT_ASSETS` to the extracted `devkit` folder, then run:

```sh
cd "$POCKETGPT_REPO"
export DEVKIT_ASSETS="/absolute/path/to/pocketgpt-devkit-assets/devkit"
pio run -d espdev_ai/firmware -e local_devkit
python tools/verify_pack.py "$DEVKIT_ASSETS/weights"
```

The build must finish with `SUCCESS`, and the model check with `PASS`.

### 2. Install

```sh
python -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" erase_region 0xE000 0x2000
python -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" --baud 460800 write_flash \
  0x0 espdev_ai/firmware/.pio/build/local_devkit/bootloader.bin \
  0x8000 espdev_ai/firmware/.pio/build/local_devkit/partitions.bin \
  0x10000 espdev_ai/firmware/.pio/build/local_devkit/firmware.bin \
  0x410000 "$DEVKIT_ASSETS/weights/weights_q4.bin" \
  0x1210000 "$DEVKIT_ASSETS/weights/scales_f16.bin" \
  0x1310000 "$DEVKIT_ASSETS/weights/kept_tensors.bin" \
  0x1E30000 "$DEVKIT_ASSETS/tokenizer/tokenfs.bin"
python -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" verify_flash \
  0x10000 espdev_ai/firmware/.pio/build/local_devkit/firmware.bin \
  0x410000 "$DEVKIT_ASSETS/weights/weights_q4.bin" \
  0x1210000 "$DEVKIT_ASSETS/weights/scales_f16.bin" \
  0x1310000 "$DEVKIT_ASSETS/weights/kept_tensors.bin" \
  0x1E30000 "$DEVKIT_ASSETS/tokenizer/tokenfs.bin"
```

### 3. Chat over USB

```sh
python -m serial.tools.miniterm "$POCKETGPT_PORT" 115200 --eol LF --echo
```

Reset the board. Wait for `[TIED30M_PACKAGE] PASS`, `[TIED30M_HEAD_CRC] PASS` and `[READY]`. Type `hello` and press Enter. Exit the monitor with **Ctrl+]**. Do not continue if a model check fails.

## XIAO with touchscreen keyboard

Requires **XIAO ESP32-S3, Seeed Round Display and microSD**. Complete the shared preparation above first. With power disconnected, attach the XIAO using the [Seeed assembly guide](https://wiki.seeedstudio.com/get_start_round_display/), then reconnect USB.

### 1. Prepare the microSD

Use FAT32. From the extracted XIAO package, copy the whole folder:

```text
xiao/microsd/pocketgpt_xiao_v7b50_final_v1
```

Place it directly in the card's root. Keep all six files and their names. The card should contain `/pocketgpt_xiao_v7b50_final_v1/`, without an extra `microsd` folder above it.

### 2. Build

In the same terminal, complete the one-time [GCC16 compiler setup](TOOLCHAIN.md). It returns you to the source folder with the compiler selected. Then run:

```sh
cd "$POCKETGPT_REPO"
pio run -d xiao_ai/keyboard_interfaces/xiao_local -e local_xiao
```

Continue only after `SUCCESS`. The build checks that the firmware fits beside the model caches.

### 3. Package the firmware with the model

Set `XIAO_ASSETS` to the extracted `xiao` folder. Use a new output directory; `xiao-install` must not already exist.

```sh
export XIAO_ASSETS="/absolute/path/to/pocketgpt-xiao-assets/xiao"
python tools/package_xiao.py \
  --build-dir xiao_ai/keyboard_interfaces/xiao_local/.pio/build/local_xiao \
  --assets-dir "$XIAO_ASSETS" \
  --out-dir xiao-install
```

This checks the model components and produces the files below. Install `app_with_tail.bin`; a bare `firmware.bin` does not include the required caches.

### 4. Install

```sh
python -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" --baud 460800 write_flash \
  0x0 xiao-install/bootloader.bin \
  0x8000 xiao-install/partitions.bin \
  0x10000 xiao-install/app_with_tail.bin \
  0xB0000 xiao-install/model.bin
python -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" verify_flash \
  0x10000 xiao-install/app_with_tail.bin \
  0xB0000 xiao-install/model.bin
```

### 5. Chat on the touchscreen

Insert the microSD and reset. Type `hello` on the keyboard and press **GO**. Allow time for a response; the recorded `hello` test took about 6.4 seconds. Use **123/ABC** to switch pages and backspace to delete.

If the keyboard or model does not start, inspect the boot messages:

```sh
python -m serial.tools.miniterm "$POCKETGPT_PORT" 115200 --eol LF --echo
```

Reset with the monitor open. Check the SD folder and model/cache checks; stop if they report a mismatch. Exit with **Ctrl+]**.

[Build reference](BUILD.md) · [Recorded validation](RELEASE_STATUS.md) · [Model license and attribution](MODEL_RELEASE_NOTES.md)
