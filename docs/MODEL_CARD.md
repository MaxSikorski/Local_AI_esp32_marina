# Model and artifact formats

PocketGPT: decoder-only Transformer; 12 layers; hidden size 448; 8 query heads and 2 KV heads; FFN 1,344; RoPE; RMSNorm; 6,144-token SentencePiece vocabulary. Runtime context is 64 tokens and generation is limited to 24 tokens.

The tied training architecture has **30,460,864 independent trained parameters**. The saved checkpoint contains **33,213,376 tensor elements**, because embedding/head values were serialized twice. Equal values in this checkpoint do not mean the two saved tensors share storage.

Deployment formats differ:

- Original mixed Q4/FP16 export: **25,746,576 bytes**; duplicated FP16 embedding/head storage.
- DevKit deduplicated export: **20,241,616 bytes** including a 64-byte validation footer. Transformer Q4 with FP16 scales; FP16 shared embedding/head. `lm_head.weight` aliases the embedding and adds zero payload bytes. `tools/verify_pack.py` verifies hashes, sizes, footer and alias metadata.
- XIAO deployment: **21,706,088 bytes**, including flash caches. Q4 head with five INT8 repair rows, FP16 embeddings/norms, transformer weights distributed across Flash, PSRAM-loaded SD data and SD residual files. Do not interchange these assets with the DevKit pack.

The DevKit builder derives a deduplicated pack from a specific already-quantized mixed pack; it does not train or export a raw checkpoint. Pass both `--source` and `--out-dir`. Fixed binary input hashes prevent accidental use of another model. The original manifest's private path is not required or reproduced.

Training provenance is documented in [DATA_PROVENANCE.md](DATA_PROVENANCE.md), with inspected-source hashes in PROVENANCE_EVIDENCE.json. Four conversational revisions are recorded. Historical general-corpus versions and the exact tokenizer mix remain unresolved. Weights/tokenizer release terms are still separate from the MIT code license.

Recorded generation rates are approximately 1.41 tokens/s on the specified DevKit and 0.742 tokens/s on XIAO. These are prior single-run measurements, not a controlled cross-platform benchmark or a promise for another build.
