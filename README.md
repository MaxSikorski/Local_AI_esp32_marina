# PocketGPT on ESP32-S3

![PocketGPT handheld device with its touchscreen keyboard and a recorded answer](docs/images/pocketgpt-device.jpg)

A language model trained from scratch and running offline on an ESP32-S3. Use a touchscreen keyboard on XIAO or chat over USB on DevKit.

[Watch the project video](https://youtu.be/LgCNVUndTH8) · [Hardware on Amazon](https://amzn.to/4B00LyJ)

## Run PocketGPT

**The trained weights and tokenizer are available. No training or API key is needed.** Download the source, build the firmware and install the matching model package using the guide for your board:

| Your hardware | Start here |
|---|---|
| XIAO ESP32-S3: 8 MB Flash / 8 MB PSRAM, Seeed Round Display and microSD | [Touchscreen setup](docs/ASSETS.md#xiao-with-touchscreen-keyboard) |
| ESP32-S3 DevKit: 32 MB octal Flash / 16 MB PSRAM | [USB chat setup](docs/ASSETS.md#devkit) |

Start with the shared preparation steps at the top of the [installation guide](docs/ASSETS.md). Model packages are in [Releases](https://github.com/mmm1712/Local_AI_esp32/releases/tag/v0.1.0-model-assets); firmware is built from this repository. The two boards use different packages.

## What can PocketGPT actually do?

This is a personal experiment with limited capabilities. It can produce short greetings and basic conversational replies, but answers can be wrong, repetitive or unrelated. Even basic arithmetic is unreliable.

These are recorded hardware outputs:

| Prompt | Response | Board |
|---|---|---|
| `hello` | `Hello.` | XIAO |
| `how are you?` | `I'm doing okay.` | XIAO |
| `say something nice` | `You're doing better than you think.` | DevKit |

The XIAO tests took about 6.4 seconds for `hello` and 11.8 seconds for `how are you?`. These examples do not guarantee the same quality for other questions. [XIAO recordings](xiao_ai/recorded_results/ui_2026-10-13/serial_results.txt) · [DevKit recording](espdev_ai/recorded_results/2026-10-12_tied30m_dedup_devkit/tests/03.txt)

## Model specifications

| Characteristic | Value |
|---|---|
| Independent trained parameters | 30,460,864 |
| Architecture | Decoder-only Transformer; 12 layers; hidden size 448 |
| Attention | 8 query heads, 2 key/value heads |
| Tokenizer | SentencePiece; 6,144 tokens |
| Runtime limits | 64-token context; up to 24 generated tokens per response |
| Recorded generation speed | DevKit: about 1.41 tokens/s; XIAO: about 0.742 tokens/s |
| Deployment size | DevKit: 20,241,616 bytes; XIAO: 21,706,088 bytes |

Speeds are individual measurements, not guarantees. The packages use different mixed-precision layouts. See [model and quantization details](docs/MODEL_CARD.md).

## Training

The [original training code and settings](training/README.md) are included. **Base V2** names the second base-model version, trained from random initialization on about 300 million token positions before later refinement. Exact historical retraining requires additional data and records described in that guide.

## Optional: Gemini or Ollama

The [network interface](docs/NETWORK_SETUP.md) uses the same XIAO/display hardware, but runs a different firmware. Answers come from Gemini's servers or Ollama on your PC. This option does not use PocketGPT's weights.

## Reference

- [Build reference](docs/BUILD.md) · [Validation results](docs/RELEASE_STATUS.md)
- [Data sources](docs/DATA_PROVENANCE.md) · [Third-party notices](docs/THIRD_PARTY.md) · [Privacy](docs/PRIVACY.md)
- [Code license](LICENSE) · [Model license](MODEL_LICENSE) · [License scope](LICENSE_STATUS.md)
