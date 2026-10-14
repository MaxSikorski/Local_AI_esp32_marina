# Model and tokenizer provenance review

Reviewed 2026-10-13. This is an evidence inventory, not a blanket grant of rights to all artifacts. No dataset text or training corpus is distributed here.

## Selected tied model

The inspected comparison script loads `native_control_repair_v7_plus600.pt`, explicitly assigns the head to the embedding for the tied branch, runs 50 updates, and saves `tied_plus050.pt`. It does **not** start that branch from the separately trained untied `native_precision_repair_v7b_plus050.pt`. The older training-lineage audit describes that untied branch and must not be copied as the exact final tied-model lineage.

The earlier local lineage audit supports Base V2 → precondition/continuation → hardnegative repair → real repair v2 → native-control v7+600. Base V2 used TinyStories/FineWeb-Edu general streams plus a conversational stream. The separate earlier Friend Base v1 run is not a verified ancestor. This review inspected retained scripts/manifests and prior audit evidence; it did not rerun training or independently re-audit every historical checkpoint edge.

## Data sources

The conversational manifest records exact revisions and license labels. The upstream cards were also checked on the review date. Current card labels are not proof that unpinned historical downloads had identical terms.

| Source | Recorded revision | License label | Evidence / unresolved detail |
|---|---|---|---|
| [TinyStories](https://huggingface.co/datasets/roneneldan/TinyStories) | Not pinned | CDLA-Sharing-1.0 on current card | Local general-corpus input; exact upstream snapshot still missing |
| [FineWeb-Edu](https://huggingface.co/datasets/HuggingFaceFW/fineweb-edu) | Not pinned for these general streams | ODC-By-1.0; card also references Common Crawl terms | Do not substitute a revision from another experiment |
| [SODA](https://huggingface.co/datasets/allenai/soda) | `fdc848ab0183208ea7808206c91c724414d0a071` | CC-BY-4.0 | Conversational manifest and current card agree |
| [OASST1](https://huggingface.co/datasets/OpenAssistant/oasst1) | `fdf72ae0827c1cda404aff25b6603abec9e3399b` | Apache-2.0 | Conversational manifest and current card agree |
| [UltraChat 200k](https://huggingface.co/datasets/HuggingFaceH4/ultrachat_200k) | `c02ed026a1a2d6d16596b96149514a9a60ae66fd` | MIT | Conversational manifest and current card agree |
| [smol-smoltalk](https://huggingface.co/datasets/HuggingFaceTB/smol-smoltalk) | `f80219b491a28e79600fa320e075752f1ea0303e` | Apache-2.0 | Conversational manifest and current card agree |
| Local Friend / repair examples | Local files | Curated local examples | Historical source documentation is incomplete; see MODEL_RELEASE_NOTES.md |

The source manifest's hash and the exact inspected script hashes are retained in `PROVENANCE_EVIDENCE.json`. Source identifiers are relative labels, without private paths.

## Tokenizer is a separate provenance chain

The saved tokenizer metadata names a local combined corpus. Its corpus-builder script targets TinyStories, FineWeb-Edu, OASST1, **[Dolly-15k](https://huggingface.co/datasets/databricks/databricks-dolly-15k)** and **[GSM8K](https://huggingface.co/datasets/openai/gsm8k)**. The latter cards state CC-BY-SA-3.0 and MIT respectively.

A script's configured inputs alone do not prove every source was included in the exact saved corpus. A matching execution log or frozen source manifest for this tokenizer was not found in the inspected tokenizer directory. Do not omit these possible sources, but do not advertise their inclusion as established fact either. SentencePiece software licensing does not automatically determine the license of trained tokenizer artifacts.

## Publication decision

The author’s code remains MIT. On 2026-10-13 the author also selected a separate MIT grant for released deployment weights and tokenizer artifacts, to the extent of the author’s rights. They are available as [release packages](ASSETS.md); preserve [attribution and uncertainty disclosures](MODEL_RELEASE_NOTES.md). Availability of exact deployment files does not resolve all historical data reconstruction questions. This inventory does not establish that model redistribution is prohibited. In particular, [CDLA-Sharing-1.0 section 3.5](https://cdla.dev/sharing-1-0/) distinguishes computational Results from redistributed Data; a dataset's sharing terms must not automatically be presented as the model's license.

Outstanding concrete items:

1. Identify the historical TinyStories/FineWeb-Edu downloads used by the general corpus.
2. Recover or reconstruct the exact tokenizer-corpus source manifest, including whether Dolly/GSM8K were actually used.
3. Confirm authorship/provenance of local repair examples and retain required source attribution.
4. Keep the separately selected MODEL_LICENSE and dataset notices with redistributed assets; do not apply it to the training datasets or unrelated model weights.
