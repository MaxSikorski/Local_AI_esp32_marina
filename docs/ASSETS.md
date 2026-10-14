# Download model assets and install locally

The deployment weights and tokenizer are available in [GitHub Releases](https://github.com/mmm1712/Local_AI_esp32/releases/tag/v0.1.0-model-assets). Build the firmware from this repository and use the package matching your board. These files support the published deployment; they do not include the full historical training corpus or make exact retraining reproducible.

| Board | Download |
|---|---|
| ESP32-S3 DevKit, 32 MB Flash / 16 MB PSRAM | [pocketgpt-devkit-assets-v0.1.0.zip](https://github.com/mmm1712/Local_AI_esp32/releases/download/v0.1.0-model-assets/pocketgpt-devkit-assets-v0.1.0.zip) |
| XIAO ESP32-S3, 8 MB Flash / 8 MB PSRAM, Round Display and microSD | [pocketgpt-xiao-assets-v0.1.0.zip](https://github.com/mmm1712/Local_AI_esp32/releases/download/v0.1.0-model-assets/pocketgpt-xiao-assets-v0.1.0.zip) |

Also download [SHA256SUMS.txt](https://github.com/mmm1712/Local_AI_esp32/releases/download/v0.1.0-model-assets/SHA256SUMS.txt). Keep all three release files together; on macOS/Linux, run `shasum -a 256 -c SHA256SUMS.txt` before extracting. Both archives must be present for every line to report OK. Windows PowerShell can use `Get-FileHash filename.zip -Algorithm SHA256` to compare each hash manually. Each archive also contains ASSET_MANIFEST.json and its own checksum list.

MIT applies to the author's rights in these released weights/tokenizer. Preserve MODEL_LICENSE and ATTRIBUTION.md. See [release provenance and attribution](MODEL_RELEASE_NOTES.md). Dataset text and teacher-model weights are not included.

## Before installation

These commands write the selected board's Flash and replace its current application. Keep any recovery backup private. Use the matching board and asset package, close Serial Monitor before flashing, and replace `/dev/cu.usbmodemYOURPORT` with the actual port (`COM3` is an example on Windows). Never use the DevKit addresses on XIAO. Do not use force flags to bypass secure-boot, encryption or compatibility checks.

Install Python tools in an environment of your choice:

```sh
python3 -m pip install esptool==4.11.0
```

The examples below use esptool 4 command names. The [Espressif documentation](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html) explains write_flash and verify_flash. The default Flash mode/frequency are retained from the built bootloader; use the pinned board configuration from this project.

## DevKit

1. Extract the DevKit ZIP outside the source tree. It contains `pocketgpt-devkit-assets/devkit/weights/` and `devkit/tokenizer/`.
2. [Build the DevKit firmware](BUILD.md#devkit-local-inference). Return to the repository root afterward.
3. In the same terminal, set the asset location and port, then verify the model:

```sh
DEVKIT_ASSETS=/absolute/path/to/pocketgpt-devkit-assets/devkit
POCKETGPT_PORT=/dev/cu.usbmodemYOURPORT
python3 tools/verify_pack.py "$DEVKIT_ASSETS/weights"
```

4. Only when ready to replace the application, clear stale OTA selection and write the required components:

```sh
python3 -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" erase_region 0xE000 0x2000
python3 -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" --baud 460800 write_flash \
  0x0 espdev_ai/firmware/.pio/build/local_devkit/bootloader.bin \
  0x8000 espdev_ai/firmware/.pio/build/local_devkit/partitions.bin \
  0x10000 espdev_ai/firmware/.pio/build/local_devkit/firmware.bin \
  0x410000 "$DEVKIT_ASSETS/weights/weights_q4.bin" \
  0x1210000 "$DEVKIT_ASSETS/weights/scales_f16.bin" \
  0x1310000 "$DEVKIT_ASSETS/weights/kept_tensors.bin" \
  0x1E30000 "$DEVKIT_ASSETS/tokenizer/tokenfs.bin"
```

5. Compare the written application and runtime assets:

```sh
python3 -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" verify_flash \
  0x10000 espdev_ai/firmware/.pio/build/local_devkit/firmware.bin \
  0x410000 "$DEVKIT_ASSETS/weights/weights_q4.bin" \
  0x1210000 "$DEVKIT_ASSETS/weights/scales_f16.bin" \
  0x1310000 "$DEVKIT_ASSETS/weights/kept_tensors.bin" \
  0x1E30000 "$DEVKIT_ASSETS/tokenizer/tokenfs.bin"
```

6. Reset and open Serial Monitor at 115200 baud. Require `[TIED30M_PACKAGE] PASS`, `[TIED30M_HEAD_CRC] PASS` and `[READY]`. Send `hello` with a newline. The SPIFFS `tokenfs.bin` already includes the runtime tokenizer files; the additional SentencePiece/export files in the archive support inspection and tooling.

## XIAO with touchscreen keyboard

1. Extract the XIAO ZIP. It contains `pocketgpt-xiao-assets/xiao/`.
2. Use a microSD card readable by the project's Arduino SD library (FAT32 is the suggested format). Copy the entire folder `xiao/microsd/pocketgpt_xiao_v7b50_final_v1` to the card root. Preserve the exact folder and filenames. Do not place another `microsd` folder above it. This folder contains six files: embeddings, two transformer streams and three tokenizer tables.
3. [Build the XIAO local firmware](BUILD.md#xiao-local-keyboard--built-and-serial-tested-on-hardware), including the [GCC16 setup](TOOLCHAIN.md). Return to the source repository root.
4. Package that build together with the released cache/model components. Choose a new output directory:

```sh
XIAO_ASSETS=/absolute/path/to/pocketgpt-xiao-assets/xiao
POCKETGPT_PORT=/dev/cu.usbmodemYOURPORT
python3 tools/package_xiao.py \
  --build-dir xiao_ai/keyboard_interfaces/xiao_local/.pio/build/local_xiao \
  --assets-dir "$XIAO_ASSETS" \
  --out-dir xiao-install
```

The packager verifies the exact three Flash model/cache hashes, partition table and firmware size. It places K10 and app-tail cache bytes into `app_with_tail.bin`. Do not flash bare firmware.bin for a fresh installation.

5. Only when ready to replace the application, install:

```sh
python3 -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" --baud 460800 write_flash \
  0x0 xiao-install/bootloader.bin \
  0x8000 xiao-install/partitions.bin \
  0x10000 xiao-install/app_with_tail.bin \
  0xB0000 xiao-install/model.bin
python3 -m esptool --chip esp32s3 --port "$POCKETGPT_PORT" verify_flash \
  0x10000 xiao-install/app_with_tail.bin \
  0xB0000 xiao-install/model.bin
```

6. Insert the prepared microSD and reset the board. Open Serial Monitor at 115200 baud and inspect boot/cache/model errors. The display should show the keyboard. Type `hello`, press GO and allow time for the response. Use the serial tests/retained results to compare behavior; stop if cache or digest checks fail.

No NVS image is supplied or required. Wi-Fi credentials are not needed for the local firmware. Network Gemini/Ollama firmware is a separate installation described in [NETWORK_SETUP.md](NETWORK_SETUP.md).

## Validation scope

The published model assets match the retained working candidate hashes. XIAO cache assembly was checked offline, and earlier tests on already provisioned hardware are retained in the repository. Fresh-board installation using these instructions has not been exercised end to end. This release adds the missing deployment files; it does not claim new hardware tests or full historical retraining reproducibility.
