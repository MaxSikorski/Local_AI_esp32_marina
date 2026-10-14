# Contributing

Small fixes, clearer docs, and bug reports are welcome. Keep each pull request focused on one change. For a bigger change, open an issue first so we can discuss it.

## Find the right code

- `espdev_ai/firmware`: offline inference on the 32 MB Flash / 16 MB PSRAM DevKit.
- `xiao_ai/keyboard_interfaces/xiao_local`: offline inference on XIAO with the Round Display and microSD.
- `xiao_ai/keyboard_interfaces/xiao_network`: the separate Gemini/Ollama interface.
- `tools` and `tests`: packaging, verification, and serial testing.
- `training`: historical training scripts. Read the [training guide](training/README.md) before running them; some required inputs are not included.

## Check your change

You can work on docs and Python tools without a board or model weights. The unit tests use Python's standard library.

From the repository root, with uv installed, run:

```sh
uv run --no-project python -B -m unittest discover -s tests -v
git diff --check
```

For docs-only changes, check the preview and make sure the links work. You don't need to build firmware for those.

For firmware changes, follow the [build guide](docs/BUILD.md) for the board you changed. XIAO local also needs the [documented toolchain](docs/TOOLCHAIN.md). Keep the firmware size guard enabled; the app shares Flash space with fixed model caches.

Changes to inference, storage, or touch input also need testing on the affected board. If you don't have one, say so in the PR and ask for help testing. A successful build alone doesn't confirm hardware behavior.

## Open a pull request

Fork the repository and make a branch for your change. In the PR, explain what was wrong, what you changed, and how you checked it. Mention anything you couldn't test. Include a regression test for a bug fix when practical.

Keep credentials, board backups, model files, and build outputs out of the PR. Review any logs you share. The [privacy guide](docs/PRIVACY.md) explains what to leave out. From a clean source checkout, you can also run:

```sh
uv run --no-project python -B tools/verify_release.py .
```

This catches some common secret patterns and local metadata, but you still need to review the files yourself. Preserve existing license and attribution notices.

## Report a bug

Include your board and memory configuration, firmware variant, commit or release, model asset version, and build tool versions. Describe the steps to reproduce it, what you expected, and what happened. For model responses, include the prompt and reply if you can share them. Remove passwords, API keys, and private details from logs before posting.
