# How PocketGPT was trained

This folder contains the original Python code used to define the PocketGPT model, prepare data, train its tokenizer and train the base model from random initialization.

To run PocketGPT on your board, follow the [installation guide](../docs/ASSETS.md). The model weights and tokenizer are available in [GitHub Releases](https://github.com/mmm1712/Local_AI_esp32/releases/tag/v0.1.0-model-assets).

## Start with these files

| File | What it does |
|---|---|
| `pocketlm/pocketlm_v1.py` | Defines the Transformer model |
| `pocketlm/training/pocketlm_30m.py` | Sets the architecture to 30.46 million independent parameters |
| `base_v2_config.json` | Summarizes the main training settings |
| `pocketlm/training/train_friend_base_30m_v2_300m_v1.py` | Runs the main base-model training |

**Base V2** is the internal name for the second version of the base model, before later training and refinement.

## Main training run

| Setting | Value |
|---|---|
| Initialization | Random weights, seed 42 |
| Processed token positions | 300,001,280 |
| General-text positions | 270,001,280 |
| Conversational positions | 30,000,000 |
| Optimizer updates | 58,594 |
| Batch size | 32 sequences |
| Sequence length | 160 tokens |
| Optimizer | AdamW |

The total is 58,594 updates × 32 sequences × 160 positions. It counts processed positions, not distinct words. Further training and deployment optimizations followed this base run.

## Data preparation and supporting files

| Files | Purpose |
|---|---|
| `pocketlm/training/build_pretrain_corpus.py`, `tokenize_pretrain.py` | Prepare and tokenize the first general-text stream |
| `pocketlm/training/build_pretrain_stage1b.py`, `tokenize_pretrain_stage1b.py` | Prepare and tokenize the second general-text stream |
| `pocketlm/training/build_stage1b_validation.py`, `tokenize_stage1b_validation.py` | Prepare separate validation data |
| `pocketlm/training/build_friend_base_chat_corpus_v1.py`, `build_friend_base_chat_corpus_v2.py`, `tokenize_friend_base_chat_corpus_v2.py` | Prepare conversational data; v2 uses helpers from v1 |
| `pocketlm/tokenizer/build_tokenizer_corpus.py`, `train_tokenizer_candidates.py` | Prepare tokenizer data and train candidate tokenizers |
| `pocketlm/training/audit_friend_base_v2_training_stream_v1.py` | Check training inputs and construct the fixed sampling schedule |
| `checkpoints/_audit_friend_base_v2_training_stream_v1/schedule_spec.json` | Saved sampling schedule |
| `pocketlm/quantization/kv_cache_int8.py` | Cache implementation required by the model code |
| `pocketlm/training/train_pretrain_30m.py` | Earlier approximately 50M-token general-text experiment |
| `pocketlm/training/train_friend_base_30m.py`, `friend_base_v1_config.py` | Separate earlier Friend Base v1 experiment |
| `pocketlm/training/benchmark_training.py` | Measure training performance using random token inputs |

The earlier experiments are included for reference; they are separate from the main 300M-token Base V2 run. Original filenames are retained so imports and historical records remain traceable.

## Inspect the training script

From this `training` directory, create a Python environment and view the available options:

```sh
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
python3 -m pocketlm.training.train_friend_base_30m_v2_300m_v1 --help
```

These commands install dependencies and display help. They do not start training. On Windows, activate the environment with `.venv\Scripts\Activate.ps1` in PowerShell.

## Running the historical training process

The original runner expects specific prepared data files. It does not download or rebuild them automatically. The required paths and hashes are listed in `source_paths()` and `EXPECTED_SHA` in the audit script. Two additional validation arrays are expected under `data/pocketlm/friend_distill_pilot_v4`.

After supplying the matching inputs, the following commands check them and start training respectively:

```sh
python3 -m pocketlm.training.train_friend_base_30m_v2_300m_v1 --dry-run --device cpu
python3 -m pocketlm.training.train_friend_base_30m_v2_300m_v1 --train --device mps
```

`--train` performs the full training run and writes checkpoints. `mps` selects Apple Silicon acceleration; `cpu` and `cuda` are also accepted. Paths resolve relative to this `training` directory. Data-preparation scripts may download datasets or create files, so inspect their inputs and output locations before running them.

## What is available to reproduce the project?

**For deployment:** the released quantized weights and tokenizer can be downloaded using the [installation guide](../docs/ASSETS.md).

**For repeating the historical training run:** the original tokenized data streams, local examples, validation arrays and a complete historical dependency lock are not bundled. Some general-data revisions and the exact tokenizer training mix remain incompletely documented. The later refinement/export stages are also not fully packaged. See [data provenance](../docs/DATA_PROVENANCE.md).

The released SentencePiece tokenizer preserves the original vocabulary and normalization behavior, but training-path metadata was removed. Its file hash therefore differs from the original tokenizer hash expected by the historical runner. Downloading it alone does not satisfy the runner's original input checks. Do not bypass those checks to claim an exact reproduction.

<details>
<summary>Historical verification and source-file details</summary>

The retained update-zero checkpoint matched all 111 freshly initialized state tensors with seed 42 and had an empty optimizer. The final optimizer states recorded step 58,594, supporting random initialization for Base V2.

- Original training script SHA256: `09f24315915bd301d9e83fc279fc955920c6674cf745d35b247fdd1d0377e76a`.
- Historical final Base V2 checkpoint SHA256: `2cf3a7eb6a2ed8026fb98cec0da4895762ce8d9590f4108c30efa682d8c7c20f`.
- [SOURCE_PROVENANCE.json](SOURCE_PROVENANCE.json) records original and published source hashes. Publication cleanup removed ordinary comments while preserving the Python syntax structure. The changed script hash matters when resuming checkpoints with source-binding checks.
- The historical checkpoint reported PyTorch 2.13.0 on MPS. `requirements.txt` lists dependencies; it is not a recovered historical lockfile. Identical results across library versions and devices are not established.
- The example commands above follow the original command-line parser; they are not a recovered shell-history transcript.

</details>
