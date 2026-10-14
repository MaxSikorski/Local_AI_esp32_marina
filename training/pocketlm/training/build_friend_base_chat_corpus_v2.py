"""Build the replacement 30M-token Friend conversational corpus v2.

The first real invocation must use ``--dry-run``. It performs source loading,
selection, tokenization, de-duplication, leakage checks, and concentration
audits in memory and writes nothing. ``--write`` is a separate second phase
and refuses to overwrite any path. No model or optimizer is used here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import statistics
from collections import Counter, defaultdict
from dataclasses import asdict
from pathlib import Path

import sentencepiece as spm
from datasets import load_dataset

from pocketlm.training import build_friend_base_chat_corpus_v1 as v1


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data/pocketlm/friend_base_chat_v2"
AUDIT = ROOT / "checkpoints/_audit_friend_base_chat_corpus_v2"
TRAIN_OUT = OUT / "friend_base_chat_v2_train.jsonl"
VAL_OUT = OUT / "friend_base_chat_v2_val.jsonl"
MANIFEST_OUT = OUT / "friend_base_chat_v2_manifest.json"

VERSION = "friend_base_chat_corpus_v2"
SEED = 2042
TARGET_TRAIN_TOKENS = 30_000_000
TARGET_VAL_TOKENS = 1_000_000
                                                                          
                                                                         
                                                                           
                                                                       
                                                   
SODA_TRAIN_TARGET = 25_000_000
SODA_VAL_TARGET = 500_000

ULTRACHAT_DATASET = "HuggingFaceH4/ultrachat_200k"
ULTRACHAT_REVISION = "c02ed026a1a2d6d16596b96149514a9a60ae66fd"
ULTRACHAT_TRAIN_SPLIT = "train_sft"
ULTRACHAT_VAL_SPLIT = "test_sft"

SMOLTALK_DATASET = "HuggingFaceTB/smol-smoltalk"
SMOLTALK_REVISION = "f80219b491a28e79600fa320e075752f1ea0303e"
SMOLTALK_TRAIN_SPLIT = "train"
SMOLTALK_VAL_SPLIT = "test"

                                                                          
                                                                        
OPENING_CAPS = {1: 20_000, 2: 1_000, 3: 250}
PROGRESS_TOKENS = 1_000_000

INSTRUCTIONAL_RE = re.compile(
    r"\b(?:write|generate|compose|draft|create|provide|list|summarize|translate|"
    r"explain step|step[- ]by[- ]step|code|program|essay|article|recipe)\b",
    re.I,
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError("v2 corpus contract failure: " + message)


def stable_hash(text: str) -> str:
    return hashlib.sha256((str(SEED) + "\0" + text).encode("utf-8")).hexdigest()


def opening_key(text: str, words: int) -> str:
    parts = re.findall(r"[a-z0-9']+", v1.normalized(text))
    return " ".join(parts[:words]) if parts else "<none>"


def canonical_line(row: v1.Example) -> str:
    return json.dumps(asdict(row), sort_keys=True, ensure_ascii=False, separators=(",", ":")) + "\n"


class Collector:
    def __init__(self, tokenizer, references: list[dict]):
        self.tokenizer = tokenizer
        self.reference_prompt = {v1.normalized(row["prompt"]) for row in references}
        self.reference_answer = {v1.normalized(row["answer"]) for row in references}
        self.reference_ngrams: dict[str, dict[tuple[str, ...], set[int]]] = {
            "prompt": defaultdict(set), "answer": defaultdict(set)
        }
        self.reference_sets: dict[str, list[set[tuple[str, ...]]]] = {"prompt": [], "answer": []}
        for field in ("prompt", "answer"):
            for index, row in enumerate(references):
                grams = v1.word_ngrams(row[field])
                self.reference_sets[field].append(grams)
                for gram in grams:
                    self.reference_ngrams[field][gram].add(index)

        self.ids: set[str] = set()
        self.prompts: set[str] = set()
        self.answers: set[str] = set()
        self.train: list[v1.Example] = []
        self.val: list[v1.Example] = []
        self.train_tokens = 0
        self.val_tokens = 0
        self.source_tokens: Counter[str] = Counter()
        self.source_rows: Counter[str] = Counter()
        self.split_source_tokens: Counter[tuple[str, str]] = Counter()
        self.rejections: Counter[str] = Counter()
        self.openings: dict[int, Counter[str]] = {n: Counter() for n in OPENING_CAPS}

    def near_reference(self, field: str, grams: set[tuple[str, ...]]) -> bool:
        if len(grams) < 4:
            return False
        candidates: set[int] = set()
        for gram in grams:
            candidates.update(self.reference_ngrams[field].get(gram, ()))
        return any(v1.jaccard(grams, self.reference_sets[field][index]) >= 0.82 for index in candidates)

    def add(
        self,
        *,
        split: str,
        id: str,
        source: str,
        source_split: str,
        source_id: str,
        prompt: object,
        answer: object,
        license: str,
        min_answer_tokens: int = 3,
        exempt_opening_caps: bool = False,
    ) -> bool:
        prompt, answer = v1.clean(prompt), v1.clean(answer)
        reason = v1.text_rejection(prompt, answer)
        if reason:
            self.rejections[source + ":" + reason] += 1
            return False
        lengths = v1.tokenize_candidate(
            self.tokenizer, prompt, answer, min_answer_tokens=min_answer_tokens
        )
        if lengths is None:
            self.rejections[source + ":token_length_or_unk"] += 1
            return False
        normalized_prompt, normalized_answer = v1.normalized(prompt), v1.normalized(answer)
        if id in self.ids or normalized_prompt in self.prompts or normalized_answer in self.answers:
            self.rejections[source + ":global_duplicate"] += 1
            return False
        if normalized_prompt in self.reference_prompt or normalized_answer in self.reference_answer:
            self.rejections[source + ":v4_exact_overlap"] += 1
            return False
        prompt_grams, answer_grams = v1.word_ngrams(prompt), v1.word_ngrams(answer)
        if self.near_reference("prompt", prompt_grams) or self.near_reference("answer", answer_grams):
            self.rejections[source + ":v4_near_overlap"] += 1
            return False
        if split == "train" and not exempt_opening_caps:
            for width, cap in OPENING_CAPS.items():
                if self.openings[width][opening_key(answer, width)] >= cap:
                    self.rejections[source + f":opening_{width}_cap"] += 1
                    return False

        row = v1.Example(
            id=id,
            source=source,
            source_split=source_split,
            source_id=source_id,
            prompt=prompt,
            answer=answer,
            prompt_tokens=lengths[0],
            answer_tokens=lengths[1],
            sequence_tokens=lengths[2],
            license=license,
        )
        target = self.train if split == "train" else self.val
        target.append(row)
        self.ids.add(id)
        self.prompts.add(normalized_prompt)
        self.answers.add(normalized_answer)
        if split == "train":
            self.train_tokens += row.sequence_tokens
            for width in OPENING_CAPS:
                self.openings[width][opening_key(answer, width)] += 1
        else:
            self.val_tokens += row.sequence_tokens
        self.source_tokens[source] += row.sequence_tokens
        self.source_rows[source] += 1
        self.split_source_tokens[(split, source)] += row.sequence_tokens
        return True


def add_local(collector: Collector) -> None:
    require(v1.sha256(v1.PRECONDITION) == v1.EXPECTED_SHA["precondition"], "local Friend SHA")
    rows = v1.load_jsonl(v1.PRECONDITION)
    require(len(rows) == 400, "local Friend rows")
    for row in rows:
        require(
            collector.add(
                split="train",
                id="local_" + row["id"],
                source="local_friend",
                source_split="frozen_train",
                source_id=row["id"],
                prompt=row["prompt"],
                answer=row["answer"],
                license="repository-authored",
                min_answer_tokens=1,
                exempt_opening_caps=True,
            ),
            "frozen local Friend row rejected " + row["id"],
        )


def add_oasst(collector: Collector) -> dict:
    result = {}
    for source_split, target_split in (("train", "train"), ("validation", "val")):
        dataset = load_dataset(
            v1.OASST_DATASET, split=source_split, revision=v1.OASST_REVISION
        )
        by_id = {row["message_id"]: row for row in dataset}
        accepted = 0
        for row in dataset:
            if not v1.oasst_metadata_ok(row):
                collector.rejections["oasst1:metadata"] += 1
                continue
            parent = by_id.get(row.get("parent_id"))
            if not parent or parent.get("role") != "prompter" or parent.get("lang") != "en" or parent.get("deleted"):
                collector.rejections["oasst1:parent"] += 1
                continue
            if collector.add(
                split=target_split,
                id="oasst1_" + row["message_id"],
                source="oasst1",
                source_split=source_split,
                source_id=row["message_id"],
                prompt=parent["text"],
                answer=row["text"],
                license="apache-2.0",
            ):
                accepted += 1
        result[source_split] = {"rows_scanned": len(dataset), "rows_accepted": accepted}
    return result


def alternating_pairs(messages: list[dict]):
    for index in range(len(messages) - 1):
        left, right = messages[index], messages[index + 1]
        if left.get("role") == "user" and right.get("role") == "assistant":
            yield index, left.get("content"), right.get("content")


def add_soda(collector: Collector) -> dict:
    dataset = load_dataset(
        v1.SODA_DATASET, split="train", streaming=True, revision=v1.SODA_REVISION
    )
    scanned = pairs = accepted_train = accepted_val = 0
    next_progress = PROGRESS_TOKENS
    for row in dataset:
        scanned += 1
        source_id = str(row.get("original_index"))
        dialogue, speakers = row.get("dialogue") or [], row.get("speakers") or []
        if len(dialogue) < 2 or len(dialogue) != len(speakers):
            collector.rejections["soda:invalid_dialogue"] += 1
            continue
        is_val = int(stable_hash("soda-split:" + source_id)[:16], 16) % 1000 < 40
        split = "val" if is_val else "train"
        if split == "train" and collector.split_source_tokens[("train", "soda")] >= SODA_TRAIN_TARGET:
            continue
        current_val_soda = collector.split_source_tokens[("val", "soda")]
        if split == "val" and current_val_soda >= SODA_VAL_TARGET:
            continue
        for turn in range(len(dialogue) - 1):
            if speakers[turn] == speakers[turn + 1]:
                continue
            pairs += 1
            if collector.add(
                split=split,
                id=f"soda_{source_id}_{turn:02d}",
                source="soda",
                source_split="stable_hash_val" if is_val else "stable_hash_train",
                source_id=source_id,
                prompt=dialogue[turn],
                answer=dialogue[turn + 1],
                license="cc-by-4.0",
            ):
                if is_val:
                    accepted_val += 1
                else:
                    accepted_train += 1
        current_train_soda = collector.split_source_tokens[("train", "soda")]
        if current_train_soda >= next_progress:
            print(f"SODA train accepted tokens: {current_train_soda:,}", flush=True)
            next_progress += PROGRESS_TOKENS
        current_val_soda = collector.split_source_tokens[("val", "soda")]
        if current_train_soda >= SODA_TRAIN_TARGET and current_val_soda >= SODA_VAL_TARGET:
            break
    soda_train_tokens = collector.split_source_tokens[("train", "soda")]
    soda_val_tokens = collector.split_source_tokens[("val", "soda")]
    require(soda_train_tokens >= SODA_TRAIN_TARGET, "SODA train token target")
    require(soda_val_tokens >= SODA_VAL_TARGET, "SODA validation token target")
    return {
        "rows_scanned": scanned,
        "pairs_seen": pairs,
        "accepted_train_rows": accepted_train,
        "accepted_val_rows": accepted_val,
        "train_tokens": soda_train_tokens,
        "validation_tokens": soda_val_tokens,
    }


def add_ultrachat(collector: Collector) -> dict:
    result = {}
    for source_split, target_split, target_tokens in (
        (ULTRACHAT_TRAIN_SPLIT, "train", TARGET_TRAIN_TOKENS),
        (ULTRACHAT_VAL_SPLIT, "val", TARGET_VAL_TOKENS),
    ):
        dataset = load_dataset(
            ULTRACHAT_DATASET,
                                                                              
                                                                             
                                                                               
                                                                          
                                                                
            data_dir="default/" + source_split,
            split="train",
            revision=ULTRACHAT_REVISION,
            streaming=True,
        )
        scanned = pairs = accepted = 0
        start_tokens = collector.train_tokens if target_split == "train" else collector.val_tokens
        next_progress = ((start_tokens // PROGRESS_TOKENS) + 1) * PROGRESS_TOKENS
        for row in dataset:
            scanned += 1
            prompt_id = str(row.get("prompt_id"))
            for turn, prompt, answer in alternating_pairs(row.get("messages") or []):
                pairs += 1
                if collector.add(
                    split=target_split,
                    id=f"ultrachat_{prompt_id}_{turn:02d}",
                    source="ultrachat_200k",
                    source_split=source_split,
                    source_id=prompt_id,
                    prompt=prompt,
                    answer=answer,
                    license="mit",
                ):
                    accepted += 1
                current = collector.train_tokens if target_split == "train" else collector.val_tokens
                if current >= target_tokens:
                    break
            current = collector.train_tokens if target_split == "train" else collector.val_tokens
            if current >= next_progress:
                print(f"UltraChat {target_split} total tokens: {current:,}", flush=True)
                next_progress += PROGRESS_TOKENS
            if current >= target_tokens:
                break
        current = collector.train_tokens if target_split == "train" else collector.val_tokens
        result[source_split] = {
            "rows_scanned": scanned,
            "pairs_seen": pairs,
            "rows_accepted": accepted,
            "total_split_tokens_after_source": current,
            "source_exhausted_before_total_target": current < target_tokens,
        }
    return result


def add_smoltalk(collector: Collector) -> dict:
    result = {}
    for source_split, target_split, target_tokens in (
        (SMOLTALK_TRAIN_SPLIT, "train", TARGET_TRAIN_TOKENS),
        (SMOLTALK_VAL_SPLIT, "val", TARGET_VAL_TOKENS),
    ):
        dataset = load_dataset(
            SMOLTALK_DATASET,
            split=source_split,
            revision=SMOLTALK_REVISION,
            streaming=True,
        )
        scanned = pairs = accepted = 0
        next_progress = (
            ((collector.train_tokens if target_split == "train" else collector.val_tokens)
             // PROGRESS_TOKENS) + 1
        ) * PROGRESS_TOKENS
        for row_index, row in enumerate(dataset):
            scanned += 1
            source_name = str(row.get("source") or "unknown")
            source_id = f"{source_split}:{source_name}:{row_index}"
            for turn, prompt, answer in alternating_pairs(row.get("messages") or []):
                pairs += 1
                if collector.add(
                    split=target_split,
                    id=f"smoltalk_{source_split}_{row_index:07d}_{turn:02d}",
                    source="smol_smoltalk",
                    source_split=source_split,
                    source_id=source_id,
                    prompt=prompt,
                    answer=answer,
                    license="apache-2.0",
                ):
                    accepted += 1
                current = collector.train_tokens if target_split == "train" else collector.val_tokens
                if current >= target_tokens:
                    break
            current = collector.train_tokens if target_split == "train" else collector.val_tokens
            if current >= next_progress:
                print(f"SmolTalk {target_split} total tokens: {current:,}", flush=True)
                next_progress += PROGRESS_TOKENS
            if current >= target_tokens:
                break
        current = collector.train_tokens if target_split == "train" else collector.val_tokens
        require(current >= target_tokens, f"SmolTalk fills {target_split} total target")
        result[source_split] = {
            "rows_scanned": scanned,
            "pairs_seen": pairs,
            "rows_accepted": accepted,
            "total_split_tokens_after_source": current,
        }
    return result


def percentile(values: list[int], fraction: float) -> float:
    ordered = sorted(values)
    at = (len(ordered) - 1) * fraction
    low, high = int(at), min(int(at) + 1, len(ordered) - 1)
    return float(ordered[low] + (ordered[high] - ordered[low]) * (at - low))


def stats(values) -> dict:
    values = list(values)
    return {
        "min": min(values), "max": max(values), "mean": statistics.mean(values),
        "median": statistics.median(values), "p90": percentile(values, 0.90),
        "p99": percentile(values, 0.99),
    }


def row_audit(rows: list[v1.Example]) -> dict:
    openings = {
        str(width): Counter(opening_key(row.answer, width) for row in rows).most_common(30)
        for width in OPENING_CAPS
    }
    return {
        "rows": len(rows),
        "sequence_tokens": sum(row.sequence_tokens for row in rows),
        "source_rows": dict(sorted(Counter(row.source for row in rows).items())),
        "source_tokens": dict(sorted(Counter({source: sum(r.sequence_tokens for r in rows if r.source == source) for source in {r.source for r in rows}}).items())),
        "prompt_token_stats": stats(row.prompt_tokens for row in rows),
        "answer_token_stats": stats(row.answer_tokens for row in rows),
        "sequence_token_stats": stats(row.sequence_tokens for row in rows),
        "question_prompts": sum(row.prompt.rstrip().endswith("?") for row in rows),
        "question_answers": sum(row.answer.rstrip().endswith("?") for row in rows),
        "top_openings": openings,
    }


def content_sha(rows: list[v1.Example]) -> str:
    digest = hashlib.sha256()
    for row in rows:
        digest.update(canonical_line(row).encode("utf-8"))
    return digest.hexdigest()


def atomic_write(path: Path, text: str) -> None:
    require(not path.exists() and not path.is_symlink(), "refusing overwrite " + str(path))
    temporary = Path(str(path) + ".tmp")
    require(not temporary.exists(), "fresh temporary " + str(temporary))
    with temporary.open("x", encoding="utf-8") as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    os.link(temporary, path)
    temporary.unlink()


def write_outputs(train, val, manifest, audit) -> None:
    require(not OUT.exists() and not OUT.is_symlink(), "fresh corpus directory")
    require(not AUDIT.exists() and not AUDIT.is_symlink(), "fresh audit directory")
    OUT.mkdir(parents=True)
    AUDIT.mkdir(parents=True)
    atomic_write(TRAIN_OUT, "".join(canonical_line(row) for row in train))
    atomic_write(VAL_OUT, "".join(canonical_line(row) for row in val))
    manifest["files"] = {
        str(TRAIN_OUT.relative_to(ROOT)): v1.sha256(TRAIN_OUT),
        str(VAL_OUT.relative_to(ROOT)): v1.sha256(VAL_OUT),
    }
    atomic_write(MANIFEST_OUT, json.dumps(manifest, indent=2, sort_keys=True, ensure_ascii=False) + "\n")
    atomic_write(AUDIT / "summary.json", json.dumps(audit, indent=2, sort_keys=True, ensure_ascii=False) + "\n")
    report = (
        "# Friend Base chat corpus v2\n\n"
        "**CORPUS READY FOR TOKENIZATION AUDIT.** This is a replacement corpus; it does not append v1.\n\n"
        f"- Train rows: {len(train):,}\n- Train document tokens: {audit['train']['sequence_tokens']:,}\n"
        f"- Validation rows: {len(val):,}\n- Validation document tokens: {audit['validation']['sequence_tokens']:,}\n"
        "- V4 exact/normalized/near leakage admitted: 0\n- Training updates: 0\n"
    )
    atomic_write(AUDIT / "REPORT.md", report)
    paths = [TRAIN_OUT, VAL_OUT, MANIFEST_OUT, AUDIT / "summary.json", AUDIT / "REPORT.md"]
    atomic_write(
        AUDIT / "SHA256SUMS.txt",
        "".join(f"{v1.sha256(path)}  {path.relative_to(ROOT)}\n" for path in paths),
    )
    require(sum(1 for _ in TRAIN_OUT.open(encoding="utf-8")) == len(train), "train read-back rows")
    require(sum(1 for _ in VAL_OUT.open(encoding="utf-8")) == len(val), "validation read-back rows")


def plan() -> dict:
    return {
        "version": VERSION,
        "purpose": "replacement Friend Base chat corpus; do not concatenate v1",
        "target_train_document_tokens": TARGET_TRAIN_TOKENS,
        "target_validation_document_tokens": TARGET_VAL_TOKENS,
        "source_design": {
            "local_friend": "all 400 frozen repository rows",
            "oasst1": "all strict high-quality English parent/assistant pairs",
            "soda": {"train_target_tokens": SODA_TRAIN_TARGET, "validation_target_tokens": SODA_VAL_TARGET},
            "ultrachat_200k": "all 160-token-compatible SFT pairs after hygiene filters",
            "smol_smoltalk": "fills the remaining target from the small-model Apache-2.0 subset",
        },
        "source_revisions": {
            "soda": v1.SODA_REVISION,
            "oasst1": v1.OASST_REVISION,
            "ultrachat_200k": ULTRACHAT_REVISION,
            "smol_smoltalk": SMOLTALK_REVISION,
        },
        "opening_caps": OPENING_CAPS,
        "tokenization_contract": "EOS USER prompt ASSISTANT answer END; max 160",
        "expected_download": "UltraChat and smol-smoltalk parquet plus existing pinned datasets; approximately 2 GB",
        "training_updates": 0,
    }


def build(write: bool) -> dict:
    require(v1.sha256(v1.TOKENIZER) == v1.EXPECTED_SHA["tokenizer"], "tokenizer SHA")
    tokenizer = spm.SentencePieceProcessor(model_file=str(v1.TOKENIZER))
    references, v4_binding = v1.load_v4_reference()
    collector = Collector(tokenizer, references)
    add_local(collector)
    oasst = add_oasst(collector)
    soda = add_soda(collector)
    ultrachat = add_ultrachat(collector)
    smoltalk = add_smoltalk(collector)

    require(collector.train_tokens >= TARGET_TRAIN_TOKENS, "train token target")
    require(collector.val_tokens >= TARGET_VAL_TOKENS, "validation token target")
    require(len(collector.ids) == len(collector.train) + len(collector.val), "unique IDs")
    require(len(collector.prompts) == len(collector.train) + len(collector.val), "unique prompts")
    require(len(collector.answers) == len(collector.train) + len(collector.val), "unique answers")
    train_sources = {(row.source, row.source_id) for row in collector.train}
    val_sources = {(row.source, row.source_id) for row in collector.val}
    require(not (train_sources & val_sources), "source-level train/validation separation")
    for width, cap in OPENING_CAPS.items():
        nonlocal_friend = [row for row in collector.train if row.source != "local_friend"]
        require(max(Counter(opening_key(row.answer, width) for row in nonlocal_friend).values()) <= cap, f"opening {width} cap")

    train_audit, val_audit = row_audit(collector.train), row_audit(collector.val)
    manifest = {
        **plan(),
        "row_counts": {"train": len(collector.train), "validation": len(collector.val)},
        "actual_document_tokens": {"train": collector.train_tokens, "validation": collector.val_tokens},
        "sources": {
            "local_friend": {"license": "repository-authored", "rows": 400},
            "oasst1": {"dataset": v1.OASST_DATASET, "revision": v1.OASST_REVISION, "license": "apache-2.0", "selection": oasst},
            "soda": {"dataset": v1.SODA_DATASET, "revision": v1.SODA_REVISION, "license": "cc-by-4.0", "selection": soda},
            "ultrachat_200k": {"dataset": ULTRACHAT_DATASET, "revision": ULTRACHAT_REVISION, "license": "mit", "selection": ultrachat},
            "smol_smoltalk": {"dataset": SMOLTALK_DATASET, "revision": SMOLTALK_REVISION, "license": "apache-2.0", "selection": smoltalk},
        },
        "v4_source_binding": v4_binding,
        "canonical_content_sha256": {"train": content_sha(collector.train), "validation": content_sha(collector.val)},
    }
    audit = {
        "classification": "CORPUS_READY" if write else "DRY_RUN_PASS",
        "train": train_audit,
        "validation": val_audit,
        "global_unique_ids": True,
        "global_unique_normalized_prompts": True,
        "global_unique_normalized_answers": True,
        "v4_leakage_admitted": 0,
        "opening_caps": OPENING_CAPS,
        "rejections": dict(sorted(collector.rejections.items())),
        "selection": {"oasst1": oasst, "soda": soda, "ultrachat_200k": ultrachat, "smol_smoltalk": smoltalk},
        "manifest_preview": manifest,
        "samples": [asdict(row) for row in collector.train[:10] + collector.val[:5]],
        "training_updates": 0,
        "optimizer_constructed": False,
        "model_weights_loaded": False,
    }
    if write:
        write_outputs(collector.train, collector.val, manifest, audit)
    return audit


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--plan", action="store_true")
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--write", action="store_true")
    args = parser.parse_args()
    if args.plan:
        print(json.dumps(plan(), indent=2, sort_keys=True))
        print("Files written: NONE")
    else:
        audit = build(write=args.write)
        print(json.dumps({
            "classification": audit["classification"],
            "train": audit["train"],
            "validation": audit["validation"],
            "selection": audit["selection"],
            "rejections": audit["rejections"],
        }, indent=2, sort_keys=True))
        print("Files written: " + ("YES" if args.write else "NONE"))
    print("Models loaded: NONE")
    print("Optimizer constructed: NO")
    print("Training: NOT RUN")


if __name__ == "__main__":
    main()
