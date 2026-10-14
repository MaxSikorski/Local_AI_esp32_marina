# Original PocketGPT training sources

These are original project scripts, not replacement examples. Ordinary comments were removed for publication; their Python AST is unchanged. Historical names are retained because imports and checkpoint evidence refer to them. `SOURCE_PROVENANCE.json` records original and published hashes. Docstrings remain where they describe CLI behavior. No training data, personal conversations, weights or tokenizer artifacts are bundled.

## Start here

| Files | Purpose |
|---|---|
| `pocketlm/pocketlm_v1.py` | Trainable transformer architecture |
| `pocketlm/training/pocketlm_30m.py` | 30M model factory |
| `pocketlm/quantization/kv_cache_int8.py` | Architecture's cache dependency |
| `pocketlm/training/train_friend_base_30m_v2_300m_v1.py` | Final Base V2 training runner, initialized randomly |
| `pocketlm/training/audit_friend_base_v2_training_stream_v1.py` | Fixed data hashes and deterministic window schedule |
| `base_v2_config.json` | Readable transcription of the runner's RECIPE; runner code is authoritative |
| `checkpoints/_audit_friend_base_v2_training_stream_v1/schedule_spec.json` | Original saved schedule, not weights |
| `pocketlm/training/build_pretrain_corpus.py`, `tokenize_pretrain.py` | First general-data stream |
| `pocketlm/training/build_pretrain_stage1b.py`, `tokenize_pretrain_stage1b.py` | Second general-data stream |
| `pocketlm/training/build_stage1b_validation.py`, `tokenize_stage1b_validation.py` | Separate general validation stream |
| `pocketlm/training/build_friend_base_chat_corpus_v1.py`, `build_friend_base_chat_corpus_v2.py`, `tokenize_friend_base_chat_corpus_v2.py` | Conversational data preparation; v2 imports v1 helpers |
| `pocketlm/tokenizer/build_tokenizer_corpus.py`, `train_tokenizer_candidates.py` | Tokenizer corpus and candidate training |
| `pocketlm/training/train_pretrain_30m.py` | Earlier approximately 50M-token general experiment, not the 300M run |
| `pocketlm/training/train_friend_base_30m.py`, `friend_base_v1_config.py` | Separate earlier Friend Base v1 experiment, not a verified ancestor of Base V2 |
| `pocketlm/training/benchmark_training.py` | Random-token performance benchmark, not evidence of the final training run |

## Verified historical Base V2 run

300,001,280 processed token positions = 58,594 updates × 32 sequences × 160 positions. General: 270,001,280; conversational: 30,000,000. The retained update-zero checkpoint matched all 111 freshly initialized state tensors with seed 42; its optimizer was empty. The final optimizer states recorded step 58,594. These findings support random initialization for Base V2. The later repair/tied-export stages are separate and are not fully packaged here.

Original runner SHA256: `09f24315915bd301d9e83fc279fc955920c6674cf745d35b247fdd1d0377e76a`.
Final historical Base V2 checkpoint SHA256: `2cf3a7eb6a2ed8026fb98cec0da4895762ce8d9590f4108c30efa682d8c7c20f`.

## Inspecting and running

Use a separate Python environment, then from this `training` directory:

```sh
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
python3 -m pocketlm.training.train_friend_base_30m_v2_300m_v1 --help
```

Requirements list dependencies, not a recovered historical lockfile. The historical checkpoint reported PyTorch 2.13.0 on MPS; cross-version/cross-device bitwise reproduction is not established.

**This is a source archive, not a turnkey training distribution.** `--dry-run` validates historical artifacts and fails until those inputs exist; it does not download them. The original runner requires all 12 input files and hashes declared by `source_paths()` / `EXPECTED_SHA`, plus two passive V4 validation arrays under `data/pocketlm/friend_distill_pilot_v4`. Paths resolve relative to this `training` directory. Data builders also need their declared local inputs; inspect them before execution. Some builders download datasets or create files. Do not point them at your only historical data copy.

Only after obtaining the matching artifacts can the historical commands be used:

```sh
python3 -m pocketlm.training.train_friend_base_30m_v2_300m_v1 --dry-run --device cpu
python3 -m pocketlm.training.train_friend_base_30m_v2_300m_v1 --train --device mps
```

These are commands derived from the original parser, not recovered shell history. `--train` performs a costly full run and writes checkpoints. Comment removal changes the runner hash; this copy must not be presented as byte-identical to the historical runner or used to resume historical checkpoints without understanding its binding checks.

## Missing for exact reproduction

Frozen general-data revisions, original tokenized streams and metadata, tokenizer model and exact tokenizer-corpus manifest, local conversational examples and their confirmed provenance, V4 validation arrays, a full historical dependency lock, and the subsequent repair/export lineage. See [data provenance](../docs/DATA_PROVENANCE.md). Do not redistribute datasets simply because these scripts can access them. Training from these sources today is not a promise of identical final weights.
