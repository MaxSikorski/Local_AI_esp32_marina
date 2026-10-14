# Connect XIAO to Gemini or Ollama

This guide accompanies [`xiao_network.ino`](../xiao_ai/keyboard_interfaces/xiao_network/xiao_network.ino). It is one touchscreen sketch with two selectable backends.

**Validation:** compiled successfully with the dependencies below. Gemini/Ollama connectivity and this network sketch's touchscreen behavior have not been hardware-tested for this release. The separate offline PocketGPT UI was tested; that does not validate this sketch.

| Mode | Where the model runs | Connection |
|---|---|---|
| Gemini | Google's servers | XIAO → Wi-Fi → HTTPS API |
| Ollama | Your PC | XIAO → local Wi-Fi → PC's HTTP API |
| Offline PocketGPT | XIAO itself | Separate local firmware; no network API |

## 1. Hardware

You need:

- Seeed Studio XIAO ESP32-S3.
- Seeed Studio Round Display for XIAO, with the matching headers installed.
- USB-C **data** cable and a computer with Arduino IDE.
- A 2.4 GHz Wi-Fi network that the XIAO can join.
- For Ollama: a PC capable of running your selected model on the same reachable network.

Disconnect power before attaching the XIAO to the display. Follow the matching pin labels and orientation in the [Seeed Round Display assembly guide](https://wiki.seeedstudio.com/get_start_round_display/); do not offset the header by a pin. Connect USB to the XIAO. No external breadboard wiring or microSD model files are required for this network sketch.

**Already using offline PocketGPT?** Installing this Arduino sketch with its default partition map can overwrite local model data. Keep a private backup and the matching local model/SD assets before switching. See [BUILD.md](BUILD.md) for returning to the local firmware. Never upload a full Flash backup to GitHub.

## 2. Install Arduino dependencies

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Add the official ESP32 board index in Preferences → Additional Boards Manager URLs:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
3. In Boards Manager, install **esp32 by Espressif Systems, version 2.0.17**. This is the version used for the compilation check; newer versions are not claimed tested.
4. In Library Manager, install **ArduinoJson by Benoit Blanchon, version 7.4.3**.
5. Download/clone this repository and open the complete folder
   `xiao_ai/keyboard_interfaces/xiao_network`.
6. Open `xiao_network.ino`. Keep `touchscreen_keyboard.h`, `google_root_certificates.h` and the entire `src` directory beside it. Do not copy only the .ino file.

The display library is included in `src/display`. No separate TFT_eSPI installation or display-pin changes are needed for this hardware. Preserve the included third-party license files.

## 3. Compile and upload

In Arduino IDE select:

| Setting | Value |
|---|---|
| Board | XIAO_ESP32S3 |
| USB CDC On Boot | Enabled |
| Port | The port that appears when your XIAO is connected |
| Serial Monitor speed | 115200 baud |
| Serial Monitor line ending | Newline |

Use the selected board's defaults for the other options. Click **Verify** first, then **Upload**. Close other programs holding the serial port. If the port is absent, try a known data cable and follow the [official XIAO ESP32-S3 bootloader instructions](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/).

Open Serial Monitor. If the initial question has already scrolled away, reset the board with the monitor open. Reset starts configuration again.

## 4. Configure the sketch — no source edits or secrets in Git

Send **one value per prompt**, using Enter or Send after each. Do not paste the whole setup as one line.

| Prompt | Gemini | Ollama |
|---|---|---|
| Backend | `gemini` | `ollama` |
| Wi-Fi SSID | Your 2.4 GHz network name | Your 2.4 GHz network name |
| Wi-Fi password | Your network password | Your network password |
| Backend setting | Your Gemini API key | `http://YOUR_PC_LAN_IP:11434` |
| Model name | Exact available Gemini model ID, without `models/` | Exact locally installed tag, e.g. `qwen3:4b` |

`YOUR_PC_LAN_IP` is a placeholder, not a value to type literally. No personal network address, password or API key is supplied in this repository.

The sketch does not echo configuration values and calls `WiFi.persistent(false)`. Runtime settings must be re-entered after reset. This is not secure erasure of credentials left by older firmware, nor a guarantee that freed RAM is wiped. Serial Monitor's input field/history may still expose typed secrets: keep it out of screenshots and recordings.

## 5. Gemini setup

1. Open [Google AI Studio](https://aistudio.google.com/), sign in and open its API keys page.
2. Create a key for the appropriate project following [Google's API-key guide](https://ai.google.dev/gemini-api/docs/api-key). Keep the key private; restrict it to the intended API where supported.
3. Choose a Gemini model available to that project. Use its exact API model ID, not its friendly display name. Model availability, quota and billing depend on your account.
4. Enter the five values above through Serial Monitor. Never paste the key into the sketch, README or issue tracker.
5. Wait for `Ready`. The sketch also synchronizes its clock for certificate validation; if it reports `Clock not ready`, wait briefly and retry.

Questions leave the XIAO and are sent to Google's servers. The sketch uses the supplied public CA certificate to verify HTTPS; the certificate in `google_root_certificates.h` is **not a private key**. Do not disable certificate verification to bypass a TLS error.

## 6. Ollama setup on your PC

Install [Ollama](https://ollama.com/download), then download a local model:

```sh
ollama pull qwen3:4b
ollama list
```

The example tag is configurable; select a model your PC can run. Keep the PC awake. Local inference requires the PC, even though it does not require cloud inference after downloading the model.

Ollama normally listens only on localhost. Enable LAN access following the [official server configuration guide](https://docs.ollama.com/faq#how-do-i-configure-ollama-server):

- **macOS app:** run `launchctl setenv OLLAMA_HOST "0.0.0.0:11434"`, then quit and reopen Ollama.
- **Windows app:** quit Ollama, add user environment variable `OLLAMA_HOST` with value `0.0.0.0:11434`, then restart it.
- **Linux service:** run `sudo systemctl edit ollama.service`, add the configuration below, then reload/restart the service.

```ini
[Service]
Environment="OLLAMA_HOST=0.0.0.0:11434"
```

```sh
sudo systemctl daemon-reload
sudo systemctl restart ollama
```

Find the PC's IPv4 address in its active network settings. Enter `http://YOUR_PC_LAN_IP:11434` in the XIAO setup, **without `/api/generate`**; the sketch appends that path. `localhost`, `127.0.0.1` and `0.0.0.0` are not the PC's destination address from XIAO.

Allow inbound TCP 11434 only on a trusted LAN. Avoid guest-network client isolation. Do not port-forward Ollama to the internet: this sketch uses HTTP without API authentication or encryption. No CORS changes are needed for the XIAO client.

## 7. Ask a question

After the sketch prints `Ready`, enter a question in Serial Monitor or type on the touchscreen and press **GO**. Use **123/ABC** to switch keyboard pages, backspace to delete, and space to separate words. Begin with a short prompt such as `Hello`.

The actual answer depends on the selected model; this guide does not provide a fabricated success transcript. Generation is synchronous, so wait for a response before pressing GO again. This sketch sends individual prompts, not a stored multi-turn conversation.

Once setup is complete, send `/setup` in Serial Monitor to change backend or settings. If setup is incomplete and you need to start over, reset the board; `/setup` is recognized in chat mode, not every setup stage.

## 8. Troubleshooting

| Symptom | Check |
|---|---|
| No serial text | Correct port, USB CDC enabled, 115200 baud; reopen monitor and reset |
| Setup never advances | Newline enabled; send one value at a time |
| Wi-Fi connection failed | SSID/password, 2.4 GHz availability and signal; the sketch asks for SSID again |
| Gemini HTTP 400/403/404 | Exact model ID, key/project permissions and request compatibility |
| Gemini HTTP 429 | Project quota/rate limit; check the API dashboard |
| Clock or TLS failure | Internet/NTP access, clock and current trusted CA; keep HTTPS verification enabled |
| Ollama request failed | Running server, exact installed model tag, LAN address/port, firewall and client isolation |
| Screen/touch problem | Correct Round Display, header alignment, complete included display source; network UI hardware behavior is unverified in this release |
| Local PocketGPT stops working after switching | Restore the complete matching local layout and SD assets; a firmware-only upload may not restore overwritten weights |

When reporting a problem, share the backend, dependency versions and error/status code. Remove API keys, SSIDs, passwords, personal addresses and unrelated serial output first.
