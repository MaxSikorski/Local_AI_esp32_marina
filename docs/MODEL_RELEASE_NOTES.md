# Model and tokenizer release notes

Release: v0.1.0-model-assets. Author: mmm1712. The author selected MIT on 2026-10-13 for the released model weights and tokenizer, to the extent of the author's rights. See MODEL_LICENSE. This does not relicense source datasets or third-party materials. No training corpus, firmware executable or whole-board backup is distributed in these asset packages.

## Dataset attribution

Contains information learned from [FineWeb-Edu](https://huggingface.co/datasets/HuggingFaceFW/fineweb-edu), made available under the [ODC Attribution License 1.0](https://opendatacommons.org/licenses/by/1-0/). Its card also discusses Common Crawl and source-content rights. The dataset itself is not redistributed here.

The documented training sources also include:

- [TinyStories, Ronen Eldan and Yuanzhi Li](https://huggingface.co/datasets/roneneldan/TinyStories): [CDLA-Sharing-1.0](https://cdla.dev/sharing-1-0/). Section 3.5 distinguishes computational results from redistributed data.
- [SODA, Allen Institute for AI](https://huggingface.co/datasets/allenai/soda): [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), recorded revision fdc848ab0183208ea7808206c91c724414d0a071.
- [OpenAssistant OASST1](https://huggingface.co/datasets/OpenAssistant/oasst1): [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0), revision fdf72ae0827c1cda404aff25b6603abec9e3399b.
- [UltraChat 200k, HuggingFaceH4](https://huggingface.co/datasets/HuggingFaceH4/ultrachat_200k): [MIT](https://opensource.org/license/mit), revision c02ed026a1a2d6d16596b96149514a9a60ae66fd.
- [smol-smoltalk, HuggingFaceTB](https://huggingface.co/datasets/HuggingFaceTB/smol-smoltalk): Apache 2.0, revision f80219b491a28e79600fa320e075752f1ea0303e.

The tokenizer corpus builder additionally names [Databricks Dolly-15k](https://huggingface.co/datasets/databricks/databricks-dolly-15k), copyright 2023 Databricks, Inc., [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/), including material attributed to Wikipedia editors and contributors, and [GSM8K](https://huggingface.co/datasets/openai/gsm8k), MIT. Their exact inclusion in the saved tokenizer corpus has not been established; they are acknowledged as possible sources rather than confirmed proportions. No Dolly or Wikipedia records are redistributed. The MIT grant does not override any rights applicable to underlying third-party material.

These sources were cleaned, filtered, tokenized and mixed for training. Weights were further trained and quantized for deployment. Original general-data revisions and the complete tokenizer source mix remain unpinned; current cards are supporting evidence, not a recovered historical source lock. Asset availability therefore does not imply exact retraining reproducibility or a blanket third-party rights clearance.

## Training lineage

The Base V2 update-zero checkpoint was verified against a fresh seed-42 initialization. See DATA_PROVENANCE.md in the source repository for the checkpoint lineage and remaining reconstruction limits.

## Privacy and compatibility

Numeric deployment files are copied unchanged from the inspected working candidates. Public metadata has machine-local paths removed. The SentencePiece model's training-only path metadata was previously removed; token pieces, scores, IDs and normalization behavior are preserved. Packages contain no NVS image, Wi-Fi settings or API credentials. Signature screening is limited and does not guarantee absence of memorized training content.

The two board layouts differ. Use the matching package and source commit. Build firmware from the source distribution.
