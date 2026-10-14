# Validation results

Updated 2026-10-13. Firmware source is in this repository; trained model/tokenizer packages are in [Releases](https://github.com/mmm1712/Local_AI_esp32/releases/tag/v0.1.0-model-assets). Start with the [installation guide](ASSETS.md).

## Recorded checks

| Component | Result |
|---|---|
| DevKit firmware | PlatformIO build passed; 431,696-byte application |
| XIAO local firmware | Full source/library build with the official GCC16 compiler passed; 450,672 bytes, within the 454,656-byte cache limit |
| XIAO local hardware | Boot/cache checks and three USB inference tests passed; display, digits, deletion, ABC switching and GO confirmed |
| XIAO network firmware | Compile-checked: 892,941 bytes Flash and 48,240 bytes static RAM; Gemini/Ollama hardware connectivity was not tested |
| DevKit model package | Hashes, size, footer and shared-matrix checks passed |
| XIAO model package | Flash/cache hashes, 84 matrix placements and offline cache assembly checked |
| Source checks | Nine tests passed; 28 Python files parsed; local Markdown links checked |

Build checks used Apple Silicon macOS. Hardware recordings describe the tested builds; later source naming/comment cleanup and documentation changes were not new hardware tests. No board was flashed for the model asset release.

## Hardware evidence

- [DevKit inference results](../espdev_ai/recorded_results/2026-10-12_tied30m_dedup_devkit/)
- [XIAO USB and touchscreen results](../xiao_ai/recorded_results/ui_2026-10-13/)

## Release scope

- Firmware is built from source. No prebuilt firmware, NVS images, whole-board backups or credentials are distributed.
- Released weights/tokenizer allow deployment of the existing model. The complete historical training data and later refinement/export process are not bundled. See [training](../training/README.md).
- Keep the [model attribution](MODEL_RELEASE_NOTES.md) and [third-party notices](THIRD_PARTY.md). Publishing firmware binaries would require reviewing the linked dependencies' obligations.
- The [privacy checker](../tools/verify_release.py) screens common secret patterns and local metadata. It is not a guarantee that every possible secret is absent.
