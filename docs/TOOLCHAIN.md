# XIAO compiler setup

XIAO's local firmware uses the official Espressif **GCC 16.1.0** compiler. Complete this once, in the same terminal as the [installation guide](ASSETS.md). The commands below were checked on **Apple Silicon macOS**.

## Apple Silicon macOS

Start from the repository folder with its Python environment active. Use a new `pocketgpt-toolchain` folder beside the repository:

```sh
cd "$POCKETGPT_REPO"
mkdir ../pocketgpt-toolchain
cd ../pocketgpt-toolchain
curl -fL --retry 2 -o gcc16.tar.xz \
  https://github.com/espressif/crosstool-NG/releases/download/esp-16.1.0_20260609/xtensa-esp-elf-16.1.0_20260609-aarch64-apple-darwin.tar.xz
printf '%s  %s\n' \
  be4eb81c12c9bbf6ead79c758bfa81b8e6ec9a695372f946ac924c70c32f273d \
  gcc16.tar.xz | shasum -a 256 -c -
```

Continue only after `gcc16.tar.xz: OK`. Extract and register the package metadata for PlatformIO:

```sh
tar -xf gcc16.tar.xz
python - <<'PYTHON'
import json
from pathlib import Path
p = Path('xtensa-esp-elf/package.json')
backup = p.with_name('package.upstream.json')
if backup.exists():
    raise SystemExit('Package already adapted; use the existing compiler')
backup.write_bytes(p.read_bytes())
m = json.loads(p.read_text())
m['name'] = 'toolchain-xtensa-esp-elf'
m['version'] = '16.1.0+20260609'
p.write_text(json.dumps(m, indent=2) + '\n')
PYTHON
export POCKETGPT_GCC16_PACKAGE="$PWD/xtensa-esp-elf"
"$POCKETGPT_GCC16_PACKAGE/bin/xtensa-esp32s3-elf-g++" --version
cd "$POCKETGPT_REPO"
```

Check that the compiler reports **GCC 16.1.0**, then return to [XIAO: Build](ASSETS.md#2-build).

When using a new terminal later, activate the repository's `.venv` and set `POCKETGPT_GCC16_PACKAGE` to the same extracted `xtensa-esp-elf` folder. No new download is needed.

## Other hosts

The [official release](https://github.com/espressif/crosstool-NG/releases/tag/esp-16.1.0_20260609) also provides Intel macOS, Linux and Windows builds. Use the matching archive and its own checksum; the Apple Silicon archive above cannot run on those hosts. Those build environments have not been validated for this project.

Only package name/version metadata are adapted. Compiler binaries are unchanged; keep the complete extracted package and its supplied licenses. See [build validation](RELEASE_STATUS.md).
