# Reproduce the XIAO GCC16 setup

Verified on Apple Silicon macOS, 2026-10-13. This procedure installs build tools only; it never flashes hardware.

The compiler is the official Espressif **esp-16.1.0_20260609** release. The previous development directory was a PlatformIO wrapper around this compiler, not a privately patched compiler.

Sources: [official release](https://github.com/espressif/crosstool-NG/releases/tag/esp-16.1.0_20260609), [Espressif tool URLs and checksums](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-tools.html#xtensa-esp-elf).

## Apple Silicon macOS

Run from a directory outside the public repository. Use a new `pocketgpt-toolchain` directory; do not overwrite an existing toolchain.

```sh
mkdir pocketgpt-toolchain
cd pocketgpt-toolchain
curl -fL --retry 2 -o gcc16.tar.xz \
  https://github.com/espressif/crosstool-NG/releases/download/esp-16.1.0_20260609/xtensa-esp-elf-16.1.0_20260609-aarch64-apple-darwin.tar.xz
printf '%s  %s\n' \
  be4eb81c12c9bbf6ead79c758bfa81b8e6ec9a695372f946ac924c70c32f273d \
  gcc16.tar.xz | shasum -a 256 -c -
```

**Continue only if the checksum says OK.** Then:

```sh
tar -xf gcc16.tar.xz
python3 - <<'PY'
import json
from pathlib import Path
p = Path('xtensa-esp-elf/package.json')
backup = p.with_name('package.upstream.json')
if backup.exists():
    raise SystemExit('Package already adapted; refusing to overwrite metadata')
backup.write_bytes(p.read_bytes())
m = json.loads(p.read_text())
m['name'] = 'toolchain-xtensa-esp-elf'
m['version'] = '16.1.0+20260609'
p.write_text(json.dumps(m, indent=2) + '\n')
PY
export POCKETGPT_GCC16_PACKAGE="$PWD/xtensa-esp-elf"
"$POCKETGPT_GCC16_PACKAGE/bin/xtensa-esp32s3-elf-g++" --version
```

Expected compiler version: `crosstool-NG esp-16.1.0_20260609`, GCC `16.1.0`. Only package name/version metadata are adapted for PlatformIO. Compiler executables and upstream licenses are unchanged. Keep the complete extracted directory, including its license files.

In the same terminal, enter this repository's `xiao_ai/keyboard_interfaces/xiao_local` directory and run `pio run`. If PlatformIO is not installed, install it in a Python virtual environment (`python3 -m venv .venv`, activate it, then `python -m pip install platformio==6.1.19`). Verification used PlatformIO Core 6.1.19.

## Validation performed

- Downloaded the archive above; SHA-256 matched the pinned official digest.
- Unpacked it into a separate directory and applied only the package metadata adaptation.
- Compared GCC, G++ and LD executable hashes with the development compiler: identical.
- Built all XIAO source and libraries from a separate source copy using the freshly extracted toolchain.
- Firmware file: 450,672 bytes; 3,984 bytes remain before the K10 cache. Build guard passed. No flash/upload occurred in this verification.

The upstream metadata and Espressif documentation use different umbrella license labels. Preserve the actual bundled per-component licenses; do not relicense the compiler under this project's MIT license.

## Other hosts

Espressif publishes Intel macOS, Linux and Windows builds on the linked release. Their checksums differ. The project has not been validated on those hosts; this guide does not claim cross-platform build verification.
